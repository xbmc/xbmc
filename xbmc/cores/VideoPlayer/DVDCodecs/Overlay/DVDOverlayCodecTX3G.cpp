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
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/auto_buffer.h"
#include "utils/log.h"

#include <cstddef>

// 3GPP/TX3G (aka MPEG-4 Timed Text) Subtitle support
// 3GPP -> 3rd Generation Partnership Program
// adapted from https://github.com/HandBrake/HandBrake/blob/master/libhb/dectx3gsub.c;

#define LEN_CHECK(x) \
  do \
  { \
    if ((end - pos) < static_cast<std::ptrdiff_t>(x)) \
      return OC_ERROR; \
  } while (0)

// NOTE: None of these macros check for buffer overflow
#define READ_U8() \
  *pos; \
  pos += 1;
#define READ_U16() \
  (pos[0] << 8) | pos[1]; \
  pos += 2;
#define READ_U32() \
  (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3]; \
  pos += 4;
#define READ_ARRAY(n) \
  pos; \
  pos += n;
#define SKIP_ARRAY(n) pos += n;

#define FOURCC(str) \
  ((((uint32_t)str[0]) << 24) | (((uint32_t)str[1]) << 16) | (((uint32_t)str[2]) << 8) | \
   (((uint32_t)str[3]) << 0))

typedef enum
{
  BOLD = 0x1,
  ITALIC = 0x2,
  UNDERLINE = 0x4
} FaceStyleFlag;

// NOTE: indices in terms of *character* (not: byte) positions
typedef struct
{
  uint16_t bgnChar;
  uint16_t endChar;
  uint16_t fontID;
  uint8_t faceStyleFlags; // FaceStyleFlag
  uint8_t fontSize;
  uint32_t textColorRGBA;
} StyleRecord;

CDVDOverlayCodecTX3G::CDVDOverlayCodecTX3G() : CDVDOverlayCodec("TX3G Subtitle Decoder")
{
  m_pOverlay = nullptr;
  m_textColor =
      KODI::SUBTITLES::colors[CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_SUBTITLES_COLOR)];
}

CDVDOverlayCodecTX3G::~CDVDOverlayCodecTX3G()
{
  Dispose();
}

bool CDVDOverlayCodecTX3G::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (hints.codec != AV_CODEC_ID_MOV_TEXT)
    return false;

  Dispose();

  return Initialize();
}

void CDVDOverlayCodecTX3G::Dispose()
{
  if (m_pOverlay)
  {
    m_pOverlay->Release();
    m_pOverlay = nullptr;
  }
}

