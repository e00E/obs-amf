#include "gsl.h"

#include <AMF/core/Context.h>
#include <AMF/core/Surface.h>

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi.h>

#include <cstdint>
#include <vector>

// OBS reuses a limited number of handles (which is not documented). There is
// some work we need to do in obs_textre_from_handle once per handle which we
// cache here.
struct ObsTexture {
  uint32_t handle;
  CComPtr<ID3D11Texture2D> texture;
  CComPtr<IDXGIKeyedMutex> mutex;
};

// We pass textures to AMF to create a surface from. These textures need to stay
// alive until AMF notifies us that the surface is no longer needed. After which
// we can reuse them.
struct AmfTexture {
  CComPtr<ID3D11Texture2D> texture;
  // Set when we create a surface from the texture. Unset when we are notified
  // that the surface is no longer used by AMF through OnSurfaceDataRelease.
  // Stores an observer to this TextureEncoder.
  amf::AMFSurface *surface;

  inline bool in_use() const noexcept { return surface; }
};

class TextureEncoder : private amf::AMFSurfaceObserver {
  amf::AMFContextPtr amf_context;
  CComPtr<ID3D11Device> device;
  CComPtr<ID3D11DeviceContext> context;
  uint32_t texture_width;
  uint32_t texture_height;
  DXGI_FORMAT texture_format;
  std::vector<ObsTexture> obs_textures;
  // TODO: Use mutex? Not sure when observer callback can happen.
  std::vector<AmfTexture> amf_textures;

  // Retrieve the texture from obs_textures or create and insert it.
  ObsTexture &obs_texture_from_handle(uint32_t handle);
  // Retrieve an unused texture from amf_textures or create and insert it.
  AmfTexture &unused_amf_texture();
  // From AMFSurfaceObserver. Marksthe texture in amf_textures as unused.
  void OnSurfaceDataRelease(amf::AMFSurface *) override;

public:
  // amf_context must have been initialized with the same device.
  TextureEncoder(amf::AMFContextPtr, CComPtr<ID3D11Device>,
                 CComPtr<ID3D11DeviceContext>, uint32_t width, uint32_t height,
                 amf::AMF_SURFACE_FORMAT);
  ~TextureEncoder() noexcept;

  // Delete moving because it would invalidate the surface observer pointer to
  // this. Delete copying because it would mess with the caches.
  TextureEncoder(const TextureEncoder &) = delete;
  TextureEncoder(TextureEncoder &&) = delete;
  TextureEncoder &operator=(const TextureEncoder &) = delete;
  TextureEncoder &operator=(TextureEncoder &&) = delete;

  not_null<amf::AMFSurfacePtr>
  texture_to_surface(uint32_t handle, uint64_t lock_key, uint64_t &next_key);
};
