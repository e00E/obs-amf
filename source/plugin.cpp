// The definition of the OBS visible plugin.

#include "encoder.h"
#include "encoder_details.h"
#include "gsl.h"
#include "util.h"

#include <fmt/core.h>
#include <obs-module.h>

#include <exception>
#include <memory>
#include <span>

namespace {

struct EncoderPlugin {
  czstring id;
  czstring name;
  czstring codec;
  SurfaceTypeE surface_type;
  const EncoderDetails &details;
};

template <EncoderPlugin ep> void register_encoder() {
  obs_encoder_info info{
      .id = ep.id,
      .type = OBS_ENCODER_VIDEO,
      .codec = ep.codec,
      .get_name = [](auto) { return ep.name; },
      .create = [](auto settings, auto encoder) -> void * {
        try {
          return new Encoder(ep.details, *settings, *encoder, ep.surface_type);

        } catch (const std::exception &e) {
          log(LOG_ERROR, fmt::format("Plugin::Plugin: {}", e.what()));
          return nullptr;
        }
      },
      .destroy = [](auto data) { delete static_cast<Encoder *>(data); },
      .encode =
          [](auto data, auto frame, auto packet, auto received_packet) {
            return static_cast<Encoder *>(data)->encode(
                CpuSurface{.frame = frame}, *packet, *received_packet);
          },
      .get_defaults =
          [](auto data) {
            for (const auto &setting : ep.details.settings()) {
              setting->obs_default(*data);
            }
          },
      .get_properties =
          [](auto) {
            auto &properties = *obs_properties_create();
            for (const auto &setting : ep.details.settings()) {
              setting->obs_property(properties);
            }
            return &properties;
          },
      .get_extra_data =
          [](auto data, auto extra_data, auto size) {
            const auto span = static_cast<Encoder *>(data)->get_extra_data();
            *extra_data = span.data();
            *size = span.size();
            return true;
          },
      .caps = ep.surface_type == SurfaceTypeE::Gpu
                  ? OBS_ENCODER_CAP_PASS_TEXTURE
                  : 0,
      .encode_texture =
          [](auto *data, auto handle, auto pts, auto lock_key, auto *next_key,
             auto *packet, auto *received_packet) {
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

  register_encoder<EncoderPlugin{.id = "amf avc cpu",
                                 .name = "AMF AVC CPU",
                                 .codec = "h264",
                                 .surface_type = SurfaceTypeE::Cpu,
                                 .details = encoder_details_avc}>();
  register_encoder<EncoderPlugin{.id = "amf avc gpu",
                                 .name = "AMF AVC GPU",
                                 .codec = "h264",
                                 .surface_type = SurfaceTypeE::Gpu,
                                 .details = encoder_details_avc}>();
  register_encoder<EncoderPlugin{.id = "amf hevc cpu",
                                 .name = "AMF HEVC CPU",
                                 .codec = "hevc",
                                 .surface_type = SurfaceTypeE::Cpu,
                                 .details = encoder_details_hevc}>();
  register_encoder<EncoderPlugin{.id = "amf hevc gpu",
                                 .name = "AMF HEVC GPU",
                                 .codec = "hevc",
                                 .surface_type = SurfaceTypeE::Gpu,
                                 .details = encoder_details_hevc}>();
  return true;
}

MODULE_EXPORT void obs_module_unload() {}
