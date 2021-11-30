#include "texture_encoder.h"

#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <combaseapi.h>

#include <algorithm>
#include <stdexcept>

namespace {

DXGI_FORMAT amf_surface_format_to_dx11(amf::AMF_SURFACE_FORMAT format) {
  // In the OBS UI the possible values are NV12, I420, I444, RGB. When anything
  // other than NV12 is selected OBS does not call the texture encoding
  // callback so we did not bother to research the other mappings.
  switch (format) {
  case amf::AMF_SURFACE_NV12:
    return DXGI_FORMAT_NV12;
  default:
    throw std::runtime_error("unknown surface format");
  }
}

} // namespace

TextureEncoder::TextureEncoder(amf::AMFContextPtr amf_context_,
                               CComPtr<ID3D11Device> device_,
                               CComPtr<ID3D11DeviceContext> context_,
                               uint32_t width, uint32_t height,
                               amf::AMF_SURFACE_FORMAT format)
    : amf_context{amf_context_}, device{device_}, context{context_},
      texture_width{width}, texture_height{height},
      texture_format{amf_surface_format_to_dx11(format)} {}

TextureEncoder::~TextureEncoder() noexcept {
  // Unregister all observers because we are getting destroyed.
  for (auto &texture : amf_textures) {
    if (texture.surface) {
      texture.surface->RemoveObserver(this);
    }
  }
}

ObsTexture &TextureEncoder::obs_texture_from_handle(uint32_t handle) {
  const auto cached{std::find_if(
      obs_textures.begin(), obs_textures.end(),
      [=](const auto &cached) { return cached.handle == handle; })};
  if (cached != obs_textures.end()) {
    return *cached;
  }

  CComPtr<ID3D11Texture2D> texture;
  if (device->OpenSharedResource(
          reinterpret_cast<HANDLE>(static_cast<uintptr_t>(handle)),
          IID_PPV_ARGS(&texture)) < 0) {
    throw std::runtime_error("OpenSharedResource");
  }
  CComPtr<IDXGIKeyedMutex> mutex;
  if (texture->QueryInterface(IID_PPV_ARGS(&mutex)) < 0) {
    throw std::runtime_error("QueryInterface");
  }
  texture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
  obs_textures.push_back({handle, texture, mutex});
  return obs_textures.back();
}

AmfTexture &TextureEncoder::unused_amf_texture() {
  const auto cached{
      std::find_if(amf_textures.begin(), amf_textures.end(),
                   [=](const auto &texture) { return !texture.in_use(); })};
  if (cached != amf_textures.end()) {
    return *cached;
  }

  D3D11_TEXTURE2D_DESC desc = {
      .Width = texture_width,
      .Height = texture_height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = texture_format,
      .SampleDesc = {.Count = 1},
      .BindFlags = D3D11_BIND_RENDER_TARGET,
  };
  CComPtr<ID3D11Texture2D> texture;
  if (device->CreateTexture2D(&desc, NULL, &texture) < 0) {
    throw std::runtime_error("CreateTexture2d");
  }
  amf_textures.push_back({.texture = texture, .surface = nullptr});
  return amf_textures.back();
}

void TextureEncoder::OnSurfaceDataRelease(amf::AMFSurface *surface) {
  const auto texture{std::find_if(
      amf_textures.begin(), amf_textures.end(),
      [=](const auto &texture) { return texture.surface == surface; })};
  ASSERT_(texture != amf_textures.end());
  texture->surface = nullptr;
}

not_null<amf::AMFSurfacePtr>
TextureEncoder::texture_to_surface(uint32_t handle, uint64_t lock_key,
                                   uint64_t &next_key) {
  // There are things copied from jim-nvenc whose purpose is unclear:
  // - Why wouldn't OBS check for GS_INVALID_HANDLE itself before calling the
  // encoder?
  // - What is the purpose of lock_key and next_key? Based on testing the only
  // use of them that is needed is ReleaseSync(next_key). OBS freezes If that
  // isn't done.

  if (handle == GS_INVALID_HANDLE) {
    throw std::runtime_error("GS_INVALID_HANDLE");
  }
  auto &obs_texture = obs_texture_from_handle(handle);
  auto &amf_texture = unused_amf_texture();
  obs_texture.mutex->AcquireSync(lock_key, INFINITE);
  context->CopyResource(amf_texture.texture, obs_texture.texture);
  obs_texture.mutex->ReleaseSync(next_key);
  amf::AMFSurfacePtr surface;
  if (amf_context->CreateSurfaceFromDX11Native(amf_texture.texture, &surface,
                                               this) != AMF_OK) {
    throw std::runtime_error("CreateSurfaceFromDX11Native");
  }
  amf_texture.surface = surface;
#if 0
  log(LOG_DEBUG, "obs_textures {}, amf_textures {}", obs_textures.size(),
      amf_textures.size());
#endif
  return surface;
}
