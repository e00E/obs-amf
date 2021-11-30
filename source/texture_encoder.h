#include "gsl.h"

#include <AMF/core/Context.h>
#include <AMF/core/Surface.h>

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi.h>

#include <cstdint>
#include <vector>

struct CachedTexture {
  uint32_t handle;
  CComPtr<ID3D11Texture2D> texture;
  CComPtr<IDXGIKeyedMutex> mutex;
};

class TextureEncoder {
  amf::AMFContextPtr amf_context;
  CComPtr<ID3D11Device> device;
  CComPtr<ID3D11DeviceContext> context;
  // OBS reuses a limited number of handles (which is not documented). There is
  // some work we need to do in texture_to_surface once per handle which is
  // cached here.
  std::vector<CachedTexture> obs_texture_cache;

  // Retrieve the texture from obs_texture_cache or create and insert it.
  CachedTexture &texture_from_handle(uint32_t handle);

public:
  // amf_context must have been initialized with the same device.
  TextureEncoder(amf::AMFContextPtr, CComPtr<ID3D11Device>,
                 CComPtr<ID3D11DeviceContext>);
  not_null<amf::AMFSurfacePtr>
  texture_to_surface(uint32_t handle, uint64_t lock_key, uint64_t &next_key);
};
