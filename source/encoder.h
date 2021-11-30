#pragma once

#include "amf.h"
#include "encoder_details.h"
#include "gsl.h"
#include "texture_encoder.h"

#include <AMF/components/Component.h>
#include <AMF/core/Context.h>
#include <AMF/core/Factory.h>
#include <AMF/core/Plane.h>
#include <AMF/core/Surface.h>
#include <obs-module.h>

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi.h>

#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

struct CpuSurface {
  const not_null<encoder_frame *> frame;
};

struct GpuSurface {
  uint32_t handle;
  int64_t pts;
  uint64_t lock_key;
  not_null<uint64_t *> next_key;
};

using SurfaceTypeV = std::variant<CpuSurface, GpuSurface>;

enum class SurfaceTypeE { Cpu, Gpu };

class Encoder {
  Amf amf;
  EncoderDetails details;
  CComPtr<ID3D11Device> d11_device;
  CComPtr<ID3D11DeviceContext> d11_context;
  amf::AMFContextPtr amf_context;
  // set if gpu surface was requested in constructor
  std::optional<TextureEncoder> texture_encoder;
  amf::AMFComponentPtr encoder;
  uint32_t dx11_adapter_index;
  uint32_t width;
  uint32_t height;
  amf::AMF_SURFACE_FORMAT surface_format;
  std::vector<uint8_t> extra_data;
  // When returning a packet we need to give it a data pointer. It is not
  // specified how long that pointer has to stay alive for. We assume it must
  // live until the next call to encode. That data we store here.
  std::vector<uint8_t> packet_buffer;

  void initialize_dx11();
  void apply_settings(obs_data &a, obs_encoder &);
  void set_extra_data();
  void send_frame_to_encoder(SurfaceTypeV);
  // Returns whether a packet was received.
  bool retrieve_packet_from_encoder(encoder_packet &);
  // surface is created on CPU
  amf::AMFSurfacePtr obs_frame_to_surface(const encoder_frame &);
  // surface is created on GPU
  amf::AMFSurfacePtr obs_texture_to_surface(uint32_t handle, uint64_t lock_key,
                                            uint64_t &next_key);

public:
  Encoder(EncoderDetails, obs_data &, obs_encoder &, SurfaceTypeE);
  bool encode(SurfaceTypeV, encoder_packet &, bool &received_packet) noexcept;
  std::span<uint8_t> get_extra_data() noexcept;
};
