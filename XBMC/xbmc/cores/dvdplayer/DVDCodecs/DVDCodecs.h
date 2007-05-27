
#pragma once

// enum CodecID

extern "C" {
#define HAVE_MMX
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef _LINUX
#pragma warning(disable:4244)
#include "..\ffmpeg\avcodec.h"
#else
#include <ffmpeg/avcodec.h>
#endif
}

// 0x100000 is the video starting range

// 0x200000 is the audio starting range

// special options that can be passed to a codec
class CDVDCodecOption
{
public:
  CDVDCodecOption(std::string name, std::string value) { m_name = name; m_value = value; }
  std::string m_name;
  std::string m_value;
};
typedef std::vector<CDVDCodecOption> CDVDCodecOptions;
