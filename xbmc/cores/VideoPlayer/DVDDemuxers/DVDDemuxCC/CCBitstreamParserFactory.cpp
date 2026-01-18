/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CCBitstreamParserFactory.h"

#include "H264AVCCBitstreamParser.h"
#include "H264AnnexBBitstreamParser.h"
#include "MPEG2CCBitstreamParser.h"
#include "utils/log.h"

std::unique_ptr<ICCBitstreamParser> CCBitstreamParserFactory::CreateParser(
    AVCodecID codec, std::span<const uint8_t> extradata)
{
  if (codec == AV_CODEC_ID_MPEG2VIDEO)
  {
    CLog::Log(LOGDEBUG, "CCBitstreamParserFactory: Creating MPEG2CCBitstreamParser");
    return std::make_unique<CMPEG2CCBitstreamParser>();
  }
  else if (codec == AV_CODEC_ID_H264)
  {
    // Detect AVCC vs Annex B format from extradata
    if (extradata.size() >= 7)
    {
      // AVCC format: first byte is 0x01 (AVCC version)
      if (extradata[0] == 1)
      {
        CLog::Log(
            LOGDEBUG,
            "CCBitstreamParserFactory: Creating H264AVCCBitstreamParser (detected AVCC format)");
        return std::make_unique<CH264AVCCBitstreamParser>();
      }
    }
    CLog::Log(
        LOGDEBUG,
        "CCBitstreamParserFactory: Creating H264AnnexBBitstreamParser (detected Annex B format)");
    return std::make_unique<CH264AnnexBBitstreamParser>();
  }

  CLog::Log(LOGWARNING, "CCBitstreamParserFactory: Unsupported codec for CC parsing: {}", codec);
  return nullptr;
}
