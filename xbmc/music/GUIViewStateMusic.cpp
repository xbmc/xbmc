/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStateMusic.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/LibraryPaths.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "guilib/WindowIDs.h"
#include "playlists/PlayListFileItemClassify.h"
#include "playlists/PlayListTypes.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/ContentNames.h"
#include "utils/FileExtensionProvider.h"
#include "utils/SortUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "view/ViewStateNames.h"
#include "view/ViewStateSettings.h"

using namespace KODI;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;

PLAYLIST::Id CGUIViewStateWindowMusic::GetPlaylist() const
{
  return PLAYLIST::Id::TYPE_MUSIC;
}

bool CGUIViewStateWindowMusic::AutoPlayNextItem()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  return settings->GetBool(CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM) &&
         !settings->GetBool(CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT);
}

std::optional<KODI::MEDIA::MediaSection> CGUIViewStateWindowMusic::GetLockType()
{
  return KODI::MEDIA::MediaSection::MUSIC;
}

std::string CGUIViewStateWindowMusic::GetExtensions()
{
  return CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
}

std::vector<CMediaSource>& CGUIViewStateWindowMusic::GetSources()
{
  return CGUIViewState::GetSources();
}

CGUIViewStateMusicSearch::CGUIViewStateMusicSearch(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;

  AddSortMethod(
      SortBy::TITLE, sortAttribute, 556,
      LABEL_MASKS("%T - %A", "%D", "%L", "%A")); // Title - Artist, Duration | Label, Artist
  SetSortMethod(SortBy::TITLE);

  const CViewState* viewState = CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS);
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSearch::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV,
               CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS));
}

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CMusicDatabaseDirectory dir;
  NodeType nodeType = dir.GetDirectoryChildType(items.GetPath());

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_LIBRARYTRACKFORMAT);
  if (strTrack.empty())
    strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  std::string strAlbum = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_strMusicLibraryAlbumFormat;
  if (strAlbum.empty())
    strAlbum = "%B"; // album
  CLog::Log(LOGDEBUG, "Custom album format = [{}]", strAlbum);
  SortAttribute sortAttribute = SortAttributeNone;
  if (settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;
  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME))
    sortAttribute = static_cast<SortAttribute>(sortAttribute | SortAttributeUseArtistSortName);

  switch (nodeType)
  {
    case NodeType::OVERVIEW:
    {
      AddSortMethod(SortBy::NONE, 551,
                    LABEL_MASKS("%F", "", "%L", "")); // Filename, empty | Foldername, empty
      SetSortMethod(SortBy::NONE);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::TOP100:
    {
      AddSortMethod(SortBy::NONE, 551,
                    LABEL_MASKS("%F", "", "%L", "")); // Filename, empty | Foldername, empty
      SetSortMethod(SortBy::NONE);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::GENRE:
    {
      AddSortMethod(SortBy::GENRE, 515,
                    LABEL_MASKS("%F", "", "%G", "")); // Filename, empty | Genre, empty
      SetSortMethod(SortBy::GENRE);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrder::ASCENDING);
    }
    break;
    case NodeType::ROLE:
    {
      AddSortMethod(SortBy::NONE, 576,
                    LABEL_MASKS("%F", "", "%G", "")); // Filename, empty | Genre, empty
      SetSortMethod(SortBy::PLAYCOUNT);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::YEAR:
    {
      AddSortMethod(SortBy::LABEL, 562,
                    LABEL_MASKS("%F", "", "%Y", "")); // Filename, empty | Year, empty
      SetSortMethod(SortBy::LABEL);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrder::ASCENDING);
    }
    break;
    case NodeType::ARTIST:
    {
      AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                    LABEL_MASKS("%F", "", "%A", "")); // Filename, empty | Artist, empty
      AddSortMethod(SortBy::DATE_ADDED, sortAttribute, 570,
                    LABEL_MASKS("%F", "", "%A", "%a")); // Filename, empty | Artist, dateAdded
      SetSortMethod(SortBy::ARTIST);

      const CViewState* viewState =
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ARTISTS);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
    case NodeType::ALBUM:
    {
      // album
      AddSortMethod(SortBy::ALBUM, sortAttribute, 558,
                    LABEL_MASKS("%F", "", strAlbum,
                                "%A")); // Filename, empty | Userdefined (default=%B), Artist
      // artist
      AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                    LABEL_MASKS("%F", "", strAlbum, "%A")); // Filename, empty | Userdefined, Artist
      // artist / year
      AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                    LABEL_MASKS("%F", "", strAlbum,
                                "%A / %Y")); // Filename, empty | Userdefined, Artist / Year
      // discs
      AddSortMethod(
          SortBy::TOTAL_DISCS, sortAttribute, 38077,
          LABEL_MASKS("%F", "", strAlbum, "%b")); // Filename, empty | Userdefined, Total discs
      // year
      AddSortMethod(SortBy::YEAR, 562,
                    LABEL_MASKS("%F", "", strAlbum, "%Y")); // Filename, empty | Userdefined, Year
      // original release year
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        AddSortMethod(
            SortBy::ORIG_DATE, sortAttribute, 38079,
            LABEL_MASKS("%F", "", strAlbum, "%e")); // Filename, empty | Userdefined, Original date
      // album date added
      AddSortMethod(
          SortBy::DATE_ADDED, sortAttribute, 570,
          LABEL_MASKS("%F", "", strAlbum, "%a")); // Filename, empty | Userdefined, dateAdded
      // play count
      AddSortMethod(
          SortBy::PLAYCOUNT, 567,
          LABEL_MASKS("%F", "", strAlbum, "%V")); // Filename, empty | Userdefined, Play count
      // last played
      AddSortMethod(
          SortBy::LAST_PLAYED, 568,
          LABEL_MASKS("%F", "", strAlbum, "%p")); // Filename, empty | Userdefined, last played
      // rating
      AddSortMethod(SortBy::RATING, 563,
                    LABEL_MASKS("%F", "", strAlbum, "%R")); // Filename, empty | Userdefined, Rating
      // userrating
      AddSortMethod(
          SortBy::USER_RATING, 38018,
          LABEL_MASKS("%F", "", strAlbum, "%r")); // Filename, empty | Userdefined, UserRating

      const CViewState* viewState =
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS);
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
    case NodeType::ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(
          SortBy::NONE, 552,
          LABEL_MASKS("%F", "", strAlbum, "%a")); // Filename, empty | Userdefined, dateAdded
      SetSortMethod(SortBy::NONE);

      SetViewAsControl(
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS)->m_viewMode);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SortBy::NONE, 552,
                    LABEL_MASKS(strTrack, "%a")); // Userdefined, dateAdded | empty, empty
      SetSortMethod(SortBy::NONE);

      SetViewAsControl(
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS)->m_viewMode);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(
          SortBy::LAST_PLAYED, 568,
          LABEL_MASKS("%F", "", strAlbum, "%p")); // Filename, empty | Userdefined, last played

      SetViewAsControl(
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS)->m_viewMode);
    }
    break;
    case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SortBy::LAST_PLAYED, 568,
                    LABEL_MASKS(strTrack, "%p")); // Userdefined, last played | empty, empty

      SetViewAsControl(
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS)->m_viewMode);
    }
    break;
    case NodeType::ALBUM_TOP100:
    {
      AddSortMethod(
          SortBy::NONE, 551,
          LABEL_MASKS("%F", "", strAlbum, "%V")); // Filename, empty | Userdefined, Play count
      SetSortMethod(SortBy::NONE);

      SetViewAsControl(DEFAULT_VIEW_LIST);
      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::SINGLES:
    {
      AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                    LABEL_MASKS("%A - %T", "%D")); // Artist, Title, Duration| empty, empty
      AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                    LABEL_MASKS("%A - %T", "%Y")); // Artist, Title, Year| empty, empty
      AddSortMethod(SortBy::TITLE, sortAttribute, 556,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::LABEL, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
      AddSortMethod(SortBy::TIME, 180,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::RATING, 563, LABEL_MASKS("%T - %A", "%R")); // Title - Artist, Rating
      AddSortMethod(SortBy::USER_RATING, 38018,
                    LABEL_MASKS("%T - %A", "%r")); // Title - Artist, UserRating
      AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
      // original release date  (singles can be re-released)
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        AddSortMethod(SortBy::ORIG_DATE, 38079,
                      LABEL_MASKS("%T - %A", "%e")); // Title, Artist, Original Date
      AddSortMethod(SortBy::DATE_ADDED, 570,
                    LABEL_MASKS("%T - %A", "%a")); // Title - Artist, DateAdded | empty, empty
      AddSortMethod(SortBy::PLAYCOUNT, 567,
                    LABEL_MASKS("%T - %A", "%V")); // Title - Artist, PlayCount
      AddSortMethod(SortBy::LAST_PLAYED, 568,
                    LABEL_MASKS(strTrack, "%p")); // Userdefined, last played | empty, empty

      const CViewState* viewState =
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS);
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
    case NodeType::ALBUM_TOP100_SONGS:
    case NodeType::SONG:
    {
      AddSortMethod(SortBy::TRACK_NUMBER, 554,
                    LABEL_MASKS(strTrack, "%D")); // Userdefined, Duration| empty, empty
      AddSortMethod(SortBy::TITLE, sortAttribute, 556,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(
          SortBy::ALBUM, sortAttribute, 558,
          LABEL_MASKS("%B - %T - %A", "%D")); // Album, Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                    LABEL_MASKS("%A - %T", "%D")); // Artist, Title, Duration| empty, empty
      AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                    LABEL_MASKS("%A - %T", "%Y")); // Artist, Title, Year| empty, empty
      AddSortMethod(SortBy::LABEL, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
      AddSortMethod(SortBy::TIME, 180,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::RATING, 563, LABEL_MASKS("%T - %A", "%R")); // Title - Artist, Rating
      AddSortMethod(SortBy::USER_RATING, 38018,
                    LABEL_MASKS("%T - %A", "%r")); // Title - Artist, UserRating
      AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
      // original release date
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        AddSortMethod(SortBy::ORIG_DATE, 38079,
                      LABEL_MASKS("%T - %A", "%e")); // Title, Artist, Original Date
      AddSortMethod(SortBy::DATE_ADDED, 570,
                    LABEL_MASKS("%T - %A", "%a")); // Title - Artist, DateAdded | empty, empty
      AddSortMethod(SortBy::PLAYCOUNT, 567,
                    LABEL_MASKS("%T - %A", "%V")); // Title - Artist, PlayCount
      AddSortMethod(SortBy::LAST_PLAYED, 568,
                    LABEL_MASKS(strTrack, "%p")); // Userdefined, last played | empty, empty
      AddSortMethod(SortBy::BPM, 38080,
                    LABEL_MASKS(strTrack, "%f")); // Userdefined, bpm, empty,empty

      const CViewState* viewState =
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS);
      // the "All Albums" entries always default to SortBy::ALBUM as this is most logical - user can always
      // change it and the change will be saved for this particular path
      if (dir.IsAllItem(items.GetPath()))
        SetSortMethod(SortBy::ALBUM);
      else
        SetSortMethod(viewState->m_sortDescription);

      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
    case NodeType::SONG_TOP100:
    {
      AddSortMethod(SortBy::NONE, 576, LABEL_MASKS("%T - %A", "%V"));
      SetSortMethod(SortBy::PLAYCOUNT);

      SetViewAsControl(
          CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS)->m_viewMode);

      SetSortOrder(SortOrder::NONE);
    }
    break;
    case NodeType::DISC:
    {
      AddSortMethod(SortBy::NONE, 427, LABEL_MASKS("%L")); // Use the existing label
      SetSortMethod(SortBy::NONE);
    }
    break;
  default:
    break;
  }

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicDatabase::SaveViewState()
{
  CMusicDatabaseDirectory dir;
  NodeType nodeType = dir.GetDirectoryChildType(m_items.GetPath());

  switch (nodeType)
  {
    case NodeType::ARTIST:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV,
                   CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ARTISTS));
      break;
    case NodeType::ALBUM:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV,
                   CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS));
      break;
    case NodeType::SINGLES:
    case NodeType::SONG:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV,
                   CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS));
      break;
    default:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
      break;
  }
}

