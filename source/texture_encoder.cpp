#include "texture_encoder.h"

#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <combaseapi.h>

#include <algorithm>
#include <stdexcept>

TextureEncoder::TextureEncoder(amf::AMFContextPtr amf_context_,
                               CComPtr<ID3D11Device> device_,
                               CComPtr<ID3D11DeviceContext> context_)
    : amf_context{amf_context_}, device{device_}, context{context_} {}

not_null<amf::AMFSurfacePtr>
TextureEncoder::texture_to_surface(uint32_t handle, uint64_t lock_key,
                                   uint64_t &next_key) {
  // There are things copied from jim-nvenc whose purpose is unclear:
  // - Why wouldn't OBS check for GS_INVALID_HANDLE itself before calling the
  // encoder?
  // - What is the purpose of lock_key and next_key? Based on testing the only
  // use of them that is needed is ReleaseSync(next_key). OBS freezes If that
  // isn't done.
  // - Nvenc and the other AMF texture encoder plugin use the mutex only to copy
  // the texture to another texture they own. In nvenc this seems to be related
  // to the lifetime of the texture. On the other hand for AMF no lifetime
  // requirements are documented and things worked when testing without it.

  if (handle == GS_INVALID_HANDLE) {
    throw std::runtime_error("GS_INVALID_HANDLE");
  }
  auto texture = texture_from_handle(handle);
  texture.mutex->AcquireSync(lock_key, INFINITE);
  const auto _ = gsl::finally([&]() { texture.mutex->ReleaseSync(next_key); });
  amf::AMFSurfacePtr surface;
  if (amf_context->CreateSurfaceFromDX11Native(texture.texture, &surface,
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
  obs_texture_cache.push_back({handle, texture, mutex});
  return obs_texture_cache.back();
}
