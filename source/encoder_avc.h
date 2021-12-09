#include "encoder.h"

#include "settings.h"

#include <obs-module.h>

class EncoderAvc : public Encoder {
  void configure_encoder_with_obs_user_settings(amf::AMFComponent &,
                                                obs_data &) override;
  void set_color_range(amf::AMFPropertyStorage &, ColorRange) override;
  PacketInfo get_packet_info(amf::AMFPropertyStorage &) override;

public:
  static const std::span<const std::unique_ptr<const Setting>> settings;

  EncoderAvc();
};
