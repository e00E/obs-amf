#include "encoder.h"

#include "settings.h"
#include "util.h"

#include <AMF/components/ColorSpace.h>
#include <AMF/core/Data.h>
#include <AMF/core/Factory.h>
#include <fmt/core.h>
#include <obs-module.h>

#include <combaseapi.h>
#include <dxgi.h>

#include <exception>
#include <stdexcept>

namespace {

amf::AMF_SURFACE_FORMAT obs_format_to_amf(video_format format) {
  // In the OBS UI the possible values are NV12, I420, I444, RGB so we do not
  // map other values.
  switch (format) {
  case VIDEO_FORMAT_I420:
    return amf ::AMF_SURFACE_YUV420P;
  case VIDEO_FORMAT_NV12:
    return amf::AMF_SURFACE_NV12;
  case VIDEO_FORMAT_RGBA:
    return amf::AMF_SURFACE_RGBA;
  // not supported by AMF
  case VIDEO_FORMAT_I444:
  default:
    throw std::runtime_error("unknown color format");
  }
}

struct Color {
  AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM profile;
  AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM transfer_characteristic;
  AMF_COLOR_PRIMARIES_ENUM primaries;
  ColorRange range;
};

Color obs_color_space_to_amf(video_colorspace obs_space,
                             video_range_type obs_range) {
  Color color{
      .profile = AMF_VIDEO_CONVERTER_COLOR_PROFILE_UNKNOWN,
      .transfer_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_UNDEFINED,
      .primaries = AMF_COLOR_PRIMARIES_UNDEFINED,
      .range = ColorRange::Full,
  };

  switch (obs_range) {
  case VIDEO_RANGE_PARTIAL:
    color.range = ColorRange::Partial;
    break;
  case VIDEO_RANGE_FULL:
    color.range = ColorRange::Full;
    break;
  default:
    throw std::runtime_error("unknown color range");
  }

  // TODO: It is hard to find documentation on these color space properties. It
  // would be nice if an expert could double check these.
  const auto is_full{color.range == ColorRange::Full};
  switch (obs_space) {
  case VIDEO_CS_601:
    color.profile = is_full ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_601
                            : AMF_VIDEO_CONVERTER_COLOR_PROFILE_601;
    color.transfer_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE170M;
    break;
  case VIDEO_CS_709:
    color.profile = is_full ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709
                            : AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
    color.transfer_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_BT709;
    break;
  case VIDEO_CS_SRGB:
    color.profile = is_full ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709
                            : AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
    color.transfer_characteristic =
        AMF_COLOR_TRANSFER_CHARACTERISTIC_IEC61966_2_1;
    break;
  default:
    throw std::runtime_error("unknown color space");
  }

  return color;
}

static void copy_obs_frame_to_amf_surface(const encoder_frame &frame,
                                          amf::AMFSurface &surface) {
  const size_t plane_count{surface.GetPlanesCount()};
  for (size_t i{0}; i < plane_count; ++i) {
    auto &plane = *surface.GetPlaneAt(i);
    const auto plane_data = static_cast<uint8_t *>(plane.GetNative());
    const auto height{static_cast<size_t>(plane.GetHeight())};
    const auto plane_linesize{static_cast<size_t>(plane.GetHPitch())};
    const auto *const frame_data{frame.data[i]};
    const auto frame_linesize{static_cast<size_t>(frame.linesize[i])};
    if (plane_linesize > frame_linesize) {
      for (size_t line{0}; line < height; ++line) {
        std::memcpy(plane_data + plane_linesize * line,
                    frame_data + frame_linesize * line, frame_linesize);
      }
    } else if (plane_linesize == frame_linesize) {
      std::memcpy(plane_data, frame_data, plane_linesize * height);
    } else {
      throw std::runtime_error(
          fmt::format("plane {} linesize {} is smaller than frame linesize {}",
                      i, plane_linesize, frame_linesize));
    }
  }
}

// The PTS as it was given to us by OBS. Stored in encoder input so that we can
// assign it to the obs output packet.
const not_null<cwzstring> pts_property{L"obs_pts"};

} // namespace

Encoder::Encoder(EncoderDetails details_) : details{details_} {}

