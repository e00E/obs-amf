#include "encoder_avc.h"

#include "util.h"

#include <AMF/components/VideoEncoderVCE.h>

#include <limits>

void EncoderAvc::configure_encoder_with_obs_user_settings(
    amf::AMFComponent &encoder, obs_data &obs_data) {
  set_property_(encoder, AMF_VIDEO_ENCODER_USAGE,
                AMF_VIDEO_ENCODER_USAGE_TRANSCONDING);
  for (const auto &setting : settings) {
    setting->amf_property(obs_data, encoder);
  }
}

void EncoderAvc::set_color_range(amf::AMFPropertyStorage &encoder,
                                 ColorRange color_range) {
  set_property(encoder, AMF_VIDEO_ENCODER_FULL_RANGE_COLOR,
               color_range == ColorRange::Full);
}

PacketInfo EncoderAvc::get_packet_info(amf::AMFPropertyStorage &output) {
  const auto packet_type{
      get_property<int64_t>(output, AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE)};
  switch (packet_type) {
  case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR:
    return {true};
  case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_I:
    return {true};
  case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_P:
    return {false};
  case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_B:
    return {false};
  }
  throw std::runtime_error(fmt::format("unknown packet type {}", packet_type));
}

EncoderAvc::EncoderAvc(obs_data &obs_data, obs_encoder &obs_encoder)
    : Encoder(obs_data, obs_encoder,
              {
                  .amf_encoder_name = AMFVideoEncoderVCE_AVC,
                  .extra_data_property = AMF_VIDEO_ENCODER_EXTRADATA,
                  .frame_rate_property = AMF_VIDEO_ENCODER_FRAMERATE,
                  .input_color_properties =
                      {.profile = AMF_VIDEO_ENCODER_INPUT_COLOR_PROFILE,
                       .transfer_characteristic =
                           AMF_VIDEO_ENCODER_INPUT_TRANSFER_CHARACTERISTIC,
                       .primaries = AMF_VIDEO_ENCODER_INPUT_COLOR_PRIMARIES},
                  .output_color_properties =
                      {.profile = AMF_VIDEO_ENCODER_OUTPUT_COLOR_PROFILE,
                       .transfer_characteristic =
                           AMF_VIDEO_ENCODER_OUTPUT_TRANSFER_CHARACTERISTIC,
                       .primaries = AMF_VIDEO_ENCODER_OUTPUT_COLOR_PRIMARIES},
              }) {}

