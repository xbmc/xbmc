/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicEmbeddedCoverLoaderFFmpeg.h"

#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "tags/MusicInfoTag.h"
#include "utils/EmbeddedArt.h"

#include <map>
#include <string>

using namespace MUSIC_INFO;
using namespace XFILE;

void CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(AVFormatContext* fctx,
                                                       CMusicInfoTag& tag,
                                                       EmbeddedArt* art /* nullptr */)
{
  if (!fctx || !fctx->streams)
    return;
  for (size_t i = 0; i < fctx->nb_streams; ++i)
  {
    const AVStream* fctx_pic = fctx->streams[i];
    if (!fctx_pic || (fctx_pic->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;

    AVCodecID pic_id = fctx_pic->codecpar->codec_id;
    const std::map<AVCodecID, std::string> mime_map = {{AV_CODEC_ID_MJPEG, "image/jpeg"},
                                                       {AV_CODEC_ID_PNG, "image/png"},
                                                       {AV_CODEC_ID_BMP, "image/bmp"}};

    auto it = mime_map.find(pic_id);
    if (it != mime_map.end())
    {
      const auto& mimetype = it->second;
      size_t pic_size = fctx_pic->attached_pic.size;
      uint8_t* pic = fctx_pic->attached_pic.data;

      tag.SetCoverArtInfo(pic_size, mimetype);
      if (art)
        art->Set(pic, pic_size, mimetype, "thumb");
      break; // just need one cover
    }
  }
}

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

bool CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(const std::string& strFileName,
                                                       CMusicInfoTag& tag,
                                                       EmbeddedArt* art /* = nullptr */)
{
  CFile file;
  if (!file.Open(strFileName))
    return false;

  int bufferSize = 4096;
  int blockSize = file.GetChunkSize();
  if (blockSize > 1)
    bufferSize = blockSize;

  uint8_t* buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
  if (!buffer)
    return false;

  AVIOContext* ioctx =
      avio_alloc_context(buffer, bufferSize, 0, &file, vfs_file_read, nullptr, vfs_file_seek);
  if (!ioctx)
  {
    av_free(buffer);
    return false;
  }

  if (file.IoControl(IOControl::SEEK_POSSIBLE, nullptr) != 1)
    ioctx->seekable = 0;

  AVFormatContext* fctx = avformat_alloc_context();
  if (!fctx)
  {
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }
  fctx->pb = ioctx;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(ioctx, &iformat, strFileName.c_str(), nullptr, 0, 0);

  if (avformat_open_input(&fctx, strFileName.c_str(), iformat, nullptr) < 0)
  {
    if (fctx)
      avformat_close_input(&fctx);
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }

  // We only need stream metadata (attached_pic), not full parse — keep it fast.
  fctx->flags |= AVFMT_FLAG_NOPARSE;

  bool ok = false;
  if (avformat_find_stream_info(fctx, nullptr) >= 0)
  {
    GetEmbeddedCover(fctx, tag, art);
    ok = true;
  }

  avformat_close_input(&fctx);
  av_free(ioctx->buffer);
  av_free(ioctx);
  return ok;
}
