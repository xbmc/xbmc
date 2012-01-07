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

#include "demuxer_MPEGAudio.h"

cParserMPEG2Audio::cParserMPEG2Audio(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID)
 : cParser(streamer, streamID)
{
  m_FrameOffset               = 0;
  m_CurrentOffset             = 0;
  m_NextFrameOffset           = 0;
  m_CurrentFrameStartIndex    = 0;
  m_Offset                    = 0;
  m_PTS                       = 0;
  m_DTS                       = 0;
  m_FrameSize                 = 0;
  m_FrameFreeFormatFrameSize  = 0;
  m_FrameFreeFormatNextHeader = 0;
  m_FrameHeader               = 0;
  m_HeaderCount               = 0;
  m_SampleRate                = 0;
  m_Channels                  = 0;
  m_BitRate                   = 0;
  m_demuxer                   = demuxer;
  m_firstPUSIseen             = false;
  m_PESStart                  = false;
  m_FetchTimestamp            = true;
  m_FrameInBufferPtr          = m_FrameInBuffer;
  m_NextDTS                   = 0;

  for (int i = 0; i < AV_PARSER_PTS_NB; i++)
  {
    m_CurrentFrameDTS[i] = DVD_NOPTS_VALUE;
    m_CurrentFramePTS[i] = DVD_NOPTS_VALUE;
    m_CurrentFrameEnd[i] = 0;
    m_CurrentFrameOffset[i] = 0;
  }

  for (int i = 0; i < MPA_MAX_CODED_FRAME_SIZE; i++)
  {
    m_FrameInBuffer[i] = 0;
  }
}

cParserMPEG2Audio::~cParserMPEG2Audio()
{
}

void cParserMPEG2Audio::Parse(unsigned char *data, int size, bool pusi)
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
    int hlen = ParsePESHeader(data, size);
    if (hlen <= 0)
      return;

    data += hlen;
    size -= hlen;

    m_PESStart = false;

    assert(size >= 0);
    if(size == 0)
      return;
  }

  while (size > 0)
  {
    uint8_t *outbuf;
    int      outlen;

    int rlen = FindHeaders(&outbuf, &outlen, data, size, m_curPTS, m_curDTS);
    if (rlen < 0)
      break;

    m_curPTS = DVD_NOPTS_VALUE;
    m_curDTS = DVD_NOPTS_VALUE;

    if (outlen)
    {
      sStreamPacket pkt;
      pkt.id       = m_streamID;
      pkt.data     = outbuf;
      pkt.size     = outlen;
      pkt.duration = 90000 * 1152 / m_SampleRate;
      pkt.dts      = m_DTS;
      if (pkt.dts == DVD_NOPTS_VALUE)
        pkt.dts = m_NextDTS;
      pkt.pts      = pkt.dts;
      m_NextDTS    = pkt.dts + pkt.duration;

      SendPacket(&pkt);
    }
    data += rlen;
    size -= rlen;
  }
}

