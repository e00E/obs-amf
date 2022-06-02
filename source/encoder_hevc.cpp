#include "encoder_hevc.h"

#include "util.h"

#include <AMF/components/VideoEncoderHEVC.h>

#include <limits>

void EncoderHevc::configure_encoder_with_obs_user_settings(
    amf::AMFComponent &encoder, obs_data &obs_data) {
  set_property_fallible(encoder, AMF_VIDEO_ENCODER_HEVC_USAGE,
                        AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCONDING);
  for (const auto &setting : settings) {
    setting->amf_property(obs_data, encoder);
  }
}

void EncoderHevc::set_color_range(amf::AMFPropertyStorage &encoder,
                                  ColorRange color_range) {
  int64_t value;
  switch (color_range) {
  case ColorRange::Partial:
    value = AMF_VIDEO_ENCODER_HEVC_NOMINAL_RANGE_STUDIO;
  case ColorRange::Full:
    value = AMF_VIDEO_ENCODER_HEVC_NOMINAL_RANGE_FULL;
  }
  set_property_fallible(encoder, AMF_VIDEO_ENCODER_HEVC_NOMINAL_RANGE, value);
}

PacketInfo EncoderHevc::get_packet_info(amf::AMFPropertyStorage &output) {
  const auto packet_type{
      get_property<int64_t>(output, AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE)};
  switch (packet_type) {
  case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_IDR:
    return {true};
  case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_I:
    return {true};
  case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_P:
    return {false};
  }
  throw std::runtime_error(fmt::format("unknown packet type {}", packet_type));
}

EncoderHevc::EncoderHevc()
    : Encoder({
          .amf_encoder_name = AMFVideoEncoder_HEVC,
          .extra_data_property = AMF_VIDEO_ENCODER_HEVC_EXTRADATA,
          .frame_rate_property = AMF_VIDEO_ENCODER_HEVC_FRAMERATE,
          .input_color_properties =
              {.profile = AMF_VIDEO_ENCODER_HEVC_INPUT_COLOR_PROFILE,
               .transfer_characteristic =
                   AMF_VIDEO_ENCODER_HEVC_INPUT_TRANSFER_CHARACTERISTIC,
               .primaries = AMF_VIDEO_ENCODER_HEVC_INPUT_COLOR_PRIMARIES},
          .output_color_properties =
              {.profile = AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PROFILE,
               .transfer_characteristic =
                   AMF_VIDEO_ENCODER_HEVC_OUTPUT_TRANSFER_CHARACTERISTIC,
               .primaries = AMF_VIDEO_ENCODER_HEVC_OUTPUT_COLOR_PRIMARIES},
      }) {}