CGUIViewStateMusicSmartPlaylist::CGUIViewStateMusicSmartPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;
  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME))
    sortAttribute = static_cast<SortAttribute>(sortAttribute | SortAttributeUseArtistSortName);
  const CViewState* viewState = CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS);

  if (items.GetContent() == MEDIA::CONTENT::SONGS || items.GetContent() == MEDIA::CONTENT::MIXED)
  {
    std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
    AddSortMethod(SortBy::TRACK_NUMBER, 554,
                  LABEL_MASKS(strTrack, "%D")); // Userdefined, Duration| empty, empty
    AddSortMethod(SortBy::TITLE, sortAttribute, 556,
                  LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
    AddSortMethod(
        SortBy::ALBUM, sortAttribute, 558,
        LABEL_MASKS("%B - %T - %A", "%D")); // Album, Title, Artist, Duration| empty, empty
    AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                  LABEL_MASKS("%A - %T", "%D")); // Artist, Title, Duration| empty, empty
    AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                  LABEL_MASKS("%A - %T", "%Y")); // Artist, Title, Year| empty, empty
    AddSortMethod(SortBy::LABEL, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
    AddSortMethod(SortBy::TIME, 180,
                  LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
    AddSortMethod(SortBy::RATING, 563,
                  LABEL_MASKS("%T - %A", "%R")); // Title, Artist, Rating| empty, empty
    AddSortMethod(SortBy::USER_RATING, 38018,
                  LABEL_MASKS("%T - %A", "%r")); // Title - Artist, UserRating
    AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
    AddSortMethod(SortBy::DATE_ADDED, 570,
                  LABEL_MASKS("%T - %A", "%a")); // Title - Artist, DateAdded | empty, empty
    AddSortMethod(SortBy::PLAYCOUNT, 567,
                  LABEL_MASKS("%T - %A", "%V")); // Title - Artist, PlayCount
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      AddSortMethod(SortBy::ORIG_DATE, 38079,
                    LABEL_MASKS("%T - %A", "%e")); // Title - Artist, original date, empty, empty
    AddSortMethod(SortBy::BPM, 38080,
                  LABEL_MASKS("%T - %A", "%f")); // Title - Artist, bpm, empty, empty

    if (PLAYLIST::IsSmartPlayList(items) || items.IsLibraryFolder())
      AddPlaylistOrder(items, LABEL_MASKS(strTrack, "%D"));
    else
    {
      SetSortMethod(viewState->m_sortDescription);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }

    SetViewAsControl(
        CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS)->m_viewMode);
  }
  else if (items.GetContent() == MEDIA::CONTENT::ALBUMS)
  {
    std::string strAlbum = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_strMusicLibraryAlbumFormat;
    if (strAlbum.empty())
      strAlbum = "%B"; // album
    // album
    AddSortMethod(SortBy::ALBUM, sortAttribute, 558,
                  LABEL_MASKS("%F", "", strAlbum,
                              "%A")); // Filename, empty | Userdefined (default=%B), Artist
    // artist
    AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                  LABEL_MASKS("%F", "", strAlbum, "%A")); // Filename, empty | Userdefined, Artist
    // artist / year
    AddSortMethod(
        SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
        LABEL_MASKS("%F", "", strAlbum, "%A / %Y")); // Filename, empty | Userdefined, Artist / Year
    // discs
    AddSortMethod(
        SortBy::TOTAL_DISCS, sortAttribute, 38077,
        LABEL_MASKS("%F", "", strAlbum, "%b")); // Filename, empty | Userdefined, Total discs
    // year
    AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%F", "", strAlbum, "%Y"));
    // original release date
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      AddSortMethod(
          SortBy::ORIG_DATE, 38079,
          LABEL_MASKS("%F", "", strAlbum, "%e")); // Filename, empty | Userdefined, Original date
    // album date added
    AddSortMethod(
        SortBy::DATE_ADDED, sortAttribute, 570,
        LABEL_MASKS("%F", "", strAlbum, "%a")); // Filename, empty | Userdefined, dateAdded
    // play count
    AddSortMethod(
        SortBy::PLAYCOUNT, 567,
        LABEL_MASKS("%F", "", strAlbum, "%V")); // Filename, empty | Userdefined, Play count
    // last played
    AddSortMethod(
        SortBy::LAST_PLAYED, 568,
        LABEL_MASKS("%F", "", strAlbum, "%p")); // Filename, empty | Userdefined, last played
    // rating
    AddSortMethod(SortBy::RATING, 563,
                  LABEL_MASKS("%F", "", strAlbum, "%R")); // Filename, empty | Userdefined, Rating
    // userrating
    AddSortMethod(
        SortBy::USER_RATING, 38018,
        LABEL_MASKS("%F", "", strAlbum, "%r")); // Filename, empty | Userdefined, UserRating

    if (PLAYLIST::IsSmartPlayList(items) || items.IsLibraryFolder())
      AddPlaylistOrder(items, LABEL_MASKS("%F", "", strAlbum, "%D"));
    else
    {
      SetSortMethod(viewState->m_sortDescription);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }

    SetViewAsControl(
        CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_ALBUMS)->m_viewMode);
  }
  else
  {
    CLog::Log(LOGERROR,"Music Smart Playlist must be one of songs, mixed or albums");
  }

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSmartPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV,
               CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS));
}

