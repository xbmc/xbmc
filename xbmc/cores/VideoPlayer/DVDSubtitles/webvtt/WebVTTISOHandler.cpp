/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebVTTISOHandler.h"

#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/CharArrayParser.h"
#include "utils/CharsetConverter.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstring>
#include <stdint.h>

// WebVTT in MP4 encapsulated subtitles (ISO/IEC 14496-30:2014)
// This format type that differ from the text WebVTT, it makes use
// of ISO BMFF byte stream where the data are enclosed in boxes (first 4 byte
// specify the size and next 4 byte specify the type).
// Start/Stop times info are not included, the start time is the current pts
// provided from the decoder, the stop time is defined from the pts of the next
// packages (depends from the box type).

namespace
{
constexpr int defaultSubDuration = 20 * static_cast<int>(DVD_TIME_BASE);
// VTTEmptyCueBox
constexpr uint32_t ISO_BOX_TYPE_VTTE = StreamUtils::MakeFourCC('v', 't', 't', 'e');
// VTTCueBox
constexpr uint32_t ISO_BOX_TYPE_VTTC = StreamUtils::MakeFourCC('v', 't', 't', 'c');
// VTTContinuationCueBox
constexpr uint32_t ISO_BOX_TYPE_VTTX = StreamUtils::MakeFourCC('v', 't', 't', 'x');
// CueIDBox
constexpr uint32_t ISO_BOX_TYPE_IDEN = StreamUtils::MakeFourCC('i', 'd', 'e', 'n');
// CueSettingsBox
constexpr uint32_t ISO_BOX_TYPE_STTG = StreamUtils::MakeFourCC('s', 't', 't', 'g');
// CuePayloadBox
constexpr uint32_t ISO_BOX_TYPE_PAYL = StreamUtils::MakeFourCC('p', 'a', 'y', 'l');
} // unnamed namespace


void CWebVTTISOHandler::DecodeStream(const char* buffer,
                                     int bufferSize,
                                     double pts,
                                     std::vector<subtitleData>* subList,
                                     double& prevSubStopTime)
{
  CCharArrayParser sampleData;
  sampleData.Reset(buffer, bufferSize);

  // A sample data package can contain:
  // - One VTTE
  // or
  // - One or more VTTC and/or VTTX (where all share the same start/stop times)

  while (sampleData.CharsLeft() > 0)
  {
    if (sampleData.CharsLeft() < MP4_BOX_HEADER_SIZE)
    {
      CLog::Log(LOGWARNING, "{} - Incomplete box header found", __FUNCTION__);
      break;
    }

    uint32_t boxSize = sampleData.ReadNextUnsignedInt();
    uint32_t boxType = sampleData.ReadNextUnsignedInt();
    if (boxType == ISO_BOX_TYPE_VTTE)
    {
      // VTTE is used to set the stop time value to previously subtitles
      prevSubStopTime = pts;
      sampleData.SkipChars(boxSize - MP4_BOX_HEADER_SIZE);
    }
    else if (boxType == ISO_BOX_TYPE_VTTC)
    {
      // VTTC can be used to set the stop time value to previously subtitles
      prevSubStopTime = pts;
      m_subtitleData = subtitleData();
      m_subtitleData.startTime = pts;
      m_subtitleData.stopTime = pts + defaultSubDuration;
      if (ParseVTTCueBox(sampleData, boxSize - MP4_BOX_HEADER_SIZE, subList))
        subList->emplace_back(m_subtitleData);
    }
    else if (boxType == ISO_BOX_TYPE_VTTX)
    {
      // VTTX could be used to set the stop time value to previously subtitles
      prevSubStopTime = pts;
      m_subtitleData = subtitleData();
      m_subtitleData.startTime = pts;
      m_subtitleData.stopTime = pts + defaultSubDuration;
      if (ParseVTTCueBox(sampleData, boxSize - MP4_BOX_HEADER_SIZE, subList))
        subList->emplace_back(m_subtitleData);
    }
    else
    {
      // Skip unsupported box types
      sampleData.SkipChars(boxSize - MP4_BOX_HEADER_SIZE);
    }
  }
}

bool CWebVTTISOHandler::ParseVTTCueBox(CCharArrayParser& sampleData,
                                       int remainingCueBoxChars,
                                       std::vector<subtitleData>* subList)
{
  std::string cueId;
  std::string cueSettings;
  std::string subtitleText;

  // No order is imposed between box types,
  // so we have to process the data after retrieving them.
  while (remainingCueBoxChars > 0)
  {
    if (remainingCueBoxChars < MP4_BOX_HEADER_SIZE)
    {
      CLog::Log(LOGWARNING, "{} - Incomplete VTT Cue box header found", __FUNCTION__);
      return false;
    }
    uint32_t boxSize = sampleData.ReadNextUnsignedInt();
    uint32_t boxType = sampleData.ReadNextUnsignedInt();
    int payloadLength = boxSize - MP4_BOX_HEADER_SIZE;
    remainingCueBoxChars -= MP4_BOX_HEADER_SIZE;
    remainingCueBoxChars -= payloadLength;
    std::string payload = sampleData.ReadNextString(payloadLength);

    if (boxType == ISO_BOX_TYPE_IDEN) // Optional
    {
      cueId = payload;
    }
    else if (boxType == ISO_BOX_TYPE_STTG) // Optional
    {
      cueSettings = payload;
    }
    else if (boxType == ISO_BOX_TYPE_PAYL)
    {
      subtitleText = payload;
    }
  }

  m_subtitleData.cueSettings.id = cueId;
  GetCueSettings(cueSettings);
  CalculateTextPosition(subtitleText);
  ConvertSubtitle(subtitleText);
  m_subtitleData.text = subtitleText;

  return true;
}
