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
  XdmxSetLogLevel(XDMX_LOG_LEVEL_INFO);
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
    dmxStream.a->iBitRate = pStream->GetProperty(XDMX_PROP_TAG_BITRATE).int32Val;
    dmxStream.a->iBlockAlign = pStream->GetProperty(XDMX_PROP_TAG_FRAME_SIZE).int32Val;
    dmxStream.a->iChannels = pStream->GetProperty(XDMX_PROP_TAG_CHANNELS).int32Val;
    dmxStream.a->iSampleRate = pStream->GetProperty(XDMX_PROP_TAG_SAMPLE_RATE).int32Val;
    dmxStream.a->iBitsPerSample = pStream->GetProperty(XDMX_PROP_TAG_BIT_DEPTH).int32Val;
    switch(dmxStream.a->iBitsPerSample)
    {
    case 16:
      dmxStream.a->codec = CODEC_ID_PCM_S16BE;
      break;
    case 24:
      dmxStream.a->codec = CODEC_ID_PCM_S16BE;
      break;
    default:
      dmxStream.a->codec = CODEC_ID_NONE;
      break;
    }
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
    pStream->SetProperty('xbid', dmxStream.g->iId);

    CLog::Log(LOGDEBUG, "%s: Added stream. Index: %d, Id: %d, Type: %s", __FUNCTION__, dmxStream.g->iId, dmxStream.g->iPhysicalId, pTypeName);

    if (dmxStream.v->codec == CODEC_ID_VC1)
    {
      for(;;)
      {
        DemuxPacket* pDmx = GetNextPacket();
        if (!pDmx)
          break;

        if (pDmx->iStreamId == dmxStream.g->iId)
        {
          // Copy ExtraData
          dmxStream.v->ExtraSize = 36;
          dmxStream.v->ExtraData = malloc(dmxStream.v->ExtraSize + 8);
          memcpy(dmxStream.v->ExtraData, pDmx->pData, dmxStream.v->ExtraSize);
          memset((char*)dmxStream.v->ExtraData + dmxStream.v->ExtraSize, 0, 8);

          // Parse sequence header
          // TODO: Is this the best place for this?
          // TODO: Find sequence header start code: 0x00 0x00 0x01 0x0F
          unsigned char* pHeader = (unsigned char*)dmxStream.v->ExtraData + 4;
          if ((pHeader[0] >> 6) == 0x3) // Advanced Profile
          {
            dmxStream.v->iWidth = (((pHeader[2] << 4) | (pHeader[3] >> 4)) + 1) << 1;
            dmxStream.v->iHeight = ((((pHeader[3] & 0x0F) << 8) | pHeader[4]) + 1) << 1;
          }

          m_PacketQueue.push_back(pDmx);
          break;
        }
      }
    }
  }
}

void CDVDDemuxTS::Dispose()
{
  if (m_pInnerDemux)
  {
    m_pInnerDemux->Close();
    delete m_pInnerDemux;
  }
  m_pInnerDemux = NULL;
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
  if (m_PacketQueue.size())
  {
    DemuxPacket* pDmx = m_PacketQueue.front();
    m_PacketQueue.pop_front();
    return pDmx;
  }

  return GetNextPacket();
}

DemuxPacket* CDVDDemuxTS::GetNextPacket()
{
  CParserPayload* pPayload = NULL;
  for (;;)
  {
    pPayload = m_pInnerDemux->GetPayload();
    if (!pPayload)
      return NULL;

    // TODO: Find a cleaner/safer way to do this
    if (!pPayload->GetStream()->IsValueEmpty(pPayload->GetStream()->GetProperty('xbid')))
      break;

    delete pPayload; // This data is unknown to us, but still must be released
  }

  DemuxPacket* pDmx = new DemuxPacket;
  pDmx->iSize = pPayload->GetSize();
  pDmx->pData = pPayload->Detach();
  pDmx->pts = pPayload->GetPts() * DVD_TIME_BASE;
  pDmx->dts = pPayload->GetDts() * DVD_TIME_BASE;
  pDmx->iGroupId = 0;
  pDmx->duration = 0; // Unknown as of yet
  pDmx->iStreamId = pPayload->GetStream()->GetProperty('xbid').int32Val;

  m_StreamCounterList[pPayload->GetStream()->GetId()] += pDmx->iSize;

  delete pPayload;
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

void CDVDDemuxTS::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  if (!m_pInnerDemux || iStreamId >= (int)m_StreamList.size())
    return;
  CElementaryStream* pEStream = m_pInnerDemux->GetStreamById(m_StreamList[iStreamId]->iPhysicalId);
  strName.Format("%4.4s", pEStream->GetProperty(XDMX_PROP_TAG_FOURCC));
}