void Encoder::finish_construction(obs_data &obs_data,
                                  obs_encoder &obs_encoder) {
  initialize_dx11();

  auto &amf_factory{amf.init()};
  if (amf_factory.CreateContext(&amf_context) != AMF_OK) {
    throw std::runtime_error("AMFFactory::CreateContext");
  }
  if (amf_context->InitDX11(d11_device) != AMF_OK) {
    throw std::runtime_error("AMFContext::InitDX11");
  }

  if (amf_factory.CreateComponent(amf_context, details.amf_encoder_name,
                                  &amf_encoder) != AMF_OK) {
    throw std::runtime_error("AMFFactory::CreateComponent");
  }
  apply_settings(obs_data, obs_encoder);
  if (amf_encoder->Init(surface_format, width, height) != AMF_OK) {
    throw std::runtime_error("AMFComponent::Init");
  }

  texture_encoder.emplace(amf_context, d11_device, d11_context, width, height,
                          surface_format);

  set_extra_data();
}

void Encoder::initialize_dx11() {
  obs_video_info info;
  if (!obs_get_video_info(&info)) {
    throw std::runtime_error("obs_get_video_info");
  }
  CComPtr<IDXGIFactory> d11_factory;
  if (CreateDXGIFactory1(IID_PPV_ARGS(&d11_factory)) < 0) {
    throw std::runtime_error("CreateDXGIFactory1");
  }
  CComPtr<IDXGIAdapter> adapter;
  if (d11_factory->EnumAdapters(info.adapter, &adapter) < 0) {
    throw std::runtime_error("EnumAdapters");
  }
  DXGI_ADAPTER_DESC desc;
  adapter->GetDesc(&desc);
  // TODO: what is this constant?
  if (desc.VendorId != 0x1002) {
    throw std::runtime_error(fmt::format("invalid vendor {}", desc.VendorId));
  }
  if (D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr,
                        0, D3D11_SDK_VERSION, &d11_device, nullptr,
                        &d11_context) < 0) {
    throw std::runtime_error("D3D11CreateDevice");
  }
}

void Encoder::apply_settings(obs_data &data, obs_encoder &obs_encoder) {
  const auto *const encoder_video = obs_encoder_video(&obs_encoder);
  ASSERT_(encoder_video);
  const auto &voi = *video_output_get_info(encoder_video);
  width = obs_encoder_get_width(&obs_encoder);
  height = obs_encoder_get_height(&obs_encoder);
  surface_format = obs_format_to_amf(voi.format);

  configure_encoder_with_obs_user_settings(*amf_encoder, data);
  // important for rate control
  set_property_(*amf_encoder, details.frame_rate_property,
                AMFConstructRate(voi.fps_num, voi.fps_den));

  const auto color = obs_color_space_to_amf(voi.colorspace, voi.range);
  set_color_range(*amf_encoder, color.range);
  set_property_(*amf_encoder, details.input_color_properties.profile,
                static_cast<int64_t>(color.profile));
  set_property_(*amf_encoder,
                details.input_color_properties.transfer_characteristic,
                static_cast<int64_t>(color.transfer_characteristic));
  set_property_(*amf_encoder, details.input_color_properties.primaries,
                static_cast<int64_t>(color.primaries));
  // Unsure whether needs to be set on output.
  set_property_(*amf_encoder, details.output_color_properties.profile,
                static_cast<int64_t>(color.profile));
  set_property_(*amf_encoder,
                details.output_color_properties.transfer_characteristic,
                static_cast<int64_t>(color.transfer_characteristic));
  set_property_(*amf_encoder, details.output_color_properties.primaries,
                static_cast<int64_t>(color.primaries));
}

void Encoder::set_extra_data() {
  const auto variant{
      get_property<amf::AMFVariant>(*amf_encoder, details.extra_data_property)};
  if (variant.type != amf::AMF_VARIANT_INTERFACE) {
    throw std::runtime_error("extradata property is not interface");
  }
  auto &buffer{*static_cast<amf::AMFBuffer *>(variant.pInterface)};
  const auto size{buffer.GetSize()};
  extra_data.resize(size);
  std::memcpy(extra_data.data(), buffer.GetNative(), size);
}
bool Encoder::encode(SurfaceType surface_type, encoder_packet &packet,
                     bool &received_packet) noexcept {
  // The OBS encoder interface expects one input frame to be immediately
  // encoded and returned.
  //
  // The AMF interface expects that it is allowed to take multiple input
  // frames before returning potentially multiple output frames. There is
  // still a one to one correspondence but it is not clear when output
  // frames become available. Retrieval has to be performed through polling.
  //
  // To workaround this mismatch, for every call to this function, we
  // attempt one encode and one retrieval. This naturally connects the
  // polling nature of AMF with OBS. It has the downside that it introduces
  // at least one frame delay and potentially more when the encoder decides
  // it wants to wait for more input frames. This delay is acceptable.
  //
  // An alternative approach to eliminate the one frame delay (but not the
  // larger) is to poll for some fixed amount after submitting. This has the
  // downside of being more complex and blocking the return of this function
  // while polling.
  //
  // We attempt to retrive a packet first before submitting a new frame
  // because this ensures that we cannot run into a full input queue.
  try {
    received_packet = retrieve_packet_from_encoder(packet);
  } catch (const std::exception &e) {
    log(LOG_ERROR, "Error: retrieve_packet_from_encoder: {}", e.what());
    return false;
  }
  try {
    send_frame_to_encoder(surface_type);
  } catch (const std::exception &e) {
    log(LOG_ERROR, "Error: send_frame_to_encoder: {}", e.what());
    return false;
  }
  return true;
}

