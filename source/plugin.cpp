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

void *create(obs_data *settings, obs_encoder *encoder,
             EncoderDetails details) noexcept {
  try {
    return new Encoder(std::move(details), *settings, *encoder);
  } catch (const std::exception &e) {
    log(LOG_ERROR, fmt::format("Plugin::Plugin: {}", e.what()));
    return nullptr;
  }
}

void destroy(void *data) noexcept { delete static_cast<Encoder *>(data); }

bool encode(void *data, struct encoder_frame *frame,
            struct encoder_packet *packet, bool *received_packet) noexcept {
  return static_cast<Encoder *>(data)->encode(*frame, *packet,
                                              *received_packet);
}

void get_defaults(obs_data *data,
                  std::span<const std::unique_ptr<Setting>> settings) noexcept {
  for (const auto &setting : settings) {
    setting->obs_default(*data);
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

bool get_extra_data(void *data, uint8_t **extra_data, size_t *size) noexcept {
  const auto span = static_cast<Encoder *>(data)->get_extra_data();
  *extra_data = span.data();
  *size = span.size();
  return true;
}

} // namespace

OBS_DECLARE_MODULE()

MODULE_EXPORT bool obs_module_load(void) {
  // Correct codec is important because the name is passed to ffmpeg which needs
  // to recognize it.

  obs_encoder_info avc{
      .id = "amf avc",
      .type = OBS_ENCODER_VIDEO,
      .codec = "h264",
      .get_name = [](auto) { return "AMF AVC"; },
      .create = [](auto s,
                   auto e) { return create(s, e, encoder_details_avc); },
      .destroy = destroy,
      .encode = encode,
      .get_defaults =
          [](auto d) { get_defaults(d, encoder_details_avc.settings()); },
      .get_properties =
          [](auto) { return get_properties(encoder_details_avc.settings()); },
      .get_extra_data = get_extra_data,
  };
  obs_register_encoder(&avc);

  obs_encoder_info hevc{
      .id = "amf hevc",
      .type = OBS_ENCODER_VIDEO,
      .codec = "hevc",
      .get_name = [](auto) { return "AMF HEVC"; },
      .create =
          [](auto s, auto e) noexcept {
            return ::create(s, e, encoder_details_hevc);
          },
      .destroy = destroy,
      .encode = encode,
      .get_defaults =
          [](auto d) { get_defaults(d, encoder_details_hevc.settings()); },
      .get_properties =
          [](auto) { return get_properties(encoder_details_hevc.settings()); },
      .get_extra_data = get_extra_data,
  };
  obs_register_encoder(&hevc);

  return true;
}

MODULE_EXPORT void obs_module_unload(void) {}
