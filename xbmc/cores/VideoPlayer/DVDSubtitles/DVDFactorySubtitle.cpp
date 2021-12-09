/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFactorySubtitle.h"

#include "DVDSubtitleParserMPL2.h"
#include "DVDSubtitleParserMicroDVD.h"
#include "DVDSubtitleParserSSA.h"
#include "DVDSubtitleParserSami.h"
#include "DVDSubtitleParserSubrip.h"
#include "DVDSubtitleParserVplayer.h"
#include "DVDSubtitleStream.h"
#include "SubtitleParserWebVTT.h"
#include "utils/StringUtils.h"

#include <cstring>
#include <memory>

CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(std::string& strFile)
{
  std::string line;
  int i;

  std::unique_ptr<CDVDSubtitleStream> pStream(new CDVDSubtitleStream());
  if(!pStream->Open(strFile))
  {
    return nullptr;
  }

  for (int t = 0; t < 256; t++)
  {
    if (pStream->ReadLine(line))
    {
      if ((sscanf(line.c_str(), "{%d}{}", &i) == 1) ||
          (sscanf(line.c_str(), "{%d}{%d}", &i, &i) == 2))
      {
        return new CDVDSubtitleParserMicroDVD(std::move(pStream), strFile);
      }
      else if (sscanf(line.c_str(), "[%d][%d]", &i, &i) == 2)
      {
        return new CDVDSubtitleParserMPL2(std::move(pStream), strFile);
      }
      else if (sscanf(line.c_str(), "%d:%d:%d%*c%d --> %d:%d:%d%*c%d", &i, &i, &i, &i, &i, &i, &i,
                      &i) == 8)
      {
        return new CDVDSubtitleParserSubrip(std::move(pStream), strFile);
      }
      else if (sscanf(line.c_str(), "%d:%d:%d:", &i, &i, &i) == 3)
      {
        return new CDVDSubtitleParserVplayer(std::move(pStream), strFile);
      }
      else if (!StringUtils::CompareNoCase(line, "!: This is a Sub Station Alpha v", 32) ||
               !StringUtils::CompareNoCase(line, "ScriptType: v4.00", 17) ||
               !StringUtils::CompareNoCase(line, "Dialogue: Marked", 16) ||
               !StringUtils::CompareNoCase(line, "Dialogue: ", 10) ||
               !StringUtils::CompareNoCase(line, "[Events]", 8))
      {
        return new CDVDSubtitleParserSSA(std::move(pStream), strFile);
      }
      else if (line == "<SAMI>")
      {
        return new CDVDSubtitleParserSami(std::move(pStream), strFile);
      }
      else if (!StringUtils::CompareNoCase(line, "WEBVTT", 6))
      {
        return new CSubtitleParserWebVTT(std::move(pStream), strFile);
      }
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

