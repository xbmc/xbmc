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
#include <vdr/remux.h>
#include <vdr/channels.h>
#include "config.h"
#include "receiver.h"
#include "demuxer.h"
#include "demuxer_AAC.h"
#include "demuxer_AC3.h"
#include "demuxer_DTS.h"
#include "demuxer_h264.h"
#include "demuxer_MPEGAudio.h"
#include "demuxer_MPEGVideo.h"
#include "demuxer_Subtitle.h"
#include "demuxer_Teletext.h"

#define PTS_MASK 0x1ffffffffLL
//#define PTS_MASK 0x7ffffLL

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL-1)
#endif

int64_t PesGetPTS(const uint8_t *buf, int len)
{
  /* assume mpeg2 pes header ... */
  if (PesIsVideoPacket(buf) || PesIsAudioPacket(buf)) {

    if ((buf[6] & 0xC0) != 0x80)
      return DVD_NOPTS_VALUE;
    if ((buf[6] & 0x30) != 0)
      return DVD_NOPTS_VALUE;

    if ((len > 13) && (buf[7] & 0x80)) { /* pts avail */
      int64_t pts;
      pts  = ((int64_t)(buf[ 9] & 0x0E)) << 29 ;
      pts |= ((int64_t) buf[10])         << 22 ;
      pts |= ((int64_t)(buf[11] & 0xFE)) << 14 ;
      pts |= ((int64_t) buf[12])         <<  7 ;
      pts |= ((int64_t)(buf[13] & 0xFE)) >>  1 ;
      return pts;
    }
  }
  return DVD_NOPTS_VALUE;
}

int64_t PesGetDTS(const uint8_t *buf, int len)
{
  if (PesIsVideoPacket(buf) || PesIsAudioPacket(buf))
  {
    if ((buf[6] & 0xC0) != 0x80)
      return DVD_NOPTS_VALUE;
    if ((buf[6] & 0x30) != 0)
      return DVD_NOPTS_VALUE;

    if (len > 18 && (buf[7] & 0x40)) { /* dts avail */
      int64_t dts;
      dts  = ((int64_t)( buf[14] & 0x0E)) << 29 ;
      dts |=  (int64_t)( buf[15]         << 22 );
      dts |=  (int64_t)((buf[16] & 0xFE) << 14 );
      dts |=  (int64_t)( buf[17]         <<  7 );
      dts |=  (int64_t)((buf[18] & 0xFE) >>  1 );
      return dts;
    }
  }
  return DVD_NOPTS_VALUE;
}

// --- cParser -------------------------------------------------

cParser::cParser(cLiveStreamer *streamer, int streamID)
 : m_Streamer(streamer)
 , m_streamID(streamID)
 , m_FoundFrame(false)
{
  m_curPTS    = DVD_NOPTS_VALUE;
  m_curDTS    = DVD_NOPTS_VALUE;
  m_LastDTS   = DVD_NOPTS_VALUE;
  m_epochDTS  = 0;
  m_badDTS    = 0;
}

int64_t cParser::Rescale(int64_t a)
{
  uint64_t b = DVD_TIME_BASE;
  uint64_t c = 90000;
  uint64_t r = c/2;

  if (b<=INT_MAX && c<=INT_MAX){
    if (a<=INT_MAX)
      return (a * b + r)/c;
    else
      return a/c*b + (a%c*b + r)/c;
  }
  else
  {
    uint64_t a0= a&0xFFFFFFFF;
    uint64_t a1= a>>32;
    uint64_t b0= b&0xFFFFFFFF;
    uint64_t b1= b>>32;
    uint64_t t1= a0*b1 + a1*b0;
    uint64_t t1a= t1<<32;

    a0 = a0*b0 + t1a;
    a1 = a1*b1 + (t1>>32) + (a0<t1a);
    a0 += r;
    a1 += a0<r;

    for (int i=63; i>=0; i--)
    {
      a1+= a1 + ((a0>>i)&1);
      t1+=t1;
      if (c <= a1)
      {
        a1 -= c;
        t1++;
      }
    }
    return t1;
  }
}

/*
 * Extract DTS and PTS and update current values in stream
 */
int cParser::ParsePESHeader(uint8_t *buf, size_t len)
{
  /* parse PES header */
  unsigned int hdr_len = PesHeaderLength(buf);

  /* parse PTS */
  int64_t pts = PesGetPTS(buf, len);
  int64_t dts = PesGetDTS(buf, len);
  if (dts == DVD_NOPTS_VALUE)
    dts = pts;
  
  dts = dts & PTS_MASK;
  pts = pts & PTS_MASK;
  
  if(pts != 0) m_curDTS = dts;
  if(dts != 0) m_curPTS = pts;
  
  return hdr_len;
}

void cParser::BufferPacket(sStreamPacket *pkt)
{
  // create new packet
  sStreamPacket* p = new sStreamPacket(*pkt);

  // copy payload
  p->data = (uint8_t*)malloc(pkt->size);
  memcpy(p->data, pkt->data, pkt->size);

  // push to queue
  m_queue.push(p);
}