int CDVDOverlayCodecTX3G::Decode(DemuxPacket* pPacket)
{
  double PTSStartTime = 0;
  double PTSStopTime = 0;

  CDVDOverlayCodec::GetAbsoluteTimes(PTSStartTime, PTSStopTime, pPacket);

  // do not move this. READ_XXXX macros modify pos.
  uint8_t* pos = pPacket->pData;
  uint8_t* end = pPacket->pData + pPacket->iSize;

  // Parse the packet as a TX3G TextSample.
  // Look for a single StyleBox ('styl') and
  // read all contained StyleRecords.
  // Ignore all other box types.
  // NOTE: Buffer overflows on read are not checked.
  // ALSO: READ_XXXX/SKIP_XXXX macros will modify pos.
  LEN_CHECK(2);
  uint16_t textLength = READ_U16();
  LEN_CHECK(textLength);
  uint8_t* text = READ_ARRAY(textLength);

  int numStyleRecords = 0;
  // reserve one more style slot for broken encoders

  XUTILS::auto_buffer bgnStyle(textLength + 1);
  XUTILS::auto_buffer endStyle(textLength + 1);

  memset(bgnStyle.get(), 0, textLength + 1);
  memset(endStyle.get(), 0, textLength + 1);

  int bgnColorIndex = 0, endColorIndex = 0;
  uint32_t textColorRGBA = m_textColor;
  while (pos < end)
  {
    // Read TextSampleModifierBox
    LEN_CHECK(4);
    uint32_t size = READ_U32();
    if (size == 0)
      size = pos - end; // extends to end of packet
    if (size == 1)
    {
      CLog::Log(LOGDEBUG, "CDVDOverlayCodecTX3G: TextSampleModifierBox has unsupported large size");
      break;
    }
    LEN_CHECK(4);
    uint32_t type = READ_U32();
    if (type == FOURCC("uuid"))
    {
      CLog::Log(LOGDEBUG,
                "CDVDOverlayCodecTX3G: TextSampleModifierBox has unsupported extended type");
      break;
    }

    if (type == FOURCC("styl"))
    {
      // Found a StyleBox. Parse the contained StyleRecords
      if (numStyleRecords != 0)
      {
        CLog::Log(LOGDEBUG,
                  "CDVDOverlayCodecTX3G: found additional StyleBoxes on subtitle; skipping");
        LEN_CHECK(size);
        SKIP_ARRAY(size);
        continue;
      }

      LEN_CHECK(2);
      numStyleRecords = READ_U16();
      for (int i = 0; i < numStyleRecords; i++)
      {
        StyleRecord curRecord;
        LEN_CHECK(12);
        curRecord.bgnChar = READ_U16();
        curRecord.endChar = READ_U16();
        curRecord.fontID = READ_U16();
        curRecord.faceStyleFlags = READ_U8();
        curRecord.fontSize = READ_U8();
        curRecord.textColorRGBA = READ_U32();
        // clamp bgnChar/bgnChar to textLength,
        // we alloc enough space above and this
        // fixes borken encoders that do not handle
        // endChar correctly.
        if (curRecord.bgnChar > textLength)
          curRecord.bgnChar = textLength;
        if (curRecord.endChar > textLength)
          curRecord.endChar = textLength;

        bgnStyle.get()[curRecord.bgnChar] |= curRecord.faceStyleFlags;
        endStyle.get()[curRecord.endChar] |= curRecord.faceStyleFlags;
        bgnColorIndex = curRecord.bgnChar;
        endColorIndex = curRecord.endChar;
        textColorRGBA = curRecord.textColorRGBA;
      }
    }
    else
    {
      // Found some other kind of TextSampleModifierBox. Skip it.
      LEN_CHECK(size);
      SKIP_ARRAY(size);
    }
  }

  // Copy text to out and add HTML markup for the style records
  int charIndex = 0;
  std::string strUTF8;
  // index over textLength chars to include broken encoders,
  // so we pickup closing styles on broken encoders
  for (pos = text, end = text + textLength; pos <= end; pos++)
  {
    if ((*pos & 0xC0) == 0x80)
    {
      // Is a non-first byte of a multi-byte UTF-8 character
      strUTF8.append((const char*)pos, 1);
      continue; // ...without incrementing 'charIndex'
    }

    uint8_t bgnStyles = bgnStyle.get()[charIndex];
    uint8_t endStyles = endStyle.get()[charIndex];

    if (endStyles & BOLD)
      strUTF8.append("{\\b0}");
    if (endStyles & ITALIC)
      strUTF8.append("{\\i0}");
    if (endStyles & UNDERLINE)
      strUTF8.append("{\\u0}");
    if (endColorIndex == charIndex && textColorRGBA != m_textColor)
      strUTF8.append("{\\c}");

    // invert the order from above so we bracket the text correctly.
    if (bgnColorIndex == charIndex && textColorRGBA != m_textColor)
    {
      uint32_t color = ColorUtils::ConvertToBGR(ColorUtils::ConvertToARGB(textColorRGBA));
      strUTF8 += "{\\c&H" + StringUtils::Format("{:6x}", color) + "&}";
    }

    if (bgnStyles & UNDERLINE)
      strUTF8.append("{\\u1}");
    if (bgnStyles & ITALIC)
      strUTF8.append("{\\i1}");
    if (bgnStyles & BOLD)
      strUTF8.append("{\\b1}");

    // stuff the UTF8 char
    strUTF8.append((const char*)pos, 1);

    // this is a char index, not a byte index.
    charIndex++;
  }

  if (strUTF8.empty())
    return OC_BUFFER;

  if (strUTF8[strUTF8.size() - 1] == '\n')
    strUTF8.erase(strUTF8.size() - 1);

  // erase unsupport tags
  CRegExp tags;
  if (tags.RegComp("(\\{[^\\}]*\\})"))
  {
    int pos = 0;
    while ((pos = tags.RegFind(strUTF8.c_str(), pos)) >= 0)
    {
      std::string tag = tags.GetMatch(0);
      if (tag == "{\\b0}" || tag == "{\\b1}" || tag == "{\\i0}" || tag == "{\\i1}" ||
          tag == "{\\u0}" || tag == "{\\u1}" || tag == "{\\c}" ||
          StringUtils::StartsWith(tag, "{\\c&H"))
      {
        pos += tag.length();
        continue;
      }
      strUTF8.erase(pos, tag.length());
    }
  }

  // We have to remove all \r because it causes the line to display empty box "tofu"
  StringUtils::Replace(strUTF8, "\r", "");

  AddSubtitle(strUTF8.c_str(), PTSStartTime, PTSStopTime);

  return m_pOverlay ? OC_DONE : OC_OVERLAY;
}

void CDVDOverlayCodecTX3G::Reset()
{
  Dispose();
  Flush();
}

void CDVDOverlayCodecTX3G::Flush()
{
  if (m_pOverlay)
  {
    m_pOverlay->Release();
    m_pOverlay = nullptr;
  }

  FlushSubtitles();
}

CDVDOverlay* CDVDOverlayCodecTX3G::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  return m_pOverlay->Acquire();
}
