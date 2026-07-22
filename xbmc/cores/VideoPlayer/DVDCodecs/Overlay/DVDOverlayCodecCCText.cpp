/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecCCText.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cctype>

namespace
{
// Muxed Closed Caption subtitles don't have the stop PTS value,
// the stop value can be taken from: the start PTS of the next subtitle,
// or, the start PTS of the next subtitle without text -if any-,
// otherwise we fallback to 20 secs duration (long duration so as not
// to cause side effects on texts that expand like karaoke, but slowly)
constexpr double DEFAULT_DURATION = 20.0 * (double)DVD_TIME_BASE;
} // namespace

CDVDOverlayCodecCCText::CDVDOverlayCodecCCText()
  : CDVDOverlayCodec("CC Text Subtitle Decoder"), m_pOverlay(nullptr)
{
  m_prevSubId = NO_SUBTITLE_ID;
  m_prevPTSStart = 0.0;
  m_prevText.clear();
  m_changePrevStopTime = false;
}

bool CDVDOverlayCodecCCText::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  m_pOverlay.reset();
  return Initialize();
}

OverlayMessage CDVDOverlayCodecCCText::Decode(DemuxPacket* pPacket)
{
  if (!pPacket)
    return OverlayMessage::OC_ERROR;

  uint8_t* data = pPacket->pData;
  std::string text((char*)data, (char*)(data + pPacket->iSize));

  // We delete some old Events to avoid allocating too much,
  // this can easily happen with karaoke text or live videos
  int lastId = DeleteSubtitles(50, 200);
  if (m_prevSubId != NO_SUBTITLE_ID)
  {
    if (lastId == NO_SUBTITLE_ID)
    {
      m_prevPTSStart = 0.0;
      m_prevText.clear();
      m_changePrevStopTime = false;
    }
    m_prevSubId = lastId;
  }

  double PTSStartTime = 0;
  double PTSStopTime = 0;

  CDVDOverlayCodec::GetAbsoluteTimes(PTSStartTime, PTSStopTime, pPacket);
  CDVDSubtitleTagSami TagConv;

  PTSStopTime = PTSStartTime + DEFAULT_DURATION;

  if (TagConv.Init())
  {
    // Escape literal '<' as &lt; so ConvertLine's tag parser doesn't treat them
    // as HTML tags. Leave '<' followed by a letter or '/' untouched (valid tags).
    for (size_t i = 0; i < text.size(); ++i)
    {
      if (text[i] == '<')
      {
        const size_t next = i + 1;
        if (next >= text.size() ||
            (!std::isalpha(static_cast<unsigned char>(text[next])) && text[next] != '/'))
        {
          text.replace(i, 1, "&lt;");
          i += 3; // skip past the 4-char entity (loop will i++ once more)
        }
      }
    }
    TagConv.ConvertLine(text);
    TagConv.CloseTag(text);

    if (!text.empty())
    {
      if (m_prevText == text)
      {
        // Extend the duration of previously added event
        ChangeSubtitleStopTime(m_prevSubId, PTSStartTime + DEFAULT_DURATION);
      }
      else
      {
        // Set the stop time of previously added event based on current start PTS
        if (m_changePrevStopTime)
          ChangeSubtitleStopTime(m_prevSubId, PTSStartTime);

        m_prevSubId = AddSubtitle(text, PTSStartTime, PTSStopTime);
        m_changePrevStopTime = true;
      }
    }
    else if (m_prevText != text)
    {
      // Set the stop time of previously added event based on current start PTS
      ChangeSubtitleStopTime(m_prevSubId, PTSStartTime);
      m_changePrevStopTime = false;
    }

    m_prevText = text;
    m_prevPTSStart = PTSStartTime;
  }
  else
    CLog::Log(LOGERROR, "{} - Failed to initialize tag converter", __FUNCTION__);

  return m_pOverlay ? OverlayMessage::OC_DONE : OverlayMessage::OC_OVERLAY;
}

void CDVDOverlayCodecCCText::PostProcess(std::string& text)
{
  // The data that come from InputStream could contains \r chars
  // we have to remove them all because it causes to display empty box "tofu"
  //! @todo This must be removed after the rework of the CC decoders
  StringUtils::Replace(text, "\r", "");

  // Escape literal '{' (not followed by '\') as \{ to prevent the subtitle
  // renderer from treating them as override block openers.
  std::string result;
  result.reserve(text.size());
  bool inTag = false;
  for (size_t i = 0; i < text.size(); ++i)
  {
    if (!inTag && text[i] == '{')
    {
      if (i + 1 < text.size() && text[i + 1] == '\\')
      {
        inTag = true; // ASS override tag opener, keep as-is
        result += '{';
      }
      else
      {
        result += "\\{"; // literal '{' from CC text
      }
    }
    else if (inTag && text[i] == '}')
    {
      inTag = false;
      result += '}';
    }
    else
    {
      result += text[i];
    }
  }
  text = std::move(result);

  CSubtitlesAdapter::PostProcess(text);
}

void CDVDOverlayCodecCCText::Reset()
{
  Flush();
}

void CDVDOverlayCodecCCText::Flush()
{
  m_pOverlay.reset();
  m_prevSubId = NO_SUBTITLE_ID;
  m_prevPTSStart = 0.0;
  m_prevText.clear();
  m_changePrevStopTime = false;

  FlushSubtitles();
}

std::shared_ptr<CDVDOverlay> CDVDOverlayCodecCCText::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = CreateOverlay();
  m_pOverlay->SetTextAlignEnabled(true);
  return m_pOverlay;
}
