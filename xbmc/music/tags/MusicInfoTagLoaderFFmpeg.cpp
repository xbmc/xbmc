/*
 *      Copyright (C) 2014 Arne Morten Kvarving
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

#include "MusicInfoTagLoaderFFmpeg.h"
#include "MusicInfoTag.h"
#include "filesystem/File.h"
#include "cores/FFmpeg.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

using namespace MUSIC_INFO;
using namespace XFILE;

static int vfs_file_read(void *h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static off_t vfs_file_seek(void *h, off_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if(whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

CMusicInfoTagLoaderFFmpeg::CMusicInfoTagLoaderFFmpeg(void)
{
}

CMusicInfoTagLoaderFFmpeg::~CMusicInfoTagLoaderFFmpeg()
{
}

bool CMusicInfoTagLoaderFFmpeg::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);

  CFile file;
  if (!file.Open(strFileName))
    return false;

  uint8_t* buffer = (uint8_t*)av_malloc(32768);
  AVIOContext* ioctx = avio_alloc_context(buffer, 32768, 0, &file, vfs_file_read, NULL, vfs_file_seek);

  AVFormatContext* fctx = avformat_alloc_context();
  fctx->pb = ioctx;

  if (file.IoControl(IOCTRL_SEEK_POSSIBLE, NULL) == 0)
    ioctx->seekable = 0;

  ioctx->max_packet_size = 32768;

  AVInputFormat* iformat=NULL;
  av_probe_input_buffer(ioctx, &iformat, strFileName.c_str(), NULL, 0, 0);

  bool contains = false;
  if (avformat_open_input(&fctx, strFileName.c_str(), iformat, NULL) < 0)
  {
    if (fctx)
      avformat_close_input(&fctx);
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }

  AVDictionaryEntry* avtag=NULL;
  while ((avtag = av_dict_get(fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (StringUtils::EqualsNoCase(URIUtils::GetExtension(strFileName), ".mka"))
    {
      if (strcasecmp(avtag->key,"title") == 0)
        tag.SetTitle(avtag->value);
      if (strcasecmp(avtag->key,"artist") == 0)
        tag.SetArtist(avtag->value);
      if (strcasecmp(avtag->key,"album") == 0)
        tag.SetAlbum(avtag->value);
      if (strcasecmp(avtag->key,"part_number") == 0 ||
          strcasecmp(avtag->key,"track") == 0)
        tag.SetTrackNumber(strtol(avtag->value, NULL, 10));
    }
  }

  if (!tag.GetTitle().empty())
    tag.SetLoaded(true);

  avformat_close_input(&fctx);
  av_free(ioctx->buffer);
  av_free(ioctx);

  return true;
}
