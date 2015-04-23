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
#include "cores/AudioEngine/AEResampleFactory.h"

#include "cores/dvdplayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/dvdplayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/dvdplayer/DVDStreamInfo.h"
#include "cores/dvdplayer/DVDCodecs/DVDFactoryCodec.h"
#include "music/tags/TagLoaderTagLib.h"
#include "utils/log.h"
#include "URL.h"
#include "utils/StringUtils.h"

DVDPlayerCodec::DVDPlayerCodec()
{
  m_CodecName = "DVDPlayer";
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  m_pAudioCodec = NULL;
  m_nAudioStream = -1;
  m_audioPos = 0;
  m_pPacket = NULL;
  m_nDecodedLen = 0;
  m_bInited = false;
  m_pResampler = NULL;
  m_needConvert = false;
  m_srcFrameSize = 0;
}

DVDPlayerCodec::~DVDPlayerCodec()
{
  DeInit();
}

void DVDPlayerCodec::SetContentType(const std::string &strContent)
{
  m_strContentType = strContent;
  StringUtils::ToLower(m_strContentType);
}

bool DVDPlayerCodec::Init(const std::string &strFile, unsigned int filecache)
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

  m_nDecodedLen = 0;

  std::string strFileToOpen = strFile;

  CURL urlFile(strFile);
  if (urlFile.IsProtocol("shout") )
    strFileToOpen.replace(0, 8, "http://");

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

  //  Extract ReplayGain info
  // tagLoaderTagLib.Load will try to determine tag type by file extension, so set fallback by contentType
  std::string strFallbackFileExtension = "";
  if (m_strContentType == "audio/aacp" ||
      m_strContentType == "audio/aac")
    strFallbackFileExtension = "m4a";
  else if (m_strContentType == "audio/x-ms-wma")
    strFallbackFileExtension = "wma";
  else if (m_strContentType == "audio/x-ape" ||
           m_strContentType == "audio/ape")
    strFallbackFileExtension = "ape";
  CTagLoaderTagLib tagLoaderTagLib;
  tagLoaderTagLib.Load(strFile, m_tag, strFallbackFileExtension);

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
    m_BitsPerCodedSample = static_cast<CDemuxStreamAudio*>(pStream)->iBitsPerSample;
  }
  if (nErrors >= 10)
  {
    CLog::Log(LOGDEBUG, "%s: Could not decode data", __FUNCTION__);
    return false;
  }

  // test if seeking is supported
  m_bCanSeek = false;
  if (m_pInputStream->Seek(0, SEEK_POSSIBLE))
  {
    if (Seek(1) != DVD_NOPTS_VALUE)
    {
      // rewind stream to beginning
      Seek(0);
      m_bCanSeek = true;
    }
    else
    {
      m_pInputStream->Seek(0, SEEK_SET);
      m_pDemuxer->Reset();
    }
  }

  if (m_Channels == 0) // no data - just guess and hope for the best
    m_Channels = 2;

  if (m_SampleRate == 0)
    m_SampleRate = 44100;

  m_TotalTime = m_pDemuxer->GetStreamLength();
  m_Bitrate = m_pAudioCodec->GetBitRate();
  if (!m_Bitrate && m_TotalTime)
  {
    m_Bitrate = (int)(((m_pInputStream->GetLength()*1000) / m_TotalTime) * 8);
  }
  m_pDemuxer->GetStreamCodecName(m_nAudioStream,m_CodecName);

  m_needConvert = false;
  if (NeedConvert(m_DataFormat))
  {
    m_needConvert = true;
    m_pResampler = ActiveAE::CAEResampleFactory::Create();
    m_pResampler->Init(CAEUtil::GetAVChannelLayout(m_ChannelInfo),
                       m_ChannelInfo.Count(),
                       m_SampleRate,
                       CAEUtil::GetAVSampleFormat(AE_FMT_FLOAT),
                       CAEUtil::DataFormatToUsedBits(AE_FMT_FLOAT),
                       CAEUtil::DataFormatToDitherBits(AE_FMT_FLOAT),
                       CAEUtil::GetAVChannelLayout(m_ChannelInfo),
                       m_ChannelInfo.Count(),
                       m_SampleRate,
                       CAEUtil::GetAVSampleFormat(m_DataFormat),
                       CAEUtil::DataFormatToUsedBits(m_DataFormat),
                       CAEUtil::DataFormatToDitherBits(m_DataFormat),
                       false,
                       false,
                       NULL,
                       AE_QUALITY_UNKNOWN,
                       false);
    m_planes = AE_IS_PLANAR(m_DataFormat) ? m_ChannelInfo.Count() : 1;
    m_srcFormat = m_DataFormat;
    m_srcFrameSize = (CAEUtil::DataFormatToBits(m_DataFormat)>>3) * m_ChannelInfo.Count();
    m_DataFormat = AE_FMT_FLOAT;
    m_BitsPerSample = CAEUtil::DataFormatToBits(m_DataFormat);
  }

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

  delete m_pResampler;
  m_pResampler = NULL;

  // cleanup format information
  m_TotalTime = 0;
  m_SampleRate = 0;
  m_EncodedSampleRate = 0;
  m_BitsPerSample = 0;
  m_DataFormat = AE_FMT_INVALID;
  m_Channels = 0;
  m_Bitrate = 0;

  m_audioPos = 0;
  m_nDecodedLen = 0;

  m_strFileName = "";
  m_bInited = false;
}

