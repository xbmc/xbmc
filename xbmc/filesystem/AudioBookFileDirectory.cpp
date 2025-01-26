/*
 *  Copyright (C) 2014 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioBookFileDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "Util.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "imagefiles/ImageFileURL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

using namespace XFILE;
using namespace MUSIC_INFO;

static int cfile_file_read(void *h, uint8_t* buf, int size)
{
  CFile* pFile = static_cast<CFile*>(h);
  return pFile->Read(buf, size);
}

static int64_t cfile_file_seek(void *h, int64_t pos, int whence)
{
  CFile* pFile = static_cast<CFile*>(h);
  if(whence == AVSEEK_SIZE)
    return pFile->GetLength();
  else
    return pFile->Seek(pos, whence & ~AVSEEK_FORCE);
}

CAudioBookFileDirectory::~CAudioBookFileDirectory(void)
{
  if (m_fctx)
    avformat_close_input(&m_fctx);
  if (m_ioctx)
  {
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
  }
}

bool CAudioBookFileDirectory::GetDirectory(const CURL& url,
                                           CFileItemList &items)
{
  if (!m_fctx && !ContainsFiles(url))
    return true;

  std::string title;
  std::string author;
  std::string album;
  std::string desc;
  std::vector<std::string> artistsort;
  std::vector<std::string> tagdata;
  std::vector<std::string> separators{" feat. ", " ft. ", " Feat. ", " Ft. ",  ";", ":",
                                      "|",       "#",     "/",       " with ", "&"};
  const std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep); // add custom music separator from as.xml

  const int end_time_m4b_file =
      m_fctx->streams[0]->duration * av_q2d(m_fctx->streams[0]->time_base);
  const int end_time_mka_file = m_fctx->duration * av_q2d(av_get_time_base_q());
  const bool isAudioBook = url.IsFileType("m4b");
  // Some tags are relevant to the whole album - these are read first
  CMusicInfoTag albumtag;

  AVDictionaryEntry* tag=nullptr;
  while ((tag = av_dict_get(m_fctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
  {
    if (isAudioBook)
    {
      if (StringUtils::CompareNoCase(tag->key, "title") == 0)
        title = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
        album = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
        author = tag->value;
      else if (StringUtils::CompareNoCase(tag->key, "description") == 0)
        desc = tag->value;
    }
    else
    {
      std::string key = StringUtils::ToUpper(tag->key);
      // track is matroska's discnumber when at level 50 (these tags) as set by mp3tag
      // part_number is the matroska spec key
      if (key == "TRACK" || key == "PART_NUMBER")
        albumtag.SetDiscNumber(std::stoi(tag->value));
      else if (key == "SUBTITLE" || key == "SETSUBTITLE")
        albumtag.SetDiscSubtitle(tag->value);
      else if (key == "TITLE")
        albumtag.SetAlbum(tag->value);
      else if (key == "ALBUM")
        albumtag.SetAlbum(tag->value);
      else if (key == "ARTIST")
        albumtag.SetArtist(tag->value);
      else if (key == "ARTISTSORT" || key == "ARTIST SORT")
        albumtag.SetArtistSort(
            StringUtils::Join(StringUtils::Split(tag->value, separators), musicsep));
      else if (key == "ALBUMARTIST" || key == "ALBUM ARTIST")
        albumtag.SetAlbumArtist(
            StringUtils::Join(StringUtils::Split(tag->value, separators), musicsep));
      else if (key == "ALBUMARTSTS" || key == "ALBUM ARTISTS")
        albumtag.SetAlbumArtist(StringUtils::Split(tag->value, separators));
      else if (key == "ALBUMARTISTSORT" || key == "ALBUM ARTIST SORT")
        albumtag.SetAlbumArtistSort(
            StringUtils::Join(StringUtils::Split(tag->value, separators), musicsep));
      else if (key == "MUSICBRAINZ_ARTISTID")
        albumtag.SetMusicBrainzArtistID(StringUtils::Split(tag->value, separators));
      else if (key == "MUSICBRAINZ_ALBUMARTISTID")
        albumtag.SetMusicBrainzAlbumArtistID(StringUtils::Split(tag->value, separators));
      else if (key == "MUSICBRAINZ_ALBUMARTIST")
        albumtag.SetAlbumArtist(tag->value);
      else if (key == "MUSICBRAINZ_ALBUMID")
        albumtag.SetMusicBrainzAlbumID(tag->value);
      else if (key == "MUSICBRAINZ_RELEASEGROUPID")
        albumtag.SetMusicBrainzReleaseGroupID(tag->value);
      else if (key == "MUSICBRAINZ_ALBUMSTATUS")
        albumtag.SetAlbumReleaseStatus(tag->value);
      else if (key == "MUSICBRAINZ_ALBUMTYPE")
        albumtag.SetMusicBrainzReleaseType(tag->value);
      else if (key == "PUBLISHER")
        albumtag.SetRecordLabel(tag->value);
      // mp3tag info shows year but the value is stored in date_recorded
      // equates to TDRC in id3v2.4 ISO 8601 yyyy-mm-dd or part thereof
      else if (key == "YEAR" || key == "DATE_RECORDED")
        albumtag.SetReleaseDate(tag->value);
      else if (key == "ORIGYEAR") // ISO 8601 as above. Equates to TDOR in id3v2.4 (set by mp3tag)
        albumtag.SetOriginalDate(tag->value);
      else if (key == "MOOD")
        albumtag.SetMood(tag->value);
      // genre could be comma delimited or not. Temporarily add the comma just in case.  true trims
      // any whitespace around the genre(s)
      else if (key == "GENRE")
      {
        separators.push_back(",");
        albumtag.SetGenre(StringUtils::Split(tag->value, separators), true);
        separators.pop_back();
      }
      // comma separated list of role, person
      else if (key == "INVOLVEDPEOPLE")
      {
        tagdata = StringUtils::Split(tag->value, ",");
        AddCommaDelimitedString(tagdata, separators, albumtag);
      }
      else if (key == "SUBTITLE" || key == "SETSUBTITLE")
        albumtag.SetDiscSubtitle(tag->value);
      else if (key == "REMIXED_BY")
        albumtag.AddArtistRole("Remixer", tag->value);
      else if (key == "COMMENT")
        albumtag.SetComment(tag->value);
    }
  }

  std::string thumb;
  if (m_fctx->nb_chapters > 1)
    thumb = IMAGE_FILES::URLFromFile(url.Get(), "music");

  for (size_t i=0;i<m_fctx->nb_chapters;++i)
  {
    tag=nullptr;
    std::string chaptitle = StringUtils::Format(g_localizeStrings.Get(25010), i + 1);
    std::string chapauthor;
    std::string chapalbum;

    std::shared_ptr<CFileItem> item(new CFileItem(url.Get(), false));
    *item->GetMusicInfoTag() = albumtag;

    auto addRole = [&](const std::string& role, const std::string& value)
    {
      if (!value.empty())
        item->GetMusicInfoTag()->AddArtistRole(role, StringUtils::Split(value, separators));
    };

    while ((tag=av_dict_get(m_fctx->chapters[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
      if (isAudioBook)
      {
        if (StringUtils::CompareNoCase(tag->key, "title") == 0)
          chaptitle = tag->value;
        else if (StringUtils::CompareNoCase(tag->key, "artist") == 0)
          chapauthor = tag->value;
        else if (StringUtils::CompareNoCase(tag->key, "album") == 0)
          chapalbum = tag->value;
      }
      else
      {
        std::string key = StringUtils::ToUpper(tag->key);
        if (key == "TITLE")
          item->GetMusicInfoTag()->SetTitle(tag->value);
        else if (key == "ARTIST")
          item->GetMusicInfoTag()->SetArtist(tag->value);
        else if (key == "MUSICBRAINZ_TRACKID")
          item->GetMusicInfoTag()->SetMusicBrainzTrackID(tag->value);
        else if (key == "COMPOSER")
          addRole("Composer", tag->value);
        else if (key == "LYRICIST")
          addRole("Lyricist", tag->value);
        else if (key == "CONDUCTOR")
          addRole("Conductor", tag->value);
        else if (key == "WRITER")
          addRole("Writer", tag->value);
        else if (key == "ARRANGER")
          addRole("Arranger", tag->value);
        else if (key == "BAND")
          addRole("Band", tag->value);
        else if (key == "ENGINEER")
          addRole("Engineer", tag->value);
        else if (key == "PRODUCER")
          addRole("Producer", tag->value);
        else if (key == "REMIXED_BY")
          addRole("Remixer", tag->value);
        else if (key == "YEAR" || key == "DATE_RECORDED")
          item->GetMusicInfoTag()->SetReleaseDate(tag->value);
        else if (key == "ORIGYEAR")
          item->GetMusicInfoTag()->SetOriginalDate(tag->value);
        else if (key == "SUBTITLE" || key == "SETSUBTITLE")
          item->GetMusicInfoTag()->SetDiscSubtitle(tag->value);
        else if (key == "COMMENT")
          item->GetMusicInfoTag()->SetComment(tag->value);
        else if (key == "MOOD")
          item->GetMusicInfoTag()->SetMood(tag->value);
        else if (key == "GENRE")
        {
          separators.push_back(",");
          item->GetMusicInfoTag()->SetGenre(StringUtils::Split(tag->value, separators), true);
          separators.pop_back();
        }
        // comma separated list of instrument, person
        else if (key == "INSTRUMENTS")
        {
          tagdata = StringUtils::Split(tag->value, ",");
          AddCommaDelimitedString(tagdata, separators, *item->GetMusicInfoTag());
        }
        // comma separated list of role, person
        else if (key == "INVOLVEDPEOPLE")
        {
          tagdata = StringUtils::Split(tag->value, ",");
          AddCommaDelimitedString(tagdata, separators, *item->GetMusicInfoTag());
        }
      }
      /* The comma separated lists are outside the Matroska spec
         (see https://www.matroska.org/technical/tagging.html) as it states to use multiple simple
         tags for eg 2 or more composers.  However, ffmpeg returns just the last tag and drops the
         rest (https://trac.ffmpeg.org/ticket/9641).  Therefore until (if) it gets fixed, this is
         the best solution.
       */
    }
    if (isAudioBook)
    {
      item->GetMusicInfoTag()->SetTitle(chaptitle);
      item->GetMusicInfoTag()->SetAlbum(chapalbum.empty() ? album.empty() ? title : album
                                                          : chapalbum);
      item->GetMusicInfoTag()->SetArtist(chapauthor.empty() ? author : chapauthor);
      if (!desc.empty())
        item->GetMusicInfoTag()->SetComment(desc);
    }
    item->GetMusicInfoTag()->SetTrackNumber(i+1);
    item->GetMusicInfoTag()->SetLoaded(true);

    item->SetLabel(StringUtils::Format("{0:02}. {1} - {2}", i + 1,
                                       item->GetMusicInfoTag()->GetAlbum(),
                                       item->GetMusicInfoTag()->GetTitle()));
    item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(m_fctx->chapters[i]->start *
                                                       av_q2d(m_fctx->chapters[i]->time_base)));
    item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(m_fctx->chapters[i]->end *
                                                     av_q2d(m_fctx->chapters[i]->time_base)));
    if (item->GetEndOffset() < 0 ||
        item->GetEndOffset() > CUtil::ConvertMilliSecsToSecs(m_fctx->duration))
    {
      if (i < m_fctx->nb_chapters - 1)
        item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(
            m_fctx->chapters[i + 1]->start * av_q2d(m_fctx->chapters[i + 1]->time_base)));
      else
      {
        item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(end_time_mka_file)); // mka file
        if (item->GetEndOffset() < 0)
          item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(end_time_m4b_file)); // m4b file
      }
    }
    item->GetMusicInfoTag()->SetDuration(
        CUtil::ConvertMilliSecsToSecsInt(item->GetEndOffset() - item->GetStartOffset()));
    item->SetProperty("item_start", item->GetStartOffset());
    item->SetProperty("audio_bookmark", item->GetStartOffset());
    if (!thumb.empty())
      item->SetArt("thumb", thumb);
    items.Add(item);
  }

  return true;
}

