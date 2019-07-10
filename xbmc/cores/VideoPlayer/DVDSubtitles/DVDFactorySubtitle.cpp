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

#include <cstring>
#include <memory>

CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(std::string& strFile)
{
  char line[1024];
  int i;

  std::unique_ptr<CDVDSubtitleStream> pStream(new CDVDSubtitleStream());
  if(!pStream->Open(strFile))
  {
    return nullptr;
  }

  for (int t = 0; t < 256; t++)
  {
    if (pStream->ReadLine(line, sizeof(line)))
    {
      if ((sscanf (line, "{%d}{}", &i)==1) ||
          (sscanf (line, "{%d}{%d}", &i, &i)==2))
      {
        return new CDVDSubtitleParserMicroDVD(std::move(pStream), strFile.c_str());
      }
      else if (sscanf(line, "[%d][%d]", &i, &i) == 2)
      {
        return new CDVDSubtitleParserMPL2(std::move(pStream), strFile.c_str());
      }
      else if (sscanf(line, "%d:%d:%d%*c%d --> %d:%d:%d%*c%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8)
      {
        return new CDVDSubtitleParserSubrip(std::move(pStream), strFile.c_str());
      }
      else if (sscanf(line, "%d:%d:%d:", &i, &i, &i) == 3)
      {
        return new CDVDSubtitleParserVplayer(std::move(pStream), strFile.c_str());
      }
      else if ((!memcmp(line, "Dialogue: Marked", 16)) || (!memcmp(line, "Dialogue: ", 10)))
      {
        return new CDVDSubtitleParserSSA(std::move(pStream), strFile.c_str());
      }
      else if (strstr (line, "<SAMI>"))
      {
        return new CDVDSubtitleParserSami(std::move(pStream), strFile.c_str());
      }
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

