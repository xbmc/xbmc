/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecTX3G.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitles/SubtitlesStyle.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/CharArrayParser.h"
#include "utils/ColorUtils.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <vector>

// 3GPP/TX3G (aka MPEG-4 Timed Text) Subtitle support
// 3GPP -> 3rd Generation Partnership Program
// adapted from https://github.com/HandBrake/HandBrake/blob/master/libhb/dectx3gsub.c;

namespace
{
enum FaceStyleFlag
{
  BOLD = 0x1,
  ITALIC = 0x2,
  UNDERLINE = 0x4
};

struct StyleRecord
{
  uint16_t startChar; // index in terms of character (not byte) position
  uint16_t endChar; // index in terms of character (not byte) position
  uint16_t fontID;
  uint8_t faceStyleFlags; // FaceStyleFlag
  uint8_t fontSize;
  UTILS::COLOR::Color textColorARGB;
  unsigned int textColorAlphaCh;
};

constexpr uint32_t BOX_TYPE_UUID = StreamUtils::MakeFourCC('u', 'u', 'i', 'd');
constexpr uint32_t BOX_TYPE_STYL = StreamUtils::MakeFourCC('s', 't', 'y', 'l'); // TextStyleBox

void ConvertStyleToTags(std::string& strUTF8, const StyleRecord& style, bool closingTags)
{
  if (style.faceStyleFlags & BOLD)
    strUTF8.append(closingTags ? "{\\b0}" : "{\\b1}");
  if (style.faceStyleFlags & ITALIC)
    strUTF8.append(closingTags ? "{\\i0}" : "{\\i1}");
  if (style.faceStyleFlags & UNDERLINE)
    strUTF8.append(closingTags ? "{\\u0}" : "{\\u1}");
  if (style.textColorARGB != UTILS::COLOR::WHITE)
  {
    if (closingTags)
      strUTF8 += "{\\c}";
    else
    {
      UTILS::COLOR::Color color = UTILS::COLOR::ConvertToBGR(style.textColorARGB);
      strUTF8 += StringUtils::Format("{{\\c&H{:06x}&}}", color);
    }
  }
  if (style.textColorAlphaCh != 255)
  {
    // Libass use inverted alpha channel 0==opaque
    unsigned int alpha = 0;
    if (!closingTags)
      alpha = 255 - style.textColorAlphaCh;
    strUTF8 += StringUtils::Format("{{\\1a&H{:02x}&}}", alpha);
  }
}
} // unnamed namespace

CDVDOverlayCodecTX3G::CDVDOverlayCodecTX3G() : CDVDOverlayCodec("TX3G Subtitle Decoder")
{
}

bool CDVDOverlayCodecTX3G::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (hints.codec != AV_CODEC_ID_MOV_TEXT)
    return false;

  m_pOverlay.reset();

  return Initialize();
}

