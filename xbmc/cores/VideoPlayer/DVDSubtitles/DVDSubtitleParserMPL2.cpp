/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserMPL2.h"

#include "DVDStreamInfo.h"
#include "DVDSubtitleTagMicroDVD.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/RegExp.h"

#include <cstdlib>

CDVDSubtitleParserMPL2::CDVDSubtitleParserMPL2(std::unique_ptr<CDVDSubtitleStream>&& stream,
                                               const std::string& filename)
  : CDVDSubtitleParserText(std::move(stream), filename, "MPL2 Subtitle Parser"),
    m_framerate(DVD_TIME_BASE / 10.0)
{
}

bool CDVDSubtitleParserMPL2::Open(CDVDStreamInfo& hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  if (!Initialize())
    return false;

  // MPL2 is time-based, with 0.1s accuracy
  m_framerate = DVD_TIME_BASE / 10.0;

  CRegExp reg;
  if (!reg.RegComp("\\[([0-9]+)\\]\\[([0-9]+)\\](.+)"))
    return false;
  CDVDSubtitleTagMicroDVD TagConv;
  std::string line;

  while (m_pStream->ReadLine(line))
  {
    int pos = reg.RegFind(line);
    if (pos > -1)
    {
      std::string startFrame(reg.GetMatch(1));
      std::string endFrame(reg.GetMatch(2));
      std::string text(reg.GetMatch(3));

      double iPTSStartTime = m_framerate * std::atoi(startFrame.c_str());
      double iPTSStopTime = m_framerate * std::atoi(endFrame.c_str());

      TagConv.ConvertLine(text);
      AddSubtitle(text, iPTSStartTime, iPTSStopTime);
    }
  }

  m_collection.Add(CreateOverlay());

  return true;
}