int cParserMPEG2Audio::FindHeaders(uint8_t **poutbuf, int *poutbuf_size,
                                   uint8_t *buf, int buf_size,
                                   int64_t pts, int64_t dts)
{
  *poutbuf      = NULL;
  *poutbuf_size = 0;

  /* add a new packet descriptor */
  if (pts != DVD_NOPTS_VALUE || dts != DVD_NOPTS_VALUE)
  {
    int i = (m_CurrentFrameStartIndex + 1) & (AV_PARSER_PTS_NB - 1);
    m_CurrentFrameStartIndex  = i;
    m_CurrentFrameOffset[i]   = m_CurrentOffset;
    m_CurrentFrameEnd[i]      = m_CurrentOffset + buf_size;
    m_CurrentFramePTS[i]      = pts;
    m_CurrentFrameDTS[i]      = dts;
  }

  if (m_FetchTimestamp)
  {
    m_FetchTimestamp  = false;
    FetchTimestamp(0, false);
  }

  const uint8_t *buf_ptr = buf;
  while (buf_size > 0)
  {
    int len = m_FrameInBufferPtr - m_FrameInBuffer;
    if (m_FrameSize == 0)
    {
      /* special case for next header for first frame in free
         format case (XXX: find a simpler method) */
      if (m_FrameFreeFormatNextHeader != 0)
      {
        m_FrameInBuffer[3] = m_FrameFreeFormatNextHeader;
        m_FrameInBuffer[2] = m_FrameFreeFormatNextHeader>>8;
        m_FrameInBuffer[1] = m_FrameFreeFormatNextHeader>>16;
        m_FrameInBuffer[0] = m_FrameFreeFormatNextHeader>>24;
        m_FrameInBufferPtr = m_FrameInBuffer + 4;
        m_FrameFreeFormatNextHeader = 0;
        goto got_header;
      }
      /* no header seen : find one. We need at least MPA_HEADER_SIZE
         bytes to parse it */
      len = min(MPA_HEADER_SIZE - len, buf_size);
      if (len > 0)
      {
        memcpy(m_FrameInBufferPtr, buf_ptr, len);
        buf_ptr            += len;
        buf_size           -= len;
        m_FrameInBufferPtr += len;
      }
      if ((m_FrameInBufferPtr - m_FrameInBuffer) >= MPA_HEADER_SIZE)
      {
      got_header:
        MPADecodeHeader s;
        uint32_t header = ((m_FrameInBuffer[0] << 24) | (m_FrameInBuffer[1] << 16) | (m_FrameInBuffer[2] <<  8) | m_FrameInBuffer[3]);
        if (CheckHeader(header) && DecodeHeader(&s, header))
        {
          if ((header&SAME_HEADER_MASK) != (m_FrameHeader&SAME_HEADER_MASK) && m_FrameHeader)
            m_HeaderCount = -3;
          m_FrameHeader = header;
          m_FrameSize   = s.frame_size;
          m_SampleRate  = s.sample_rate;
          m_Channels    = s.nb_channels;
          m_BitRate     = s.bit_rate;
          m_HeaderCount++;
        }
        else
        {
          m_HeaderCount = -2;
          /* no sync found : move by one byte (inefficient, but simple!) */
          memmove(m_FrameInBuffer, m_FrameInBuffer + 1, m_FrameInBufferPtr - m_FrameInBuffer - 1);
          m_FrameInBufferPtr--;
          //LOGDBG("skip %x", header);
          /* reset free format frame size to give a chance
             to get a new bitrate */
          m_FrameFreeFormatFrameSize = 0;
        }
      }
    }
    else if (len < m_FrameSize)
    {
      if (m_FrameSize > MPA_MAX_CODED_FRAME_SIZE)
        m_FrameSize = MPA_MAX_CODED_FRAME_SIZE;
      len                 = min(m_FrameSize - len, buf_size);
      memcpy(m_FrameInBufferPtr, buf_ptr, len);
      buf_ptr            += len;
      m_FrameInBufferPtr += len;
      buf_size           -= len;
    }

    if(m_FrameSize > 0 && buf_ptr - buf == m_FrameInBufferPtr - m_FrameInBuffer && buf_size + buf_ptr - buf >= m_FrameSize)
    {
      if(m_HeaderCount > 0)
      {
        *poutbuf      = buf;
        *poutbuf_size = m_FrameSize;
      }
      buf_ptr             = buf + m_FrameSize;
      m_FrameInBufferPtr  = m_FrameInBuffer;
      m_FrameSize         = 0;
      break;
    }

    //    next_data:
    if (m_FrameSize > 0 && (m_FrameInBufferPtr - m_FrameInBuffer) >= m_FrameSize)
    {
      if (m_HeaderCount > 0)
      {
        *poutbuf      = m_FrameInBuffer;
        *poutbuf_size = m_FrameInBufferPtr - m_FrameInBuffer;
      }
      m_FrameInBufferPtr = m_FrameInBuffer;
      m_FrameSize = 0;
      break;
    }
  }
  int index = buf_ptr - buf;

  /* update the file pointer */
  if (*poutbuf_size)
  {
    /* fill the data for the current frame */
    m_FrameOffset     = m_NextFrameOffset;

    /* offset of the next frame */
    m_NextFrameOffset = m_CurrentOffset + index;
    m_FetchTimestamp  = true;
  }
  if (index < 0)
    index = 0;
  m_CurrentOffset += index;
  return index;
}

