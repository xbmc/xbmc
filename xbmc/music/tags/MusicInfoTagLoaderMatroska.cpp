/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderMatroska.h"

#include "MatroskaTagMapping.h"
#include "MatroskaTagReader.h"
#include "MusicCodecInfoFFmpeg.h"
#include "MusicInfoTag.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/File.h"
#include "music/MusicEmbeddedCoverLoaderFFmpeg.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/EmbeddedArt.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/mem.h>
}

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <commons/ilog.h>

using namespace MUSIC_INFO;
using namespace XFILE;

namespace
{
constexpr size_t FFMPEG_BUFFER_SIZE = 32768;

int VfsRead(void* h, uint8_t* buf, int size)
{
  return static_cast<CFile*>(h)->Read(buf, size);
}

int64_t VfsSeek(void* h, int64_t pos, int whence)
{
  auto* file = static_cast<CFile*>(h);
  return whence == AVSEEK_SIZE ? file->GetLength() : file->Seek(pos, whence & ~AVSEEK_FORCE);
}

//! Closes the demuxer context and its IO with the scope, however Load() returns.
struct CloseContext
{
  AVFormatContext* fctx;
  AVIOContext* ioctx;
  ~CloseContext()
  {
    if (fctx)
      avformat_close_input(&fctx);
    if (ioctx)
    {
      av_free(ioctx->buffer);
      av_free(ioctx);
    }
  }
};
} // unnamed namespace

/*!
* Used by Matroska files with no chapters (most common) or with a single (one song)
* Typically these are Matroska files split by Chapter start times with each chapter having
* song tags but no chapter name or chapter tags.
*/
bool CMusicInfoTagLoaderMatroska::Load(const std::string& strFileName,
                                       CMusicInfoTag& tag,
                                       EmbeddedArt* art)
{
  tag.SetLoaded(false);

  std::vector<std::string> separators{";", " feat. ", " ft. ", " Feat. ", " Ft. ", ":",
                                      "|", "#",       "/",     " with ",  "&"};
  const std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep);

  /*!
  * One context for the three things that need one: the tag reader when FFmpeg is the reader this
  * build has, the cover art, and the codec details. Each of them would otherwise open the file
  * again, which over SMB or NFS is what the whole Matroska read is trying to keep down.
  */
  CFile file;
  if (!file.Open(strFileName))
    return false;

  uint8_t* buffer = static_cast<uint8_t*>(av_malloc(FFMPEG_BUFFER_SIZE));
  if (!buffer)
    return false;

  AVIOContext* ioctx =
      avio_alloc_context(buffer, FFMPEG_BUFFER_SIZE, 0, &file, VfsRead, nullptr, VfsSeek);
  if (!ioctx)
  {
    av_free(buffer);
    return false;
  }

  AVFormatContext* fctx = avformat_alloc_context();
  if (!fctx)
  {
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }
  fctx->pb = ioctx;
  fctx->flags |= AVFMT_FLAG_CUSTOM_IO;

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
  fctx->flags |= AVFMT_FLAG_NOPARSE;
  avformat_find_stream_info(fctx, nullptr);

  const CloseContext closer{fctx, ioctx};

  const MatroskaAlbum album = ReadMatroskaTags(CURL(strFileName), fctx);
  if (!album.hasAlbumTags())
    return true;

  for (const auto& t : album.fileTags)
    MatroskaTagMapping::MapTag(t.first, t.second, separators, musicsep, tag);

  /*!
  * A file with more than one chapter is an album, and CAudioBookFileDirectory expands it into one
  * song per chapter before this loader is ever reached. What is left here carries no chapter or a
  * single one, whose tags describe this very song.
  */
  if (!album.chapters.empty())
    for (const auto& t : album.chapters[0].tags)
      MatroskaTagMapping::MapTag(t.first, t.second, separators, musicsep, tag);

  // Look for any embedded cover art
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(fctx, tag, art);

  // Get Codec data using FFmpeg (taglib not accurate for all codecs yet - v2.3)
  musicCodecInfo codec_info;
  if (CMusicCodecInfoFFmpeg::GetMusicCodecInfo(fctx, codec_info))
  {
    tag.SetBitRate(codec_info.bitRate);
    tag.SetSampleRate(codec_info.sampleRate);
    tag.SetNoOfChannels(codec_info.channels);
    tag.SetDuration(codec_info.duration);
  }

  if (!tag.GetAlbum().empty() || !tag.GetTitle().empty())
    tag.SetLoaded(true);

  return true;
}
