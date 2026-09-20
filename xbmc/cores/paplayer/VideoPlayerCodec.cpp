/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerCodec.h"

#include "ServiceBroker.h"
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

#include <algorithm>

extern "C"
{
#include <libavcodec/defs.h>
}

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
  for (auto stream : m_pDemuxer->GetStreams())
  {
    if (stream && stream->type == StreamType::AUDIO)
    {
      m_nAudioStream = stream->uniqueId;
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

  // Extract ReplayGain info
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

  if (!InitFormatFromStream(static_cast<CDemuxStreamAudio*>(pStream), true))
    return false;

  // Test seeking is supported (and rewind stream)
  m_bCanSeek = false;
  if (m_pInputStream->Seek(0, DVDSTREAM_SEEK_POSSIBLE))
  {
    if (Seek(1))
    {
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

  m_strFileName = file.GetDynPath();
  m_bInited = true;

  return true;
}

bool VideoPlayerCodec::InitFormatFromStream(CDemuxStreamAudio* stream, bool allowFormatFallback)
{
  // A demuxer may rebuild its stream objects as it reads, so take what the probe below needs
  // rather than holding the pointer across the reads it makes
  const int64_t demuxerId = stream->demuxerId;
  const int uniqueId = stream->uniqueId;
  const int bitsPerCodedSample = stream->iBitsPerSample;

  // Reset format information
  m_srcFormat = {};
  m_format = {};
  m_channels = 0;
  m_bitsPerSample = 0;
  m_bitsPerCodedSample = 0;
  m_planes = 0;

  // The probe loop calls ReadPCM()
  m_pResampler.reset();
  m_needConvert = false;

  // The probe has to consume whatever it reads, so a skip left over from an earlier seek must not
  // eat those packets. The caller seeks after us, which sets it again.
  m_skipToPts = DVD_NOPTS_VALUE;
  m_skipDecodedOutput = false;

  // Decode up to 10 packets to let the codec describe its output.
  // The first frame decoded may not reflect true format (eg. a partial DTS frame decodes as the 48kHz
  // core while the frames after it carry the 96kHz extension). Keep reading until two consecutive
  // frames agree.
  //
  // On a stream change this reads from demuxer current position rather than from the
  // point we are about to seek to, so ffmpeg can log container parse errors here before the caller's Seek()
  // puts it back on a cluster boundary. They are expected and recovered from. Probing after the seek
  // would silence them, at the cost of a second seek to undo the packets the probe consumes.
  int nErrors = 0;
  bool stable = false;
  AEAudioFormat probed{};
  for (int nPacket = 0; nPacket < 10 && !stable; nPacket++)
  {
    uint8_t dummy[256];
    size_t nSize = 256;
    const int read = ReadPCM(dummy, nSize, &nSize);
    if (read == READ_ERROR)
      ++nErrors;
    else if (read == READ_EOF)
      break; // there is nothing left to describe the stream with

    const AEAudioFormat previous = probed;
    probed = m_pAudioCodec->GetFormat();

    stable = probed.m_channelLayout.Count() != 0 && probed.m_sampleRate != 0 &&
             probed.m_frameSize != 0 && probed.m_sampleRate == previous.m_sampleRate &&
             probed.m_channelLayout.Count() == previous.m_channelLayout.Count() &&
             probed.m_dataFormat == previous.m_dataFormat;

    // Force move to next frame
    m_nDecodedLen = 0;
  }
  if (nErrors >= 10)
  {
    CLog::Log(LOGDEBUG, "{}: Could not decode data", __FUNCTION__);
    return false;
  }

  if (!stable)
    CLog::Log(LOGDEBUG,
              "{}: Format of audio stream uniqueId={} did not settle, using {} Hz, {} channels",
              __FUNCTION__, uniqueId, probed.m_sampleRate, probed.m_channelLayout.Count());

  m_srcFormat = probed;
  m_format = m_srcFormat;
  m_channels = m_srcFormat.m_channelLayout.Count();
  m_bitsPerSample = CAEUtil::DataFormatToBits(m_srcFormat.m_dataFormat);
  m_bitsPerCodedSample = bitsPerCodedSample;

  if (m_channels == 0 || m_srcFormat.m_sampleRate == 0)
  {
    // Guessing only when opening the file
    if (!allowFormatFallback)
    {
      CLog::Log(LOGERROR, "{}: Could not determine the format of audio stream uniqueId={}",
                __FUNCTION__, uniqueId);
      return false;
    }

    if (m_channels == 0) // no data - just guess and hope for the best
    {
      m_srcFormat.m_channelLayout = CAEChannelInfo(AE_CH_LAYOUT_2_0);
      m_channels = m_srcFormat.m_channelLayout.Count();
    }

    if (m_srcFormat.m_sampleRate == 0)
      m_srcFormat.m_sampleRate = 44100;
  }

  m_format = m_srcFormat;

  // Update duration, bitrate and codec name for this stream
  m_TotalTime = m_pDemuxer->GetStreamLength();
  m_bitRate = m_pAudioCodec->GetBitRate();
  if (!m_bitRate && m_TotalTime)
    m_bitRate = (int)(((m_pInputStream->GetLength() * 1000) / m_TotalTime) * 8);

  m_CodecName = m_pDemuxer->GetStreamCodecName(demuxerId, m_nAudioStream);

  m_planes = AE_IS_PLANAR(m_srcFormat.m_dataFormat) ? m_channels : 1;

  if (NeedConvert(m_srcFormat.m_dataFormat))
  {
    m_needConvert = true;
    // Some codecs (notably TrueHD over slow sources like NFS) do not populate
    // m_frameSize within the 10-packet sanity loop above even when channels
    // and sample rate are already known. For PCM-decoded data the frame size
    // is determined by bits-per-sample and channel count - both already known
    // at this point - so compute it as a fallback instead of failing silently.
    if (m_srcFormat.m_frameSize == 0)
    {
      // The fallback only handles interleaved formats: the downstream divisor
      // in ReadPCM (m_frameSize / m_channels) assumes interleaved layout, and
      // for planar the per-plane sample width is too small to safely divide by
      // m_channels (e.g. 8-bit planar stereo would integer-divide to 0).
      // Bail out for planar - preserves pre-existing behavior for that path.
      if (AE_IS_PLANAR(m_srcFormat.m_dataFormat))
        return false;
      const unsigned int bytesPerSample = CAEUtil::DataFormatToBits(m_srcFormat.m_dataFormat) >> 3;
      // Also bail if bits-per-sample is <8: a zero frame size would propagate
      // to ReadPCM and divide-by-zero on m_frameSize.
      if (bytesPerSample == 0)
        return false;
      m_srcFormat.m_frameSize = bytesPerSample * m_channels;
    }

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

    m_pResampler->Init(dstConfig, srcConfig, false, false, M_SQRT1_2, NULL, AE_QUALITY_UNKNOWN,
                       false, 0.0f);

    m_format.m_dataFormat = AE_FMT_FLOAT;
    m_bitsPerSample = CAEUtil::DataFormatToBits(m_format.m_dataFormat);
  }

  // Drop partial frame so ReadPCM() starts on a frame boundary
  m_nDecodedLen = 0;
  m_audioFrame = {};
  m_pAudioCodec->Reset();

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

  // A container can generally only seek to a point at or before the time asked for - for matroska
  // the start of a cluster, which can be a second or more earlier. Decoding straight from there
  // replays audio the caller believes it is already past, and since the caller keeps counting from
  // the time it requested, so times drift.
  // Note where we were asked to be so ReadPCM() can drop the packets in between.
  m_skipToPts = ret ? DVD_MSEC_TO_TIME(iSeekTime) : DVD_NOPTS_VALUE;
  m_skipDecodedOutput = false;

  return ret;
}

bool VideoPlayerCodec::PacketIsBeforeSeek(const DemuxPacket& packet)
{
  if (m_skipToPts == DVD_NOPTS_VALUE)
    return false;

  const double pts = packet.pts != DVD_NOPTS_VALUE ? packet.pts : packet.dts;
  if (pts == DVD_NOPTS_VALUE)
  {
    // Nothing to compare against, so hand the packet over and stop trying for this seek
    m_skipToPts = DVD_NOPTS_VALUE;
    return false;
  }

  // Keep the packet the requested time falls in, and any that starts at or after it.
  if (pts >= m_skipToPts || pts + packet.duration > m_skipToPts)
  {
    m_skipToPts = DVD_NOPTS_VALUE;
    return false;
  }

  return true;
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

    m_skipDecodedOutput = PacketIsBeforeSeek(*pPacket);

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

  if (bytes && m_skipDecodedOutput)
  {
    // Audio from before the seek is decoded rather than skipped, so that a codec carrying state
    // from one frame to the next has it, and dropped here instead. A codec that needs more than
    // one packet to produce anything relies on returning without data, so do the same.
    m_audioFrame = {};
    *actualsize = 0;
    return READ_SUCCESS;
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
  DemuxPacket* pPacket = nullptr;

  m_nDecodedLen = 0;
  DVDAudioFrame audioframe;

  m_pAudioCodec->GetData(audioframe);
  if (audioframe.nb_frames)
  {
    return READ_SUCCESS;
  }

  do
  {
    if (pPacket)
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    pPacket = m_pDemuxer->Read();
  } while (pPacket && (pPacket->iStreamId != m_nAudioStream || PacketIsBeforeSeek(*pPacket)));

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
      if (profile == AV_PROFILE_DTS_HD_HRA)
        format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_DTSHD;
      else if (profile == AV_PROFILE_DTS_HD_MA || profile == AV_PROFILE_DTS_HD_MA_X ||
               profile == AV_PROFILE_DTS_HD_MA_X_IMAX)
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

CDemuxStreamAudio* VideoPlayerCodec::GetAudioStream(int uniqueId) const
{
  const std::vector<CDemuxStreamAudio*> audioStreams = GetAudioStreams();
  const auto it = std::ranges::find_if(audioStreams, [uniqueId](const CDemuxStreamAudio* stream)
                                       { return stream->uniqueId == uniqueId; });
  return it != audioStreams.end() ? *it : nullptr;
}

std::vector<CDemuxStreamAudio*> VideoPlayerCodec::GetAudioStreams() const
{
  std::vector<CDemuxStreamAudio*> audioStreams;
  if (!m_pDemuxer)
    return audioStreams;

  for (auto* stream : m_pDemuxer->GetStreams())
  {
    if (stream && stream->type == StreamType::AUDIO)
      audioStreams.push_back(static_cast<CDemuxStreamAudio*>(stream));
  }
  return audioStreams;
}

int VideoPlayerCodec::GetStreamCount() const
{
  return static_cast<int>(GetAudioStreams().size());
}

void VideoPlayerCodec::GetStreamInfo(int index, AudioStreamInfo& info) const
{
  auto audioStreams = GetAudioStreams();
  if (index < 0 || index >= static_cast<int>(audioStreams.size()))
  {
    info.valid = false;
    return;
  }

  CDemuxStreamAudio* stream = audioStreams[index];
  info.valid = true;
  info.language = stream->language;
  info.name = stream->GetStreamName();
  info.codecName = stream->codecName;
  info.codecDesc = stream->GetStreamType();
  info.channels = stream->iChannels;
  info.samplerate = stream->iSampleRate;
  info.bitspersample = stream->iBitsPerSample;
  info.bitrate = stream->iBitRate;
  info.flags = stream->flags;
}

bool VideoPlayerCodec::SetStream(int index)
{
  const std::vector<CDemuxStreamAudio*> audioStreams = GetAudioStreams();
  if (index < 0 || index >= static_cast<int>(audioStreams.size()))
  {
    CLog::Log(LOGERROR, "{}: Invalid audio stream index {}", __FUNCTION__, index);
    return false;
  }

  CDemuxStreamAudio* newStream = audioStreams[index];
  if (newStream->uniqueId == m_nAudioStream)
    return true;

  // Remember the current stream so we can fall back to it if the switch goes bad
  CDemuxStreamAudio* oldStream = nullptr;
  for (auto* stream : audioStreams)
  {
    if (stream->uniqueId == m_nAudioStream)
    {
      oldStream = stream;
      break;
    }
  }

  const int newUniqueId = newStream->uniqueId;
  const int oldUniqueId = m_nAudioStream;

  const SwitchResult result = SwitchToStream(newStream, oldStream);
  if (result == SwitchResult::NOT_STARTED)
    return false;

  if (result == SwitchResult::FAILED)
  {
    // The codec has already been destroyed, so try and restore. The switch may have rebuilt the
    // stream objects, so take them again rather than reusing the pointers from before.
    CDemuxStreamAudio* const restoreStream = GetAudioStream(oldUniqueId);
    if (restoreStream &&
        SwitchToStream(restoreStream, GetAudioStream(newUniqueId)) == SwitchResult::OK)
      CLog::Log(LOGWARNING, "{}: Restored audio stream uniqueId={} after a failed switch",
                __FUNCTION__, oldUniqueId);
    else
      CLog::Log(LOGERROR, "{}: Audio codec left unusable after a failed switch", __FUNCTION__);

    return false;
  }

  CLog::Log(LOGDEBUG, "{}: Switched to audio stream {} (uniqueId={})", __FUNCTION__, index,
            m_nAudioStream);
  return true;
}

VideoPlayerCodec::SwitchResult VideoPlayerCodec::SwitchToStream(CDemuxStreamAudio* newStream,
                                                                CDemuxStreamAudio* oldStream)
{
  const int64_t demuxerId = newStream->demuxerId;
  const int uniqueId = newStream->uniqueId;
  const bool hasOldStream = oldStream != nullptr;
  const int64_t oldDemuxerId = hasOldStream ? oldStream->demuxerId : 0;
  const int oldUniqueId = hasOldStream ? oldStream->uniqueId : -1;

  // Open the stream before describing it. A demuxer is free to complete or correct a stream's
  // properties as it opens it, and to rebuild the stream object while doing so.
  m_pDemuxer->EnableStream(demuxerId, uniqueId, true);
  m_pDemuxer->OpenStream(demuxerId, uniqueId);
  newStream = GetAudioStream(uniqueId);

  // Build the codec before anything else is disturbed, so that failure leaves the stream that
  // is playing untouched
  std::unique_ptr<CDVDAudioCodec> codec;
  if (newStream)
  {
    CDVDStreamInfo hint(*newStream, true);
    const CAEStreamInfo::DataType ptStreamType =
        GetPassthroughStreamType(hint.codec, hint.samplerate, hint.profile);

    codec = CDVDFactoryCodec::CreateAudioCodec(hint, *m_processInfo, true, true, ptStreamType);
  }

  if (!codec)
  {
    CLog::Log(LOGERROR, "{}: Could not create audio codec for stream uniqueId={}", __FUNCTION__,
              uniqueId);
    if (uniqueId != m_nAudioStream)
      m_pDemuxer->EnableStream(demuxerId, uniqueId, false);
    return SwitchResult::NOT_STARTED;
  }

  // Past this point we are committed to the new stream

  if (hasOldStream)
    m_pDemuxer->EnableStream(oldDemuxerId, oldUniqueId, false);

  m_nAudioStream = uniqueId;

  // m_audioFrame borrows buffers owned by the codec, so it must be dropped first
  m_nDecodedLen = 0;
  m_audioFrame = {};
  m_pAudioCodec = std::move(codec);

  m_pDemuxer->Flush();

  return InitFormatFromStream(newStream, false) ? SwitchResult::OK : SwitchResult::FAILED;
}
