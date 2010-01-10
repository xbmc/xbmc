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

#include "system.h"
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef _LINUX
#include "stdint.h"
#else
#define INT64_C __int64
#endif
#include "PVRManager.h"
#include "DVDDemuxPVRClient.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#ifdef HAS_FILESYSTEM_MMS
#include "DVDInputStreams/DVDInputStreamMMS.h"
#endif
#include "DVDInputStreams/DVDInputStreamRTMP.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h" // for DVD_TIME_BASE
#include "utils/Win32Exception.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FileSystem/File.h"
#include "utils/log.h"
#include "Thread.h"
#include "utils/TimeUtils.h"

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
  InitializeCriticalSection(&m_critSection);
}

CDVDDemuxPVRClient::~CDVDDemuxPVRClient()
{
  DeleteCriticalSection(&m_critSection);

  for (std::map<int, CDemuxStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it)
  {
    CDemuxStream* s = it->second;
    free(s);
  }
}

/*
 * Open input stream
 */
bool CDVDDemuxPVRClient::Open(CDVDInputStream* pInput)
{
  Abort();

  m_pInput = pInput;
  return true;
}

/*
 * Reset the entire demuxer (same result as closing and opening it)
 */
void CDVDDemuxPVRClient::Reset()
{
}

/*
 * Aborts any internal reading that might be stalling main thread
 * NOTICE - this can be called from another thread
 */
void CDVDDemuxPVRClient::Abort()
{
  m_streams.clear();
}

/*
 * Flush the demuxer, if any data is kept in buffers, this should be freed now
 */
void CDVDDemuxPVRClient::Flush()
{
}

/*
 * Read a packet, returns NULL on error
 *
 */
DemuxPacket* CDVDDemuxPVRClient::Read()
{
  DemuxPacket* pPacket = (DemuxPacket*)malloc(sizeof(DemuxPacket));
  if (!m_pInput->Read((uint8_t*)pPacket, sizeof(DemuxPacket)))
  {
    free(pPacket);
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }

  if (m_streams.find(pPacket->iStreamId) == m_streams.end())
    RequestStreams();

  return pPacket;
}

/*
 * returns the total time in msec
 */
int CDVDDemuxPVRClient::GetStreamLength()
{
  return 1000000;
}

/*
 * returns the stream or NULL on error, starting from 0
 */
CDemuxStream* CDVDDemuxPVRClient::GetStream(int iStreamId)
{
  if (m_streams.find(iStreamId) == m_streams.end())
    RequestStreams();

  if (m_streams.find(iStreamId) == m_streams.end())
    return NULL;

  return m_streams[iStreamId];
}

/*
 * Request streams from PVRClient
 */
void CDVDDemuxPVRClient::RequestStreams()
{
  PVR_STREAMPROPS *props = g_PVRManager.GetCurrentStreamProps();

  for (int i = 0; i < props->nstreams; ++i)
  {
    if (props->stream[i].codec_type == CODEC_TYPE_AUDIO)
      m_streams[props->stream[i].id]              = new CDemuxStreamAudioPVRClient(this);
    else if (props->stream[i].codec_type == CODEC_TYPE_VIDEO)
      m_streams[props->stream[i].id]              = new CDemuxStreamVideoPVRClient(this);
    else if (props->stream[i].codec_type == CODEC_TYPE_SUBTITLE)
      m_streams[props->stream[i].id]              = new CDemuxStreamSubtitlePVRClient(this);

    if (m_streams.find(props->stream[i].id) != m_streams.end())
    {
      m_streams[props->stream[i].id]->codec       = (CodecID)props->stream[i].codec_id;
      m_streams[props->stream[i].id]->iId         = props->stream[i].id;
      m_streams[props->stream[i].id]->iPhysicalId = props->stream[i].physid;
    }
    CLog::Log(LOGDEBUG,"CDVDDemuxPVRClient::RequestStreams(): added codec_id %d", m_streams[props->stream[i].id]->codec);
  }
}

/*
 * return nr of streams, 0 if none
 */
int CDVDDemuxPVRClient::GetNrOfStreams()
{
  return m_streams.size();
}

/*
 * returns opened filename
 */
std::string CDVDDemuxPVRClient::GetFileName()
{
  if(m_pInput && m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