OverlayMessage CDVDOverlayCodecTX3G::Decode(DemuxPacket* pPacket)
{
  double PTSStartTime = 0;
  double PTSStopTime = 0;

  CDVDOverlayCodec::GetAbsoluteTimes(PTSStartTime, PTSStopTime, pPacket);

  char* data = reinterpret_cast<char*>(pPacket->pData);

  // Parse the packet as a TX3G TextSample.
  CCharArrayParser sampleData;
  sampleData.Reset(data, pPacket->iSize);

  uint16_t textLength = 0;
  char* text = nullptr;
  if (sampleData.CharsLeft() >= 2)
    textLength = sampleData.ReadNextUnsignedShort();
  if (sampleData.CharsLeft() >= textLength)
  {
    text = data + sampleData.GetPosition();
    sampleData.SkipChars(textLength);
  }
  if (!text)
    return OverlayMessage::OC_ERROR;

  std::vector<StyleRecord> styleRecords;

  // Read all TextSampleModifierBox types
  while (sampleData.CharsLeft() > 0)
  {
    if (sampleData.CharsLeft() < MP4_BOX_HEADER_SIZE)
    {
      CLog::Log(LOGWARNING, "{} - Incomplete box header found", __FUNCTION__);
      break;
    }

    uint32_t boxSize = sampleData.ReadNextUnsignedInt();
    uint32_t boxType = sampleData.ReadNextUnsignedInt();

    if (boxType == BOX_TYPE_UUID)
    {
      CLog::Log(LOGDEBUG, "{} - Sample data has unsupported extended type 'uuid'", __FUNCTION__);
    }
    else if (boxType == BOX_TYPE_STYL)
    {
      // Parse the contained StyleRecords
      if (styleRecords.size() != 0)
      {
        CLog::Log(LOGDEBUG, "{} - Found additional TextStyleBox, skipping", __FUNCTION__);
        sampleData.SkipChars(boxSize - MP4_BOX_HEADER_SIZE);
        continue;
      }

      if (sampleData.CharsLeft() < 2)
      {
        CLog::Log(LOGWARNING, "{} - Incomplete TextStyleBox header found", __FUNCTION__);
        return OverlayMessage::OC_ERROR;
      }
      uint16_t styleCount = sampleData.ReadNextUnsignedShort();

      // Get the data of each style record
      // Each style is ordered by starting character offset, and the starting
      // offset of one style record shall be greater than or equal to the
      // ending character offset of the preceding record,
      // styles records shall not overlap their character ranges.
      for (int i = 0; i < styleCount; i++)
      {
        if (sampleData.CharsLeft() < 12)
        {
          CLog::Log(LOGWARNING, "{} - Incomplete StyleRecord found, skipping", __FUNCTION__);
          sampleData.SkipChars(sampleData.CharsLeft());
          continue;
        }

        StyleRecord styleRec;
        styleRec.startChar = sampleData.ReadNextUnsignedShort();
        styleRec.endChar = sampleData.ReadNextUnsignedShort();
        styleRec.fontID = sampleData.ReadNextUnsignedShort();
        styleRec.faceStyleFlags = sampleData.ReadNextUnsignedChar();
        styleRec.fontSize = sampleData.ReadNextUnsignedChar();
        styleRec.textColorARGB = UTILS::COLOR::ConvertToARGB(sampleData.ReadNextUnsignedInt());
        styleRec.textColorAlphaCh = (styleRec.textColorARGB & 0xFF000000) >> 24;
        // clamp bgnChar/bgnChar to textLength, we alloc enough space above and
        // this fixes broken encoders that do not handle endChar correctly.
        if (styleRec.startChar > textLength)
          styleRec.startChar = textLength;
        if (styleRec.endChar > textLength)
          styleRec.endChar = textLength;

        // Skip zero-length style
        if (styleRec.startChar == styleRec.endChar)
          continue;

        styleRecords.emplace_back(styleRec);
      }
    }
    else
    {
      // Other types of TextSampleModifierBox are not supported
      sampleData.SkipChars(boxSize - MP4_BOX_HEADER_SIZE);
    }
  }

  uint16_t charIndex = 0;
  size_t styleIndex = 0;
  std::string strUTF8;
  bool skipChars = false;
  // Parse the text to add the converted styles records,
  // index over textLength chars to include broken encoders,
  // so we pickup closing styles on broken encoders
  for (char* curPos = text; curPos <= text + textLength; curPos++)
  {
    if ((*curPos & 0xC0) == 0x80)
    {
      // Is a non-first byte of a multi-byte UTF-8 character
      strUTF8.append(static_cast<const char*>(curPos), 1);
      continue; // ...without incrementing 'charIndex'
    }

    // Go through styles, a style can end where another one begins
    while (styleIndex < styleRecords.size())
    {
      if (styleRecords[styleIndex].startChar == charIndex)
      {
        ConvertStyleToTags(strUTF8, styleRecords[styleIndex], false);
        break;
      }
      else if (styleRecords[styleIndex].endChar == charIndex)
      {
        ConvertStyleToTags(strUTF8, styleRecords[styleIndex], true);
        styleIndex++;
      }
      else
        break;
    }

    if (*curPos == '{') // erase unsupported tags
      skipChars = true;

    // Skip all \r because it causes the line to display empty box "tofu"
    if (!skipChars && *curPos != '\0' && *curPos != '\r')
      strUTF8.append(static_cast<const char*>(curPos), 1);

    if (*curPos == '}')
      skipChars = false;

    charIndex++;
  }

  if (strUTF8.empty())
    return OverlayMessage::OC_BUFFER;

  AddSubtitle(strUTF8, PTSStartTime, PTSStopTime);

  return m_pOverlay ? OverlayMessage::OC_DONE : OverlayMessage::OC_OVERLAY;
}

void CDVDOverlayCodecTX3G::PostProcess(std::string& text)
{
  if (text[text.size() - 1] == '\n')
    text.erase(text.size() - 1);
  CSubtitlesAdapter::PostProcess(text);
}

void CDVDOverlayCodecTX3G::Reset()
{
  Flush();
}

void CDVDOverlayCodecTX3G::Flush()
{
  m_pOverlay.reset();
  FlushSubtitles();
}

std::shared_ptr<CDVDOverlay> CDVDOverlayCodecTX3G::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  return m_pOverlay;
}
