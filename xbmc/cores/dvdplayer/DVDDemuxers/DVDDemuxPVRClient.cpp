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
#include "pvr/PVRManager.h"
#ifdef _WIN32
#include <libavcodec/avcodec.h>
#endif

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
    CPVRManager::GetClients()->DemuxReset();

  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxPVRClient::Abort()
{
  if(m_pInput)
    CPVRManager::GetClients()->DemuxAbort();
}

void CDVDDemuxPVRClient::Flush()
{
  if(m_pInput)
    CPVRManager::GetClients()->DemuxFlush();
}

DemuxPacket* CDVDDemuxPVRClient::Read()
{
  DemuxPacket* pPacket = CPVRManager::GetClients()->ReadDemuxStream();
  if (!pPacket)
    return CDVDDemuxUtils::AllocateDemuxPacket(0);

  if (pPacket->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    UpdateStreams((PVR_STREAM_PROPERTIES*)pPacket->pData);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (pPacket->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    Reset();
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }

  return pPacket;
}

CDemuxStream* CDVDDemuxPVRClient::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_PVR_STREAMS) return NULL;
    return m_streams[iStreamId];
}

void CDVDDemuxPVRClient::RequestStreams()
{
  PVR_STREAM_PROPERTIES *props = CPVRManager::GetClients()->GetCurrentStreamProperties();

  for (unsigned int i = 0; i < props->iStreamCount; ++i)
  {
    if (props->stream[i].iCodecType == CODEC_TYPE_AUDIO)
    {
      CDemuxStreamAudioPVRClient* st = new CDemuxStreamAudioPVRClient(this);
      st->iChannels       = props->stream[i].iChannels;
      st->iSampleRate     = props->stream[i].iSampleRate;
      st->iBlockAlign     = props->stream[i].iBlockAlign;
      st->iBitRate        = props->stream[i].iBitRate;
      st->iBitsPerSample  = props->stream[i].iBitsPerSample;
      m_streams[props->stream[i].iStreamIndex] = st;
    }
    else if (props->stream[i].iCodecType == CODEC_TYPE_VIDEO)
    {
      CDemuxStreamVideoPVRClient* st = new CDemuxStreamVideoPVRClient(this);
      st->iFpsScale       = props->stream[i].iFPSScale;
      st->iFpsRate        = props->stream[i].iFPSRate;
      st->iHeight         = props->stream[i].iHeight;
      st->iWidth          = props->stream[i].iWidth;
      st->fAspect         = props->stream[i].fAspect;
      m_streams[props->stream[i].iStreamIndex] = st;
    }
    else if (props->stream[i].iCodecId == CODEC_ID_DVB_TELETEXT)
    {
      m_streams[props->stream[i].iStreamIndex] = new CDemuxStreamTeletext();
    }
    else if (props->stream[i].iCodecType == CODEC_TYPE_SUBTITLE)
    {
      CDemuxStreamSubtitlePVRClient* st = new CDemuxStreamSubtitlePVRClient(this);
      st->identifier      = props->stream[i].iIdentifier;
      m_streams[props->stream[i].iStreamIndex] = st;
    }
    else
      m_streams[props->stream[i].iStreamIndex] = new CDemuxStream();

    m_streams[props->stream[i].iStreamIndex]->codec       = (CodecID)props->stream[i].iCodecId;
    m_streams[props->stream[i].iStreamIndex]->iId         = props->stream[i].iIdentifier;
    m_streams[props->stream[i].iStreamIndex]->iPhysicalId = props->stream[i].iPhysicalId;
    m_streams[props->stream[i].iStreamIndex]->language[0] = props->stream[i].strLanguage[0];
    m_streams[props->stream[i].iStreamIndex]->language[1] = props->stream[i].strLanguage[1];
    m_streams[props->stream[i].iStreamIndex]->language[2] = props->stream[i].strLanguage[2];
    m_streams[props->stream[i].iStreamIndex]->language[3] = props->stream[i].strLanguage[3];

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): added stream %d:%d with codec_id %d",
        m_streams[props->stream[i].iStreamIndex]->iId,
        m_streams[props->stream[i].iStreamIndex]->iPhysicalId,
        m_streams[props->stream[i].iStreamIndex]->codec);
  }
}

void CDVDDemuxPVRClient::UpdateStreams(PVR_STREAM_PROPERTIES *props)
{
  for (unsigned int i = 0; i < props->iStreamCount; ++i)
  {
    if (m_streams[props->stream[i].iStreamIndex] == NULL ||
        m_streams[props->stream[i].iStreamIndex]->codec != (CodecID)props->stream[i].iCodecId)
    {
      CLog::Log(LOGERROR,"Invalid stream inside UpdateStreams");
      continue;
    }

    if (m_streams[props->stream[i].iStreamIndex]->type == STREAM_AUDIO)
    {
      CDemuxStreamAudioPVRClient* st = (CDemuxStreamAudioPVRClient*) m_streams[props->stream[i].iStreamIndex];
      st->iChannels       = props->stream[i].iChannels;
      st->iSampleRate     = props->stream[i].iSampleRate;
      st->iBlockAlign     = props->stream[i].iBlockAlign;
      st->iBitRate        = props->stream[i].iBitRate;
      st->iBitsPerSample  = props->stream[i].iBitsPerSample;
    }
    else if (m_streams[props->stream[i].iStreamIndex]->type == STREAM_VIDEO)
    {
      CDemuxStreamVideoPVRClient* st = (CDemuxStreamVideoPVRClient*) m_streams[props->stream[i].iStreamIndex];
      st->iFpsScale       = props->stream[i].iFPSScale;
      st->iFpsRate        = props->stream[i].iFPSRate;
      st->iHeight         = props->stream[i].iHeight;
      st->iWidth          = props->stream[i].iWidth;
      st->fAspect         = props->stream[i].fAspect;
    }
    else if (m_streams[props->stream[i].iStreamIndex]->type == STREAM_SUBTITLE)
    {
      CDemuxStreamSubtitlePVRClient* st = (CDemuxStreamSubtitlePVRClient*) m_streams[props->stream[i].iStreamIndex];
      st->identifier      = props->stream[i].iIdentifier;
    }

    m_streams[props->stream[i].iStreamIndex]->language[0] = props->stream[i].strLanguage[0];
    m_streams[props->stream[i].iStreamIndex]->language[1] = props->stream[i].strLanguage[1];
    m_streams[props->stream[i].iStreamIndex]->language[2] = props->stream[i].strLanguage[2];
    m_streams[props->stream[i].iStreamIndex]->language[3] = props->stream[i].strLanguage[3];

    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::UpdateStreams(): update stream %d:%d with codec_id %d",
        m_streams[props->stream[i].iStreamIndex]->iId,
        m_streams[props->stream[i].iStreamIndex]->iPhysicalId,
        m_streams[props->stream[i].iStreamIndex]->codec);
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

void CDVDDemuxPVRClient::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    if (stream->codec == CODEC_ID_AC3)
      strName = "ac3";
    else if (stream->codec == CODEC_ID_MP2)
      strName = "mp2";
    else if (stream->codec == CODEC_ID_AAC)
      strName = "aac";
    else if (stream->codec == CODEC_ID_DTS)
      strName = "dca";
    else if (stream->codec == CODEC_ID_MPEG2VIDEO)
      strName = "mpeg2video";
    else if (stream->codec == CODEC_ID_H264)
      strName = "h264";
  }
}