int64_t DVDPlayerCodec::Seek(int64_t iSeekTime)
{
  // default to announce backwards seek if !m_pPacket to not make FFmpeg
  // skip mpeg audio frames at playback start
  bool seekback = true;

  if (m_pPacket)
  {
    seekback = (DVD_MSEC_TO_TIME(iSeekTime) > m_pPacket->pts);
    CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
  }
  m_pPacket = NULL;

  bool ret = m_pDemuxer->SeekTime((int)iSeekTime, seekback);
  m_pAudioCodec->Reset();

  m_nDecodedLen = 0;

  if (!ret)
    return DVD_NOPTS_VALUE;

  return iSeekTime;
}

int DVDPlayerCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_nDecodedLen > 0)
  {
    int nLen = (size<m_nDecodedLen)?size:m_nDecodedLen;
    *actualsize = nLen;
    if (m_needConvert)
    {
      int samples = *actualsize / (m_BitsPerSample>>3);
      int frames = samples / m_Channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioPlanes, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioPlanes[i] += frames*m_srcFrameSize/m_planes;
      }
    }
    else
    {
      memcpy(pBuffer, m_audioPlanes[0], *actualsize);
      m_audioPlanes[0] += (*actualsize);
    }
    m_nDecodedLen -= nLen;
    return READ_SUCCESS;
  }

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

  // scale decoded bytes to destination format
  m_nDecodedLen = m_pAudioCodec->GetData(m_audioPlanes);
  if (m_needConvert)
    m_nDecodedLen *= (m_BitsPerSample>>3) / (m_srcFrameSize / m_Channels);

  *actualsize = (m_nDecodedLen <= size) ? m_nDecodedLen : size;
  if (*actualsize > 0)
  {
    if (m_needConvert)
    {
      int samples = *actualsize / (m_BitsPerSample>>3);
      int frames = samples / m_Channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioPlanes, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioPlanes[i] += frames*m_srcFrameSize/m_planes;
      }
    }
    else
    {
      memcpy(pBuffer, m_audioPlanes[0], *actualsize);
      m_audioPlanes[0] += *actualsize;
    }
    m_nDecodedLen -= *actualsize;
  }

  return READ_SUCCESS;
}

bool DVDPlayerCodec::CanInit()
{
  return true;
}

bool DVDPlayerCodec::CanSeek()
{
  return m_bCanSeek;
}

bool DVDPlayerCodec::NeedConvert(AEDataFormat fmt)
{
  if (AE_IS_RAW(fmt))
    return false;

  switch(fmt)
  {
    case AE_FMT_U8:
    case AE_FMT_S16NE:
    case AE_FMT_S32NE:
    case AE_FMT_FLOAT:
    case AE_FMT_DOUBLE:
      return false;
    default:
      return true;
  }
}
