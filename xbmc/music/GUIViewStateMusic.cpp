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
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "playlists/PlayListTypes.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/SortUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "view/ViewStateSettings.h"

using namespace KODI;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;

PLAYLIST::Id CGUIViewStateWindowMusic::GetPlaylist() const
{
  return PLAYLIST::TYPE_MUSIC;
}

bool CGUIViewStateWindowMusic::AutoPlayNextItem()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  return settings->GetBool(CSettings::SETTING_MUSICPLAYER_AUTOPLAYNEXTITEM) &&
         !settings->GetBool(CSettings::SETTING_MUSICPLAYER_QUEUEBYDEFAULT);
}

std::string CGUIViewStateWindowMusic::GetLockType()
{
  return "music";
}

std::string CGUIViewStateWindowMusic::GetExtensions()
{
  return CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
}

VECSOURCES& CGUIViewStateWindowMusic::GetSources()
{
  return CGUIViewState::GetSources();
}

CGUIViewStateMusicSearch::CGUIViewStateMusicSearch(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  SortAttribute sortAttribute = SortAttributeNone;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    sortAttribute = SortAttributeIgnoreArticle;

  AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D", "%L", "%A"));  // Title - Artist, Duration | Label, Artist
  SetSortMethod(SortByTitle);

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavsongs");
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSearch::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, CViewStateSettings::GetInstance().Get("musicnavsongs"));
}