namespace {

using S = std::unique_ptr<const Setting>;

const S settings_[] = {
    S{new EnumSetting{
        "profile",
        "Profile",
        AMF_VIDEO_ENCODER_PROFILE,
        {{AMF_VIDEO_ENCODER_PROFILE_BASELINE, "Baseline"},
         {AMF_VIDEO_ENCODER_PROFILE_MAIN, "Main"},
         {AMF_VIDEO_ENCODER_PROFILE_HIGH, "High"},
         {AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_BASELINE,
          "Constrained Baseline"},
         {AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_HIGH, "Constrained High"}},
        1}},
    S{new EnumSetting{"tier",
                      "Tier",
                      AMF_VIDEO_ENCODER_PROFILE_LEVEL,
                      {
                          {10, "1"},
                          {11, "1.1"},
                          {12, "1.2"},
                          {13, "1.3"},
                          {2, "2"},
                          {21, "2.1"},
                          {22, "2.2"},
                          {3, "3"},
                          {31, "3.1"},
                          {32, "3.2"},
                          {4, "4"},
                          {41, "4.1"},
                          {42, "4.2"},
                      },
                      12}},
    // Do not allow setting MAX_LTR_FRAMES because the way the documentation
    // reads setting to a nonzero values only makes sense if we manually managed
    // them on a per frame basis which we do not.
    S{new BoolSetting{"low latency mode", "Low Latency Mode",
                      AMF_VIDEO_ENCODER_LOWLATENCY_MODE, false}},
    S{new EnumSetting{
        "rate control method",
        "Rate Control Method",
        AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD,
        {{AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CONSTANT_QP, "Constant QP"},
         {AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CBR, "Constant Bit Rate"},
         {AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR,
          "Peak Constrainer Variable Bit Rate"},
         {AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR,
          "Latency Constrained Variable Bit Rate"},
         {AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_QUALITY_VBR,
          "Quality Variable Bit Rate"}},
        0}},
    S{new IntSetting{"target bit rate", "Target Bit Rate",
                     AMF_VIDEO_ENCODER_TARGET_BITRATE, 1,
                     std::numeric_limits<int>::max(), 10000000}},
    S{new IntSetting{"peak bit rate", "Peak Bit Rate",
                     AMF_VIDEO_ENCODER_PEAK_BITRATE, 1,
                     std::numeric_limits<int>::max(), 30000000}},
    S{new BoolSetting{"skip frame enable", "Skip Frames for Rate Control",
                      AMF_VIDEO_ENCODER_RATE_CONTROL_SKIP_FRAME_ENABLE, false}},
    S{new IntSetting{"min qp i", "Minimum I Frame QP", AMF_VIDEO_ENCODER_MIN_QP,
                     0, 51, 18}},
    S{new IntSetting{"max qp i", "Maximum I Frame QP", AMF_VIDEO_ENCODER_MAX_QP,
                     0, 51, 46}},
    S{new IntSetting{"qp i", "CQP: I Frame QP", AMF_VIDEO_ENCODER_QP_I, 0, 51,
                     26}},
    S{new IntSetting{"qp p", "CQP: P Frame QP", AMF_VIDEO_ENCODER_QP_P, 0, 51,
                     26}},
    S{new IntSetting{"qp b", "CQP: B Frame QP", AMF_VIDEO_ENCODER_QP_B, 0, 51,
                     26}},
    S{new IntSetting{"vbv buffer size", "VBV Buffer Size",
                     AMF_VIDEO_ENCODER_VBV_BUFFER_SIZE, 1,
                     std::numeric_limits<int>::max(), 20000000}},
    S{new IntSetting{"initial vbv buffer fullness",
                     "Initial VBV Buffer Fullness",
                     AMF_VIDEO_ENCODER_INITIAL_VBV_BUFFER_FULLNESS, 0, 64, 64}},
    S{new BoolSetting{"enforce hrd", "Enforce Hypothetical Reference Decoder",
                      AMF_VIDEO_ENCODER_ENFORCE_HRD, false}},
    S{new BoolSetting{"preencode enable", "Pre-analysis Assisted Rate Control",
                      AMF_VIDEO_ENCODER_PREENCODE_ENABLE, false}},
    // https://github.com/GPUOpen-LibrariesAndSDKs/AMF/issues/72
    S{new BoolSetting{"enable vbaq",
                      "Enable Variance Based Adaptive Quantization",
                      AMF_VIDEO_ENCODER_ENABLE_VBAQ, false}},
    S{new BoolSetting{"filler data enable", "Filler Data for CBR",
                      AMF_VIDEO_ENCODER_FILLER_DATA_ENABLE, false}},
    // Skipping MAX_AU_SIZE because we couldn't find what it means.
    S{new IntSetting{"max num reframes", "Maximum Reference Frames",
                     AMF_VIDEO_ENCODER_MAX_NUM_REFRAMES, 0, 16, 1}},
    // The following block can only be set when max num reframes is > 1.
    // Skipping them because need to find a clean way to conditionally disable
    // them and they are conditionally disable them and they are niche.
    /*
    result.emplace_back(new IntSetting{"b pic delta qp", "B Frame Delta QP",
                                       AMF_VIDEO_ENCODER_B_PIC_DELTA_QP, -10,
    10, 4}); result.emplace_back( new IntSetting{"ref b pic delta qp",
    "Reference B Frame Delta QP", AMF_VIDEO_ENCODER_REF_B_PIC_DELTA_QP, -10, 10,
    2}); result.emplace_back(new IntSetting{ "intra refresh num mbs per slot",
    "Intra Refresh Macroblocks Per Slot",
        AMF_VIDEO_ENCODER_INTRA_REFRESH_NUM_MBS_PER_SLOT, 0,
        std::numeric_limits<int>::max(), 0});
    result.emplace_back(new IntSetting{"b pic pattern", "B Frame Pattern",
                                       AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0, 3,
    3}); result.emplace_back( new BoolSetting{"b reference enable", "Use B
    Frames As References", AMF_VIDEO_ENCODER_B_REFERENCE_ENABLE, true});
    */
    S{new IntSetting{"header insertion spacing", "Header Insertion Spacing",
                     AMF_VIDEO_ENCODER_HEADER_INSERTION_SPACING, 0, 1000, 0}},
    S{new IntSetting{"idr period", "IDR Period", AMF_VIDEO_ENCODER_IDR_PERIOD,
                     0, 1000, 30}},
    S{new IntSetting{"slices per frame", "Slices Per Frame",
                     AMF_VIDEO_ENCODER_SLICES_PER_FRAME, 1,
                     std::numeric_limits<int>::max(), 1}},
    S{new BoolSetting{"deblocking filter", "Deblocking Filter",
                      AMF_VIDEO_ENCODER_DE_BLOCKING_FILTER, true}},
    S{new EnumSetting{"cabac enable",
                      "CABAC",
                      AMF_VIDEO_ENCODER_CABAC_ENABLE,
                      {{AMF_VIDEO_ENCODER_UNDEFINED, "Undefined"},
                       {AMF_VIDEO_ENCODER_CABAC, "CABAC"},
                       {AMF_VIDEO_ENCODER_CALV, "CALV"}},
                      0}},
    S{new EnumSetting{"scantype",
                      "Scan Type",
                      AMF_VIDEO_ENCODER_SCANTYPE,
                      {{0, "Progressive"}, {1, "Interlaced"}},
                      0}},
    S{new EnumSetting{"quality preset",
                      "Quality Preset",
                      AMF_VIDEO_ENCODER_QUALITY_PRESET,
                      {{AMF_VIDEO_ENCODER_QUALITY_PRESET_QUALITY, "Quality"},
                       {AMF_VIDEO_ENCODER_QUALITY_PRESET_BALANCED, "Balanced"},
                       {AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED, "Speed"}},
                      1}},
    S{new BoolSetting{"motion half pixel", "Half Pixel Motion Estimation",
                      AMF_VIDEO_ENCODER_MOTION_HALF_PIXEL, true}},
    S{new BoolSetting{"motion quarter pixel", "Quarter Pixel Motion Estimation",
                      AMF_VIDEO_ENCODER_MOTION_QUARTERPIXEL, true}},
    S{new EnumSetting{"bit depth",
                      "Bit Depth",
                      AMF_VIDEO_ENCODER_COLOR_BIT_DEPTH,
                      {{8, "8"}, {10, "10"}, {16, "16"}},
                      0}},
};

} // namespace

const std::span<const S> EncoderAvc::settings{settings_};
