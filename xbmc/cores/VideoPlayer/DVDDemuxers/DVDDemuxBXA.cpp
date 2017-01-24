/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#include "DVDDemuxBXA.h"
#include "DVDDemuxUtils.h"
#include "utils/StringUtils.h"
#include "TimingConstants.h"

// AirTunes audio Demuxer.

class CDemuxStreamAudioBXA
  : public CDemuxStreamAudio
{
  std::string    m_codec;
public:
  CDemuxStreamAudioBXA(CDVDDemuxBXA *parent, const std::string& codec)
    : m_codec(codec)

  {}
};

CDVDDemuxBXA::CDVDDemuxBXA() : CDVDDemux()
{
  m_pInput = NULL;
  m_stream = NULL;
  m_bytes = 0;
  memset(&m_header, 0x0, sizeof(Demux_BXA_FmtHeader));
}

CDVDDemuxBXA::~CDVDDemuxBXA()
{
  Dispose();
}

bool CDVDDemuxBXA::Open(CDVDInputStream* pInput)
{
  Abort();

  Dispose();

  if(!pInput || !pInput->IsStreamType(DVDSTREAM_TYPE_FILE))
    return false;

  if(pInput->Read((uint8_t *)&m_header, sizeof(Demux_BXA_FmtHeader)) < 1)
    return false;

  // file valid?
  if (strncmp(m_header.fourcc, "BXA ", 4) != 0 || m_header.type != BXA_PACKET_TYPE_FMT_DEMUX)
  {
    pInput->Seek(0, SEEK_SET);
    return false;
  }

  m_pInput = pInput;

  m_stream = new CDemuxStreamAudioBXA(this, "BXA");

  if(!m_stream)
    return false;

  m_stream->iSampleRate     = m_header.sampleRate;
  m_stream->iBitsPerSample  = m_header.bitsPerSample;
  m_stream->iBitRate        = m_header.sampleRate * m_header.channels * m_header.bitsPerSample;
  m_stream->iChannels       = m_header.channels;
  m_stream->type            = STREAM_AUDIO;
  m_stream->codec           = AV_CODEC_ID_PCM_S16LE;

  return true;
}

void CDVDDemuxBXA::Dispose()
{
  delete m_stream;
  m_stream = NULL;

  m_pInput = NULL;
  m_bytes = 0;

  memset(&m_header, 0x0, sizeof(Demux_BXA_FmtHeader));
}

void CDVDDemuxBXA::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxBXA::Abort()
{
  if(m_pInput)
    return m_pInput->Abort();
}

void CDVDDemuxBXA::Flush()
{
}

#define BXA_READ_SIZE 4096
DemuxPacket* CDVDDemuxBXA::Read()
{
  if(!m_pInput)
    return NULL;

  DemuxPacket* pPacket = CDVDDemuxUtils::AllocateDemuxPacket(BXA_READ_SIZE);

  if (!pPacket)
  {
    if (m_pInput)
      m_pInput->Close();
    return NULL;
  }

  pPacket->iSize = m_pInput->Read(pPacket->pData, BXA_READ_SIZE);
  pPacket->iStreamId = 0;

  if(pPacket->iSize < 1)
  {
    delete pPacket;
    pPacket = NULL;
  }
  else
  {
    int n = (m_header.channels * m_header.bitsPerSample * m_header.sampleRate)>>3;
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

CDemuxStream* CDVDDemuxBXA::GetStream(int iStreamId) const
{
  if(iStreamId != 0)
    return NULL;

  return m_stream;
}

std::vector<CDemuxStream*> CDVDDemuxBXA::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  if (m_stream != nullptr)
  {
    streams.push_back(m_stream);
  }

  return streams;
}

int CDVDDemuxBXA::GetNrOfStreams() const
{
  return (m_stream == NULL ? 0 : 1);
}

std::string CDVDDemuxBXA::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxBXA::GetStreamCodecName(int iStreamId)
{
  if (m_stream && iStreamId == 0)
    return "BXA";
  else
    return "";
}