void cParserMPEG2Audio::FetchTimestamp(int off, bool remove)
{
  m_DTS = DVD_NOPTS_VALUE;
  m_PTS = DVD_NOPTS_VALUE;
  m_Offset = 0;
  for (int i = 0; i < AV_PARSER_PTS_NB; i++)
  {
    if (   m_NextFrameOffset + off >= m_CurrentFrameOffset[i]
        &&(m_FrameOffset           <  m_CurrentFrameOffset[i] || !m_FrameOffset)
        && m_CurrentFrameEnd[i])
    {
      m_DTS    = m_CurrentFrameDTS[i];
      m_PTS    = m_CurrentFramePTS[i];
      m_Offset = m_NextFrameOffset - m_CurrentFrameOffset[i];
      if (remove)
        m_CurrentFrameOffset[i] = -1;
    }
  }
}

bool cParserMPEG2Audio::DecodeHeader(MPADecodeHeader *s, uint32_t header)
{
  const uint16_t FrequencyTable[3] = { 44100, 48000, 32000 };
  const uint16_t BitrateTable[2][3][15] =
  {
    {
      {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      {0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384 },
      {0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320 }
    },
    {
      {0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256},
      {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},
      {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160}
    }
  };

  int sample_rate, frame_size, mpeg25, padding;
  int sample_rate_index, bitrate_index;
  if (header & (1<<20))
  {
    s->lsf = (header & (1<<19)) ? 0 : 1;
    mpeg25 = 0;
  }
  else
  {
    s->lsf = 1;
    mpeg25 = 1;
  }

  s->layer              = 4 - ((header >> 17) & 3);
  /* extract frequency */
  sample_rate_index     = (header >> 10) & 3;
  sample_rate           = FrequencyTable[sample_rate_index] >> (s->lsf + mpeg25);
  sample_rate_index    += 3 * (s->lsf + mpeg25);
  s->sample_rate_index  = sample_rate_index;
  s->error_protection   = ((header >> 16) & 1) ^ 1;
  s->sample_rate        = sample_rate;

  bitrate_index         = (header >> 12) & 0xf;
  padding               = (header >> 9) & 1;
  //extension           = (header >> 8) & 1;
  s->mode               = (header >> 6) & 3;
  s->mode_ext           = (header >> 4) & 3;
  //copyright           = (header >> 3) & 1;
  //original            = (header >> 2) & 1;
  //emphasis            = header & 3;

  if (s->mode == MPA_MONO)
    s->nb_channels = 1;
  else
    s->nb_channels = 2;

  if (bitrate_index != 0)
  {
    frame_size  = BitrateTable[s->lsf][s->layer - 1][bitrate_index];
    s->bit_rate = frame_size * 1000;
    switch(s->layer)
    {
    case 1:
      frame_size = (frame_size * 12000) / sample_rate;
      frame_size = (frame_size + padding) * 4;
      break;
    case 2:
      frame_size = (frame_size * 144000) / sample_rate;
      frame_size += padding;
      break;
    default:
    case 3:
      frame_size = (frame_size * 144000) / (sample_rate << s->lsf);
      frame_size += padding;
      break;
    }
    s->frame_size = frame_size;
  }
  else
  {
    /* if no frame size computed, signal it */
    return false;
  }

  m_demuxer->SetAudioInformation(s->nb_channels, s->sample_rate, s->bit_rate, 0, 0);

#if defined(DEBUG)
#define MODE_EXT_MS_STEREO 2
#define MODE_EXT_I_STEREO  1
  printf("layer%d, %d Hz, %d kbits/s, ", s->layer, s->sample_rate, s->bit_rate);
  if (s->nb_channels == 2)
  {
    if (s->layer == 3)
    {
      if (s->mode_ext & MODE_EXT_MS_STEREO)
        printf("ms-");
      if (s->mode_ext & MODE_EXT_I_STEREO)
        printf("i-");
    }
    printf("stereo");
  }
  else
  {
    printf("mono");
  }
  printf("\n");
#endif
  return true;
}
