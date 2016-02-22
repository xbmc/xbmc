/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef VNSI_DEMUXER_SUBTITLE_H
#define VNSI_DEMUXER_SUBTITLE_H

#include "demuxer.h"

// --- cParserSubtitle -------------------------------------------------

class cParserSubtitle : public cParser
{
private:
  bool        m_firstPUSIseen;
  bool        m_PESStart;
  uint8_t    *m_subtitleBuffer;
  int         m_subtitleBufferSize;
  int         m_subtitleBufferPtr;
  int64_t     m_lastDTS;
  int64_t     m_lastPTS;
  int         m_lastLength;
  int         m_curLength;

public:
  cParserSubtitle(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID);
  virtual ~cParserSubtitle();

  virtual void Parse(unsigned char *data, int size, bool pusi);
};


#endif // VNSI_DEMUXER_SUBTITLE_H
