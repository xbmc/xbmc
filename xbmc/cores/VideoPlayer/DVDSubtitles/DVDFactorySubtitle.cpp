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

CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(std::string& strFile)
{
  char line[1024];
  int i;
  CDVDSubtitleParser* pParser = NULL;

  CDVDSubtitleStream* pStream = new CDVDSubtitleStream();
  if(!pStream->Open(strFile))
  {
    delete pStream;
    return NULL;
  }

  for (int t = 0; !pParser && t < 256; t++)
  {
    if (pStream->ReadLine(line, sizeof(line)))
    {
      if ((sscanf (line, "{%d}{}", &i)==1) ||
          (sscanf (line, "{%d}{%d}", &i, &i)==2))
      {
        pParser = new CDVDSubtitleParserMicroDVD(pStream, strFile.c_str());
        pStream = NULL;
      }
      else if (sscanf(line, "[%d][%d]", &i, &i) == 2)
      {
        pParser = new CDVDSubtitleParserMPL2(pStream, strFile.c_str());
        pStream = NULL;
      }
      else if (sscanf(line, "%d:%d:%d%*c%d --> %d:%d:%d%*c%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8)
      {
        pParser = new CDVDSubtitleParserSubrip(pStream, strFile.c_str());
        pStream = NULL;
      }
      else if (sscanf(line, "%d:%d:%d:", &i, &i, &i) == 3)
      {
        pParser = new CDVDSubtitleParserVplayer(pStream, strFile.c_str());
        pStream = NULL;
      }
      else if ((!memcmp(line, "Dialogue: Marked", 16)) || (!memcmp(line, "Dialogue: ", 10)))
      {
        pParser =  new CDVDSubtitleParserSSA(pStream, strFile.c_str());
        pStream = NULL;
      }
      else if (strstr (line, "<SAMI>"))
      {
        pParser = new CDVDSubtitleParserSami(pStream, strFile.c_str());
        pStream = NULL;
      }
    }
    else
    {
      break;
    }
  }
  if (pStream)
    delete pStream;
  return pParser;
}

