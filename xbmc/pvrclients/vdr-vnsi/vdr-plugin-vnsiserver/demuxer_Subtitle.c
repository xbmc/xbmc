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
    m_subtitleBufferPtr = 0;
    m_firstPUSIseen = true;
  }

  /* Wait for first pusi */
  if (!m_firstPUSIseen)
    return;

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

  if (m_subtitleBufferPtr < 6)
    return;

  uint32_t startcode =
    (m_subtitleBuffer[0] << 24) |
    (m_subtitleBuffer[1] << 16) |
    (m_subtitleBuffer[2] << 8) |
    (m_subtitleBuffer[3]);

  if (startcode == 0x1be)
  {
    m_firstPUSIseen = false;
    return;
  }

  int psize = m_subtitleBuffer[4] << 8 | m_subtitleBuffer[5];

  if (m_subtitleBufferPtr != psize + 6)
    return;

  m_firstPUSIseen = false;

  int hlen = ParsePESHeader(m_subtitleBuffer, m_subtitleBufferPtr);
  if (hlen < 0)
    return;

  if (m_lastDTS == DVD_NOPTS_VALUE)
  {
    m_lastDTS = m_curDTS;
    m_lastPTS = m_curPTS;
  }

  psize -= hlen - 6;
  uint8_t *buf = m_subtitleBuffer + hlen;

  if (psize < 2 || buf[0] != 0x20 || buf[1] != 0x00)
    return;

  psize -= 2;
  buf += 2;

  if (psize >= 6)
  {
    // end_of_PES_data_field_marker
    if (buf[psize - 1] == 0xff)
    {
      sStreamPacket pkt;
      pkt.id       = m_streamID;
      pkt.data     = buf;
      pkt.size     = psize - 1;
      pkt.duration = m_curDTS-m_lastDTS;
      pkt.dts      = m_curPTS;
      pkt.pts      = m_curPTS;
      SendPacket(&pkt);

      m_lastDTS = m_curDTS;
      m_lastPTS = m_curPTS;
    }
  }

}
