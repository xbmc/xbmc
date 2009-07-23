/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "DVDDemuxUtils.h"
#include "DVDClock.h"

#include "DVDDemuxTS.h"

int XdmxLogFunc(const char* format, ...)
{
  CStdString msg;
  va_list va;
  va_start(va, format);
  msg.FormatV(format, va);
  CLog::Log(LOGDEBUG, msg.c_str());
  va_end(va);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CXdmxInputStream::CXdmxInputStream(CDVDInputStream* pInputStream) :
  m_pInputStream(pInputStream)
{

}

CXdmxInputStream::~CXdmxInputStream()
{
  
}

unsigned int CXdmxInputStream::Read(unsigned char* buf, unsigned int len)
{
  if (m_pInputStream)
    return m_pInputStream->Read(buf, len);
  return 0;
}

__int64 CXdmxInputStream::Seek(__int64 offset, int whence)
{
  if (m_pInputStream)
    return m_pInputStream->Seek(offset, whence);
  return 0;
}

__int64 CXdmxInputStream::GetLength()
{
  if (m_pInputStream)
    return m_pInputStream->GetLength();
  return 0;
}

bool CXdmxInputStream::IsEOF()
{
  if (m_pInputStream)
    return m_pInputStream->IsEOF();
  return true;
}

CDVDInputStream* CXdmxInputStream::GetInnerStream()
{
  return m_pInputStream;
}


/////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxTS::CDVDDemuxTS() :
  m_pInnerDemux(NULL),
  m_pInput(NULL),
  m_pProgram(NULL)
{
  XdmxSetLogLevel(XDMX_LOG_LEVEL_ERROR);
  XdmxSetLogFunc(XdmxLogFunc);  
}

CDVDDemuxTS::~CDVDDemuxTS()
{
  delete m_pInput;
  delete m_pInnerDemux;
}

bool CDVDDemuxTS::Open(CDVDInputStream* pInput, TSTransportType type)
{
  m_pInnerDemux = CreateTSDemux(type);
  m_pInput = new CXdmxInputStream(pInput);
  if (!m_pInnerDemux->Open(m_pInput))
  {
    delete m_pInput;
    m_pInput = NULL;
    delete m_pInnerDemux;
    m_pInnerDemux = NULL;
    return false;
  }

  // Store a reference to the Transport Stream
  CTransportStream* pTransportStream = m_pInnerDemux->GetTransportStream();

  // Get the last Program in the Transport Stream
  m_pProgram = pTransportStream->GetProgram(pTransportStream->GetProgramCount() - 1); 

  // Fetch stream information
  for (unsigned int s = 0; s < m_pProgram->GetStreamCount(); s++)
    AddStream(m_pProgram->GetStream(s));
  return true;
}

void CDVDDemuxTS::AddStream(CElementaryStream* pStream)
{
  union
  {
    CDemuxStream*      g;
    CDemuxStreamAudio* a;
    CDemuxStreamVideo* v;
  } dmxStream;

  dmxStream.g = NULL;
  char* pTypeName = "Unsupported";
  switch (pStream->GetElementType())
  {
  case ES_STREAM_TYPE_VIDEO_MPEG2_2:
    dmxStream.v = new CDemuxStreamVideo();
    dmxStream.v->codec = CODEC_ID_MPEG2VIDEO;
    pTypeName = "MPEG2 Video";
    break;
  case ES_STREAM_TYPE_VIDEO_H264:
    dmxStream.v = new CDemuxStreamVideo();
    dmxStream.v->codec = CODEC_ID_H264;
    pTypeName = "H264 Video";
    break;
  case ES_STREAM_TYPE_VIDEO_VC1:
    dmxStream.v = new CDemuxStreamVideo();
    dmxStream.v->codec = CODEC_ID_VC1;
    pTypeName = "VC1 Video";
    break;
  case ES_STREAM_TYPE_AUDIO_AC3:
    dmxStream.a = new CDemuxStreamAudio();
    dmxStream.a->codec = CODEC_ID_AC3;
    pTypeName = "AC3 Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_DTS:
    dmxStream.a = new CDemuxStreamAudio();
    dmxStream.a->codec = CODEC_ID_DTS;
    pTypeName = "DTS Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_MPEG2_AAC:
    dmxStream.a = new CDemuxStreamAudio();
    dmxStream.a->codec = CODEC_ID_AAC;
    pTypeName = "AAC (MPEG-2 Part 7 Audio)";
    break;
  case ES_PRIVATE_TYPE_LPCM:
    dmxStream.a = new CDemuxStreamAudio();
    dmxStream.a->codec = CODEC_ID_PCM_S24BE;
    pTypeName = "LPCM (Private)";
    break;
  default:
    break;
  }
  if (dmxStream.g)
  {
    dmxStream.g->iId = m_StreamList.size();
    dmxStream.g->iPhysicalId = pStream->GetId();
    m_StreamList.push_back(dmxStream.g); // Add to stream list
    m_StreamMap[dmxStream.g->iPhysicalId] = dmxStream.g->iId; // Map physical stream id (coming from demux) to index (coming from dvdplayer)
    CLog::Log(LOGDEBUG, "%s: Added stream. Index: %d, Id: %d, Type: %s", __FUNCTION__, dmxStream.g->iId, dmxStream.g->iPhysicalId, pTypeName);
  }
}

void CDVDDemuxTS::Dispose()
{
  
}

void CDVDDemuxTS::Reset()
{
  
}

void CDVDDemuxTS::Abort()
{
  
}

void CDVDDemuxTS::Flush()
{
  
}

DemuxPacket* CDVDDemuxTS::Read()
{
  CParserPayload* pPayload = NULL;
  for (;;)
  {
    pPayload = m_pInnerDemux->GetPayload();
    if (!pPayload)
      return NULL;

    if (m_StreamMap.find(pPayload->GetStream()->GetId()) != m_StreamMap.end())
      break;
  }

  DemuxPacket* pDmx = new DemuxPacket;
  pDmx->iSize = pPayload->GetSize();
  pDmx->pData = pPayload->Detach();
  pDmx->pts = pPayload->GetPts() * DVD_TIME_BASE;
  pDmx->dts = pPayload->GetDts() * DVD_TIME_BASE;
  pDmx->iGroupId = 0;
  pDmx->duration = 0; // Unknown
  pDmx->iStreamId = m_StreamMap[pPayload->GetStream()->GetId()];

  m_StreamCounterList[pPayload->GetStream()->GetId()] += pDmx->iSize;
  return pDmx;
}

bool CDVDDemuxTS::SeekTime(int time, bool backwards /*= false*/, double* startpts /*= NULL*/)
{
  return false;
}

void CDVDDemuxTS::SetSpeed(int iSpeed)
{
  
}

int CDVDDemuxTS::GetStreamLength()
{
  return 0;
}

CDemuxStream* CDVDDemuxTS::GetStream(int iStreamId)
{
  if (!m_pProgram || m_StreamList.size() <= (unsigned int)iStreamId)
    return NULL;
  return m_StreamList[iStreamId];
}

int CDVDDemuxTS::GetNrOfStreams()
{
  return m_StreamList.size();
}

std::string CDVDDemuxTS::GetFileName()
{
  if (m_pInput && m_pInput->GetInnerStream())
    return m_pInput->GetInnerStream()->GetFileName();
  return "";
}

