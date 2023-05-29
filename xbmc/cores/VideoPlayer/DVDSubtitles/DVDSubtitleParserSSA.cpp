/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSSA.h"

#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "settings/SettingsComponent.h"
#include "settings/SubtitlesSettings.h"

using namespace KODI;

CDVDSubtitleParserSSA::CDVDSubtitleParserSSA(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                             const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "SSA Subtitle Parser"),
    m_libass(std::make_shared<CDVDSubtitlesLibass>())
{
  m_libass->Configure();
}

bool CDVDSubtitleParserSSA::Open(CDVDStreamInfo& hints)
{

  if (!CDVDSubtitleParserText::Open())
    return false;

  const std::string& data = m_pStream->GetData();
  if (!m_libass->CreateTrack(const_cast<char*>(data.c_str()), data.length()))
    return false;

  auto overlay = std::make_shared<CDVDOverlaySSA>(m_libass);
  overlay->iPTSStartTime = 0.0;
  overlay->iPTSStopTime = DVD_NOPTS_VALUE;
  auto overrideStyles{
      CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetOverrideStyles()};
  overlay->SetForcedMargins(overrideStyles != SUBTITLES::OverrideStyles::STYLES_POSITIONS &&
                            overrideStyles != SUBTITLES::OverrideStyles::POSITIONS);
  m_collection.Add(overlay);

  return true;
}
