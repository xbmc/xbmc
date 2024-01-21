/*
 *  Copyright (C) 2014 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioBookFileDirectory.h"

#include "FileItem.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "Util.h"
#include "cores/FFmpeg.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

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
  bool isAudioBook = url.IsFileType("m4b");
  CMusicInfoTag albumtag;// Some tags are relevant to an album - these are read first

  AVDictionaryEntry* tag=nullptr;
  const std::string assep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  std::vector<std::string> separators{";", "/", ",", "&", " and "};

  if (assep.find_first_of(";/,&and") == std::string::npos)
    separators.push_back(assep); // add custom music separator from as.xml
  while ((tag = av_dict_get(m_fctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
  {
    if(isAudioBook)
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
      if (key == "TITLE")
        albumtag.SetAlbum(tag->value);
      else if (key == "ALBUM")
        albumtag.SetAlbum(tag->value);
      else if (key == "ARTIST")
        albumtag.SetArtist(tag->value);
      else if (key == "MUSICBRAINZ_ARTISTID")
        albumtag.SetMusicBrainzArtistID(StringUtils::Split(
            tag->value,
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
      else if (key == "MUSICBRAINZ_ALBUMARTISTID")
        albumtag.SetMusicBrainzAlbumArtistID(StringUtils::Split(
            tag->value,
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
      else if (key == "MUSICBRAINZ_ALBUMARTIST")
        albumtag.SetAlbumArtist(tag->value);
      else if (key == "MUSICBRAINZ_ALBUMID")
        albumtag.SetMusicBrainzAlbumID(tag->value);
      else if (key == "MUSICBRAINZ_RELEASEGROUPID")
        albumtag.SetMusicBrainzReleaseGroupID(tag->value);
      else if (key == "PUBLISHER")
        albumtag.SetRecordLabel(tag->value);
    }
  }

  std::string thumb;
  if (m_fctx->nb_chapters > 1)
  {
    thumb = CTextureUtils::GetWrappedImageURL(url.Get(), "music");
    if (isAudioBook)
      albumtag.SetAlbumReleaseType(CAlbum::ReleaseType::Audiobook);
    else
      albumtag.SetAlbumReleaseType(CAlbum::ReleaseType::Album);
  }
  CMusicInfoTag tracktag;// some tags are only relevant to a track or chapter

  for (size_t i=0;i<m_fctx->nb_chapters;++i)
  {
    tag=nullptr;
    std::string chaptitle = StringUtils::Format(g_localizeStrings.Get(25010), i + 1);
    std::string chapauthor;
    std::string chapalbum;
    std::string role;
    std::vector<std::string> instruments;
    std::string lyricist;
    std::string composer;
    std::string conductor;
    while ((tag = av_dict_get(m_fctx->chapters[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    { // read and store the tags we are interested in

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
        std::string data = tag->value;
        if (key == "TITLE")
          tracktag.SetTitle(data);
        else if (key == "ARTIST")
          tracktag.SetArtist(data);
        else if (key == "MUSICBRAINZ_TRACKID")
          tracktag.SetMusicBrainzTrackID(data);
        else if (key == "COMPOSER")
          composer = data;
        else if (key == "LYRICIST")
          lyricist = data;
        else if (key == "COMPOSER")
          composer == data;
        else if (key == "INSTRUMENTS")
          instruments =
              StringUtils::Split(data, ","); //comma separated list of instrument, performer
      }
    }
    CFileItemPtr item(new CFileItem(url.Get(), false));
    if (isAudioBook)
    {
      item->GetMusicInfoTag()->SetTitle(chaptitle);
      if (album.empty())
      item->GetMusicInfoTag()->SetAlbum(title);
      else if (chapalbum.empty())
        item->GetMusicInfoTag()->SetAlbum(album);
      else
        item->GetMusicInfoTag()->SetAlbum(chapalbum);
      if (chapauthor.empty())
        item->GetMusicInfoTag()->SetArtist(author);
      else
        item->GetMusicInfoTag()->SetArtist(chapauthor);
      if (!desc.empty())
        item->GetMusicInfoTag()->SetComment(desc);
      item->GetMusicInfoTag()->SetAlbumReleaseType(CAlbum::ReleaseType::Audiobook);
    }
    else
    {// add the tags we previously found to the MusicInfoTag() for this track
      *item->GetMusicInfoTag() = albumtag;
      if (!tracktag.GetTitle().empty())
        item->GetMusicInfoTag()->SetTitle(tracktag.GetTitle());
      if (!tracktag.GetArtist().empty())
        item->GetMusicInfoTag()->SetArtist(tracktag.GetArtist());
      else
        item->GetMusicInfoTag()->SetArtist(albumtag.GetArtist());
      if (!tracktag.GetMusicBrainzTrackID().empty())
        item->GetMusicInfoTag()->SetMusicBrainzTrackID(tracktag.GetMusicBrainzTrackID());
      if (!instruments.empty())
      {
        for (size_t i = 0; i + 1 < instruments.size(); i += 2)
        {
          std::vector<std::string> roles;
          roles = StringUtils::Split(instruments[i], separators);
          for (auto role : roles)
          {
            StringUtils::Trim(role);
            StringUtils::ToCapitalize(role);
            item->GetMusicInfoTag()->AddArtistRole(role,
                                                   StringUtils::Split(instruments[i + 1], ","));
          }
        }
      }
      if (!lyricist.empty())
        item->GetMusicInfoTag()->AddArtistRole("Lyricist",
                                               StringUtils::Split(lyricist, separators));
      if (!composer.empty())
        item->GetMusicInfoTag()->AddArtistRole("Composer",
                                               StringUtils::Split(composer, separators));
      if (!conductor.empty())
        item->GetMusicInfoTag()->AddArtistRole("Conductor",
                                               StringUtils::Split(conductor, separators));
    }
    if (isAudioBook)
      item->GetMusicInfoTag()->SetAlbumReleaseType(CAlbum::ReleaseType::Audiobook);
    else
      item->GetMusicInfoTag()->SetAlbumReleaseType(CAlbum::ReleaseType::Album);
    // do stuff common to both albums and audiobooks
    item->GetMusicInfoTag()->SetTrackNumber(i + 1);
    item->GetMusicInfoTag()->SetLoaded(true);
    item->SetLabel(StringUtils::Format("{0:02}. {1} - {2}", i + 1,
                                       item->GetMusicInfoTag()->GetAlbum(),
                                       item->GetMusicInfoTag()->GetTitle()));
    item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(m_fctx->chapters[i]->start *
                                                       av_q2d(m_fctx->chapters[i]->time_base)));
    item->SetEndOffset(m_fctx->chapters[i]->end * av_q2d(m_fctx->chapters[i]->time_base));
    int compare = m_fctx->streams[0]->duration * av_q2d(m_fctx->streams[0]->time_base);
    if (item->GetEndOffset() < 0 || item->GetEndOffset() > compare)
    {
      if (i < m_fctx->nb_chapters - 1)
        item->SetEndOffset(m_fctx->chapters[i + 1]->start *
                           av_q2d(m_fctx->chapters[i + 1]->time_base));
      else
        item->SetEndOffset(compare);
    }
    item->SetEndOffset(CUtil::ConvertSecsToMilliSecs(item->GetEndOffset()));
    item->GetMusicInfoTag()->SetDuration(
        CUtil::ConvertMilliSecsToSecsInt(item->GetEndOffset() - item->GetStartOffset()));
    item->SetProperty("item_start", item->GetStartOffset());
    if (!thumb.empty())
      item->SetArt("thumb", thumb);
    items.Add(item);
    tracktag.Clear();
  }

  return true;
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

  if (file.IoControl(IOCTRL_SEEK_POSSIBLE, nullptr) == 0)
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
