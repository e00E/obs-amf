#include "texture_encoder.h"

#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <combaseapi.h>

#include <algorithm>
#include <stdexcept>

// TODO: compare with jim-nvenc.c

TextureEncoder::TextureEncoder(amf::AMFContextPtr amf_context_,
                               CComPtr<ID3D11Device> device_,
                               CComPtr<ID3D11DeviceContext> context_)
    : amf_context{amf_context_}, device{device_}, context{context_} {}

CComPtr<ID3D11Texture2D>
TextureEncoder::create_texture(const D3D11_TEXTURE2D_DESC &textureDesc) {
  // TOOD: why are these values set but not others?
  D3D11_TEXTURE2D_DESC desc = {
      .Width = textureDesc.Width,
      .Height = textureDesc.Height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = textureDesc.Format,
      .SampleDesc = {.Count = 1},
      .BindFlags = D3D11_BIND_RENDER_TARGET,
  };
  CComPtr<ID3D11Texture2D> texture;
  if (device->CreateTexture2D(&desc, NULL, &texture) < 0) {
    throw std::runtime_error("CreateTexture2d");
  }
  return texture;
}

amf::AMFSurfacePtr TextureEncoder::texture_to_surface(uint32_t handle,
                                                      uint64_t lock_key,
                                                      uint64_t &next_key) {
  // TODO: needed? why would we get an invalid handle?
  if (handle == GS_INVALID_HANDLE) {
    // TOOD: jim-nvenc.c has non permament error handling here. Should we?
    throw std::runtime_error("invalid handle");
  }

  static const GUID AMFTextureArrayIndexGUID = {
      0x28115527,
      0xe7c3,
      0x4b66,
      {0x99, 0xd3, 0x4f, 0x2a, 0xe6, 0xb4, 0x7f, 0xaf}};
  auto texture = texture_from_handle(handle);

  // Happens one time when we get the first obs texture.
  if (!amf_input_texture) {
    D3D11_TEXTURE2D_DESC desc;
    texture.texture->GetDesc(&desc);
    amf_input_texture = create_texture(desc);
  }
  ASSERT_(amf_input_texture);

  texture.mutex->AcquireSync(lock_key, INFINITE);
  context->CopyResource(amf_input_texture.p, texture.texture.p);
  context->Flush();
  texture.mutex->ReleaseSync(next_key);

  amf::AMFSurfacePtr surface;
  if (amf_context->CreateSurfaceFromDX11Native(amf_input_texture.p, &surface,
                                               NULL) != AMF_OK) {
    throw std::runtime_error("CreateSurfaceFromDX11Native");
  }
  return surface;
}

CachedTexture &TextureEncoder::texture_from_handle(uint32_t handle) {
  const auto cached{std::find_if(
      obs_texture_cache.begin(), obs_texture_cache.end(),
      [=](const auto &cached) { return cached.handle == handle; })};
  if (cached != obs_texture_cache.end()) {
    return *cached;
  }

  CComPtr<ID3D11Texture2D> texture;
  if (device->OpenSharedResource((HANDLE)(uintptr_t)handle,
                                 IID_PPV_ARGS(&texture)) < 0) {
    throw std::runtime_error("OpenSharedResource");
  }
  CComPtr<IDXGIKeyedMutex> mutex;
  if (texture->QueryInterface(IID_PPV_ARGS(&mutex)) < 0) {
    throw std::runtime_error("QueryInterface");
  }
  texture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
  obs_texture_cache.push_back({handle, texture, mutex});
  return obs_texture_cache.back();
}
