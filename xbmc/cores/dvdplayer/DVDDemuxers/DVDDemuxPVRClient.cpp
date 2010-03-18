/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxPVRClient.h"
#include "PVRManager.h"

void CDemuxStreamVideoPVRClient::GetStreamInfo(std::string& strInfo)
{
}

void CDemuxStreamAudioPVRClient::GetStreamInfo(std::string& strInfo)
{
  switch (codec)
  {
    case CODEC_ID_AC3:
      strInfo = "AC3";
      break;
    default:
      break;
  }
}

void CDemuxStreamSubtitlePVRClient::GetStreamInfo(std::string& strInfo)
{
}

CDVDDemuxPVRClient::CDVDDemuxPVRClient() : CDVDDemux()
{
  m_pInput = NULL;
  for (int i = 0; i < MAX_PVR_STREAMS; i++) m_streams[i] = NULL;
}

CDVDDemuxPVRClient::~CDVDDemuxPVRClient()
{
  Dispose();
}

bool CDVDDemuxPVRClient::Open(CDVDInputStream* pInput)
{
  Abort();
  m_pInput = pInput;
  RequestStreams();
  return true;
}

void CDVDDemuxPVRClient::Dispose()
{
  for (int i = 0; i < MAX_PVR_STREAMS; i++)
  {
    if (m_streams[i])
    {
      if (m_streams[i]->ExtraData)
        delete[] (BYTE*)(m_streams[i]->ExtraData);
      delete m_streams[i];
    }
    m_streams[i] = NULL;
  }
  m_pInput = NULL;
}

void CDVDDemuxPVRClient::Reset()
{
  if(m_pInput)
    g_PVRManager.DemuxReset();

  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxPVRClient::Abort()
{
  if(m_pInput)
    g_PVRManager.DemuxAbort();
}

void CDVDDemuxPVRClient::Flush()
{
  if(m_pInput)
    g_PVRManager.DemuxFlush();
}

DemuxPacket* CDVDDemuxPVRClient::Read()
{
  DemuxPacket* pPacket = g_PVRManager.ReadDemuxStream();
  if (!pPacket)
    return CDVDDemuxUtils::AllocateDemuxPacket(0);

  return pPacket;
}

CDemuxStream* CDVDDemuxPVRClient::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_PVR_STREAMS) return NULL;
    return m_streams[iStreamId];
}

void CDVDDemuxPVRClient::RequestStreams()
{
  PVR_STREAMPROPS *props = g_PVRManager.GetCurrentStreamProps();

  for (int i = 0; i < props->nstreams; ++i)
  {
    if (props->stream[i].codec_type == CODEC_TYPE_AUDIO)
      m_streams[props->stream[i].id]              = new CDemuxStreamAudioPVRClient(this);
    else if (props->stream[i].codec_type == CODEC_TYPE_VIDEO)
      m_streams[props->stream[i].id]              = new CDemuxStreamVideoPVRClient(this);
    else if (props->stream[i].codec_id == CODEC_ID_DVB_TELETEXT)
      m_streams[props->stream[i].id]              = new CDemuxStreamTeletext();
    else if (props->stream[i].codec_type == CODEC_TYPE_SUBTITLE)
      m_streams[props->stream[i].id]              = new CDemuxStreamSubtitlePVRClient(this);
    else
      m_streams[props->stream[i].id]              = new CDemuxStream();

    m_streams[props->stream[i].id]->codec       = (CodecID)props->stream[i].codec_id;
    m_streams[props->stream[i].id]->iId         = props->stream[i].id;
    m_streams[props->stream[i].id]->iPhysicalId = props->stream[i].physid;

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): added codec_id %d", m_streams[props->stream[i].id]->codec);
  }
}

int CDVDDemuxPVRClient::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_PVR_STREAMS && m_streams[i]) i++;
  return i;
}

std::string CDVDDemuxPVRClient::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}