CGUIViewStateMusicPlaylist::CGUIViewStateMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;
  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME))
    sortAttribute = static_cast<SortAttribute>(sortAttribute | SortAttributeUseArtistSortName);

  std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
  AddSortMethod(SortBy::PLAYLIST_ORDER, 559, LABEL_MASKS(strTrack, "%D"));
  AddSortMethod(SortBy::TRACK_NUMBER, 554,
                LABEL_MASKS(strTrack, "%D")); // Userdefined, Duration| empty, empty
  AddSortMethod(SortBy::TITLE, sortAttribute, 556,
                LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
  AddSortMethod(SortBy::ALBUM, sortAttribute, 558,
                LABEL_MASKS("%B - %T - %A", "%D")); // Album, Title, Artist, Duration| empty, empty
  AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                LABEL_MASKS("%A - %T", "%D")); // Artist, Title, Duration| empty, empty
  AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                LABEL_MASKS("%A - %T", "%Y")); // Artist, Title, Year| empty, empty
  AddSortMethod(SortBy::LABEL, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
  AddSortMethod(SortBy::TIME, 180,
                LABEL_MASKS("%T - %A", "%D")); // Title - Artist, Duration| empty, empty
  AddSortMethod(SortBy::RATING, 563,
                LABEL_MASKS("%T - %A", "%R")); // Title - Artist, Rating| empty, empty
  AddSortMethod(SortBy::USER_RATING, 38018,
                LABEL_MASKS("%T - %A", "%r")); // Title - Artist, UserRating
  SetSortMethod(SortBy::PLAYLIST_ORDER);
  const CViewState* viewState = CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_FILES);
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
}