void cParser::SendPacket(sStreamPacket *pkt)
{
  if(pkt->dts == DVD_NOPTS_VALUE) return;
  if(pkt->pts == DVD_NOPTS_VALUE) return;

  int64_t dts = pkt->dts;
  int64_t pts = pkt->pts;

  // Rescale for XBMC
  pkt->dts      = Rescale(dts);
  pkt->pts      = Rescale(pts);
  pkt->duration = Rescale(pkt->duration);

  // buffer packets if we are not ready to send
  if (m_Streamer->IsStarting()) {
    BufferPacket(pkt);
    return;
  }

  // stream packet if queue is empty
  if(m_queue.size() == 0) {
    m_Streamer->sendStreamPacket(pkt);
    return;
  }

  BufferPacket(pkt);

  // send buffered data first
  INFOLOG("sending %i buffered packets", (int)m_queue.size());

  while(m_queue.size() > 0) {
    sStreamPacket* p = m_queue.front();
    if(p != NULL) {
      m_Streamer->sendStreamPacket(p);
      free(p->data);
      delete p;
    }
    m_queue.pop();
  }
}


// --- cTSDemuxer ----------------------------------------------------

cTSDemuxer::cTSDemuxer(cLiveStreamer *streamer, int id, eStreamType type, int pid)
  : m_Streamer(streamer)
  , m_streamID(id)
  , m_pID(pid)
  , m_streamType(type)
{
  m_pesError        = false;
  m_pesParser       = NULL;
  m_language[0]     = 0;
  m_FpsScale        = 0;
  m_FpsRate         = 0;
  m_Height          = 0;
  m_Width           = 0;
  m_Aspect          = 0.0f;
  m_Channels        = 0;
  m_SampleRate      = 0;
  m_BitRate         = 0;
  m_BitsPerSample   = 0;
  m_BlockAlign      = 0;

  if (m_streamType == stMPEG2VIDEO)
    m_pesParser = new cParserMPEG2Video(this, m_Streamer, m_streamID);
  else if (m_streamType == stH264)
    m_pesParser = new cParserH264(this, m_Streamer, m_streamID);
  else if (m_streamType == stMPEG2AUDIO)
    m_pesParser = new cParserMPEG2Audio(this, m_Streamer, m_streamID);
  else if (m_streamType == stAAC)
    m_pesParser = new cParserAAC(this, m_Streamer, m_streamID);
  else if (m_streamType == stAC3)
    m_pesParser = new cParserAC3(this, m_Streamer, m_streamID);
  else if (m_streamType == stDTS)
    m_pesParser = new cParserDTS(this, m_Streamer, m_streamID);
  else if (m_streamType == stEAC3)
    m_pesParser = new cParserAC3(this, m_Streamer, m_streamID);
  else if (m_streamType == stTELETEXT)
    m_pesParser = new cParserTeletext(this, m_Streamer, m_streamID);
  else if (m_streamType == stDVBSUB)
    m_pesParser = new cParserSubtitle(this, m_Streamer, m_streamID);
  else
  {
    ERRORLOG("Unrecognised type %i inside stream %i", m_streamType, m_streamID);
    return;
  }
}

cTSDemuxer::~cTSDemuxer()
{
  if (m_pesParser)
  {
    delete m_pesParser;
    m_pesParser = NULL;
  }
}

bool cTSDemuxer::ProcessTSPacket(unsigned char *data)
{
  if (!data)
    return false;

  bool pusi  = TsPayloadStart(data);
  int  bytes = TS_SIZE - TsPayloadOffset(data);

  if(bytes < 0 || bytes > TS_SIZE)
    return false;

  if (TsError(data))
  {
    ERRORLOG("transport error");
    return false;
  }

  if (!TsHasPayload(data))
  {
    DEBUGLOG("no payload, size %d", bytes);
    return true;
  }

  /* drop broken PES packets */
  if (m_pesError && !pusi)
  {
    return false;
  }

  /* strip ts header */
  data += TS_SIZE - bytes;

  /* handle new payload unit */
  if (pusi)
  {
    if (!PesIsHeader(data))
    {
      m_pesError = true;
      return false;
    }
    m_pesError = false;
  }

  /* Parse the data */
  if (m_pesParser)
    m_pesParser->Parse(data, bytes, pusi);

  return true;
}

void cTSDemuxer::SetLanguage(const char *language)
{
  m_language[0] = language[0];
  m_language[1] = language[1];
  m_language[2] = language[2];
  m_language[3] = 0;
}

void cTSDemuxer::SetVideoInformation(int FpsScale, int FpsRate, int Height, int Width, float Aspect)
{
  m_FpsScale        = FpsScale;
  m_FpsRate         = FpsRate;
  m_Height          = Height;
  m_Width           = Width;
  m_Aspect          = Aspect;
}

void cTSDemuxer::SetAudioInformation(int Channels, int SampleRate, int BitRate, int BitsPerSample, int BlockAlign)
{
  m_Channels        = Channels;
  m_SampleRate      = SampleRate;
  m_BlockAlign      = BlockAlign;
  m_BitRate         = BitRate;
  m_BitsPerSample   = BitsPerSample;
}

void cTSDemuxer::SetSubtitlingDescriptor(unsigned char SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId)
{
  m_subtitlingType    = SubtitlingType;
  m_compositionPageId = CompositionPageId;
  m_ancillaryPageId   = AncillaryPageId;
}
