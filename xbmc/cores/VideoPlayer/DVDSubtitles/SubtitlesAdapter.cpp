/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitlesAdapter.h"

#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDSubtitlesLibass.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

#include <memory>

CSubtitlesAdapter::CSubtitlesAdapter() : m_libass(std::make_shared<CDVDSubtitlesLibass>())
{
  m_libass->Configure();
}

CSubtitlesAdapter::~CSubtitlesAdapter()
{
}

bool CSubtitlesAdapter::Initialize()
{
  m_libass->SetSubtitleType(ADAPTED);
  return m_libass->CreateTrack() && m_libass->CreateStyle();
}

int CSubtitlesAdapter::AddSubtitle(std::string& text, double startTime, double stopTime)
{
  return AddSubtitle(text, startTime, stopTime, nullptr);
}

int CSubtitlesAdapter::AddSubtitle(std::string& text,
                                   double startTime,
                                   double stopTime,
                                   KODI::SUBTITLES::STYLE::subtitleOpts* opts)
{
  PostProcess(text);
  int ret = m_libass->AddEvent(text.c_str(), startTime, stopTime, opts);
  if (ret == ASS_NO_ID)
    return NO_SUBTITLE_ID;
  return ret;
}

void CSubtitlesAdapter::AppendToSubtitle(int subtitleId, const char* text)
{
  if (subtitleId == NO_SUBTITLE_ID)
    subtitleId = ASS_NO_ID;
  return m_libass->AppendTextToEvent(subtitleId, text);
}

int CSubtitlesAdapter::DeleteSubtitles(int nSubtitles, int threshold)
{
  int ret = m_libass->DeleteEvents(nSubtitles, threshold);
  if (ret == ASS_NO_ID)
    return NO_SUBTITLE_ID;
  return ret;
}

void CSubtitlesAdapter::ChangeSubtitleStopTime(int subtitleId, double stopTime)
{
  if (subtitleId == NO_SUBTITLE_ID)
    subtitleId = ASS_NO_ID;
  return m_libass->ChangeEventStopTime(subtitleId, stopTime);
}

void CSubtitlesAdapter::FlushSubtitles()
{
  // Flush events to avoid display duplicates events e.g. on video seek
  m_libass->FlushEvents();
}

std::shared_ptr<CDVDOverlay> CSubtitlesAdapter::CreateOverlay()
{
  // Warning with Libass the overlay does not contain image or text then is not tied to Libass Events
  // any variation of the overlay Start/Stop PTS time will cause problems with the rendering.
  // The better thing is create a single overlay without PTS stop time to avoid side effects,
  // maybe this situation could be improved in the future.

  // Side effects that happens when you create each overlay based on each Libass Event:
  // - A small delay when switching on/off the overlay renderer, cause subtitles
  //   flickering when Events have close timing. A possible cause of the
  //   delay could be the async implementation of overlay management chain.
  // - When an overlay disable the renderer, Libass has no possibility to
  //   complete the rendering of text animations (not sure if related to previous problem)
  //   with the result of displaying broken animations on the screen.
  auto overlay = std::make_shared<CDVDOverlayText>(m_libass);
  overlay->iPTSStartTime = 0.0;
  overlay->iPTSStopTime = DVD_NOPTS_VALUE;
  return overlay;
}