CGUIViewStateMusicDatabase::CGUIViewStateMusicDatabase(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  CMusicDatabaseDirectory dir;
  NODE_TYPE NodeType=dir.GetDirectoryChildType(items.GetPath());

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

  switch (NodeType)
  {
  case NODE_TYPE_OVERVIEW:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_TOP100:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_GENRE:
    {
      AddSortMethod(SortByGenre, 515, LABEL_MASKS("%F", "", "%G", ""));  // Filename, empty | Genre, empty
      SetSortMethod(SortByGenre);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderAscending);
    }
    break;
  case NODE_TYPE_ROLE:
  {
    AddSortMethod(SortByNone, 576, LABEL_MASKS("%F", "", "%G", ""));  // Filename, empty | Genre, empty
    SetSortMethod(SortByPlaycount);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  break;
  case NODE_TYPE_YEAR:
    {
      AddSortMethod(SortByLabel, 562, LABEL_MASKS("%F", "", "%Y", ""));  // Filename, empty | Year, empty
      SetSortMethod(SortByLabel);

      SetViewAsControl(DEFAULT_VIEW_LIST);

      SetSortOrder(SortOrderAscending);
    }
    break;
  case NODE_TYPE_ARTIST:
    {
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", "%A", ""));  // Filename, empty | Artist, empty
      AddSortMethod(SortByDateAdded, sortAttribute, 570, LABEL_MASKS("%F", "", "%A", "%a"));  // Filename, empty | Artist, dateAdded
      SetSortMethod(SortByArtist);

      const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavartists");
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM:
    {
      // album
      AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%F", "", strAlbum, "%A"));  // Filename, empty | Userdefined (default=%B), Artist
      // artist
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", strAlbum, "%A"));  // Filename, empty | Userdefined, Artist
      // artist / year
      AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%F", "", strAlbum, "%A / %Y"));  // Filename, empty | Userdefined, Artist / Year
      // discs
      AddSortMethod(
          SortByTotalDiscs, sortAttribute, 38077,
          LABEL_MASKS("%F", "", strAlbum, "%b")); // Filename, empty | Userdefined, Total discs
      // year
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%F", "", strAlbum, "%Y"));  // Filename, empty | Userdefined, Year
      // original release year
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        AddSortMethod(
          SortByOrigDate, sortAttribute, 38079,
          LABEL_MASKS("%F", "", strAlbum, "%e")); // Filename, empty | Userdefined, Original date
      // album date added
      AddSortMethod(SortByDateAdded, sortAttribute, 570, LABEL_MASKS("%F", "", strAlbum, "%a"));  // Filename, empty | Userdefined, dateAdded
      // play count
      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%F", "", strAlbum, "%V"));  // Filename, empty | Userdefined, Play count
      // last played
      AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%F", "", strAlbum, "%p"));  // Filename, empty | Userdefined, last played
      // rating
      AddSortMethod(SortByRating, 563, LABEL_MASKS("%F", "", strAlbum, "%R"));  // Filename, empty | Userdefined, Rating
      // userrating
      AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%F", "", strAlbum, "%r"));  // Filename, empty | Userdefined, UserRating

      const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavalbums");
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    {
      AddSortMethod(SortByNone, 552, LABEL_MASKS("%F", "", strAlbum, "%a"));  // Filename, empty | Userdefined, dateAdded
      SetSortMethod(SortByNone);

      SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavalbums")->m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    {
      AddSortMethod(SortByNone, 552, LABEL_MASKS(strTrack, "%a"));  // Userdefined, dateAdded | empty, empty
      SetSortMethod(SortByNone);

      SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavsongs")->m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    {
      AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%F", "", strAlbum, "%p"));  // Filename, empty | Userdefined, last played

      SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavalbums")->m_viewMode);
    }
    break;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    {
      AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS(strTrack, "%p"));  // Userdefined, last played | empty, empty

      SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavalbums")->m_viewMode);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100:
    {
      AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "", strAlbum, "%V"));  // Filename, empty | Userdefined, Play count
      SetSortMethod(SortByNone);

      SetViewAsControl(DEFAULT_VIEW_LIST);
      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_SINGLES:
    {
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
      AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));  // Artist, Title, Year| empty, empty
      AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
      AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title - Artist, Rating
      AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T - %A", "%r"));  // Title - Artist, UserRating
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
      // original release date  (singles can be re-released)
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
          AddSortMethod(SortByOrigDate, 38079,
                      LABEL_MASKS("%T - %A", "%e")); // Title, Artist, Original Date
      AddSortMethod(SortByDateAdded, 570,
                    LABEL_MASKS("%T - %A", "%a")); // Title - Artist, DateAdded | empty, empty
      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T - %A", "%V"));  // Title - Artist, PlayCount
      AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS(strTrack, "%p"));  // Userdefined, last played | empty, empty

      const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavsongs");
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_ALBUM_TOP100_SONGS:
  case NODE_TYPE_SONG:
    {
      AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%D"));  // Userdefined, Duration| empty, empty
      AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Title, Artist, Duration| empty, empty
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
      AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));  // Artist, Title, Year| empty, empty
      AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
      AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title - Artist, Rating
      AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T - %A", "%r"));  // Title - Artist, UserRating
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
      // original release date
      if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        AddSortMethod(SortByOrigDate, 38079,
          LABEL_MASKS("%T - %A", "%e")); // Title, Artist, Original Date
      AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T - %A", "%a"));  // Title - Artist, DateAdded | empty, empty
      AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T - %A", "%V"));  // Title - Artist, PlayCount
      AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS(strTrack, "%p"));  // Userdefined, last played | empty, empty
      AddSortMethod(SortByBPM, 38080, LABEL_MASKS(strTrack, "%f")); // Userdefined, bpm, empty,empty

      const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavsongs");
      // the "All Albums" entries always default to SortByAlbum as this is most logical - user can always
      // change it and the change will be saved for this particular path
      if (dir.IsAllItem(items.GetPath()))
        SetSortMethod(SortByAlbum);
      else
        SetSortMethod(viewState->m_sortDescription);

      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
    break;
  case NODE_TYPE_SONG_TOP100:
    {
      AddSortMethod(SortByNone, 576, LABEL_MASKS("%T - %A", "%V"));
      SetSortMethod(SortByPlaycount);

      SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavsongs")->m_viewMode);

      SetSortOrder(SortOrderNone);
    }
    break;
  case NODE_TYPE_DISC:
    {
      AddSortMethod(SortByNone, 427, LABEL_MASKS("%L")); // Use the existing label
      SetSortMethod(SortByNone);
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
  NODE_TYPE NodeType=dir.GetDirectoryChildType(m_items.GetPath());

  switch (NodeType)
  {
    case NODE_TYPE_ARTIST:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, CViewStateSettings::GetInstance().Get("musicnavartists"));
      break;
    case NODE_TYPE_ALBUM:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, CViewStateSettings::GetInstance().Get("musicnavalbums"));
      break;
    case NODE_TYPE_SINGLES:
    case NODE_TYPE_SONG:
      SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, CViewStateSettings::GetInstance().Get("musicnavsongs"));
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
  const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavsongs");

  if (items.GetContent() == "songs" || items.GetContent() == "mixed")
  {
    std::string strTrack=settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
    AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%D"));  // Userdefined, Duration| empty, empty
    AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
    AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Title, Artist, Duration| empty, empty
    AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
    AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));  // Artist, Title, Year| empty, empty
    AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
    AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title, Artist, Rating| empty, empty
    AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T - %A", "%r"));  // Title - Artist, UserRating
    AddSortMethod(SortByYear, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year
    AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T - %A", "%a"));  // Title - Artist, DateAdded | empty, empty
    AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T - %A", "%V"));  // Title - Artist, PlayCount
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      AddSortMethod(SortByOrigDate, 38079,
                    LABEL_MASKS("%T - %A", "%e")); // Title - Artist, original date, empty, empty
    AddSortMethod(SortByBPM, 38080,
                  LABEL_MASKS("%T - %A", "%f")); // Title - Artist, bpm, empty, empty

    if (items.IsSmartPlayList() || items.IsLibraryFolder())
      AddPlaylistOrder(items, LABEL_MASKS(strTrack, "%D"));
    else
    {
      SetSortMethod(viewState->m_sortDescription);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }

    SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavsongs")->m_viewMode);
  }
  else if (items.GetContent() == "albums")
  {
    std::string strAlbum = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_strMusicLibraryAlbumFormat;
    if (strAlbum.empty())
      strAlbum = "%B"; // album
    // album
    AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%F", "", strAlbum, "%A"));  // Filename, empty | Userdefined (default=%B), Artist
    // artist
    AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%F", "", strAlbum, "%A"));  // Filename, empty | Userdefined, Artist
    // artist / year
    AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%F", "", strAlbum, "%A / %Y"));  // Filename, empty | Userdefined, Artist / Year
    // discs
    AddSortMethod(
        SortByTotalDiscs, sortAttribute, 38077,
        LABEL_MASKS("%F", "", strAlbum, "%b")); // Filename, empty | Userdefined, Total discs
    // year
    AddSortMethod(SortByYear, 562, LABEL_MASKS("%F", "", strAlbum, "%Y"));
    // original release date
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      AddSortMethod(
          SortByOrigDate, 38079,
          LABEL_MASKS("%F", "", strAlbum, "%e")); // Filename, empty | Userdefined, Original date
    // album date added
    AddSortMethod(SortByDateAdded, sortAttribute, 570, LABEL_MASKS("%F", "", strAlbum, "%a"));  // Filename, empty | Userdefined, dateAdded
    // play count
    AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%F", "", strAlbum, "%V"));  // Filename, empty | Userdefined, Play count
    // last played
    AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%F", "", strAlbum, "%p"));  // Filename, empty | Userdefined, last played
    // rating
    AddSortMethod(SortByRating, 563, LABEL_MASKS("%F", "", strAlbum, "%R"));  // Filename, empty | Userdefined, Rating
    // userrating
    AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%F", "", strAlbum, "%r"));  // Filename, empty | Userdefined, UserRating

    if (items.IsSmartPlayList() || items.IsLibraryFolder())
      AddPlaylistOrder(items, LABEL_MASKS("%F", "", strAlbum, "%D"));
    else
    {
      SetSortMethod(viewState->m_sortDescription);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }

    SetViewAsControl(CViewStateSettings::GetInstance().Get("musicnavalbums")->m_viewMode);
  }
  else
  {
    CLog::Log(LOGERROR,"Music Smart Playlist must be one of songs, mixed or albums");
  }

  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateMusicSmartPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV, CViewStateSettings::GetInstance().Get("musicnavsongs"));
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
  AddSortMethod(SortByPlaylistOrder, 559, LABEL_MASKS(strTrack, "%D"));
  AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%D"));  // Userdefined, Duration| empty, empty
  AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
  AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Title, Artist, Duration| empty, empty
  AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
  AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));  // Artist, Title, Year| empty, empty
  AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS(strTrack, "%D"));
  AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Title - Artist, Duration| empty, empty
  AddSortMethod(SortByRating, 563, LABEL_MASKS("%T - %A", "%R"));  // Title - Artist, Rating| empty, empty
  AddSortMethod(SortByUserRating, 38018, LABEL_MASKS("%T - %A", "%r"));  // Title - Artist, UserRating
  SetSortMethod(SortByPlaylistOrder);
  const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicfiles");
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
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SortByNone);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else if (items.GetPath() == "special://musicplaylists/")
  { // playlists list sorts by label only, ignoring folders
    AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551,
                  LABEL_MASKS("%F", "%D", "%L", "")); // Filename, Duration | Foldername, empty
    SetSortMethod(SortByLabel);
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
        AddSortMethod(SortByLabel, sortAttribute, 551, LABEL_MASKS("%T", "%Y"));  // Filename, Duration | Foldername, empty
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));
        AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%Y"));
        AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));
        AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T", "%Y"));

        std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%D"));  // Userdefined, Duration| empty, empty
      }
      else
      {
        AddSortMethod(SortByLabel, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SortByLabel);
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
      AddSortMethod(SortByLabel, 551, LABEL_MASKS(strTrack, "%D", "%L", ""),  // Userdefined, Duration | FolderName, empty
        settings->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
      AddSortMethod(SortBySize, 553, LABEL_MASKS("%F", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
      AddSortMethod(SortByDate, 552, LABEL_MASKS("%F", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
      AddSortMethod(SortByFile, 561, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
      AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(strTrack, "%D"));  // Userdefined, Duration| empty, empty
      AddSortMethod(SortByTitle, sortAttribute, 556, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByAlbum, sortAttribute, 558, LABEL_MASKS("%B - %T - %A", "%D"));  // Album, Title, Artist, Duration| empty, empty
      AddSortMethod(SortByArtist, sortAttribute, 557, LABEL_MASKS("%A - %T", "%D"));  // Artist, Title, Duration| empty, empty
      AddSortMethod(SortByArtistThenYear, sortAttribute, 578, LABEL_MASKS("%A - %T", "%Y"));  // Artist(year), Title, Year| empty, empty
      AddSortMethod(SortByTime, 180, LABEL_MASKS("%T - %A", "%D"));  // Title, Artist, Duration| empty, empty
      AddSortMethod(SortByYear, 562, LABEL_MASKS("%T - %A", "%Y")); // Title, Artist, Year

      SetSortMethod(SortByLabel);
    }
    const CViewState *viewState = CViewStateSettings::GetInstance().Get("musicnavsongs");
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);

    SetSortOrder(SortOrderAscending);
  }
  LoadViewState(items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_NAV);
}

void CGUIViewStateWindowMusicNav::AddOnlineShares()
{
  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVirtualShares)
    return;

  VECSOURCES *musicSources = CMediaSourceSettings::GetInstance().GetSources("music");

  for (int i = 0; i < (int)musicSources->size(); ++i)
  {
    CMediaSource share = musicSources->at(i);
  }
}

VECSOURCES& CGUIViewStateWindowMusicNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  CFileItemList items;

  CDirectory::GetDirectory("library://music/", items, "", DIR_FLAG_DEFAULTS);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName = item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage = item->GetArt("icon");
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }

  AddOnlineShares();

  return CGUIViewStateWindowMusic::GetSources();
}

CGUIViewStateWindowMusicPlaylist::CGUIViewStateWindowMusicPlaylist(const CFileItemList& items) : CGUIViewStateWindowMusic(items)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT);
  if (strTrack.empty())
    strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);

  AddSortMethod(SortByNone, 551, LABEL_MASKS(strTrack, "%D", "%L", ""));  // Userdefined, Duration | FolderName, empty
  SetSortMethod(SortByNone);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderNone);

  LoadViewState(items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

void CGUIViewStateWindowMusicPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_MUSIC_PLAYLIST);
}

PLAYLIST::Id CGUIViewStateWindowMusicPlaylist::GetPlaylist() const
{
  return PLAYLIST::TYPE_MUSIC;
}

bool CGUIViewStateWindowMusicPlaylist::AutoPlayNextItem()
{
  return false;
}

bool CGUIViewStateWindowMusicPlaylist::HideParentDirItems()
{
  return true;
}

VECSOURCES& CGUIViewStateWindowMusicPlaylist::GetSources()
{
  m_sources.clear();
  //  Playlist share
  CMediaSource share;
  share.strPath = "playlistmusic://";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  // CGUIViewState::GetSources would add music plugins
  return m_sources;
}
