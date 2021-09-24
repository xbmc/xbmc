/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSubrip.h"

#include "DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/StringUtils.h"

CDVDSubtitleParserSubrip::CDVDSubtitleParserSubrip(std::unique_ptr<CDVDSubtitleStream>&& pStream,
                                                   const std::string& strFile)
  : CDVDSubtitleParserText(std::move(pStream), strFile, "SubRip Subtitle Parser")
{
}

CDVDSubtitleParserSubrip::~CDVDSubtitleParserSubrip()
{
  Dispose();
}

bool CDVDSubtitleParserSubrip::Open(CDVDStreamInfo& hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  if (!Initialize())
    return false;

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;

  char line[1024];
  std::string currLine;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    currLine = line;
    StringUtils::Trim(currLine);

    if (currLine.length() > 0)
    {
      char sep;
      int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;
      int c = sscanf(currLine.c_str(), "%d%c%d%c%d%c%d --> %d%c%d%c%d%c%d\n", &hh1, &sep, &mm1,
                     &sep, &ss1, &sep, &ms1, &hh2, &sep, &mm2, &sep, &ss2, &sep, &ms2);

      if (c == 1)
      {
        // numbering, skip it
      }
      else if (c == 14) // time info
      {
        double iPTSStartTime =
            ((double)(((hh1 * 60 + mm1) * 60) + ss1) * 1000 + ms1) * (DVD_TIME_BASE / 1000);
        double iPTSStopTime =
            ((double)(((hh2 * 60 + mm2) * 60) + ss2) * 1000 + ms2) * (DVD_TIME_BASE / 1000);

        std::string convText;
        while (m_pStream->ReadLine(line, sizeof(line)))
        {
          currLine.assign(line);
          StringUtils::Trim(currLine);

          // empty line, next subtitle is about to start
          if (currLine.length() <= 0)
            break;

          if (convText.size() > 0)
            convText += "\n";
          TagConv.ConvertLine(currLine);
          convText += currLine;
        }

        if (!convText.empty())
        {
          TagConv.CloseTag(convText);
          AddSubtitle(convText.c_str(), iPTSStartTime, iPTSStopTime);
        }
      }
    }
  }

  m_collection.Add(CreateOverlay());

  return true;
}

void CDVDSubtitleParserSubrip::Dispose()
{
  CDVDSubtitleParserCollection::Dispose();
}
