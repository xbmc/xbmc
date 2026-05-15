/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicCodecInfoFFmpeg.h"

#include "cores/FFmpeg.h"
#include "filesystem/File.h"

using namespace XFILE;

static int vfs_file_read(void* h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t vfs_file_seek(void* h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if (whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

bool CMusicCodecInfoFFmpeg::GetMusicCodecInfo(const std::string& strFileName,
                                              musicCodecInfo& codec_info)
{
  const AVCodec* decoder = nullptr;
  CFile file;
  bool haveInfo = false;
  if (!file.Open(strFileName))
    return haveInfo;

  int bufferSize = 4096;
  int blockSize = file.GetChunkSize();
  if (blockSize > 1)
    bufferSize = blockSize;
  uint8_t* buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
  if (!buffer)
    return haveInfo;

  AVIOContext* ioctx =
      avio_alloc_context(buffer, bufferSize, 0, &file, vfs_file_read, nullptr, vfs_file_seek);
  if (!ioctx)
  {
    av_free(buffer);
    return haveInfo;
  }

  AVFormatContext* fctx = avformat_alloc_context();
  if (!fctx)
  {
    av_free(ioctx->buffer);
    av_free(ioctx);
    return haveInfo;
  }
  fctx->pb = ioctx;

  if (file.IoControl(IOControl::SEEK_POSSIBLE, nullptr) != 1)
    ioctx->seekable = 0;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(ioctx, &iformat, strFileName.c_str(), nullptr, 0, 0);

  AVStream* st = nullptr;
  if (avformat_open_input(&fctx, strFileName.c_str(), iformat, nullptr) == 0)
  {
    fctx->flags |= AVFMT_FLAG_NOPARSE;
    if (avformat_find_stream_info(fctx, nullptr) >= 0)
    {
      int streamIndex = -1;
      // Look for the default audio stream first
      for (unsigned int i = 0; i < fctx->nb_streams; ++i)
      {
        if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          if (fctx->streams[i]->disposition & AV_DISPOSITION_DEFAULT)
          {
            streamIndex = i;
            break; // Found the default audio stream, no need to check further
          }
        }
      }
      if (streamIndex == -1)
      {
        for (unsigned int i = 0; i < fctx->nb_streams; ++i)
        {
          if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            streamIndex = i;
            break; // Found the first audio stream
          }
        }
      }

      if (streamIndex != -1)
      {
        st = fctx->streams[streamIndex];
        decoder = avcodec_find_decoder(st->codecpar->codec_id);
        if (decoder)
        {
          std::string codec_name = "unknown";

          codec_name = avcodec_get_name(st->codecpar->codec_id);
          int par_profile = st->codecpar->profile;
          if (st->codecpar->codec_id == AV_CODEC_ID_DTS)
          {
            switch (par_profile)
            {
              case AV_PROFILE_DTS_HD_MA_X:
                codec_name = "dtshd_ma_x";
                break;
              case AV_PROFILE_DTS_HD_MA_X_IMAX:
                codec_name = "dtshd_ma_x_imax";
                break;
              case AV_PROFILE_DTS_ES:
                codec_name = "dts_es";
                break;
              case AV_PROFILE_DTS_96_24:
                codec_name = "dts_96_24";
                break;
              case AV_PROFILE_DTS_HD_HRA:
                codec_name = "dtshd_hra";
                break;
              case AV_PROFILE_DTS_EXPRESS:
                codec_name = "dts_express";
                break;
              case AV_PROFILE_DTS_HD_MA:
                codec_name = "dtshd_ma";
                break;
              default:
                codec_name = "dca";
                break;
            }
          }
          if (st->codecpar->codec_id == AV_CODEC_ID_EAC3 &&
              par_profile == AV_PROFILE_EAC3_DDP_ATMOS)
            codec_name = "eac3_ddp_atmos";

          if (st->codecpar->codec_id == AV_CODEC_ID_TRUEHD &&
              par_profile == AV_PROFILE_TRUEHD_ATMOS)
            codec_name = "truehd_atmos";
          codec_info.codecName = codec_name;
          codec_info.bitRate = static_cast<int>(st->codecpar->bit_rate / 1000);
          codec_info.channels = st->codecpar->ch_layout.nb_channels;
          codec_info.bitsPerSample = (st->codecpar->bits_per_coded_sample != 0)
                                         ? st->codecpar->bits_per_coded_sample
                                         : st->codecpar->bits_per_raw_sample;
          codec_info.sampleRate = st->codecpar->sample_rate;
          // st->duration is in st->time_base units; rescale to whole seconds.
          // Fall back to the container duration when the stream value is unset.
          if (st->duration != AV_NOPTS_VALUE)
            codec_info.duration =
                static_cast<int>(av_rescale_q(st->duration, st->time_base, AVRational{1, 1}));
          else if (fctx->duration != AV_NOPTS_VALUE)
            codec_info.duration = static_cast<int>(fctx->duration / AV_TIME_BASE);
          else
            codec_info.duration = 0;
          haveInfo = true;
        }
      }
    }

    avformat_close_input(&fctx);
  }
  av_free(ioctx->buffer);
  av_free(ioctx);
  return haveInfo;
}
