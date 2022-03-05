/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicUtils.h"

#include "Application.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace PLAYLIST;

namespace MUSIC_UTILS
{
  class CSetArtJob : public CJob
  {
    CFileItemPtr pItem;
    std::string m_artType;
    std::string m_newArt;
  public:
    CSetArtJob(const CFileItemPtr& item, const std::string& type, const std::string& newArt)
      : pItem(item), m_artType(type), m_newArt(newArt)
    { }

    ~CSetArtJob(void) override = default;

    bool HasSongExtraArtChanged(const CFileItemPtr& pSongItem,
                                const std::string& type,
                                const int itemID,
                                CMusicDatabase& db)
    {
      if (!pSongItem->HasMusicInfoTag())
        return false;
      int idSong = pSongItem->GetMusicInfoTag()->GetDatabaseId();
      if (idSong <= 0)
        return false;
      bool result = false;
      if (type == MediaTypeAlbum)
        // Update art when song is from album
        result = (itemID == pSongItem->GetMusicInfoTag()->GetAlbumId());
      else if (type == MediaTypeArtist)
      {
        // Update art when artist is song or album artist of the song
        if (pSongItem->HasProperty("artistid"))
        {
          // Check artistid property when we have it
          for (CVariant::const_iterator_array varid = pSongItem->GetProperty("artistid").begin_array();
            varid != pSongItem->GetProperty("artistid").end_array(); varid++)
          {
            int idArtist = static_cast<int>(varid->asInteger());
            result = (itemID == idArtist);
            if (result)
              break;
          }
        }
        else
        { // Check song artists in database
          result = db.IsSongArtist(idSong, itemID);
        }
        if (!result)
        {
          // Check song album artists
          result = db.IsSongAlbumArtist(idSong, itemID);
        }
      }
      return result;
    }

    // Asynchronously update song, album or artist art in library
    // and trigger update to album & artist art of the currently playing song
    // and songs queued in the current playlist
    bool DoWork(void) override
    {
      int itemID = pItem->GetMusicInfoTag()->GetDatabaseId();
      if (itemID <= 0)
        return false;
      std::string type = pItem->GetMusicInfoTag()->GetType();
      CMusicDatabase db;
      if (!db.Open())
        return false;
      if (!m_newArt.empty())
        db.SetArtForItem(itemID, type, m_artType, m_newArt);
      else
        db.RemoveArtForItem(itemID, type, m_artType);
      // Artwork changed so set datemodified field for artist, album or song
      db.SetItemUpdated(itemID, type);

      /* Update the art of the songs of the current music playlist.
      Song thumb is often a fallback from the album and fanart is from the artist(s).
      Clear the art if it is a song from the album or by the artist
      (as song or album artist) that has modified artwork. The new artwork gets
      loaded when the playlist is shown.
      */
      bool clearcache(false);
      CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST_MUSIC);
      for (int i = 0; i < playlist.size(); ++i)
      {
        CFileItemPtr songitem = playlist[i];
        if (HasSongExtraArtChanged(songitem, type, itemID, db))
        {
          songitem->ClearArt(); // Art gets reloaded when the current playist is shown
          clearcache = true;
        }
      }
      if (clearcache)
      {
        // Clear the music playlist from cache
        CFileItemList items("playlistmusic://");
        items.RemoveDiscCache(WINDOW_MUSIC_PLAYLIST);
      }

      // Similarly update the art of the currently playing song so it shows on OSD
      if (g_application.GetAppPlayer().IsPlayingAudio() && g_application.CurrentFileItem().HasMusicInfoTag())
      {
        CFileItemPtr songitem = CFileItemPtr(new CFileItem(g_application.CurrentFileItem()));
        if (HasSongExtraArtChanged(songitem, type, itemID, db))
          g_application.UpdateCurrentPlayArt();
      }

