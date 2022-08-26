/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxBXA.h"

#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/StringUtils.h"

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
  m_stream = NULL;
  m_bytes = 0;
  memset(&m_header, 0x0, sizeof(Demux_BXA_FmtHeader));
}

CDVDDemuxBXA::~CDVDDemuxBXA()
{
  Dispose();
}

bool CDVDDemuxBXA::Open(const std::shared_ptr<CDVDInputStream>& pInput)
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

bool CDVDDemuxBXA::Reset()
{
  std::shared_ptr<CDVDInputStream> pInputStream = m_pInput;
  Dispose();
  return Open(pInputStream);
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
