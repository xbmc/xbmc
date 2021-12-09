/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSSA.h"

#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

CDVDSubtitleParserSSA::CDVDSubtitleParserSSA(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                             const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "SSA Subtitle Parser"),
    m_libass(std::make_shared<CDVDSubtitlesLibass>())
{
  m_libass->Configure();
}

CDVDSubtitleParserSSA::~CDVDSubtitleParserSSA()
{
  Dispose();
}

bool CDVDSubtitleParserSSA::Open(CDVDStreamInfo& hints)
{

  if (!CDVDSubtitleParserText::Open())
    return false;

  const std::string& data = m_pStream->GetData();
  if (!m_libass->CreateTrack(const_cast<char*>(data.c_str()), data.length()))
    return false;

  CDVDOverlaySSA* overlay = new CDVDOverlaySSA(m_libass);
  overlay->iPTSStartTime = 0.0;
  overlay->iPTSStopTime = DVD_NOPTS_VALUE;
  m_collection.Add(overlay);

  return true;
}

void CDVDSubtitleParserSSA::Dispose()
{
  CDVDSubtitleParserCollection::Dispose();
}
