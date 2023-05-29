/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserVplayer.h"

#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"

#include <cstdlib>

CDVDSubtitleParserVplayer::CDVDSubtitleParserVplayer(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                                     const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "VPlayer Subtitle Parser"),
    m_framerate(DVD_TIME_BASE)
{
}

bool CDVDSubtitleParserVplayer::Open(CDVDStreamInfo& hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  if (!Initialize())
    return false;

  // Vplayer subtitles have 1-second resolution
  m_framerate = DVD_TIME_BASE;

  // Vplayer subtitles don't have StopTime, so we use following subtitle's StartTime
  // for that, unless gap was more than 4 seconds. Then we set subtitle duration
  // for 4 seconds, to not have text hanging around in silent scenes...
  int defaultDuration = 4 * (int)m_framerate;

  CRegExp reg;
  if (!reg.RegComp("([0-9]+):([0-9]+):([0-9]+):(.+)$"))
    return false;

  int prevSubId = NO_SUBTITLE_ID;
  double prevPTSStartTime = 0.;
  std::string line;

  while (m_pStream->ReadLine(line))
  {
    if (reg.RegFind(line) > -1)
    {
      int hour = std::atoi(reg.GetMatch(1).c_str());
      int min = std::atoi(reg.GetMatch(2).c_str());
      int sec = std::atoi(reg.GetMatch(3).c_str());
      std::string currText = reg.GetMatch(4);

      double currPTSStartTime = m_framerate * (3600 * hour + 60 * min + sec);

      // We have to set the stop time for the previous line (Event)
      // by using the start time of the current line,
      // but if the duration is too long we keep the default 4 secs
      double PTSDuration = currPTSStartTime - prevPTSStartTime;

      if (PTSDuration < defaultDuration)
        ChangeSubtitleStopTime(prevSubId, currPTSStartTime);

      // A single line can contain multiple lines split by |
      StringUtils::Replace(currText, "|", "\n");

      prevSubId = AddSubtitle(currText, currPTSStartTime, currPTSStartTime + defaultDuration);

      prevPTSStartTime = currPTSStartTime;
    }
  }

  m_collection.Add(CreateOverlay());

  return true;
}
