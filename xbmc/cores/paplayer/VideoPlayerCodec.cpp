/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerCodec.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "music/tags/TagLoaderTagLib.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

VideoPlayerCodec::VideoPlayerCodec() : m_processInfo(CProcessInfo::CreateInstance())
{
  m_CodecName = "VideoPlayer";
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

void  VideoPlayerCodec::SetPassthroughStreamType(CAEStreamInfo::DataType streamType)
{
  m_srcFormat.m_streamInfo.m_type = streamType;
}

bool VideoPlayerCodec::Init(const CFileItem &file, unsigned int filecache)
{
  // take precaution if Init()ialized earlier
  if (m_bInited)
  {
    // keep things as is if Init() was done with known strFile
    if (m_strFileName == file.GetDynPath())
      return true;

    // got differing filename, so cleanup before starting over
    DeInit();
  }

  m_nDecodedLen = 0;

  CFileItem fileitem(file);
  fileitem.SetMimeType(m_strContentType);
  fileitem.SetMimeTypeForInternetFile();
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, fileitem);
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "{}: Error creating input stream for {}", __FUNCTION__, file.GetDynPath());
    return false;
  }

  //! @todo
  //! convey CFileItem::ContentLookup() into Open()
  if (!m_pInputStream->Open())
  {
    CLog::Log(LOGERROR, "{}: Error opening file {}", __FUNCTION__, file.GetDynPath());
    if (m_pInputStream.use_count() > 1)
      throw std::runtime_error("m_pInputStream reference count is greater than 1");
    m_pInputStream.reset();
    return false;
  }

  m_pDemuxer = NULL;

  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if (!m_pDemuxer)
    {
      if (m_pInputStream.use_count() > 1)
        throw std::runtime_error("m_pInputStream reference count is greater than 1");
      m_pInputStream.reset();
      CLog::Log(LOGERROR, "{}: Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "{}: Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
    {
      delete m_pDemuxer;
      m_pDemuxer = NULL;
    }
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
    CLog::Log(LOGERROR, "{}: Could not find audio stream", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    if (m_pInputStream.use_count() > 1)
      throw std::runtime_error("m_pInputStream reference count is greater than 1");
    m_pInputStream.reset();
    return false;
  }

  CDVDStreamInfo hint(*pStream, true);

  CAEStreamInfo::DataType ptStreamTye =
      GetPassthroughStreamType(hint.codec, hint.samplerate, hint.profile);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(hint, *m_processInfo, true, true, ptStreamTye);
  if (!m_pAudioCodec)
  {
    CLog::Log(LOGERROR, "{}: Could not create audio codec", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    if (m_pInputStream.use_count() > 1)
      throw std::runtime_error("m_pInputStream reference count is greater than 1");
    m_pInputStream.reset();
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
  tagLoaderTagLib.Load(file.GetDynPath(), m_tag, strFallbackFileExtension);

  // we have to decode initial data in order to get channels/samplerate
  // for sanity - we read no more than 10 packets
  int nErrors = 0;
  for (int nPacket = 0;
       nPacket < 10 && (m_channels == 0 || m_format.m_sampleRate == 0 || m_format.m_frameSize == 0);
       nPacket++)
  {
    uint8_t dummy[256];
    size_t nSize = 256;
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
    CLog::Log(LOGDEBUG, "{}: Could not decode data", __FUNCTION__);
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
      if (!m_pDemuxer->Reset())
        return false;
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
    // if we don't know the framesize yet, we will fail when converting
    if (m_srcFormat.m_frameSize == 0)
      return false;

    m_pResampler = ActiveAE::CAEResampleFactory::Create();

    SampleConfig dstConfig, srcConfig;
    dstConfig.channel_layout = CAEUtil::GetAVChannelLayout(m_srcFormat.m_channelLayout);
    dstConfig.channels = m_channels;
    dstConfig.sample_rate = m_srcFormat.m_sampleRate;
    dstConfig.fmt = CAEUtil::GetAVSampleFormat(AE_FMT_FLOAT);
    dstConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(AE_FMT_FLOAT);
    dstConfig.dither_bits = CAEUtil::DataFormatToDitherBits(AE_FMT_FLOAT);

    srcConfig.channel_layout = CAEUtil::GetAVChannelLayout(m_srcFormat.m_channelLayout);
    srcConfig.channels = m_channels;
    srcConfig.sample_rate = m_srcFormat.m_sampleRate;
    srcConfig.fmt = CAEUtil::GetAVSampleFormat(m_srcFormat.m_dataFormat);
    srcConfig.bits_per_sample = CAEUtil::DataFormatToUsedBits(m_srcFormat.m_dataFormat);
    srcConfig.dither_bits = CAEUtil::DataFormatToDitherBits(m_srcFormat.m_dataFormat);

    m_pResampler->Init(dstConfig, srcConfig,
                       false,
                       false,
                       M_SQRT1_2,
                       NULL,
                       AE_QUALITY_UNKNOWN,
                       false);

    m_planes = AE_IS_PLANAR(m_srcFormat.m_dataFormat) ? m_channels : 1;
    m_format = m_srcFormat;
    m_format.m_dataFormat = AE_FMT_FLOAT;
    m_bitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  }

  m_strFileName = file.GetDynPath();
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

  if (m_pInputStream.use_count() > 1)
    throw std::runtime_error("m_pInputStream reference count is greater than 1");
  m_pInputStream.reset();

  m_pAudioCodec.reset();

  m_pResampler.reset();

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

int VideoPlayerCodec::ReadPCM(uint8_t* pBuffer, size_t size, size_t* actualsize)
{
  if (m_nDecodedLen > 0)
  {
    size_t nLen = (size < m_nDecodedLen) ? size : m_nDecodedLen;
    *actualsize = nLen;
    if (m_needConvert)
    {
      int samples = *actualsize / (m_bitsPerSample>>3);
      int frames = samples / m_channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioFrame.data, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioFrame.data[i] += frames*m_srcFormat.m_frameSize/m_planes;
      }
    }
    else
    {
      memcpy(pBuffer, m_audioFrame.data[0], *actualsize);
      m_audioFrame.data[0] += (*actualsize);
    }
    m_nDecodedLen -= nLen;
    return READ_SUCCESS;
  }

  m_nDecodedLen = 0;
  m_pAudioCodec->GetData(m_audioFrame);
  int bytes = m_audioFrame.nb_frames * m_audioFrame.framesize;

  if (!bytes)
  {
    DemuxPacket* pPacket = nullptr;
    do
    {
      if (pPacket)
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = m_pDemuxer->Read();
    } while (pPacket && pPacket->iStreamId != m_nAudioStream);

    if (!pPacket)
    {
      return READ_EOF;
    }

    pPacket->pts = DVD_NOPTS_VALUE;
    pPacket->dts = DVD_NOPTS_VALUE;

    int ret = m_pAudioCodec->AddData(*pPacket);
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    if (ret < 0)
    {
      return READ_ERROR;
    }

    m_pAudioCodec->GetData(m_audioFrame);
    bytes = m_audioFrame.nb_frames * m_audioFrame.framesize;
  }

  m_nDecodedLen = bytes;
  // scale decoded bytes to destination format
  if (m_needConvert)
    m_nDecodedLen *= (m_bitsPerSample>>3) / (m_srcFormat.m_frameSize / m_channels);

  *actualsize = (m_nDecodedLen <= size) ? m_nDecodedLen : size;
  if (*actualsize > 0)
  {
    if (m_needConvert)
    {
      int samples = *actualsize / (m_bitsPerSample>>3);
      int frames = samples / m_channels;
      m_pResampler->Resample(&pBuffer, frames, m_audioFrame.data, frames, 1.0);
      for (int i=0; i<m_planes; i++)
      {
        m_audioFrame.data[i] += frames*m_srcFormat.m_frameSize/m_planes;
      }
    }
    else
    {
      memcpy(pBuffer, m_audioFrame.data[0], *actualsize);
      m_audioFrame.data[0] += *actualsize;
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
  pPacket->pts = DVD_NOPTS_VALUE;
  pPacket->dts = DVD_NOPTS_VALUE;
  int ret = m_pAudioCodec->AddData(*pPacket);
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

CAEStreamInfo::DataType VideoPlayerCodec::GetPassthroughStreamType(AVCodecID codecId,
                                                                   int samplerate,
                                                                   int profile)
{
  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_RAW;
  format.m_sampleRate = samplerate;
  format.m_streamInfo.m_type = CAEStreamInfo::DataType::STREAM_TYPE_NULL;
  switch (codecId)
  {
    case AV_CODEC_ID_AC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_EAC3:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_EAC3;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_DTS:
      if (profile == FF_PROFILE_DTS_HD_HRA)
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD;
      else if (profile == FF_PROFILE_DTS_HD_MA)
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_MA;
      else
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    case AV_CODEC_ID_TRUEHD:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_TRUEHD;
      format.m_streamInfo.m_sampleRate = samplerate;
      break;

    default:
      format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_NULL;
  }

  bool supports = CServiceBroker::GetActiveAE()->SupportsRaw(format);

  if (!supports && codecId == AV_CODEC_ID_DTS &&
      format.m_streamInfo.m_type != CAEStreamInfo::STREAM_TYPE_DTSHD_CORE &&
      CServiceBroker::GetActiveAE()->UsesDtsCoreFallback())
  {
    format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD_CORE;
    supports = CServiceBroker::GetActiveAE()->SupportsRaw(format);
  }

  if (supports)
    return format.m_streamInfo.m_type;
  else
    return CAEStreamInfo::DataType::STREAM_TYPE_NULL;
}
