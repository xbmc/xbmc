/*
 *  Copyright (C) 2013-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


 /**
 * \file media\hwdecoder\DecoderFilterManager.cpp
 * \brief Implements CDecoderFilterManager class.
 *
 */

#include "MediaCodecDecoderFilterManager.h"
#include <androidjni/MediaCodecList.h>
#include "utils/log.h"


CMediaCodecDecoderFilterManager::CMediaCodecDecoderFilterManager()
{
  static const char *blacklisted_decoders[] = {
    // No software decoders
    "OMX.google",
    // For Rockchip non-standard components
    "AVCDecoder",
    "AVCDecoder_FLASH",
    "FLVDecoder",
    "M2VDecoder",
    "M4vH263Decoder",
    "RVDecoder",
    "VC1Decoder",
    "VPXDecoder",
    // End of Rockchip
    NULL
  };

  unsigned int num_codecs = CJNIMediaCodecList::getCodecCount();
  for (int i = 0; i < num_codecs; i++)
  {
    CJNIMediaCodecInfo codec_info = CJNIMediaCodecList::getCodecInfoAt(i);
    if (codec_info.isEncoder())
      continue;

    std::string codecname = codec_info.getName();
    uint32_t flags = CDecoderFilter::FLAG_GENERAL_ALLOWED | CDecoderFilter::FLAG_DVD_ALLOWED;
    for (const char **ptr = blacklisted_decoders; *ptr && flags; ptr++)
    {
      if (!strnicmp(*ptr, codecname.c_str(), strlen(*ptr)))
        flags = 0;
    }
    add(CDecoderFilter(codecname, flags, 0));
    CLog::Log(LOGNOTICE, "Mediacodec decoder: %s", codecname.c_str());
  }
  Save();
}

