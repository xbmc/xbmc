/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderFFmpeg.h"

#include "MusicInfoTag.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
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

  if (file.IoControl(IOControl::SEEK_POSSIBLE, NULL) != 1)
    ioctx->seekable = 0;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(ioctx, &iformat, strFileName.c_str(), NULL, 0, 0);

  if (avformat_open_input(&fctx, strFileName.c_str(), iformat, NULL) < 0)
  {
    if (fctx)
      avformat_close_input(&fctx);
    av_free(ioctx->buffer);
    av_free(ioctx);
    return false;
  }

  // Embedded cover art, e.g. Matroska (.mka) attachments following the Matroska
  // Cover Art Guidelines (single "front cover" image named cover.jpg/cover.png)
  for (unsigned int i = 0; i < fctx->nb_streams; ++i)
  {
    const AVStream* stream = fctx->streams[i];
    if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
      continue;

    const AVDictionaryEntry* filenameTag =
        av_dict_get(stream->metadata, "filename", nullptr, AV_DICT_IGNORE_SUFFIX);
    const AVDictionaryEntry* mimeTag =
        av_dict_get(stream->metadata, "mimetype", nullptr, AV_DICT_IGNORE_SUFFIX);

    if (filenameTag && mimeTag &&
        (StringUtils::EqualsNoCase(filenameTag->value, "cover.jpg") ||
         StringUtils::EqualsNoCase(filenameTag->value, "cover.png")))
    {
      // Validate attached picture data before publishing cover art metadata.
      // Malformed/empty packets (null data or zero size) would cause undefined behavior
      // in EmbeddedArt::Set (which does m_data.assign(data, data+size)).
      if (stream->attached_pic.data != nullptr && stream->attached_pic.size > 0)
      {
        tag.SetCoverArtInfo(stream->attached_pic.size, mimeTag->value);
        if (art)
          art->Set(stream->attached_pic.data, stream->attached_pic.size, mimeTag->value);
      }
      // Per Matroska cover art guidelines, only one cover stream is expected,
      // so break even if this attachment was invalid (no point continuing to scan).
      break;
    }
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
                            if (StringUtils::CompareNoCase(avtag->key, "album") == 0)
                              tag.SetAlbum(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "artist") == 0)
                              tag.SetArtist(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "album_artist") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "album artist") == 0)
                              tag.SetAlbumArtist(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "title") == 0)
                              tag.SetTitle(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "genre") == 0)
                              tag.SetGenre(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "part_number") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "track") == 0)
                              tag.SetTrackNumber(
                                  static_cast<int>(strtol(avtag->value, nullptr, 10)));
                            else if (StringUtils::CompareNoCase(avtag->key, "disc") == 0)
                              tag.SetDiscNumber(
                                  static_cast<int>(strtol(avtag->value, nullptr, 10)));
                            else if (StringUtils::CompareNoCase(avtag->key, "date") == 0)
                              tag.SetReleaseDate(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "compilation") == 0)
                              tag.SetCompilation((strtol(avtag->value, nullptr, 10) == 0) ? false : true);
                            else if (StringUtils::CompareNoCase(avtag->key, "encoded_by") == 0) {}
                            else if (StringUtils::CompareNoCase(avtag->key, "composer") == 0)
                              tag.AddArtistRole("Composer", avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "performer") == 0) // Conductor or TPE3 tag
                              tag.AddArtistRole("Conductor", avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "TEXT") == 0)
                              tag.AddArtistRole("Lyricist", avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "TPE4") == 0)
                              tag.AddArtistRole("Remixer", avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "LABEL") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "TPUB") == 0)
                              tag.SetRecordLabel(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "copyright") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "TCOP") == 0) {} // Copyright message
                            else if (StringUtils::CompareNoCase(avtag->key, "TDRC") == 0)
                              tag.SetReleaseDate(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "TDOR") == 0  ||
                                     StringUtils::CompareNoCase(avtag->key, "TORY") == 0)
                              tag.SetOriginalDate(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key , "TDAT") == 0)
                              tag.AddReleaseDate(avtag->value, true); // MMDD part
                            else if (StringUtils::CompareNoCase(avtag->key, "TYER") == 0)
                              tag.AddReleaseDate(avtag->value); // YYYY part
                            else if (StringUtils::CompareNoCase(avtag->key, "TBPM") == 0)
                              tag.SetBPM(static_cast<int>(strtol(avtag->value, nullptr, 10)));
                            else if (StringUtils::CompareNoCase(avtag->key, "TDTG") == 0) {} // Tagging time
                            else if (StringUtils::CompareNoCase(avtag->key, "language") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "TLAN") == 0) {} // Languages
                            else if (StringUtils::CompareNoCase(avtag->key, "mood") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "TMOO") == 0)
                              tag.SetMood(avtag->value);
                            else if (StringUtils::CompareNoCase(avtag->key, "artist-sort") == 0 ||
                                     StringUtils::CompareNoCase(avtag->key, "TSOP") == 0) {}
                            else if (StringUtils::CompareNoCase(avtag->key, "TSO2") == 0) {}  // Album artist sort
                            else if (StringUtils::CompareNoCase(avtag->key, "TSOC") == 0) {}  // composer sort
                            else if (StringUtils::CompareNoCase(avtag->key, "TSST") == 0)
                              tag.SetDiscSubtitle(avtag->value);
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
