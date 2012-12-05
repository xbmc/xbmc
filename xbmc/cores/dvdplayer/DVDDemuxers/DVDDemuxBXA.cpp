/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxBXA.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "../DVDClock.h"

// AirTunes audio Demuxer.

using namespace std;

class CDemuxStreamAudioBXA
  : public CDemuxStreamAudio
{
  CDVDDemuxBXA  *m_parent;
  string         m_codec;
public:
  CDemuxStreamAudioBXA(CDVDDemuxBXA *parent, const string& codec)
    : m_parent(parent)
    , m_codec(codec)

  {}
  void GetStreamInfo(string& strInfo)
  {
    CStdString info;
    info.Format("%s", m_codec.c_str());
    strInfo = info;
  }
};

CDVDDemuxBXA::CDVDDemuxBXA() : CDVDDemux()
{
  m_pInput = NULL;
  m_stream = NULL;
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

  if(pInput->Read((BYTE *)&m_header, sizeof(Demux_BXA_FmtHeader)) < 1)
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
  m_stream->codec           = CODEC_ID_PCM_S16LE;

  return true;
}

void CDVDDemuxBXA::Dispose()
{
  delete m_stream;
  m_stream = NULL;

  m_pInput = NULL;
  m_pts    = 0;

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
      m_pts += ((double)pPacket->iSize * DVD_TIME_BASE) / n;
      pPacket->dts = m_pts;
      pPacket->pts = m_pts;
    }
    else
    {
      pPacket->dts = DVD_NOPTS_VALUE;
      pPacket->pts = DVD_NOPTS_VALUE;
    }
  }

  return pPacket;
}

CDemuxStream* CDVDDemuxBXA::GetStream(int iStreamId)
{
  if(iStreamId != 0)
    return NULL;

  return m_stream;
}

int CDVDDemuxBXA::GetNrOfStreams()
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

void CDVDDemuxBXA::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  if (m_stream && iStreamId == 0)
    strName = "BXA";
}
