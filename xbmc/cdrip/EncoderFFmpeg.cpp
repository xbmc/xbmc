/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EncoderFFmpeg.h"

#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "cores/FFmpeg.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::CDRIP;
using FFMPEG_HELP_TOOLS::FFMpegErrorToString;
using FFMPEG_HELP_TOOLS::FFMpegException;

bool CEncoderFFmpeg::Init()
{
  try
  {
    ADDON::AddonPtr addon;
    const std::string addonId = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_AUDIOCDS_ENCODER);
    bool success =
        CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, ADDON::OnlyEnabled::CHOICE_YES);
    int bitrate;
    if (success && addon)
    {
      addon->GetSettingInt("bitrate", bitrate);
      bitrate *= 1000; /* Multiply as on settings as kbps */
    }
    else
    {
      throw FFMpegException("Could not get add-on: {}", addonId);
    }

    // Hack fix about PTS on generated files.
    // - AAC need to multiply with sample rate
    //   - Note: Within Kodi it can still played without use of sample rate, only becomes by VLC the problem visible,
    // - WMA need only the multiply with 1000
    if (addonId == "audioencoder.kodi.builtin.aac")
      m_samplesCountMultiply = m_iInSampleRate;
    else if (addonId == "audioencoder.kodi.builtin.wma")
      m_samplesCountMultiply = 1000;
    else
      throw FFMpegException("Internal add-on id \"{}\" not known as usable", addonId);

    const std::string filename = URIUtils::GetFileName(m_strFile);

    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx)
      throw FFMpegException("Could not allocate output format context");

    auto buffer = static_cast<uint8_t*>(av_malloc(BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE));
    if (!buffer)
      throw FFMpegException("Could not allocate buffer");

    m_formatCtx->pb = avio_alloc_context(buffer, BUFFER_SIZE, AVIO_FLAG_WRITE, this, nullptr,
                                         avio_write_callback, avio_seek_callback);
    if (!m_formatCtx->pb)
    {
      av_free(buffer);
      throw FFMpegException("Failed to allocate ByteIOContext");
    }

    /* Guess the desired container format based on the file extension. */
    m_formatCtx->oformat = av_guess_format(nullptr, filename.c_str(), nullptr);
    if (!m_formatCtx->oformat)
      throw FFMpegException("Could not find output file format");

    m_formatCtx->url = av_strdup(filename.c_str());
    if (!m_formatCtx->url)
      throw FFMpegException("Could not allocate url");

    /* Find the encoder to be used by its name. */
    const AVCodec* codec = avcodec_find_encoder(m_formatCtx->oformat->audio_codec);
    if (!codec)
      throw FFMpegException("Unable to find a suitable FFmpeg encoder");

    /* Create a new audio stream in the output file container. */
    m_stream = avformat_new_stream(m_formatCtx, nullptr);
    if (!m_stream)
      throw FFMpegException("Failed to allocate AVStream context");

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx)
      throw FFMpegException("Failed to allocate the encoder context");

    /* Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion. */
    av_channel_layout_uninit(&m_codecCtx->ch_layout);
    av_channel_layout_default(&m_codecCtx->ch_layout, m_iInChannels);
    m_codecCtx->sample_rate = m_iInSampleRate;
    m_codecCtx->sample_fmt = codec->sample_fmts[0];
    m_codecCtx->bit_rate = bitrate;

    /* Allow experimental encoders (like FFmpeg builtin AAC encoder) */
    m_codecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    m_codecCtx->time_base.num = 1;
    m_codecCtx->time_base.den = m_iInSampleRate;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if (m_formatCtx->oformat->flags & AVFMT_GLOBALHEADER)
      m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int err = avcodec_open2(m_codecCtx, codec, nullptr);
    if (err < 0)
      throw FFMpegException("Failed to open the codec {} (error '{}')",
                            codec->long_name ? codec->long_name : codec->name,
                            FFMpegErrorToString(err));

    err = avcodec_parameters_from_context(m_stream->codecpar, m_codecCtx);
    if (err < 0)
      throw FFMpegException("Failed to copy encoder parameters to output stream (error '{}')",
                            FFMpegErrorToString(err));

    m_inFormat = GetInputFormat(m_iInBitsPerSample);
    m_outFormat = m_codecCtx->sample_fmt;
    m_needConversion = (m_outFormat != m_inFormat);

    /* calculate how many bytes we need per frame */
    m_neededFrames = m_codecCtx->frame_size;
    m_neededBytes =
        av_samples_get_buffer_size(nullptr, m_iInChannels, m_neededFrames, m_inFormat, 0);
    m_buffer = static_cast<uint8_t*>(av_malloc(m_neededBytes));
    m_bufferSize = 0;

    m_bufferFrame = av_frame_alloc();
    if (!m_bufferFrame || !m_buffer)
      throw FFMpegException("Failed to allocate necessary buffers");

    m_bufferFrame->nb_samples = m_codecCtx->frame_size;
    m_bufferFrame->format = m_inFormat;

    av_channel_layout_uninit(&m_bufferFrame->ch_layout);
    av_channel_layout_copy(&m_bufferFrame->ch_layout, &m_codecCtx->ch_layout);

    m_bufferFrame->sample_rate = m_codecCtx->sample_rate;

    err = av_frame_get_buffer(m_bufferFrame, 0);
    if (err < 0)
      throw FFMpegException("Could not allocate output frame samples (error '{}')",
                            FFMpegErrorToString(err));

    avcodec_fill_audio_frame(m_bufferFrame, m_iInChannels, m_inFormat, m_buffer, m_neededBytes, 0);

    if (m_needConversion)
    {
      int ret = swr_alloc_set_opts2(&m_swrCtx, &m_codecCtx->ch_layout, m_outFormat,
                                    m_codecCtx->sample_rate, &m_codecCtx->ch_layout, m_inFormat,
                                    m_codecCtx->sample_rate, 0, nullptr);
      if (ret || swr_init(m_swrCtx) < 0)
        throw FFMpegException("Failed to initialize the resampler");

      m_resampledBufferSize =
          av_samples_get_buffer_size(nullptr, m_iInChannels, m_neededFrames, m_outFormat, 0);
      m_resampledBuffer = static_cast<uint8_t*>(av_malloc(m_resampledBufferSize));
      m_resampledFrame = av_frame_alloc();
      if (!m_resampledBuffer || !m_resampledFrame)
        throw FFMpegException("Failed to allocate a frame for resampling");

      m_resampledFrame->nb_samples = m_neededFrames;
      m_resampledFrame->format = m_outFormat;
      av_channel_layout_uninit(&m_resampledFrame->ch_layout);
      av_channel_layout_copy(&m_resampledFrame->ch_layout, &m_codecCtx->ch_layout);
      m_resampledFrame->sample_rate = m_codecCtx->sample_rate;

      err = av_frame_get_buffer(m_resampledFrame, 0);
      if (err < 0)
        throw FFMpegException("Could not allocate output resample frame samples (error '{}')",
                              FFMpegErrorToString(err));

      avcodec_fill_audio_frame(m_resampledFrame, m_iInChannels, m_outFormat, m_resampledBuffer,
                               m_resampledBufferSize, 0);
    }

    /* set the tags */
    SetTag("album", m_strAlbum);
    SetTag("album_artist", m_strArtist);
    SetTag("genre", m_strGenre);
    SetTag("title", m_strTitle);
    SetTag("track", m_strTrack);
    SetTag("encoder", CSysInfo::GetAppName() + " FFmpeg Encoder");

    /* write the header */
    err = avformat_write_header(m_formatCtx, nullptr);
    if (err != 0)
      throw FFMpegException("Failed to write the header (error '{}')", FFMpegErrorToString(err));

    CLog::Log(LOGDEBUG, "CEncoderFFmpeg::{} - Successfully initialized with muxer {} and codec {}",
              __func__,
              m_formatCtx->oformat->long_name ? m_formatCtx->oformat->long_name
                                              : m_formatCtx->oformat->name,
              codec->long_name ? codec->long_name : codec->name);
  }
  catch (const FFMpegException& caught)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::{} - {}", __func__, caught.what());

    av_freep(&m_buffer);
    av_channel_layout_uninit(&m_bufferFrame->ch_layout);
    av_frame_free(&m_bufferFrame);
    swr_free(&m_swrCtx);
    av_channel_layout_uninit(&m_resampledFrame->ch_layout);
    av_frame_free(&m_resampledFrame);
    av_freep(&m_resampledBuffer);
    av_channel_layout_uninit(&m_codecCtx->ch_layout);
    avcodec_free_context(&m_codecCtx);
    if (m_formatCtx)
    {
      av_freep(&m_formatCtx->pb->buffer);
      av_freep(&m_formatCtx->pb);
      avformat_free_context(m_formatCtx);
    }
    return false;
  }

  return true;
}

