// The definition of the OBS visible plugin.

#include "encoder.h"
#include "encoder_details.h"
#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <exception>
#include <memory>
#include <span>

namespace {

void *create(obs_data &settings, obs_encoder &encoder,
             EncoderDetails details) noexcept {
  try {
    return new Encoder(std::move(details), settings, encoder);
  } catch (const std::exception &e) {
    log(LOG_ERROR, fmt::format("Plugin::Plugin: {}", e.what()));
    return nullptr;
  }
}

void get_defaults(obs_data &data,
                  std::span<const std::unique_ptr<Setting>> settings) noexcept {
  for (const auto &setting : settings) {
    setting->obs_default(data);
  }
}

obs_properties *
get_properties(std::span<const std::unique_ptr<Setting>> settings) noexcept {
  auto &properties = *obs_properties_create();
  for (const auto &setting : settings) {
    setting->obs_property(properties);
  }
  return &properties;
}
void destroy(void *data) noexcept { delete static_cast<Encoder *>(data); }

bool encode(void *data, struct encoder_frame *frame,
            struct encoder_packet *packet, bool *received_packet) noexcept {
  return static_cast<Encoder *>(data)->encode(*frame, *packet,
                                              *received_packet);
}

bool get_extra_data(void *data, uint8_t **extra_data, size_t *size) noexcept {
  const auto span = static_cast<Encoder *>(data)->get_extra_data();
  *extra_data = span.data();
  *size = span.size();
  return true;
}

namespace avc {

const char *get_name(void *) noexcept { return "AMF AVC"; }

void *create(obs_data *settings, obs_encoder *encoder) noexcept {
  return ::create(*settings, *encoder, encoder_details_avc);
}

void get_defaults(obs_data *data) noexcept {
  ::get_defaults(*data, encoder_details_avc.settings());
}

obs_properties *get_properties(void *) noexcept {
  return ::get_properties(encoder_details_avc.settings());
}

} // namespace avc

namespace hevc {

const char *get_name(void *) noexcept { return "AMF HEVC"; }

void *create(obs_data *settings, obs_encoder *encoder) noexcept {
  return ::create(*settings, *encoder, encoder_details_hevc);
}

void get_defaults(obs_data *data) noexcept {
  ::get_defaults(*data, encoder_details_hevc.settings());
}

obs_properties *get_properties(void *) noexcept {
  return ::get_properties(encoder_details_hevc.settings());
}

} // namespace hevc

} // namespace

OBS_DECLARE_MODULE()

MODULE_EXPORT bool obs_module_load(void) {
  // Correct codec is important because the name is passed to ffmpeg which needs
  // to recognize it.

  obs_encoder_info avc{
      .id = "amf avc",
      .type = OBS_ENCODER_VIDEO,
      .codec = "h264",
      .get_name = avc::get_name,
      .create = avc::create,
      .destroy = destroy,
      .encode = encode,
      .get_defaults = avc::get_defaults,
      .get_properties = avc::get_properties,
      .get_extra_data = get_extra_data,
  };
  obs_register_encoder(&avc);

  obs_encoder_info hevc{
      .id = "amf hevc",
      .type = OBS_ENCODER_VIDEO,
      .codec = "hevc",
      .get_name = hevc::get_name,
      .create = hevc::create,
      .destroy = destroy,
      .encode = encode,
      .get_defaults = hevc::get_defaults,
      .get_properties = hevc::get_properties,
      .get_extra_data = get_extra_data,
  };
  obs_register_encoder(&hevc);

  return true;
}

MODULE_EXPORT void obs_module_unload(void) {}