CGUIViewStateWindowMusicNav::CGUIViewStateWindowMusicNav(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;
  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_USEARTISTSORTNAME))
    sortAttribute = static_cast<SortAttribute>(sortAttribute | SortAttributeUseArtistSortName);

  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortBy::NONE, 551,
                  LABEL_MASKS("%F", "%I", "%L", "")); // Filename, Size | Foldername, empty
    SetSortMethod(SortBy::NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrder::NONE);
  }
  else if (items.GetPath() == "special://musicplaylists/")
  { // playlists list sorts by label only, ignoring folders
    AddSortMethod(SortBy::LABEL, SortAttributeIgnoreFolders, 551,
                  LABEL_MASKS("%F", "%D", "%L", "")); // Filename, Duration | Foldername, empty
    SetSortMethod(SortBy::LABEL);
  }
  else
  {
    if (VIDEO::IsVideoDb(items) &&
        items.Size() > (settings->GetBool(CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS) ? 1 : 0))
    {
      XFILE::VIDEODATABASEDIRECTORY::CQueryParams params;
      XFILE::CVideoDatabaseDirectory::GetQueryParams(items[settings->GetBool(CSettings::SETTING_FILELISTS_SHOWPARENTDIRITEMS) ? 1 : 0]->GetPath(), params);
      if (params.GetMVideoId() != -1)
      {
        AddSortMethod(SortBy::LABEL, sortAttribute, 551,
                      LABEL_MASKS("%T", "%Y")); // Filename, Duration | Foldername, empty
        AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%T", "%Y"));
        AddSortMethod(SortBy::ARTIST, sortAttribute, 557, LABEL_MASKS("%A - %T", "%Y"));
        AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));
        AddSortMethod(SortBy::ALBUM, sortAttribute, 558, LABEL_MASKS("%B - %T", "%Y"));

        std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        AddSortMethod(SortBy::TRACK_NUMBER, 554,
                      LABEL_MASKS(strTrack, "%D")); // Userdefined, Duration| empty, empty
      }
      else
      {
        AddSortMethod(SortBy::LABEL, 551,
                      LABEL_MASKS("%F", "%D", "%L", "")); // Filename, Duration | Foldername, empty
        SetSortMethod(SortBy::LABEL);
      }
    }
    else
    {
      //In navigation of music files tag data is scanned whenever present and can be used as sort criteria
      //hence sort methods available are similar to song node (not the same as only tag data)
      //Unfortunately anything here appears at all levels of file navigation even if no song files there.
      std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_LIBRARYTRACKFORMAT);
      if (strTrack.empty())
          strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
      AddSortMethod(
          SortBy::LABEL, 551,
          LABEL_MASKS(strTrack, "%D", "%L", ""), // Userdefined, Duration | FolderName, empty
          settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
              ? SortAttributeIgnoreArticle
              : SortAttributeNone);
      AddSortMethod(SortBy::SIZE, 553,
                    LABEL_MASKS("%F", "%I", "%L", "%I")); // Filename, Size | Foldername, Size
      AddSortMethod(SortBy::DATE, 552,
                    LABEL_MASKS("%F", "%J", "%L", "%J")); // Filename, Date | Foldername, Date
      AddSortMethod(SortBy::FILE, 561,
                    LABEL_MASKS("%F", "%I", "%L", "")); // Filename, Size | Label, empty
      AddSortMethod(SortBy::TRACK_NUMBER, 554,
                    LABEL_MASKS(strTrack, "%D")); // Userdefined, Duration| empty, empty
      AddSortMethod(SortBy::TITLE, sortAttribute, 556,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(
          SortBy::ALBUM, sortAttribute, 558,
          LABEL_MASKS("%B - %T - %A", "%D")); // Album, Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::ARTIST, sortAttribute, 557,
                    LABEL_MASKS("%A - %T", "%D")); // Artist, Title, Duration| empty, empty
      AddSortMethod(SortBy::ARTIST_THEN_YEAR, sortAttribute, 578,
                    LABEL_MASKS("%A - %T", "%Y")); // Artist(year), Title, Year| empty, empty
      AddSortMethod(SortBy::TIME, 180,
                    LABEL_MASKS("%T - %A", "%D")); // Title, Artist, Duration| empty, empty
      AddSortMethod(SortBy::YEAR, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year

      SetSortMethod(SortBy::LABEL);
    }
    const CViewState* viewState =
        CViewStateSettings::GetInstance().Get(VIEW_STATE::MUSIC_NAV_SONGS);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);

    SetSortOrder(SortOrder::ASCENDING);
  }
  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
}

