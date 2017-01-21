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

#include "VideoPlayerCodec.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/AEResampleFactory.h"

#include "cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "music/tags/TagLoaderTagLib.h"
#include "utils/log.h"
#include "URL.h"
#include "utils/StringUtils.h"

VideoPlayerCodec::VideoPlayerCodec()
{
  m_CodecName = "VideoPlayer";
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  m_pAudioCodec = NULL;
  m_nAudioStream = -1;
  m_nDecodedLen = 0;
  m_bInited = false;
  m_pResampler = NULL;
  m_needConvert = false;
  m_channels = 0;

  m_processInfo.reset(CProcessInfo::CreateInstance());
}

VideoPlayerCodec::~VideoPlayerCodec()
{
  DeInit();
}

AEAudioFormat VideoPlayerCodec::GetFormat()
{
  AEAudioFormat format;
  if (m_pAudioCodec)
  {
    format = m_pAudioCodec->GetFormat();
  }
  return format;
}

void VideoPlayerCodec::SetContentType(const std::string &strContent)
{
  m_strContentType = strContent;
  StringUtils::ToLower(m_strContentType);
}

bool VideoPlayerCodec::Init(const CFileItem &file, unsigned int filecache)
{
  const std::string &strFile = file.GetPath();

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

  CFileItem fileitem(file);
  fileitem.SetMimeType(m_strContentType);
  fileitem.SetMimeTypeForInternetFile();
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, fileitem);
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating input stream for %s", __FUNCTION__, strFileToOpen.c_str());
    return false;
  }

  //! @todo
  //! convey CFileItem::ContentLookup() into Open()
  if (!m_pInputStream->Open())
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
  int64_t demuxerId = -1;
  for (auto stream : m_pDemuxer->GetStreams())
  {
    if (stream && stream->type == STREAM_AUDIO)
    {
      m_nAudioStream = stream->uniqueId;
      demuxerId = stream->demuxerId;
      pStream = stream;
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

  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(hint, *m_processInfo.get());
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
  for (int nPacket=0; nPacket < 10 && (m_channels == 0 || m_format.m_sampleRate == 0); nPacket++)
  {
    BYTE dummy[256];
    int nSize = 256;
    if (ReadPCM(dummy, nSize, &nSize) == READ_ERROR)
      ++nErrors;

    m_srcFormat = m_pAudioCodec->GetFormat();
    m_format = m_srcFormat;
    m_channels = m_srcFormat.m_channelLayout.Count();
    m_bitsPerSample = CAEUtil::DataFormatToBits(m_srcFormat.m_dataFormat);
    m_bitsPerCodedSample = static_cast<CDemuxStreamAudio*>(pStream)->iBitsPerSample;
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
    if (Seek(1))
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

  if (m_channels == 0) // no data - just guess and hope for the best
  {
    m_srcFormat.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_2_0);
    m_channels = m_srcFormat.m_channelLayout.Count();
  }

  if (m_srcFormat.m_sampleRate == 0)
    m_srcFormat.m_sampleRate = 44100;

  m_TotalTime = m_pDemuxer->GetStreamLength();
  m_bitRate = m_pAudioCodec->GetBitRate();
  if (!m_bitRate && m_TotalTime)
  {
    m_bitRate = (int)(((m_pInputStream->GetLength()*1000) / m_TotalTime) * 8);
  }
  m_CodecName = m_pDemuxer->GetStreamCodecName(demuxerId, m_nAudioStream);

  m_needConvert = false;
  if (NeedConvert(m_srcFormat.m_dataFormat))
  {
    m_needConvert = true;
    m_pResampler = ActiveAE::CAEResampleFactory::Create();
    m_pResampler->Init(CAEUtil::GetAVChannelLayout(m_srcFormat.m_channelLayout),
                       m_channels,
                       m_srcFormat.m_sampleRate,
                       CAEUtil::GetAVSampleFormat(AE_FMT_FLOAT),
                       CAEUtil::DataFormatToUsedBits(AE_FMT_FLOAT),
                       CAEUtil::DataFormatToDitherBits(AE_FMT_FLOAT),
                       CAEUtil::GetAVChannelLayout(m_srcFormat.m_channelLayout),
                       m_channels,
                       m_srcFormat.m_sampleRate,
                       CAEUtil::GetAVSampleFormat(m_srcFormat.m_dataFormat),
                       CAEUtil::DataFormatToUsedBits(m_srcFormat.m_dataFormat),
                       CAEUtil::DataFormatToDitherBits(m_srcFormat.m_dataFormat),
                       false,
                       false,
                       NULL,
                       AE_QUALITY_UNKNOWN,
                       false);

    m_planes = AE_IS_PLANAR(m_srcFormat.m_dataFormat) ? m_channels : 1;
    m_format = m_srcFormat;
    m_format.m_dataFormat = AE_FMT_FLOAT;
    m_bitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  }

  m_strFileName = strFile;
  m_bInited = true;

  return true;
}

void VideoPlayerCodec::DeInit()
{
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
  m_bitsPerSample = 0;
  m_bitRate = 0;
  m_channels = 0;
  m_format.m_dataFormat = AE_FMT_INVALID;

  m_nDecodedLen = 0;

  m_strFileName = "";
  m_bInited = false;
}

bool VideoPlayerCodec::Seek(int64_t iSeekTime)
{
  // default to announce backwards seek if !m_pPacket to not make FFmpeg
  // skip mpeg audio frames at playback start
  bool seekback = true;

  bool ret = m_pDemuxer->SeekTime((int)iSeekTime, seekback);
  m_pAudioCodec->Reset();

  m_nDecodedLen = 0;

  return ret;
}

int VideoPlayerCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_nDecodedLen > 0)
  {
    int nLen = (size<m_nDecodedLen)?size:m_nDecodedLen;
    *actualsize = nLen;
    if (m_needConvert)
    {
      int samples = *actualsize / (m_bitsPerSample>>3);
      int frames = samples / m_channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioPlanes, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioPlanes[i] += frames*m_srcFormat.m_frameSize/m_planes;
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
  int bytes = m_pAudioCodec->GetData(m_audioPlanes);

  if (!bytes)
  {
    DemuxPacket* pPacket;
    do
    {
      pPacket = m_pDemuxer->Read();
    } while (pPacket && pPacket->iStreamId != m_nAudioStream);

    if (!pPacket)
    {
      return READ_EOF;
    }

    int ret = m_pAudioCodec->AddData(pPacket->pData, pPacket->iSize, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    if (ret < 0)
    {
      return READ_ERROR;
    }
  }

  m_nDecodedLen = bytes;
  // scale decoded bytes to destination format
  m_nDecodedLen = m_pAudioCodec->GetData(m_audioPlanes);
  if (m_needConvert)
    m_nDecodedLen *= (m_bitsPerSample>>3) / (m_srcFormat.m_frameSize / m_channels);

  *actualsize = (m_nDecodedLen <= size) ? m_nDecodedLen : size;
  if (*actualsize > 0)
  {
    if (m_needConvert)
    {
      int samples = *actualsize / (m_bitsPerSample>>3);
      int frames = samples / m_channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioPlanes, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioPlanes[i] += frames*m_srcFormat.m_frameSize/m_planes;
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

int VideoPlayerCodec::ReadRaw(uint8_t **pBuffer, int *bufferSize)
{
  DemuxPacket* pPacket;

  m_nDecodedLen = 0;
  DVDAudioFrame audioframe;

  m_pAudioCodec->GetData(audioframe);
  if (audioframe.nb_frames)
  {
    return READ_SUCCESS;
  }

  do
  {
    pPacket = m_pDemuxer->Read();
  } while (pPacket && pPacket->iStreamId != m_nAudioStream);

  if (!pPacket)
  {
    return READ_EOF;
  }

  int ret = m_pAudioCodec->AddData(pPacket->pData, pPacket->iSize, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  if (ret < 0)
  {
    return READ_ERROR;
  }

  m_pAudioCodec->GetData(audioframe);
  if (audioframe.nb_frames)
  {
    *bufferSize = audioframe.nb_frames;
    *pBuffer = audioframe.data[0];
  }
  else
  {
    *bufferSize = 0;
  }

  return READ_SUCCESS;
}

bool VideoPlayerCodec::CanInit()
{
  return true;
}

bool VideoPlayerCodec::CanSeek()
{
  return m_bCanSeek;
}

bool VideoPlayerCodec::NeedConvert(AEDataFormat fmt)
{
  if (fmt == AE_FMT_RAW)
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