void CEncoderFFmpeg::SetTag(const std::string& tag, const std::string& value)
{
  av_dict_set(&m_formatCtx->metadata, tag.c_str(), value.c_str(), 0);
}

int CEncoderFFmpeg::avio_write_callback(void* opaque, uint8_t* buf, int buf_size)
{
  CEncoderFFmpeg* enc = static_cast<CEncoderFFmpeg*>(opaque);
  if (enc->Write(buf, buf_size) != buf_size)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::{} - Error writing FFmpeg buffer to file", __func__);
    return -1;
  }
  return buf_size;
}

int64_t CEncoderFFmpeg::avio_seek_callback(void* opaque, int64_t offset, int whence)
{
  CEncoderFFmpeg* enc = static_cast<CEncoderFFmpeg*>(opaque);
  return enc->Seek(offset, whence);
}

ssize_t CEncoderFFmpeg::Encode(uint8_t* pbtStream, size_t nNumBytesRead)
{
  while (nNumBytesRead > 0)
  {
    size_t space = m_neededBytes - m_bufferSize;
    size_t copy = nNumBytesRead > space ? space : nNumBytesRead;

    memcpy(&m_buffer[m_bufferSize], pbtStream, copy);
    m_bufferSize += copy;
    pbtStream += copy;
    nNumBytesRead -= copy;

    /* only write full packets */
    if (m_bufferSize == m_neededBytes)
    {
      if (!WriteFrame())
        return 0;
    }
  }

  return 1;
}

