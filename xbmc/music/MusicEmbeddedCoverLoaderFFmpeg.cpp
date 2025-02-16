/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicEmbeddedCoverLoaderFFmpeg.h"

#include "cores/FFmpeg.h"
#include "tags/MusicInfoTag.h"
#include "utils/EmbeddedArt.h"

using namespace MUSIC_INFO;

void CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(AVFormatContext* fctx,
                                                       CMusicInfoTag& tag,
                                                       EmbeddedArt* art /* nullptr */)
{
  for (size_t i = 0; i < fctx->nb_streams; ++i)
  {
    const AVStream* fctx_pic = fctx->streams[i];
    if ((fctx_pic->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;

    AVCodecID pic_id = fctx_pic->codecpar->codec_id;
    const std::unordered_map<AVCodecID, std::string> mime_map = {{AV_CODEC_ID_MJPEG, "image/jpeg"},
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