namespace {

using S = std::unique_ptr<const Setting>;

const S settings_[] = {
    S{new EnumSetting{"profile",
                      "Profile",
                      AMF_VIDEO_ENCODER_HEVC_PROFILE,
                      {{AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN, "Main"}},
                      0}},
    S{new EnumSetting{"tier",
                      "Tier",
                      AMF_VIDEO_ENCODER_HEVC_TIER,
                      {{AMF_VIDEO_ENCODER_HEVC_TIER_MAIN, "Main"},
                       {AMF_VIDEO_ENCODER_HEVC_TIER_HIGH, "High"}},
                      1}},
    S{new EnumSetting{"level",
                      "Level",
                      AMF_VIDEO_ENCODER_HEVC_PROFILE_LEVEL,
                      {{AMF_LEVEL_1, "1"},
                       {AMF_LEVEL_2, "2"},
                       {AMF_LEVEL_2_1, "2.1"},
                       {AMF_LEVEL_3, "3"},
                       {AMF_LEVEL_3_1, "3.1"},
                       {AMF_LEVEL_4, "4"},
                       {AMF_LEVEL_4_1, "4.1"},
                       {AMF_LEVEL_5, "5"},
                       {AMF_LEVEL_5_1, "5.1"},
                       {AMF_LEVEL_5_2, "5.2"},
                       {AMF_LEVEL_6, "6"},
                       {AMF_LEVEL_6_1, "6.1"},
                       {AMF_LEVEL_6_2, "6.2"}},
                      12}},
    // Do not allow setting MAX_LTR_FRAMES because the way the documentation
    // reads setting to a nonzero values only makes sense if we manually managed
    // them on a per frame basis which we do not.
    S{new IntSetting{"max num reframes", "Maximum Reference Frames",
                     AMF_VIDEO_ENCODER_HEVC_MAX_NUM_REFRAMES, 0, 16, 4}},
    S{new BoolSetting{"low latency mode", "Low Latency Mode",
                      AMF_VIDEO_ENCODER_HEVC_LOWLATENCY_MODE, false}},
    S{new EnumSetting{
        "rate control method",
        "Rate Control Method",
        AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD,
        {{AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CONSTANT_QP,
          "Constant QP"},
         {AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CBR, "Constant Bit Rate"},
         {AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR,
          "Peak Constrainer Variable Bit Rate"},
         {AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR,
          "Latency Constrained Variable Bit Rate"}},
        0}},
    S{new IntSetting{"target bit rate", "Target Bit Rate",
                     AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE, 1,
                     std::numeric_limits<int>::max(), 10000000}},
    S{new IntSetting{"peak bit rate", "Peak Bit Rate",
                     AMF_VIDEO_ENCODER_HEVC_PEAK_BITRATE, 1,
                     std::numeric_limits<int>::max(), 30000000}},
    S{new BoolSetting{"skip frame enable", "Skip Frames for Rate Control",
                      AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_SKIP_FRAME_ENABLE,
                      false}},
    S{new IntSetting{"min qp i", "Minimum I Frame QP",
                     AMF_VIDEO_ENCODER_HEVC_MIN_QP_I, 0, 51, 18}},
    S{new IntSetting{"max qp i", "Maximum I Frame QP",
                     AMF_VIDEO_ENCODER_HEVC_MAX_QP_I, 0, 51, 46}},
    S{new IntSetting{"min qp p", "Minimum P Frame QP",
                     AMF_VIDEO_ENCODER_HEVC_MIN_QP_P, 0, 51, 18}},
    S{new IntSetting{"max qp p", "Maximum P Frame QP",
                     AMF_VIDEO_ENCODER_HEVC_MAX_QP_P, 0, 51, 46}},
    S{new IntSetting{"qp i", "CQP: I Frame QP", AMF_VIDEO_ENCODER_HEVC_QP_I, 0,
                     51, 26}},
    S{new IntSetting{"qp p", "CQP: P Frame QP", AMF_VIDEO_ENCODER_HEVC_QP_P, 0,
                     51, 26}},
    S{new IntSetting{"vbv buffer size", "VBV Buffer Size",
                     AMF_VIDEO_ENCODER_HEVC_VBV_BUFFER_SIZE, 1,
                     std::numeric_limits<int>::max(), 20000000}},
    S{new IntSetting{
        "initial vbv buffer fullness", "Initial VBV Buffer Fullness",
        AMF_VIDEO_ENCODER_HEVC_INITIAL_VBV_BUFFER_FULLNESS, 0, 64, 64}},
    S{new BoolSetting{"enforce hrd", "Enforce Hypothetical Reference Decoder",
                      AMF_VIDEO_ENCODER_HEVC_ENFORCE_HRD, false}},
    S{new BoolSetting{"preencode enable", "Pre-analysis Assisted Rate Control",
                      AMF_VIDEO_ENCODER_HEVC_PREENCODE_ENABLE, false}},
    // https://github.com/GPUOpen-LibrariesAndSDKs/AMF/issues/72
    S{new BoolSetting{"enable vbaq",
                      "Enable Variance Based Adaptive Quantization",
                      AMF_VIDEO_ENCODER_HEVC_ENABLE_VBAQ, false}},
    S{new BoolSetting{"filler data enable", "Filler Data for CBR",
                      AMF_VIDEO_ENCODER_HEVC_FILLER_DATA_ENABLE, false}},
    // Skipping MAX_AU_SIZE because we couldn't find what it means.
    S{new EnumSetting{
        "header insertion mode",
        "Header Insertion Mode",
        AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE,
        {{AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_NONE, "None"},
         {AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_GOP_ALIGNED,
          "GOP Aligned"},
         {AMF_VIDEO_ENCODER_HEVC_HEADER_INSERTION_MODE_IDR_ALIGNED,
          "IDR Aligned"}},
        0}},
    // 0 = infinite
    S{new IntSetting{"gop size", "GOP Size", AMF_VIDEO_ENCODER_HEVC_GOP_SIZE, 0,
                     1000, 120}},
    // Frequency of IDR being inserted as start of a GOP. 0 = never.
    S{new IntSetting{"num gops per idr", "IDR GOP Interval",
                     AMF_VIDEO_ENCODER_HEVC_NUM_GOPS_PER_IDR, 0, 65535, 1}},
    S{new BoolSetting{"deblocking filter disable", "Disable Deblocking Filter",
                      AMF_VIDEO_ENCODER_HEVC_DE_BLOCKING_FILTER_DISABLE,
                      false}},
    S{new IntSetting{"slices per frame", "Slices Per Frame",
                     AMF_VIDEO_ENCODER_HEVC_SLICES_PER_FRAME, 1,
                     std::numeric_limits<int>::max(), 1}},
    S{new EnumSetting{
        "quality preset",
        "Quality Preset",
        AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET,
        {{AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_QUALITY, "Quality"},
         {AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_BALANCED, "Balanced"},
         {AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED, "Speed"}},
        1}},
    S{new BoolSetting{"motion half pixel", "Half Pixel Motion Estimation",
                      AMF_VIDEO_ENCODER_HEVC_MOTION_HALF_PIXEL, true}},
    S{new BoolSetting{"motion quarter pixel", "Quarter Pixel Motion Estimation",
                      AMF_VIDEO_ENCODER_HEVC_MOTION_QUARTERPIXEL, true}},
    S{new EnumSetting{"bit depth",
                      "Bit Depth",
                      AMF_VIDEO_ENCODER_HEVC_COLOR_BIT_DEPTH,
                      {{8, "8"}, {10, "10"}, {16, "16"}},
                      0}}};

} // namespace

const std::span<const S> EncoderHevc::settings{settings_};