      db.Close();
      return true;
    }
  };

  class CSetSongRatingJob : public CJob
  {
    std::string strPath;
    int idSong;
    int iUserrating;
  public:
    CSetSongRatingJob(const std::string& filePath, int userrating) :
      strPath(filePath),
      idSong(-1),
      iUserrating(userrating)
    { }

    CSetSongRatingJob(int songId, int userrating) :
      strPath(),
      idSong(songId),
      iUserrating(userrating)
    { }

    ~CSetSongRatingJob(void) override = default;

    bool DoWork(void) override
    {
      // Asynchronously update song userrating in library
      CMusicDatabase db;
      if (db.Open())
      {
        if (idSong > 0)
          db.SetSongUserrating(idSong, iUserrating);
        else
          db.SetSongUserrating(strPath, iUserrating);
        db.Close();
      }

      return true;
    }
  };

  void UpdateArtJob(const CFileItemPtr& pItem,
                    const std::string& strType,
                    const std::string& strArt)
  {
    // Asynchronously update that type of art in the database
    CSetArtJob *job = new CSetArtJob(pItem, strType, strArt);
    CServiceBroker::GetJobManager()->AddJob(job, nullptr);
  }

  // Add art types required in Kodi and configured by the user
  void AddHardCodedAndExtendedArtTypes(std::vector<std::string>& artTypes, const CMusicInfoTag& tag)
  {
    for (const auto& artType : GetArtTypesToScan(tag.GetType()))
    {
      if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
        artTypes.push_back(artType);
    }
  }

  // Add art types currently assigned to the media item
  void AddCurrentArtTypes(std::vector<std::string>& artTypes, const CMusicInfoTag& tag,
    CMusicDatabase& db)
  {
    std::map<std::string, std::string> currentArt;
    db.GetArtForItem(tag.GetDatabaseId(), tag.GetType(), currentArt);
    for (const auto& art : currentArt)
    {
      if (!art.second.empty() && find(artTypes.begin(), artTypes.end(), art.first) == artTypes.end())
        artTypes.push_back(art.first);
    }
  }

  // Add art types that exist for other media items of the same type
  void AddMediaTypeArtTypes(std::vector<std::string>& artTypes, const CMusicInfoTag& tag,
    CMusicDatabase& db)
  {
    std::vector<std::string> dbArtTypes;
    db.GetArtTypes(tag.GetType(), dbArtTypes);
    for (const auto& artType : dbArtTypes)
    {
      if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
        artTypes.push_back(artType);
    }
  }

  // Add art types from available but unassigned artwork for this media item
  void AddAvailableArtTypes(std::vector<std::string>& artTypes, const CMusicInfoTag& tag,
    CMusicDatabase& db)
  {
    for (const auto& artType : db.GetAvailableArtTypesForItem(tag.GetDatabaseId(), tag.GetType()))
    {
      if (find(artTypes.begin(), artTypes.end(), artType) == artTypes.end())
        artTypes.push_back(artType);
    }
  }

  bool FillArtTypesList(CFileItem& musicitem, CFileItemList& artlist)
  {
    CMusicInfoTag &tag = *musicitem.GetMusicInfoTag();
    if (tag.GetDatabaseId() < 1 || tag.GetType().empty())
      return false;
    if (tag.GetType() != MediaTypeArtist && tag.GetType() != MediaTypeAlbum && tag.GetType() != MediaTypeSong)
      return false;

    artlist.Clear();

    CMusicDatabase db;
    db.Open();

    std::vector<std::string> artTypes;

    AddHardCodedAndExtendedArtTypes(artTypes, tag);
    AddCurrentArtTypes(artTypes, tag, db);
    AddMediaTypeArtTypes(artTypes, tag, db);
    AddAvailableArtTypes(artTypes, tag, db);

    db.Close();

    for (const auto& type : artTypes)
    {
      CFileItemPtr artitem(new CFileItem(type, false));
      // Localise the names of common types of art
      if (type == "banner")
        artitem->SetLabel(g_localizeStrings.Get(20020));
      else if (type == "fanart")
        artitem->SetLabel(g_localizeStrings.Get(20445));
      else if (type == "poster")
        artitem->SetLabel(g_localizeStrings.Get(20021));
      else if (type == "thumb")
        artitem->SetLabel(g_localizeStrings.Get(21371));
      else
        artitem->SetLabel(type);
      // Set art type as art item property
      artitem->SetProperty("arttype", type);
      // Set current art as art item thumb
      if (musicitem.HasArt(type))
        artitem->SetArt("thumb", musicitem.GetArt(type));
      artlist.Add(artitem);
    }

    return !artlist.IsEmpty();
  }

  std::string ShowSelectArtTypeDialog(CFileItemList& artitems)
  {
    // Prompt for choice
    CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (!dialog)
      return "";

    dialog->SetHeading(CVariant{ 13521 });
    dialog->Reset();
    dialog->SetUseDetails(true);
    dialog->EnableButton(true, 13516);

    dialog->SetItems(artitems);
    dialog->Open();

    if (dialog->IsButtonPressed())
    {
      // Get the new art type name
      std::string strArtTypeName;
      if (!CGUIKeyboardFactory::ShowAndGetInput(strArtTypeName, CVariant{ g_localizeStrings.Get(13516) }, false))
        return "";
      // Add new type to the list of art types
      CFileItemPtr artitem(new CFileItem(strArtTypeName, false));
      artitem->SetLabel(strArtTypeName);
      artitem->SetProperty("arttype", strArtTypeName);
      artitems.Add(artitem);

      return strArtTypeName;
    }

    return dialog->GetSelectedFileItem()->GetProperty("arttype").asString();
  }

  int ShowSelectRatingDialog(int iSelected)
  {
    CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (dialog)
    {
      dialog->SetHeading(CVariant{ 38023 });
      dialog->Add(g_localizeStrings.Get(38022));
      for (int i = 1; i <= 10; i++)
        dialog->Add(StringUtils::Format("{}: {}", g_localizeStrings.Get(563), i));
      dialog->SetSelected(iSelected);
      dialog->Open();

      int userrating = dialog->GetSelectedItem();
      userrating = std::max(userrating, -1);
      userrating = std::min(userrating, 10);
      return userrating;
    }
    return -1;
  }

  void UpdateSongRatingJob(const CFileItemPtr& pItem, int userrating)
  {
    // Asynchronously update the song user rating in music library
    const CMusicInfoTag *tag = pItem->GetMusicInfoTag();
    CSetSongRatingJob *job;
    if (tag && tag->GetType() == MediaTypeSong && tag->GetDatabaseId() > 0)
      // Use song ID when known
      job = new CSetSongRatingJob(tag->GetDatabaseId(), userrating);
    else
      job = new CSetSongRatingJob(pItem->GetPath(), userrating);
    CServiceBroker::GetJobManager()->AddJob(job, nullptr);
  }

  std::vector<std::string> GetArtTypesToScan(const MediaType& mediaType)
  {
    std::vector<std::string> arttypes;
    // Get default types of art that are to be automatically fetched during scanning
    if (mediaType == MediaTypeArtist)
    {
      arttypes = { "thumb", "fanart" };
      for (auto& artType : CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
        CSettings::SETTING_MUSICLIBRARY_ARTISTART_WHITELIST))
      {
        if (find(arttypes.begin(), arttypes.end(), artType.asString()) == arttypes.end())
          arttypes.emplace_back(artType.asString());
      }
    }
    else if (mediaType == MediaTypeAlbum)
    {
      arttypes = { "thumb" };
      for (auto& artType :
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
          CSettings::SETTING_MUSICLIBRARY_ALBUMART_WHITELIST))
      {
        if (find(arttypes.begin(), arttypes.end(), artType.asString()) == arttypes.end())
          arttypes.emplace_back(artType.asString());
      }
    }
    return arttypes;
  }

  bool IsValidArtType(const std::string& potentialArtType)
  {
    // Check length and is ascii
    return potentialArtType.length() <= 25 &&
                 std::find_if_not(potentialArtType.begin(), potentialArtType.end(),
                                  StringUtils::isasciialphanum) == potentialArtType.end();
  }

  } // namespace MUSIC_UTILS