void CAudioBookFileDirectory::AddCommaDelimitedString(const std::vector<std::string>& data,
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
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], ","));
      }
    }
  }
}

bool CAudioBookFileDirectory::Exists(const CURL& url)
{
  return CFile::Exists(url) && ContainsFiles(url);
}

bool CAudioBookFileDirectory::ContainsFiles(const CURL& url)
{
  CFile file;
  if (!file.Open(url))
    return false;

  uint8_t* buffer = (uint8_t*)av_malloc(32768);
  m_ioctx = avio_alloc_context(buffer, 32768, 0, &file, cfile_file_read,
                               nullptr, cfile_file_seek);

  m_fctx = avformat_alloc_context();
  m_fctx->pb = m_ioctx;

  if (file.IoControl(IOControl::SEEK_POSSIBLE, nullptr) == 0)
    m_ioctx->seekable = 0;

  m_ioctx->max_packet_size = 32768;

  const AVInputFormat* iformat = nullptr;
  av_probe_input_buffer(m_ioctx, &iformat, url.Get().c_str(), nullptr, 0, 0);

  bool contains = false;
  if (avformat_open_input(&m_fctx, url.Get().c_str(), iformat, nullptr) < 0)
  {
    if (m_fctx)
      avformat_close_input(&m_fctx);
    av_free(m_ioctx->buffer);
    av_free(m_ioctx);
    return false;
  }

  contains = m_fctx->nb_chapters > 1;

  return contains;
}
