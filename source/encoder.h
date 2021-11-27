#pragma once

#include "amf.h"
#include "encoder_details.h"

#include <AMF/components/Component.h>
#include <AMF/core/Context.h>
#include <AMF/core/Factory.h>
#include <AMF/core/Plane.h>
#include <AMF/core/Surface.h>
#include <obs-module.h>

#include <cstdint>
#include <span>
#include <vector>

class Encoder {
  Amf amf;
  EncoderDetails details;
  amf::AMFFactory &factory;
  amf::AMFContextPtr context;
  amf::AMFComponentPtr encoder;
  uint32_t width;
  uint32_t height;
  amf::AMF_SURFACE_FORMAT surface_format;
  std::vector<uint8_t> extra_data;
  // When returning a packet we need to give it a data pointer. It is not
  // specified how long that pointer has to stay alive for. We assume it must
  // live until the next call to encode. That data we store here.
  std::vector<uint8_t> packet_buffer;

  void apply_settings(obs_data &data, obs_encoder &obs_encoder);
  void set_extra_data();
  void send_frame_to_encoder(const encoder_frame &frame);
  // Returns whether a packet was received.
  bool retrieve_packet_from_encoder(encoder_packet &packet);

public:
  Encoder(EncoderDetails details_, obs_data &data, obs_encoder &obs_encoder);
  bool encode(const encoder_frame &frame, encoder_packet &packet,
              bool &received_packet) noexcept;
  std::span<uint8_t> get_extra_data() noexcept;
};
