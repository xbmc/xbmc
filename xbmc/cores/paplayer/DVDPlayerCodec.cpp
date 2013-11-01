/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDPlayerCodec.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include "cores/dvdplayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/dvdplayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/dvdplayer/DVDStreamInfo.h"
#include "cores/dvdplayer/DVDCodecs/DVDFactoryCodec.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "URL.h"

#include "AudioDecoder.h"

DVDPlayerCodec::DVDPlayerCodec()
{
  m_CodecName = "DVDPlayer";
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  m_pAudioCodec = NULL;
  m_nAudioStream = -1;
  m_audioPos = 0;
  m_pPacket = NULL;
  m_decoded = NULL;
  m_nDecodedLen = 0;
  m_strFileName = "";
  m_bInited = false;
}

DVDPlayerCodec::~DVDPlayerCodec()
{
  DeInit();
}

void DVDPlayerCodec::SetContentType(const CStdString &strContent)
{
  m_strContentType = strContent;
}

bool DVDPlayerCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  // take precaution if Init()ialized earlier
  if (m_bInited)
  {
    // keep things as is if Init() was done with known strFile
    if (m_strFileName == strFile)
      return true;

    // got differing filename, so cleanup before starting over
    DeInit();
  }

  m_decoded = NULL;
  m_nDecodedLen = 0;

  CStdString strFileToOpen = strFile;

  CURL urlFile(strFile);
  if (urlFile.GetProtocol() == "shout" )
    strFileToOpen.Replace("shout://","http://");

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strFileToOpen, m_strContentType);
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating input stream for %s", __FUNCTION__, strFileToOpen.c_str());
    return false;
  }

  if (!m_pInputStream->Open(strFileToOpen.c_str(), m_strContentType))
  {
    CLog::Log(LOGERROR, "%s: Error opening file %s", __FUNCTION__, strFileToOpen.c_str());
    if (m_pInputStream)
      delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  m_pDemuxer = NULL;

  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if (!m_pDemuxer)
    {
      delete m_pInputStream;
      m_pInputStream = NULL;
      CLog::Log(LOGERROR, "%s: Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s: Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
    {
      delete m_pDemuxer;
      m_pDemuxer = NULL;
    }
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  CDemuxStream* pStream = NULL;
  m_nAudioStream = -1;
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    pStream = m_pDemuxer->GetStream(i);
    if (pStream && pStream->type == STREAM_AUDIO)
    {
      m_nAudioStream = i;
      break;
    }
  }

  if (m_nAudioStream == -1)
  {
    CLog::Log(LOGERROR, "%s: Could not find audio stream", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  CDVDStreamInfo hint(*pStream, true);

  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(hint);
  if (!m_pAudioCodec)
  {
    CLog::Log(LOGERROR, "%s: Could not create audio codec", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  // we have to decode initial data in order to get channels/samplerate
  // for sanity - we read no more than 10 packets
  int nErrors = 0;
  for (int nPacket=0; nPacket < 10 && (m_Channels == 0 || m_SampleRate == 0); nPacket++)
  {
    BYTE dummy[256];
    int nSize = 256;
    if (ReadPCM(dummy, nSize, &nSize) == READ_ERROR)
      ++nErrors;

    m_DataFormat    = m_pAudioCodec->GetDataFormat();
    m_BitsPerSample = CAEUtil::DataFormatToBits(m_DataFormat);
    m_SampleRate = m_pAudioCodec->GetSampleRate();
    m_EncodedSampleRate = m_pAudioCodec->GetEncodedSampleRate();
    m_Channels = m_pAudioCodec->GetChannels();
    m_ChannelInfo = m_pAudioCodec->GetChannelMap();

  }
  if (nErrors >= 10)
  {
    CLog::Log(LOGDEBUG, "%s: Could not decode data", __FUNCTION__);
    return false;
  }

  m_nDecodedLen = 0;

  if (m_Channels == 0) // no data - just guess and hope for the best
    m_Channels = 2;

  if (m_SampleRate == 0)
    m_SampleRate = 44100;

  m_TotalTime = m_pDemuxer->GetStreamLength();
  m_Bitrate = m_pAudioCodec->GetBitRate();
  m_pDemuxer->GetStreamCodecName(m_nAudioStream,m_CodecName);

  m_strFileName = strFile;
  m_bInited = true;

  return true;
}

void DVDPlayerCodec::DeInit()
{
  if (m_pPacket)
    CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
  m_pPacket = NULL;

  if (m_pDemuxer != NULL)
  {
    delete m_pDemuxer;
    m_pDemuxer = NULL;
  }

  if (m_pInputStream != NULL)
  {
    delete m_pInputStream;
    m_pInputStream = NULL;
  }

  if (m_pAudioCodec != NULL)
  {
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  // cleanup format information
  m_TotalTime = 0;
  m_SampleRate = 0;
  m_EncodedSampleRate = 0;
  m_BitsPerSample = 0;
  m_DataFormat = AE_FMT_INVALID;
  m_Channels = 0;
  m_Bitrate = 0;

  m_audioPos = 0;
  m_decoded = NULL;
  m_nDecodedLen = 0;

  m_strFileName = "";
  m_bInited = false;
}

int64_t DVDPlayerCodec::Seek(int64_t iSeekTime)
{
  if (m_pPacket)
    CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
  m_pPacket = NULL;

  m_pDemuxer->SeekTime((int)iSeekTime, false);
  m_pAudioCodec->Reset();

  m_decoded = NULL;
  m_nDecodedLen = 0;

  return iSeekTime;
}

int DVDPlayerCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_decoded && m_nDecodedLen > 0)
  {
    int nLen = (size<m_nDecodedLen)?size:m_nDecodedLen;
    *actualsize = nLen;
    memcpy(pBuffer, m_decoded, *actualsize);
    m_nDecodedLen -= nLen;
    m_decoded += (*actualsize);
    return READ_SUCCESS;
  }

  m_decoded = NULL;
  m_nDecodedLen = 0;

  // dvdplayer returns a read error on a single invalid packet, while
  // in paplayer READ_ERROR is a fatal error.
  // Therefore skip over invalid packets here.
  int decodeLen = -1;
  for (int tries = 0; decodeLen < 0 && tries < 2; ++tries)
  {
    if (m_pPacket && m_audioPos >= m_pPacket->iSize)
    {
      CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
      m_audioPos = 0;
      m_pPacket = NULL;
    }

    if (m_pPacket == NULL)
    {
      do
      {
        m_pPacket = m_pDemuxer->Read();
      } while (m_pPacket && m_pPacket->iStreamId != m_nAudioStream);

      if (!m_pPacket)
      {
        return READ_EOF;
      }
      m_audioPos = 0;
    }

    decodeLen = m_pAudioCodec->Decode(m_pPacket->pData + m_audioPos, m_pPacket->iSize - m_audioPos);

    if (decodeLen < 0)
      m_audioPos = m_pPacket->iSize; // skip packet
  }

  if (decodeLen < 0)
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
    m_pPacket = NULL;
    m_audioPos = 0;
    return READ_ERROR;
  }

  m_audioPos += decodeLen;

  m_nDecodedLen = m_pAudioCodec->GetData(&m_decoded);

  *actualsize = (m_nDecodedLen <= size) ? m_nDecodedLen : size;
  if (*actualsize > 0)
  {
    memcpy(pBuffer, m_decoded, *actualsize);
    m_nDecodedLen -= *actualsize;
    m_decoded += (*actualsize);
  }

  return READ_SUCCESS;
}

bool DVDPlayerCodec::CanInit()
{
  return true;
}

bool DVDPlayerCodec::CanSeek()
{
  return true;
}
