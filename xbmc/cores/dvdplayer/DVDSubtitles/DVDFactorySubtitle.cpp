/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
//#include "DVDSubtitleParserSpu.h"
#include "DVDSubtitleParserSubrip.h"
#include "DVDSubtitleParserMicroDVD.h"
#include "DVDSubtitleParserMPL2.h"
#include "DVDSubtitleParserSami.h"
#include "DVDSubtitleParserSSA.h"
#include "DVDSubtitleParserVplayer.h"
#include "utils/log.h"

using namespace std;


CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(string& strFile)
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
      //   if (sscanf (line, "%d:%d:%d.%d,%d:%d:%d.%d",     &i, &i, &i, &i, &i, &i, &i, &i)==8){
      //     this->uses_time=1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer subtitle format detected\n");
      //     return FORMAT_SUBVIEWER;
      //   }

      //   if (sscanf (line, "%d:%d:%d,%d,%d:%d:%d,%d",     &i, &i, &i, &i, &i, &i, &i, &i)==8){
      //     this->uses_time=1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer subtitle format detected\n");
      //     return FORMAT_SUBVIEWER;
      //   }

      else if (strstr (line, "<SAMI>"))
      {
        pParser = new CDVDSubtitleParserSami(pStream, strFile.c_str());
        pStream = NULL;
      }
      //   /*
      //   * A RealText format is a markup language, starts with <window> tag,
      //   * options (behaviour modifiers) are possible.
      //   */
      //   if ( !strcasecmp(line, "<window") ) {
      //     this->uses_time=1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "rt subtitle format detected\n");
      //     return FORMAT_RT;
      //   }
      //   if ((!memcmp(line, "Dialogue: Marked", 16)) || (!memcmp(line, "Dialogue: ", 10))) {
      //     this->uses_time=1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "ssa subtitle format detected\n");
      //     return FORMAT_SSA;
      //   }
      //   if (sscanf (line, "%d,%d,\"%c", &i, &i, (char *) &i) == 3) {
      //     this->uses_time=0;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "pjs subtitle format detected\n");
      //     return FORMAT_PJS;
      //   }
      //   if (sscanf (line, "FORMAT=%d", &i) == 1) {
      //     this->uses_time=0;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "mpsub subtitle format detected\n");
      //     return FORMAT_MPSUB;
      //   }
      //   if (sscanf (line, "FORMAT=TIM%c", &p)==1 && p=='E') {
      //     this->uses_time=1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "mpsub subtitle format detected\n");
      //     return FORMAT_MPSUB;
      //   }
      //   if (strstr (line, "-->>")) {
      //     this->uses_time=0;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "aqtitle subtitle format detected\n");
      //     return FORMAT_AQTITLE;
      //   }
      //   if (sscanf(line, "@%d @%d", &i, &i) == 2 ||
      //sscanf(line, "%d:%d:%d.%d %d:%d:%d.%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
      //     this->uses_time = 1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "jacobsub subtitle format detected\n");
      //     return FORMAT_JACOBSUB;
      //   }
      //   if (sscanf(line, "{T %d:%d:%d:%d",&i, &i, &i, &i) == 4) {
      //     this->uses_time = 1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer 2.0 subtitle format detected\n");
      //     return FORMAT_SUBVIEWER2;
      //   }
      //   if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
      //     this->uses_time = 1;
      //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subrip 0.9 subtitle format detected\n");
      //     return FORMAT_SUBRIP09;
      //   }
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

