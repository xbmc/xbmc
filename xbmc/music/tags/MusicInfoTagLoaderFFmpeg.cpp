/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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

static int64_t vfs_file_seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if (whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

CMusicInfoTagLoaderFFmpeg::CMusicInfoTagLoaderFFmpeg(void) = default;

CMusicInfoTagLoaderFFmpeg::~CMusicInfoTagLoaderFFmpeg() = default;

bool CMusicInfoTagLoaderFFmpeg::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);

  CFile file;
  if (!file.Open(strFileName))
    return false;

  int bufferSize = 4096;
  int blockSize = file.GetChunkSize();
  if (blockSize > 1)
    bufferSize = blockSize;
  uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
  AVIOContext* ioctx = avio_alloc_context(buffer, bufferSize, 0,
                                          &file, vfs_file_read, NULL,
                                          vfs_file_seek);

  AVFormatContext* fctx = avformat_alloc_context();
  fctx->pb = ioctx;

  if (file.IoControl(IOCTRL_SEEK_POSSIBLE, NULL) != 1)
    ioctx->seekable = 0;

  AVInputFormat* iformat=NULL;
  av_probe_input_buffer(ioctx, &iformat, strFileName.c_str(), NULL, 0, 0);

  if (avformat_open_input(&fctx, strFileName.c_str(), iformat, NULL) < 0)
  {
    if (fctx)
      avformat_close_input(&fctx);
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }

  /* ffmpeg supports the return of ID3v2 metadata but has its own naming system 
     for some, but not all, of the keys. In particular the key for the conductor 
     tag TPE3 is called "performer". 
     See https://github.com/xbmc/FFmpeg/blob/master/libavformat/id3v2.c#L43
     Other keys are retuened using their 4 char name.
     Only single frame values are returned even for v2.4 fomart tags e.g. while 
     tagged with multiple TPE1 frames "artist1", "artist2", "artist3" only 
     "artist1" is returned by ffmpeg.
     Hence, like with v2.3 format tags, multiple values for artist, genre etc. 
     need to be combined when tagging into a single value using a known item 
     separator e.g. "artist1 / artist2 / artist3"

     Any changes to ID3v2 tag processing in CTagLoaderTagLib need to be 
     repeated here
  */
  auto&& ParseTag = [&tag](AVDictionaryEntry* avtag)
                          { 
                            if (strcasecmp(avtag->key, "album") == 0)
                              tag.SetAlbum(avtag->value);
                            else if (strcasecmp(avtag->key, "artist") == 0)
                              tag.SetArtist(avtag->value);
                            else if (strcasecmp(avtag->key, "album_artist") == 0 || 
                                     strcasecmp(avtag->key, "album artist") == 0)
                              tag.SetAlbumArtist(avtag->value);
                            else if (strcasecmp(avtag->key, "title") == 0)
                              tag.SetTitle(avtag->value);
                            else if (strcasecmp(avtag->key, "genre") == 0)
                              tag.SetGenre(avtag->value);
                            else if (strcasecmp(avtag->key, "part_number") == 0 || 
                                     strcasecmp(avtag->key, "track") == 0)
                              tag.SetTrackNumber(strtol(avtag->value, nullptr, 10));
                            else if (strcasecmp(avtag->key, "disc") == 0)
                              tag.SetDiscNumber(strtol(avtag->value, nullptr, 10));
                            else if (strcasecmp(avtag->key, "date") == 0)
                              tag.SetYear(strtol(avtag->value, nullptr, 10));
                            else if (strcasecmp(avtag->key, "compilation") == 0)
                              tag.SetCompilation((strtol(avtag->value, nullptr, 10) == 0) ? false : true);
                            else if (strcasecmp(avtag->key, "encoded_by") == 0) {}
                            else if (strcasecmp(avtag->key, "composer") == 0)
                              tag.AddArtistRole("Composer", avtag->value);
                            else if (strcasecmp(avtag->key, "performer") == 0) // Conductor or TPE3 tag
                              tag.AddArtistRole("Conductor", avtag->value);
                            else if (strcasecmp(avtag->key, "TEXT") == 0)
                              tag.AddArtistRole("Lyricist", avtag->value);
                            else if (strcasecmp(avtag->key, "TPE4") == 0)
                              tag.AddArtistRole("Remixer", avtag->value);
                            else if (strcasecmp(avtag->key, "LABEL") == 0 || 
                                     strcasecmp(avtag->key, "TPUB") == 0)
                              tag.SetRecordLabel(avtag->value);
                            else if (strcasecmp(avtag->key, "copyright") == 0 || 
                                     strcasecmp(avtag->key, "TCOP") == 0) {} // Copyright message
                            else if (strcasecmp(avtag->key, "TDRC") == 0)
                              tag.SetYear(strtol(avtag->value, nullptr, 10));
                            else if (strcasecmp(avtag->key, "TDRL") == 0)
                              tag.SetYear(strtol(avtag->value, nullptr, 10));
                            else if (strcasecmp(avtag->key, "TDTG") == 0) {} // Tagging time
                            else if (strcasecmp(avtag->key, "language") == 0 || 
                                     strcasecmp(avtag->key, "TLAN") == 0) {} // Languages
                            else if (strcasecmp(avtag->key, "mood") == 0 || 
                                     strcasecmp(avtag->key, "TMOO") == 0)
                              tag.SetMood(avtag->value);
                            else if (strcasecmp(avtag->key, "artist-sort") == 0 || 
                                     strcasecmp(avtag->key, "TSOP") == 0) {}
                            else if (strcasecmp(avtag->key, "TSO2") == 0) {}  // Album artist sort
                            else if (strcasecmp(avtag->key, "TSOC") == 0) {}  // composer sort
                          };

  AVDictionaryEntry* avtag=nullptr;
  while ((avtag = av_dict_get(fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
    ParseTag(avtag);

  const AVStream* st = fctx->streams[0];
  if (st)
    while ((avtag = av_dict_get(st->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
      ParseTag(avtag);

  if (!tag.GetTitle().empty())
    tag.SetLoaded(true);

  avformat_close_input(&fctx);
  av_free(ioctx->buffer);
  av_free(ioctx);

  return true;
}
