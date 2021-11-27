#pragma once

#include "gsl.h"
#include "settings.h"

#include <AMF/core/PropertyStorage.h>

#include <memory>
#include <vector>

enum class ColorRange { Partial, Full };

struct ColorProperties {
  not_null<cwzstring> profile;
  not_null<cwzstring> transfer_characteristic;
  not_null<cwzstring> primaries;
};

// information extracted from one encoder output packet
struct PacketInfo {
  bool is_key_frame;
};

// Implementation details of specific amf encoders. Used by the generic encoder
// in encoder.h.
struct EncoderDetails {
  not_null<cwzstring> amf_name;
  not_null<cwzstring> extra_data;
  not_null<cwzstring> frame_rate;
  ColorProperties input_color;
  ColorProperties output_color;
  // First element is AMF_VIDEO_ENCODER_USAGE which should be applied first
  // because it determines defaults for other properties.
  not_null<std::vector<std::unique_ptr<Setting>> (*)()> settings;
  not_null<void (*)(amf::AMFPropertyStorage &, ColorRange)> set_color_range;
  not_null<PacketInfo (*)(amf::AMFPropertyStorage &)> packet_info;
};

extern const EncoderDetails encoder_details_avc;
extern const EncoderDetails encoder_details_hevc;
