#pragma once

#include "amf.h"
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

enum class ColorRange { Partial, Full };

struct ColorProperties {
  not_null<cwzstring> profile;
  not_null<cwzstring> transfer_characteristic;
  not_null<cwzstring> primaries;
};

struct EncoderDetails {
  not_null<cwzstring> amf_encoder_name;
  not_null<cwzstring> extra_data_property;
  not_null<cwzstring> frame_rate_property;
  ColorProperties input_color_properties;
  ColorProperties output_color_properties;
};

// information extracted from one encoder output packet
struct PacketInfo {
  bool is_key_frame;
};

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
  // passed in or implemented by inheriting specialized encoders
  EncoderDetails details;
  virtual void configure_encoder_with_obs_user_settings(amf::AMFComponent &,
                                                        obs_data &) = 0;
  virtual void set_color_range(amf::AMFPropertyStorage &, ColorRange) = 0;
  virtual PacketInfo get_packet_info(amf::AMFPropertyStorage &) = 0;
  // ---

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
  Encoder(obs_data &, obs_encoder &, EncoderDetails);
  bool encode(SurfaceType, encoder_packet &, bool &received_packet) noexcept;
  std::span<uint8_t> get_extra_data() noexcept;
};
