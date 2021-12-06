#pragma once

#include "amf.h"
#include "encoder_details.h"
#include "gsl.h"
#include "texture_encoder.h"

#include <AMF/components/Component.h>
#include <AMF/core/Context.h>
#include <AMF/core/Plane.h>
#include <AMF/core/Surface.h>
#include <obs-module.h>

#include <atlbase.h>
#include <d3d11.h>

#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

// Data passed by OBS when encoding without texture support.
struct CpuSurface {
  const not_null<encoder_frame *> frame;
};

// Data passed by OBS when encoding with texture support.
struct GpuSurface {
  uint32_t handle;
  int64_t pts;
  uint64_t lock_key;
  not_null<uint64_t *> next_key;
};

using SurfaceType = std::variant<CpuSurface, GpuSurface>;

class Encoder {
  EncoderDetails details;
  // The same device that OBS is configured with.
  CComPtr<ID3D11Device> d11_device;
  CComPtr<ID3D11DeviceContext> d11_context;
  Amf amf;
  // Based on d11_device.
  amf::AMFContextPtr amf_context;
  amf::AMFComponentPtr amf_encoder;
  // Optional only so that we can delay initialization in constructor.
  std::optional<TextureEncoder> texture_encoder;

  uint32_t width;
  uint32_t height;
  amf::AMF_SURFACE_FORMAT surface_format;
  std::vector<uint8_t> extra_data;

  // When returning a packet we need to give it a data pointer. That data is
  // stored here. It is not specified how long that pointer has to stay alive.
  // We assume it must live until the next call to encode.
  std::vector<uint8_t> packet_buffer;

  void initialize_dx11();
  void apply_settings(obs_data &a, obs_encoder &);
  void set_extra_data();
  void send_frame_to_encoder(SurfaceType);
  // Returns whether a packet was received.
  bool retrieve_packet_from_encoder(encoder_packet &);
  // surface is created on CPU
  amf::AMFSurfacePtr obs_frame_to_surface(const encoder_frame &);
  // surface is created on GPU
  amf::AMFSurfacePtr obs_texture_to_surface(uint32_t handle, uint64_t lock_key,
                                            uint64_t &next_key);

public:
  // Called by the OBS encoder plugin callbacks we configure in plugin.cpp .
  Encoder(EncoderDetails, obs_data &, obs_encoder &);
  bool encode(SurfaceType, encoder_packet &, bool &received_packet) noexcept;
  std::span<uint8_t> get_extra_data() noexcept;
};
