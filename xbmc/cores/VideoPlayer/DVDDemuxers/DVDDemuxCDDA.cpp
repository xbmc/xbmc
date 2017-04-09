/*
 *      Copyright (C) 2013 Team XBMC
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

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxCDDA.h"
#include "DVDDemuxUtils.h"
#include "TimingConstants.h"

// CDDA audio demuxer based on AirTunes audio Demuxer.

class CDemuxStreamAudioCDDA
  : public CDemuxStreamAudio
{
};

CDVDDemuxCDDA::CDVDDemuxCDDA() : CDVDDemux()
{
  m_pInput = NULL;
  m_stream = NULL;
  m_bytes  = 0;
}

CDVDDemuxCDDA::~CDVDDemuxCDDA()
{
  Dispose();
}

bool CDVDDemuxCDDA::Open(CDVDInputStream* pInput)
{
  Abort();

  Dispose();

  if(!pInput || !pInput->IsStreamType(DVDSTREAM_TYPE_FILE))
    return false;

  m_pInput = pInput;

  m_stream = new CDemuxStreamAudioCDDA();

  if(!m_stream)
    return false;

  m_stream->iSampleRate     = 44100;
  m_stream->iBitsPerSample  = 16;
  m_stream->iBitRate        = 44100 * 2 * 16;
  m_stream->iChannels       = 2;
  m_stream->type            = STREAM_AUDIO;
  m_stream->codec           = AV_CODEC_ID_PCM_S16LE;

  return true;
}

void CDVDDemuxCDDA::Dispose()
{
  delete m_stream;
  m_stream = NULL;

  m_pInput = NULL;
  m_bytes  = 0;
}

void CDVDDemuxCDDA::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxCDDA::Abort()
{
  if(m_pInput)
    return m_pInput->Abort();
}

void CDVDDemuxCDDA::Flush()
{
}

#define CDDA_READ_SIZE 4096
DemuxPacket* CDVDDemuxCDDA::Read()
{
  if(!m_pInput)
    return NULL;

  DemuxPacket* pPacket = CDVDDemuxUtils::AllocateDemuxPacket(CDDA_READ_SIZE);

  if (!pPacket)
  {
    if (m_pInput)
      m_pInput->Close();
    return NULL;
  }

  pPacket->iSize = m_pInput->Read(pPacket->pData, CDDA_READ_SIZE);
  pPacket->iStreamId = 0;

  if(pPacket->iSize < 1)
  {
    delete pPacket;
    pPacket = NULL;
  }
  else
  {
    int n = m_stream->iBitRate>>3;
    if (n > 0)
    {
      m_bytes += pPacket->iSize;
      pPacket->dts = (double)m_bytes * DVD_TIME_BASE / n;
      pPacket->pts = pPacket->dts;
    }
    else
    {
      pPacket->dts = DVD_NOPTS_VALUE;
      pPacket->pts = DVD_NOPTS_VALUE;
    }
  }

  return pPacket;
}

bool CDVDDemuxCDDA::SeekTime(double time, bool backwards, double* startpts)
{
  int bytes_per_second = m_stream->iBitRate>>3;
  // clamp seeks to bytes per full sample
  int clamp_bytes = (m_stream->iBitsPerSample>>3) * m_stream->iChannels;

  // time is in milliseconds
  int64_t seekPos = m_pInput->Seek((((int64_t)time * bytes_per_second / 1000) / clamp_bytes ) * clamp_bytes, SEEK_SET) > 0;
  if (seekPos > 0)
    m_bytes = seekPos;

  if (startpts)
    *startpts = (double)m_bytes * DVD_TIME_BASE / bytes_per_second;

  return seekPos > 0;
};

int CDVDDemuxCDDA::GetStreamLength()
{
  int64_t num_track_bytes = m_pInput->GetLength();
  int bytes_per_second = (m_stream->iBitRate>>3);
  int64_t track_mseconds = num_track_bytes*1000 / bytes_per_second;
  return (int)track_mseconds;
}

CDemuxStream* CDVDDemuxCDDA::GetStream(int iStreamId) const
{
  if(iStreamId != 0)
    return NULL;

  return m_stream;
}

std::vector<CDemuxStream*> CDVDDemuxCDDA::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  if (m_stream != nullptr)
  {
    streams.push_back(m_stream);
  }

  return streams;
}

int CDVDDemuxCDDA::GetNrOfStreams() const
{
  return (m_stream == NULL ? 0 : 1);
}

std::string CDVDDemuxCDDA::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxCDDA::GetStreamCodecName(int iStreamId)
{
  if (m_stream && iStreamId == 0)
    return "pcm";
  else
    return "";
}
