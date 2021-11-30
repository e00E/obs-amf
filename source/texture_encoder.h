#include "gsl.h"

#include <AMF/core/Context.h>
#include <AMF/core/Surface.h>

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi.h>

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
  // Stores the texture we copy from OBS. Reused every time.
  // TODO: Do we need this? Why not create surface from OBS texture directly?
  CComPtr<ID3D11Texture2D> amf_input_texture;
  // This is not documented but OBS reuses a few handles. There is some work we
  // need to do in texture_to_surface once per handle which is cached here.
  std::vector<CachedTexture> obs_texture_cache;

  CComPtr<ID3D11Texture2D> create_texture(const D3D11_TEXTURE2D_DESC &);
  CachedTexture &texture_from_handle(uint32_t handle);

public:
  // amf_context must have been initialized with the same device.
  TextureEncoder(amf::AMFContextPtr, CComPtr<ID3D11Device>,
                 CComPtr<ID3D11DeviceContext>);
  amf::AMFSurfacePtr texture_to_surface(uint32_t handle, uint64_t lock_key,
                                        uint64_t &next_key);
};
