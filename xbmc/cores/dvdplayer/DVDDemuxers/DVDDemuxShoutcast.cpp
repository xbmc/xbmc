/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DVDCodecs/DVDCodecs.h"
#include "DVDInputStreams/DVDInputStreamHttp.h"
#include "DVDDemuxShoutcast.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h" // for DVD_TIME_BASE
#include "../../../utils/HttpHeader.h"

#define ICY_NOTICE1           "icy-notice1" // string
#define ICY_NOTICE2           "icy-notice2" // string
#define ICY_NAME              "icy-name"    // string
#define ICY_GENRE             "icy-genre"   // string
#define ICY_URL               "icy-url"     // string
#define ICY_PUBLIC            "icy-pub"     // int (1 / 0)
#define ICY_BITRATE           "icy-br"      // int (bitrate = val * 1000 ?)
#define ICY_METAINTERVAL      "icy-metaint" // int

#define CONTENT_TYPE_MP3      "audio/mpeg"
#define CONTENT_TYPE_AAC      "audio/aac"
#define CONTENT_TYPE_AACPLUS  "audio/aacp"

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamAudioShoutcast::GetStreamInfo(std::string& strInfo)
{
  strInfo = "Shoutcast";
}

CDVDDemuxShoutcast::CDVDDemuxShoutcast() : CDVDDemux()
{
  m_pInput = NULL;
  m_pDemuxStream = NULL;
  m_iMetaStreamInterval = 0;
}

CDVDDemuxShoutcast::~CDVDDemuxShoutcast()
{
  Dispose();
}

bool CDVDDemuxShoutcast::Open(CDVDInputStream* pInput)
{
  Dispose();

  m_pInput = pInput;

  // the input stream should be a http stream
  if (!pInput->IsStreamType(DVDSTREAM_TYPE_HTTP)) return false;
  CDVDInputStreamHttp* pInputStreamHttp = (CDVDInputStreamHttp*)pInput;

  CHttpHeader* pHeader = pInputStreamHttp->GetHttpHeader();

  std::string strMetaInt = pHeader->GetValue(ICY_METAINTERVAL);
  std::string strMimeType = pHeader->GetMimeType();

  // create new demuxer stream
  m_pDemuxStream = new CDemuxStreamAudioShoutcast();
  m_pDemuxStream->iId = 0;
  m_pDemuxStream->iPhysicalId = 0;
  m_pDemuxStream->iDuration = 0;
  m_pDemuxStream->iChannels = 2;
  m_pDemuxStream->iSampleRate = 0;

  // set meta interval
  m_iMetaStreamInterval = atoi(strMetaInt.c_str());

  if (stricmp(strMimeType.c_str(), CONTENT_TYPE_AAC) == 0 ||
      stricmp(strMimeType.c_str(), CONTENT_TYPE_AACPLUS) == 0)
  {
    // need an aac decoder first
    m_pDemuxStream->codec = CODEC_ID_AAC;
  }
  else // (stricmp(strMimeType, CONTENT_TYPE_MP3) == 0)
  {
    // default to mp3
    m_pDemuxStream->codec = CODEC_ID_MP3;
  }

  return true;
}

void CDVDDemuxShoutcast::Dispose()
{
  if (m_pDemuxStream) delete m_pDemuxStream;
  m_pDemuxStream = NULL;

  m_pInput = NULL;
}

void CDVDDemuxShoutcast::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxShoutcast::Flush()
{
}

DemuxPacket* CDVDDemuxShoutcast::Read()
{
  int iRead = 0;

  // XXX
  // if meta interval is greater than FileCurl's max read size (currently 64k)
  // it will simply fail becuse the meta-interval will get incorrect

  int iDataToRead = SHOUTCAST_BUFFER_SIZE;
  if (m_iMetaStreamInterval > 0) iDataToRead = m_iMetaStreamInterval;

  DemuxPacket* pPacket;
  pPacket = CDVDDemuxUtils::AllocateDemuxPacket(iDataToRead);
  if (pPacket)
  {
    pPacket->dts = DVD_NOPTS_VALUE;
    pPacket->pts = DVD_NOPTS_VALUE;
    pPacket->iStreamId = 0;

    // read the data
    int iRead = m_pInput->Read(pPacket->pData, iDataToRead);

    pPacket->iSize = iRead;

    if (iRead <= 0)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = NULL;
    }
  }

  if (m_iMetaStreamInterval > 0)
  {
    // we already have read m_iMetaStreamInterval bytes of streaming data
    // metadata follows
    BYTE l;
    iRead = m_pInput->Read(&l, 1);
    if (iRead > 0)
    {
      int iMetaLength = l * 16;

      if (iMetaLength > 0)
      {
        // iMetaLength cannot be larger then 16 * 255
        BYTE buffer[16 * 255];

        // skip meta data for now
        m_pInput->Read(buffer, iMetaLength);
      }
    }
  }

  return pPacket;
}

bool CDVDDemuxShoutcast::SeekTime(int time, bool backwords, double* startpts)
{
  return false;
}

int CDVDDemuxShoutcast::GetStreamLength()
{
  return 0;
}

CDemuxStream* CDVDDemuxShoutcast::GetStream(int iStreamId)
{
  return m_pDemuxStream;
}

int CDVDDemuxShoutcast::GetNrOfStreams()
{
  return 1;
}

std::string CDVDDemuxShoutcast::GetFileName()
{
  if(m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

