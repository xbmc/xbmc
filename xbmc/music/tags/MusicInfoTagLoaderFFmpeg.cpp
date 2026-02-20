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
  std::vector<std::string> separators{" feat. ", " ft. ", " Feat. ", " Ft. ",  ";", ":",
                                      "|",       "#",     "/",       " with ", "&"};
  const std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep);
  std::vector<std::string> tagdata;
  std::string value;

  auto&& ParseTag = [&](AVDictionaryEntry* avtag)
  {
    std::string key = StringUtils::ToUpper(avtag->key);
    std::string value = avtag->value;
    if (key == "ALBUM")
      tag.SetAlbum(value);
    else if (key == "ARTIST")
      tag.SetArtist(value);
    else if (key == "ALBUM_ARTIST" || key == "ALBUM ARTIST")
      tag.SetAlbumArtist(value);
    else if (key == "TITLE")
      tag.SetTitle(value);
    else if (key == "GENRE")
      tag.SetGenre(value);
    else if (key == "PART_NUMBER" || key == "TRACK")
      tag.SetTrackNumber(std::stoi(value));
    else if (key == "DISC")
      tag.SetDiscNumber(std::stoi(value));
    else if (key == "DATE")
      tag.SetReleaseDate(value);
    else if (key == "COMPILATION" || key == "TCMP")
      tag.SetCompilation((std::stoi(value)) ? false : true);
    else if (key == "ENCODED_BY")
    {
    }
    else if (key == "COMPOSER")
      tag.AddArtistRole("Composer", value);
    else if (key == "PERFORMER") // Conductor or TPE3 tag
      tag.AddArtistRole("Conductor", value);
    else if (key == "TEXT")
      tag.AddArtistRole("Lyricist", value);
    else if (key == "TPE4")
      tag.AddArtistRole("Remixer", value);
    else if (key == "LABEL" || key == "TPUB")
      tag.SetRecordLabel(value);
    else if (key == "COPYRIGHT" || key == "TCOP")
    {
    } // Copyright message
    else if (key == "TDRC")
      tag.SetReleaseDate(value);
    else if (key == "TDOR" || key == "TORY" || key == "DATE_RECORDED")
      tag.SetOriginalDate(value);
    else if (key == "TDAT")
      tag.AddReleaseDate(value, true); // MMDD part
    else if (key == "TYER")
      tag.AddReleaseDate(value); // YYYY part
    else if (key == "TBPM")
      tag.SetBPM(std::stoi(value));
    else if (key == "TDTG")
    {
    } // Tagging time
    else if (key == "LANGUAGE" || key == "TLAN")
    {
    } // Languages
    else if (key == "MOOD" || key == "TMOO")
      tag.SetMood(value);
    else if (key == "ARTIST-SORT" || key == "TSOP" || key == "ARTISTSORT" || key == "ARTIST SORT")
      tag.SetArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    else if (key == "TSO2" || key == "ALBUMARTISTSORT" || key == "ALBUM ARTIST SORT")
      tag.SetAlbumArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    else if (key == "TSOC" || key == "COMPOSERSORT" || key == "COMPOSER SORT")
      tag.SetComposerSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    else if (key == "TSST" || key == "DISCSUBTITLE")
      tag.SetDiscSubtitle(value);
    // the above values are all id3v2.3/4 frames, we could also have text frames
    else if (key == "MUSICBRAINZ ARTIST ID" || key == "MUSICBRAINZ_ARTISTID")
      tag.SetMusicBrainzArtistID(StringUtils::Split(value, separators));
    else if (key == "MUSICBRAINZ ALBUM ID" || key == "MUSICBRAINZ_ALBUMID")
      tag.SetMusicBrainzAlbumID(value);
    else if (key == "MUSICBRAINZ RELEASEGROUP ID" || key == "MUSICBRAINZ_RELEASEGROUPID")
      tag.SetMusicBrainzReleaseGroupID(value);
    else if (key == "MUSICBRAINZ ALBUM ARTIST ID" || key == "MUSICBRAINZ_ALBUMARTISTID")
      tag.SetMusicBrainzAlbumArtistID(StringUtils::Split(value, separators));
    else if (key == "MUSICBRAINZ ALBUM ARTIST" || key == "MUSICBRAINZ_ALBUMARTIST")
      tag.SetAlbumArtist(value);
    else if (key == "MUSICBRAINZ ALBUM TYPE")
      tag.SetMusicBrainzReleaseType(value);
    else if (key == "MUSICBRAINZ ALBUM STATUS")
      tag.SetAlbumReleaseStatus(value);
    else if (key == "ALBUM ARTIST" || key == "ALBUMARTIST")
      tag.SetAlbumArtist(value);
    else if (key == "ALBUM ARTIST SORT" || key == "ALBUMARTISTSORT")
      tag.SetAlbumArtistSort(value);
    else if (key == "ARTISTS")
      tag.SetMusicBrainzArtistHints(StringUtils::Split(value, separators));
    else if (key == "ALBUMARTISTS" || key == "ALBUM ARTISTS")
      tag.SetMusicBrainzAlbumArtistHints(StringUtils::Split(value, separators));
    else if (key == "WRITER")
      tag.AddArtistRole("Writer", StringUtils::Split(value, separators));
    else if (key == "PERFORMER")
    {
      tagdata = StringUtils::Split(avtag->key, separators);
      AddRole(tagdata, separators, tag);
    }
    else if (key == "ARRANGER")
    {
      tagdata = StringUtils::Split(avtag->key, separators);
      AddRole(tagdata, separators, tag);
    }
    else if (key == "REMIXED_BY")
      tag.AddArtistRole("Remixer", value);
    else if (key == "LYRICIST")
      tag.AddArtistRole("Lyricist", StringUtils::Split(value, separators));
    else if (key == "COMPOSER")
      tag.AddArtistRole("Composer", StringUtils::Split(value, separators));
    else if (key == "CONDUCTOR")
      tag.AddArtistRole("Conductor", StringUtils::Split(value, separators));
    else if (key == "ENGINEER")
      tag.AddArtistRole("Engineer", StringUtils::Split(value, separators));
  };
  
  AVDictionaryEntry* avtag=nullptr;
  while ((avtag = av_dict_get(fctx->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
    ParseTag(avtag);

  const AVStream* st = fctx->streams[0];
  if (st)
    while ((avtag = av_dict_get(st->metadata, "", avtag, AV_DICT_IGNORE_SUFFIX)))
      ParseTag(avtag);

  // Look for any embedded cover art
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(fctx, tag, art);
 
  tag.SetDuration(fctx->duration * av_q2d(av_get_time_base_q()));

  if (!tag.GetTitle().empty())
    tag.SetLoaded(true);

  avformat_close_input(&fctx);
  av_free(ioctx->buffer);
  av_free(ioctx);

  return true;
}

void CMusicInfoTagLoaderFFmpeg::AddRole(const std::vector<std::string>& data,
                                        const std::vector<std::string>& separators,
                                        MUSIC_INFO::CMusicInfoTag& musictag)
{
  if (!data.empty())
  {
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
      std::vector<std::string> roles = StringUtils::Split(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], separators));
      }
    }
  }
}
