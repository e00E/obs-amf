# Introduction

This is an [OBS](https://obsproject.com/) encoder plugin leveraging hardware acceleration on AMD GPUs through [AMF](https://github.com/GPUOpen-LibrariesAndSDKs/AMF). It has the following features:

- AVC (H264) support
- HEVC (H265) support
- ability to set almost all AMF encoder settings
- texture based encoding

It was made because the [existing](https://github.com/obsproject/obs-amd-encoder) plugin is mostly unmaintained and in a state of [decay](https://github.com/obsproject/obs-amd-encoder/issues/400). I am very thankful for the original plugin. This would not have been possible without it.

It is in functioning condition but mostly motivated by my personal use case. I am unsure how much work I want to put into making it easy to use for non technical users.

# Code

In contrast to other OBS related code I wanted to:
- use modern C++20 instead of C style C++
- follow the [Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- make use of the [Guidelines Support Library](https://github.com/microsoft/GSL) and [{fmt}](https://fmt.dev/latest/index.html)

# Building

- Git clone [obs-studio](https://github.com/obsproject/obs-studio).
- Place this folder into `obs-studio/plugins/`
- Append `add_subdirectory(amftest)` to `obs-studio/plugins/CMakeLists.txt`.
- Build OBS as you usually would and see that this plugin shows up as a project in Visual Studio.

I would like to:
- Build as a standalone project instead of intrusively integrating with obs-studio.
- Set up CI to perform automated builds and release of artifacts.

# TODOs

- Set detailed (hover) descriptions for settings.
- Query capabilities for some settings to determine maximum values.
- Make settings easier to understand and prevent misconfiguration by grouping into related settings, disabling incompatible settings like different rate control methods), grouping into commonly used and expert settings.
- Allow the user to choose the GPU device. How should this interact with texture based encoding which has to be on same GPU?
- Double check video format conversions.
- Double check color space conversions. Are we using the right values for SRGB? Should we use the extra HDR settings in AMF?
- Compare to https://github.com/obsproject/obs-studio/pull/4538 which has advanced color settings and texture support.
- Compare to jim-nvenc.c which has advanced color settings and texture support.
