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

#include "demuxer_AAC.h"

static int aac_sample_rates[16] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000, 7350
};


cParserAAC::cParserAAC(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID)
 : cParser(streamer, streamID)
{
  m_demuxer                   = demuxer;
  m_streamBuffer              = NULL;
  m_streamBufferSize          = 0;
  m_streamBufferPtr           = 0;
  m_streamParserPtr           = 0;
  m_firstPUSIseen             = false;

  m_Configured                = false;
  m_FrameLengthType           = 0;
  m_SampleRate                = 0;
}

cParserAAC::~cParserAAC()
{
  if (m_streamBuffer)
    free(m_streamBuffer);
}

void cParserAAC::Parse(unsigned char *data, int size, bool pusi)
{
  if (pusi)
  {
    /* Payload unit start */
    m_firstPUSIseen   = true;
    m_streamBufferPtr = 0;
    m_streamParserPtr = 0;
  }

  if (!m_firstPUSIseen)
    return;

  if (m_streamBuffer == NULL)
  {
    m_streamBufferSize = 4000;
    m_streamBuffer = (uint8_t*)malloc(m_streamBufferSize);
  }

  if(m_streamBufferPtr + size >= m_streamBufferSize)
  {
    m_streamBufferSize += size * 4;
    m_streamBuffer = (uint8_t*)realloc(m_streamBuffer, m_streamBufferSize);
  }

  memcpy(m_streamBuffer + m_streamBufferPtr, data, size);
  m_streamBufferPtr += size;

  if (m_streamParserPtr == 0)
  {
    if (m_streamBufferPtr < 9)
      return;

    int hlen = ParsePESHeader(data, size);
    if (hlen < 0)
      return;

    m_streamParserPtr += hlen;
  }

  int p = m_streamParserPtr;

  int l;
  while ((l = m_streamBufferPtr - p) > 3)
  {
    if(m_streamBuffer[p] == 0x56 && (m_streamBuffer[p + 1] & 0xe0) == 0xe0)
    {
      int muxlen = (m_streamBuffer[p + 1] & 0x1f) << 8 | m_streamBuffer[p + 2];

      if(l < muxlen + 3)
        break;

      ParseLATMAudioMuxElement(m_streamBuffer + p + 3, muxlen);
      p += muxlen + 3;
    }
    else
    {
      p++;
    }
  }
  m_streamParserPtr = p;
}

void cParserAAC::ParseLATMAudioMuxElement(uint8_t *data, int len)
{
  cBitstream bs(data, len * 8);

  if (!bs.readBits1())
    ReadStreamMuxConfig(&bs);

  if (!m_Configured)
    return;

  if (m_FrameLengthType != 0)
    return;

  int tmp;
  unsigned int slotLen = 0;
  do
  {
    tmp = bs.readBits(8);
    slotLen += tmp;
  } while (tmp == 255);

  if (slotLen * 8 > bs.remainingBits())
    return;

  if (m_curDTS == DVD_NOPTS_VALUE)
    return;

  sStreamPacket pkt;
  pkt.id       = m_streamID;
  pkt.size     = slotLen + 7;
  pkt.data     = (uint8_t*)malloc(pkt.size);
  pkt.dts      = m_curDTS;
  pkt.pts      = m_curDTS;
  pkt.duration = m_FrameDuration;

  /* 7 bytes of ADTS header */
  cBitstream out(pkt.data, 56);

  out.putBits(0xfff, 12); // Sync marker
  out.putBits(0, 1);      // ID 0 = MPEG 4
  out.putBits(0, 2);      // Layer
  out.putBits(1, 1);      // Protection absent
  out.putBits(2, 2);      // AOT
  out.putBits(m_SampleRateIndex, 4);
  out.putBits(1, 1);      // Private bit
  out.putBits(m_ChannelConfig, 3);
  out.putBits(1, 1);      // Original
  out.putBits(1, 1);      // Copy
  out.putBits(1, 1);      // Copyright identification bit
  out.putBits(1, 1);      // Copyright identification start
  out.putBits(slotLen, 13);
  out.putBits(0, 11);     // Buffer fullness
  out.putBits(0, 2);      // RDB in frame

  assert(out.remainingBits() == 0);

  /* AAC RDB */
  uint8_t *buf = pkt.data + 7;
  for (unsigned int i = 0; i < slotLen; i++)
    *buf++ = bs.readBits(8);

  m_curDTS += m_FrameDuration;

  SendPacket(&pkt);
  return;
}

void cParserAAC::ReadStreamMuxConfig(cBitstream *bs)
{
  int AudioMuxVersion = bs->readBits(1);
  m_AudioMuxVersion_A = 0;
  if (AudioMuxVersion)                       // audioMuxVersion
    m_AudioMuxVersion_A = bs->readBits(1);

  if(m_AudioMuxVersion_A)
    return;

  if (AudioMuxVersion)
    LATMGetValue(bs);                  // taraFullness

  bs->skipBits(1);                         // allStreamSameTimeFraming = 1
  bs->skipBits(6);                         // numSubFrames = 0
  bs->skipBits(4);                         // numPrograms = 0

  // for each program (which there is only on in DVB)
  bs->skipBits(3);                         // numLayer = 0

  // for each layer (which there is only on in DVB)
  if (!AudioMuxVersion)
    ReadAudioSpecificConfig(bs);
  else
    return;

  // these are not needed... perhaps
  m_FrameLengthType = bs->readBits(3);
  switch (m_FrameLengthType)
  {
    case 0:
      bs->readBits(8);
      break;
    case 1:
      bs->readBits(9);
      break;
    case 3:
    case 4:
    case 5:
      bs->readBits(6);                 // celp_table_index
      break;
    case 6:
    case 7:
      bs->readBits(1);                 // hvxc_table_index
      break;
  }

  if (bs->readBits(1))
  {                   // other data?
    if (AudioMuxVersion)
    {
      LATMGetValue(bs);              // other_data_bits
    }
    else
    {
      int esc;
      do
      {
        esc = bs->readBits(1);
        bs->skipBits(8);
      } while (esc);
    }
  }

  if (bs->readBits(1))                   // crc present?
    bs->skipBits(8);                     // config_crc
  m_Configured = true;
}

void cParserAAC::ReadAudioSpecificConfig(cBitstream *bs)
{
  int aot = bs->readBits(5);
  if (aot != 2)
    return;

  m_SampleRateIndex = bs->readBits(4);

  if (m_SampleRateIndex == 0xf)
    return;

  m_SampleRate    = aac_sample_rates[m_SampleRateIndex];
  m_FrameDuration = 1024 * 90000 / m_SampleRate;
  m_ChannelConfig = bs->readBits(4);

  bs->skipBits(1);      //framelen_flag
  if (bs->readBits1())  // depends_on_coder
    bs->skipBits(14);

  if (bs->readBits(1))  // ext_flag
    bs->skipBits(1);    // ext3_flag

  m_demuxer->SetAudioInformation(m_ChannelConfig, m_SampleRate, 0, 0, 0);
}