void Encoder::send_frame_to_encoder(SurfaceType surface_type) {
  amf::AMFSurfacePtr surface;
  uint64_t pts;
  if (auto s = std::get_if<CpuSurface>(&surface_type)) {
    surface = obs_frame_to_surface(*(s->frame));
    pts = s->frame->pts;
  } else if (auto s = std::get_if<GpuSurface>(&surface_type)) {
    surface = obs_texture_to_surface(s->handle, s->lock_key, *(s->next_key));
    pts = s->pts;
  } else {
    ASSERT_(false);
  }
  set_property(*surface, pts_property, pts);
  // We do not handle AMF_INPUT_FULL because the input queue should never be
  // full.
  const auto result = amf_encoder->SubmitInput(surface);
  switch (result) {
  case AMF_OK:
    break;
  case AMF_NEED_MORE_INPUT:
    log(LOG_DEBUG, "need more input");
    break;
  default:
    throw std::runtime_error(fmt::format("SubmitInput: {}", result));
  }
}

amf::AMFSurfacePtr Encoder::obs_frame_to_surface(const encoder_frame &frame) {
  amf::AMFSurfacePtr surface;
  // Need host memory so that we can write into it.
  if (amf_context->AllocSurface(amf::AMF_MEMORY_HOST, surface_format, width,
                                height, &surface) != AMF_OK) {
    throw std::runtime_error("context->AllocSurface");
  }
  copy_obs_frame_to_amf_surface(frame, *surface);
  return surface;
}

amf::AMFSurfacePtr Encoder::obs_texture_to_surface(uint32_t handle,
                                                   uint64_t lock_key,
                                                   uint64_t &next_key) {
  ASSERT_(texture_encoder);
  return texture_encoder->texture_to_surface(handle, lock_key, next_key);
}

// Returns whether a packet was received.
bool Encoder::retrieve_packet_from_encoder(encoder_packet &packet) {
  amf::AMFDataPtr data;
  const auto result = amf_encoder->QueryOutput(&data);
  if (result == AMF_REPEAT) {
    log(LOG_DEBUG, "repeat");
    return false;
  } else if (result != AMF_OK) {
    throw std::runtime_error(fmt::format("QueryOutput: {}", result));
  }
  if (!data) {
    log(LOG_DEBUG, "no data");
    return false;
  }
  amf::AMFBufferPtr buffer{data};

  const auto size = buffer->GetSize();
  packet_buffer.resize(size);
  std::memcpy(packet_buffer.data(), buffer->GetNative(), size);
  packet.data = packet_buffer.data();
  packet.size = size;

  packet.pts = get_property<int64_t>(*buffer, pts_property);
  // TODO: What is the correct DTS? The PTS of the start of the current GOP? But
  // that would only apply for "closed GOPS" with IDR frames.
  packet.dts = packet.pts;

  // packet.timebase_* is not set because it is not set by other encoders
  // either and seems to be initialized before the packet is sent to us
  // anyway. We reuse the input PTS so we assume the default timebase
  // matches.

  packet.type = OBS_ENCODER_VIDEO;

  const auto packet_info = get_packet_info(*buffer);
  packet.keyframe = packet_info.is_key_frame;

  log(LOG_DEBUG, "packet pts {} keyframe {} size {}", packet.pts,
      packet.keyframe, packet.size);
  return true;
}

std::span<uint8_t> Encoder::get_extra_data() noexcept { return extra_data; }