bool CEncoderFFmpeg::WriteFrame()
{
  int err = AVERROR_UNKNOWN;
  AVPacket* pkt = av_packet_alloc();
  if (!pkt)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::{} - av_packet_alloc failed: {}", __func__,
              strerror(errno));
    return false;
  }

  try
  {
    AVFrame* frame;
    if (m_needConversion)
    {
      //! @bug libavresample isn't const correct
      if (swr_convert(m_swrCtx, m_resampledFrame->data, m_neededFrames,
                      const_cast<const uint8_t**>(m_bufferFrame->extended_data),
                      m_neededFrames) < 0)
        throw FFMpegException("Error resampling audio");

      frame = m_resampledFrame;
    }
    else
      frame = m_bufferFrame;

    if (frame)
    {
      /* To fix correct length on wma files */
      frame->pts = m_samplesCount;
      m_samplesCount += frame->nb_samples * m_samplesCountMultiply / m_codecCtx->time_base.den;
    }

    m_bufferSize = 0;
    err = avcodec_send_frame(m_codecCtx, frame);
    if (err < 0)
      throw FFMpegException("Error sending a frame for encoding (error '{}')",
                            FFMpegErrorToString(err));

    while (err >= 0)
    {
      err = avcodec_receive_packet(m_codecCtx, pkt);
      if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
      {
        av_packet_free(&pkt);
        return (err == AVERROR(EAGAIN)) ? false : true;
      }
      else if (err < 0)
      {
        throw FFMpegException("Error during encoding (error '{}')", FFMpegErrorToString(err));
      }

      err = av_write_frame(m_formatCtx, pkt);
      if (err < 0)
        throw FFMpegException("Failed to write the frame data (error '{}')",
                              FFMpegErrorToString(err));

      av_packet_unref(pkt);
    }
  }
  catch (const FFMpegException& caught)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::{} - {}", __func__, caught.what());
  }

  av_packet_free(&pkt);

  return (err) ? false : true;
}

bool CEncoderFFmpeg::Close()
{
  if (m_formatCtx)
  {
    /* if there is anything still in the buffer */
    if (m_bufferSize > 0)
    {
      /* zero the unused space so we dont encode random junk */
      memset(&m_buffer[m_bufferSize], 0, m_neededBytes - m_bufferSize);
      /* write any remaining data */
      WriteFrame();
    }

    /* Flush if needed */
    av_freep(&m_buffer);
    av_channel_layout_uninit(&m_bufferFrame->ch_layout);
    av_frame_free(&m_bufferFrame);
    swr_free(&m_swrCtx);
    av_channel_layout_uninit(&m_resampledFrame->ch_layout);
    av_frame_free(&m_resampledFrame);
    av_freep(&m_resampledBuffer);
    m_needConversion = false;

    WriteFrame();

    /* write the trailer */
    av_write_trailer(m_formatCtx);

    /* cleanup */
    av_channel_layout_uninit(&m_codecCtx->ch_layout);
    avcodec_free_context(&m_codecCtx);
    av_freep(&m_formatCtx->pb->buffer);
    av_freep(&m_formatCtx->pb);
    avformat_free_context(m_formatCtx);
  }

  m_bufferSize = 0;

  return true;
}

AVSampleFormat CEncoderFFmpeg::GetInputFormat(int inBitsPerSample)
{
  switch (inBitsPerSample)
  {
    case 8:
      return AV_SAMPLE_FMT_U8;
    case 16:
      return AV_SAMPLE_FMT_S16;
    case 32:
      return AV_SAMPLE_FMT_S32;
    default:
      throw FFMpegException("Invalid input bits per sample");
  }
}
