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

#include <stdlib.h>
#include "config.h"

#include "demuxer_Teletext.h"

cParserTeletext::cParserTeletext(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID)
 : cParser(streamer, streamID)
{
  m_firstPUSIseen       = false;
  m_PESStart            = false;
  m_teletextBuffer      = NULL;
  m_teletextBufferSize  = 0;
  m_teletextBufferPtr   = 0;
  m_lastDTS             = DVD_NOPTS_VALUE;
  m_lastPTS             = DVD_NOPTS_VALUE;
}

cParserTeletext::~cParserTeletext()
{
  if (m_teletextBuffer)
    free(m_teletextBuffer);
}

void cParserTeletext::Parse(unsigned char *data, int size, bool pusi)
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
      ERRORLOG("Teletext PES packet contains no valid private stream 1, ignored this packet");
      m_firstPUSIseen = false;
      return;
    }

    m_lastDTS = m_curDTS;
    m_lastPTS = m_curPTS;
    int hlen = ParsePESHeader(data, size);

    m_PESStart = false;
    data      += hlen;
    size      -= hlen;

    if (data[0] < 0x10 || data[0] > 0x1F)
    {
      ERRORLOG("Teletext PES packet contains no valid identifier, ignored this packet");
      m_firstPUSIseen = false;
      return;
    }

    if (m_teletextBuffer && m_teletextBufferPtr > 0)
    {
      sStreamPacket pkt;
      pkt.id       = m_streamID;
      pkt.data     = m_teletextBuffer;
      pkt.size     = m_teletextBufferPtr;
      pkt.duration = m_curDTS-m_lastDTS;
      pkt.dts      = m_lastDTS;
      pkt.pts      = m_lastPTS;
      SendPacket(&pkt);
      m_teletextBufferPtr = 0;
    }
  }

  if (m_teletextBuffer == NULL)
  {
    m_teletextBufferSize  = 4000;
    m_teletextBuffer      = (uint8_t*)malloc(m_teletextBufferSize);
  }

  if (m_teletextBufferPtr + size + 4 >= m_teletextBufferSize)
  {
    m_teletextBufferSize  += size * 4;
    m_teletextBuffer       = (uint8_t*)realloc(m_teletextBuffer, m_teletextBufferSize);
  }

  memcpy(m_teletextBuffer+m_teletextBufferPtr, data, size);
  m_teletextBufferPtr += size;
}
