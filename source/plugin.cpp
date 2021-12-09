// The definition of the OBS visible plugin.

#include "encoder.h"
#include "encoder_avc.h"
#include "encoder_hevc.h"
#include "gsl.h"
#include "settings.h"
#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <exception>
#include <memory>

namespace {

struct EncoderPlugin {
  czstring id;
  czstring name;
  czstring codec;
  bool use_texture;
};

template <EncoderPlugin ep, typename Encoder> void register_encoder() {
  obs_encoder_info info{
      .id = ep.id,
      .type = OBS_ENCODER_VIDEO,
      .codec = ep.codec,
      .get_name = [](auto) noexcept { return ep.name; },
      .create = [](auto obs_data, auto obs_encoder) noexcept -> void * {
        try {
          auto encoder = std::make_unique<Encoder>();
          encoder->finish_construction(*obs_data, *obs_encoder);
          return encoder.release();
        } catch (const std::exception &e) {
          log(LOG_ERROR, "Plugin::Plugin: {}", e.what());
          return nullptr;
        }
      },
      .destroy =
          [](auto data) noexcept { delete static_cast<Encoder *>(data); },
      .encode =
          [](auto data, auto frame, auto packet,
             auto received_packet) noexcept {
            return static_cast<Encoder *>(data)->encode(
                CpuSurface{.frame = frame}, *packet, *received_packet);
          },
      .get_defaults =
          [](auto data) noexcept {
            for (const auto &setting : Encoder::settings) {
              setting->obs_default(*data);
            }
          },
      .get_properties =
          [](auto) noexcept {
            auto &properties = *obs_properties_create();
            for (const auto &setting : Encoder::settings) {
              setting->obs_property(properties);
            }
            return &properties;
          },
      .get_extra_data =
          [](auto data, auto extra_data, auto size) noexcept {
            const auto span = static_cast<Encoder *>(data)->get_extra_data();
            *extra_data = span.data();
            *size = span.size();
            return true;
          },
      .caps = ep.use_texture ? OBS_ENCODER_CAP_PASS_TEXTURE : 0,
      .encode_texture =
          [](auto *data, auto handle, auto pts, auto lock_key, auto *next_key,
             auto *packet, auto *received_packet) noexcept {
            return static_cast<Encoder *>(data)->encode(
                GpuSurface{.handle = handle,
                           .pts = pts,
                           .lock_key = lock_key,
                           .next_key = next_key},
                *packet, *received_packet);
          }};
  obs_register_encoder(&info);
}

} // namespace

OBS_DECLARE_MODULE()

MODULE_EXPORT bool obs_module_load() {
  // Correct codec is important because the name is passed to ffmpeg which needs
  // to recognize it.

  register_encoder<EncoderPlugin{
                       .id = "amf avc cpu",
                       .name = "AMF AVC CPU",
                       .codec = "h264",
                       .use_texture = false,
                   },
                   EncoderAvc>();
  register_encoder<EncoderPlugin{
                       .id = "amf avc gpu",
                       .name = "AMF AVC GPU",
                       .codec = "h264",
                       .use_texture = true,
                   },
                   EncoderAvc>();
  register_encoder<EncoderPlugin{
                       .id = "amf hevc cpu",
                       .name = "AMF HEVC CPU",
                       .codec = "hevc",
                       .use_texture = false,
                   },
                   EncoderHevc>();
  register_encoder<EncoderPlugin{
                       .id = "amf hevc gpu",
                       .name = "AMF HEVC GPU",
                       .codec = "hevc",
                       .use_texture = true,
                   },
                   EncoderHevc>();
  return true;
}

MODULE_EXPORT void obs_module_unload() {}
