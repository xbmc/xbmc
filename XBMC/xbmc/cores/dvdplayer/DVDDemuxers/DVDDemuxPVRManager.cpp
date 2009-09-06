/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxPVRManager.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h"
#include "Application.h"

using namespace std;

class CDemuxStreamVideoPVRManager
  : public CDemuxStreamVideo
{
  CDVDDemuxPVRManager *m_parent;
  string               m_codec;
  
public:
  CDemuxStreamVideoPVRManager(CDVDDemuxPVRManager *parent, const string& codec)
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

class CDemuxStreamAudioPVRManager
  : public CDemuxStreamAudio
{
  CDVDDemuxPVRManager *m_parent;
  string               m_codec;

public:
  CDemuxStreamAudioPVRManager(CDVDDemuxPVRManager *parent, const string& codec)
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

CDVDDemuxPVRManager::CDVDDemuxPVRManager()
  : CDVDDemux()
  , m_pInput(NULL)
  , m_pManager(&g_PVRManager)
{
}

CDVDDemuxPVRManager::~CDVDDemuxPVRManager()
{
  Dispose();
}

bool CDVDDemuxPVRManager::Open(CDVDInputStream* input)
{
  Dispose();

  if(!input->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
    return false;

  const PVRDEMUXHANDLE handle = (CDVDDemuxPVRManager*) this;
  if (!m_pManager->OpenDemux(handle))
    return false;

  m_pInput = input;

  int retries = 0;
  while (m_Streams.size() == 0 && retries < 5)
  {
    DemuxPacket* pkg = Read();
    if(!pkg)
      return false;
    CDVDDemuxUtils::FreeDemuxPacket(pkg);
  }

  return true;
}

void CDVDDemuxPVRManager::Dispose()
{
  m_pManager->DisposeDemux();
}

void CDVDDemuxPVRManager::Reset()
{
  m_pManager->ResetDemux();
}


void CDVDDemuxPVRManager::Flush()
{
  m_pManager->FlushDemux();
}

void CDVDDemuxPVRManager::SetSpeed(int iSpeed)
{
  m_pManager->SetDemuxSpeed(iSpeed);
}

bool CDVDDemuxPVRManager::SeekTime(int time, bool backwords, double* startpts)
{
  return m_pManager->SeekDemuxTime(time, backwords, startpts);;
}

int CDVDDemuxPVRManager::GetStreamLength()
{ 
  return m_pManager->GetDemuxStreamLength();
}
  
DemuxPacket* CDVDDemuxPVRManager::Read()
{
  return (DemuxPacket*) m_pManager->ReadDemux();
}

CDemuxStream* CDVDDemuxPVRManager::GetStream(int iStreamId)
{
  if(iStreamId >= 0 && iStreamId < (int)m_Streams.size())
    return m_Streams[iStreamId];

  return NULL;
}

int CDVDDemuxPVRManager::GetNrOfStreams()
{
  return m_Streams.size();
}

std::string CDVDDemuxPVRManager::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

void CDVDDemuxPVRManager::Abort()
{
  m_pManager->AbortDemux();
}

bool CDVDDemuxPVRManager::AddDemuxStream(const PVRDEMUXHANDLE handle, const PVR_DEMUXSTREAMINFO *demux)
{
  CDVDDemuxPVRManager* demuxer=(CDVDDemuxPVRManager*) handle;
  if (!demuxer)
    return false;

  if (demux->type == PVR_STREAM_AUDIO) 
  {
    CDemuxStreamAudio *audiodemux = new CDemuxStreamAudioPVRManager(demuxer, demux->name);
    audiodemux->iId             = demuxer->m_Streams.size();
    audiodemux->iPhysicalId     = demux->index;
    audiodemux->codec           = demux->codec;
    audiodemux->codec_fourcc    = demux->codec_fourcc;
    audiodemux->type            = STREAM_AUDIO;
    audiodemux->source          = demux->source;
    audiodemux->iDuration       = demux->duration;
    audiodemux->iChannels       = demux->channels;
    audiodemux->iSampleRate     = demux->sampleRate;
    audiodemux->iBlockAlign     = demux->block_align;
    audiodemux->iBitRate        = demux->bit_rate;
    audiodemux->iBitsPerSample  = demux->bits_per_sample;
    if (demux->language[0] != 0)
    {
      for (int i = 0; i < 4; i++)
        audiodemux->language[i] = demux->language[i];
    }
    demuxer->m_Streams.push_back(audiodemux);
    return true;
  } 
  else if (demux->type == PVR_STREAM_VIDEO)
  { 
    CDemuxStreamVideo *videodemux = new CDemuxStreamVideoPVRManager(demuxer, demux->name);
    videodemux->iId             = demuxer->m_Streams.size();
    videodemux->iPhysicalId     = demux->index;
    videodemux->codec           = demux->codec;
    videodemux->codec_fourcc    = demux->codec_fourcc;
    videodemux->type            = STREAM_VIDEO;
    videodemux->source          = demux->source;
    videodemux->iDuration       = demux->duration;
    videodemux->iFpsScale       = demux->fps_scale;
    videodemux->iFpsRate        = demux->fps_rate;
    videodemux->iHeight         = demux->height;
    videodemux->iWidth          = demux->width;
    videodemux->fAspect         = demux->aspect;
    videodemux->bVFR            = demux->vfr;
    if (demux->language[0] != 0)
    {
      for (int i = 0; i < 4; i++)
        videodemux->language[i] = demux->language[i];
    }
    demuxer->m_Streams.push_back(videodemux);
    return true;
  } 
    
  return false;
}

void CDVDDemuxPVRManager::DeleteDemuxStream(const PVRDEMUXHANDLE handle, int index)
{
  CDVDDemuxPVRManager* demuxer=(CDVDDemuxPVRManager*) handle;
  if (!demuxer)
    return;
  
  for(int i = 0; i < (int)demuxer->m_Streams.size(); i++)
  {
    if (demuxer->m_Streams[i]->iPhysicalId == index)
    {
      delete demuxer->m_Streams[i];
      demuxer->m_Streams.erase(demuxer->m_Streams.begin()+i);
      return;
    }
  }
}

void CDVDDemuxPVRManager::DeleteDemuxStreams(const PVRDEMUXHANDLE handle)
{
  CDVDDemuxPVRManager* demuxer=(CDVDDemuxPVRManager*) handle;
  if (!demuxer)
    return;

  for(int i = 0; i < (int)demuxer->m_Streams.size(); i++)
    delete demuxer->m_Streams[i];
  demuxer->m_Streams.clear();
}
