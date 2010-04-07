/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <stdlib.h>
#include <assert.h>
#include "config.h"

#include "demuxer_Subtitle.h"

cParserSubtitle::cParserSubtitle(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID)
 : cParser(streamer, streamID)
{
  m_firstPUSIseen       = false;
  m_PESStart            = false;
  m_subtitleBuffer      = NULL;
  m_subtitleBufferSize  = 0;
  m_subtitleBufferPtr   = 0;
  m_lastDTS             = DVD_NOPTS_VALUE;
  m_lastPTS             = DVD_NOPTS_VALUE;
  m_lastLength          = 0;
  m_curLength           = 0;
}

cParserSubtitle::~cParserSubtitle()
{
  if (m_subtitleBuffer)
    free(m_subtitleBuffer);
}

void cParserSubtitle::Parse(unsigned char *data, int size, bool pusi)
{
  if (pusi)
  {
    /* Payload unit start */
    m_PESStart      = true;
    m_firstPUSIseen = true;
  }

  /* Wait for first pusi */
  if (!m_firstPUSIseen)
    return;

  if (m_PESStart)
  {
    if (!PesIsPS1Packet(data))
    {
      esyslog("VNSI-Error: Subtitle PES packet contains no valid private stream 1, ignored this packet");
      m_firstPUSIseen = false;
      return;
    }

    m_lastDTS    = m_curDTS;
    m_lastPTS    = m_curPTS;
    m_lastLength = m_curLength;
    int hlen = ParsePESHeader(data, size);

    m_curLength = ((data[4] << 8) | data[5]) - hlen;
    m_PESStart  = false;
    data       += hlen;
    size       -= hlen;

    if (data[0] != 0x20 || data[1] != 0x00)
    {
      esyslog("VNSI-Error: Subtitle PES packet contains no valid identifier, ignored this packet");
      m_firstPUSIseen = false;
      return;
    }

    data       += 2;
    size       -= 2;

    if (m_subtitleBuffer && m_subtitleBufferPtr > 0)
    {
      if (m_lastLength <= m_subtitleBufferPtr)
      {
        sStreamPacket pkt;
        pkt.id       = m_streamID;
        pkt.data     = m_subtitleBuffer;
        pkt.size     = m_lastLength;
        pkt.duration = m_curDTS-m_lastDTS;
        pkt.dts      = m_lastDTS;
        pkt.pts      = m_lastPTS;
        SendPacket(&pkt);
      }
      else
      {
        esyslog("VNSI-Error: Subtitle PES length bigger as readed length");
      }
      m_subtitleBufferPtr = 0;
    }
  }

  if (m_subtitleBuffer == NULL)
  {
    m_subtitleBufferSize  = 4000;
    m_subtitleBuffer      = (uint8_t*)malloc(m_subtitleBufferSize);
  }

  if (m_subtitleBufferPtr + size + 4 >= m_subtitleBufferSize)
  {
    m_subtitleBufferSize  += size * 4;
    m_subtitleBuffer       = (uint8_t*)realloc(m_subtitleBuffer, m_subtitleBufferSize);
  }

  memcpy(m_subtitleBuffer+m_subtitleBufferPtr, data, size);
  m_subtitleBufferPtr += size;
}
