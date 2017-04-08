/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDFactorySubtitle.h"

#include "DVDSubtitleStream.h"
#include "DVDSubtitleParserSubrip.h"
#include "DVDSubtitleParserMicroDVD.h"
#include "DVDSubtitleParserMPL2.h"
#include "DVDSubtitleParserSami.h"
#include "DVDSubtitleParserSSA.h"
#include "DVDSubtitleParserVplayer.h"

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