std::vector<CMediaSource>& CGUIViewStateWindowMusicNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  CFileItemList items;

  CDirectory::GetDirectory(LIBRARY::MUSIC, items, "", DIR_FLAG_DEFAULTS);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName = item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage = item->GetArt(ART::TYPE::ICON);
    share.m_iDriveType = SourceType::LOCAL;
    m_sources.push_back(share);
  }

  return CGUIViewStateWindowMusic::GetSources();
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT);
  if (strTrack.empty())
    strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);

  AddSortMethod(SortBy::NONE, 551,
                LABEL_MASKS(strTrack, "%D", "%L", "")); // Userdefined, Duration | FolderName, empty
  SetSortMethod(SortBy::NONE);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrder::NONE);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

PLAYLIST::Id CGUIViewStateWindowMusicPlaylist::GetPlaylist() const
{
  return PLAYLIST::Id::TYPE_MUSIC;
}

bool CGUIViewStateWindowMusicPlaylist::AutoPlayNextItem()
{
  return false;
}

bool CGUIViewStateWindowMusicPlaylist::HideParentDirItems()
{
  return true;
}

std::vector<CMediaSource>& CGUIViewStateWindowMusicPlaylist::GetSources()
{
  m_sources.clear();
  //  Playlist share
  CMediaSource share;
  share.strPath = "playlistmusic://";
  share.m_iDriveType = SourceType::LOCAL;
  m_sources.push_back(share);

  // CGUIViewState::GetSources would add music plugins
  return m_sources;
}
