/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioLibrary.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "messaging/ApplicationMessenger.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "music/MusicThumbLoader.h"
#include "music/Song.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <memory>

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

JSONRPC_STATUS CAudioLibrary::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  CMusicDatabase musicdatabase;
  // Make db connection once if one or more properties needs db access
  for (CVariant::const_iterator_array it = parameterObject["properties"].begin_array();
       it != parameterObject["properties"].end_array(); ++it)
  {
    std::string propertyName = it->asString();
    if (propertyName == "librarylastupdated" || propertyName == "librarylastcleaned" ||
        propertyName == "artistlinksupdated" || propertyName == "songslastadded" ||
        propertyName == "albumslastadded" || propertyName == "artistslastadded" ||
        propertyName == "songsmodified" || propertyName == "albumsmodified" ||
        propertyName == "artistsmodified")
    {
      if (!musicdatabase.Open())
        return InternalError;
      else
        break;
    }
  }

  for (CVariant::const_iterator_array it = parameterObject["properties"].begin_array();
       it != parameterObject["properties"].end_array(); ++it)
  {
    std::string propertyName = it->asString();
    CVariant property;
    if (propertyName == "missingartistid")
      property = (int)BLANKARTIST_ID;
    else if (propertyName == "librarylastupdated")
      property = musicdatabase.GetLibraryLastUpdated();
    else if (propertyName == "librarylastcleaned")
      property = musicdatabase.GetLibraryLastCleaned();
    else if (propertyName == "artistlinksupdated")
      property = musicdatabase.GetArtistLinksUpdated();
    else if (propertyName == "songslastadded")
      property = musicdatabase.GetSongsLastAdded();
    else if (propertyName == "albumslastadded")
      property = musicdatabase.GetAlbumsLastAdded();
    else if (propertyName == "artistslastadded")
      property = musicdatabase.GetArtistsLastAdded();
    else if (propertyName == "genreslastadded")
      property = musicdatabase.GetGenresLastAdded();
    else if (propertyName == "songsmodified")
      property = musicdatabase.GetSongsLastModified();
    else if (propertyName == "albumsmodified")
      property = musicdatabase.GetAlbumsLastModified();
    else if (propertyName == "artistsmodified")
      property = musicdatabase.GetArtistsLastModified();

    properties[propertyName] = property;
  }

  result = properties;
  return OK;
}


JSONRPC_STATUS CAudioLibrary::GetArtists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString("musicdb://artists/"))
    return InternalError;

  bool allroles = false;
  if (parameterObject["allroles"].isBoolean())
    allroles = parameterObject["allroles"].asBoolean();

  const CVariant &filter = parameterObject["filter"];

  if (allroles)
    musicUrl.AddOption("roleid", -1000); //All roles, any negative parameter overrides implicit roleid=1 filter required for backward compatibility
  else if (filter.isMember("roleid"))
    musicUrl.AddOption("roleid", static_cast<int>(filter["roleid"].asInteger()));
  else if (filter.isMember("role"))
    musicUrl.AddOption("role", filter["role"].asString());
  // Only one of (song) genreid/genre, albumid/album or songid/song or rules type filter is allowed by filter syntax
  if (filter.isMember("genreid"))  //Deprecated. Use "songgenre" or "artistgenre"
    musicUrl.AddOption("genreid", static_cast<int>(filter["genreid"].asInteger()));
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  if (filter.isMember("songgenreid"))
    musicUrl.AddOption("genreid", static_cast<int>(filter["songgenreid"].asInteger()));
  else if (filter.isMember("songgenre"))
    musicUrl.AddOption("genre", filter["songgenre"].asString());
  else if (filter.isMember("albumid"))
    musicUrl.AddOption("albumid", static_cast<int>(filter["albumid"].asInteger()));
  else if (filter.isMember("album"))
    musicUrl.AddOption("album", filter["album"].asString());
  else if (filter.isMember("songid"))
    musicUrl.AddOption("songid", static_cast<int>(filter["songid"].asInteger()));
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("artists", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  bool albumArtistsOnly = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS);
  if (parameterObject["albumartistsonly"].isBoolean())
    albumArtistsOnly = parameterObject["albumartistsonly"].asBoolean();
  musicUrl.AddOption("albumartistsonly", albumArtistsOnly);

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  int total;
  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array();
         field != parameterObject["properties"].end_array(); ++field)
      fields.insert(field->asString());
  }

  musicdatabase.SetTranslateBlankArtist(false);
  if (!musicdatabase.GetArtistsByWhereJSON(fields, musicUrl.ToString(), result, total, sorting))
    return InternalError;

  int start, end;
  HandleLimits(parameterObject, result, total, start, end);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetArtistDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString("musicdb://artists/"))
    return InternalError;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  musicUrl.AddOption("artistid", artistID);

  CFileItemList items;
  CDatabase::Filter filter;
  if (!musicdatabase.GetArtistsByWhere(musicUrl.ToString(), filter, items) || items.Size() != 1)
    return InvalidParams;

  // Add "artist" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  param["properties"].append("artist");

  //Get roleids, roles etc. if needed
  JSONRPC_STATUS ret = GetAdditionalArtistDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItem("artistid", false, "artistdetails", items[0], param, param["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString("musicdb://albums/"))
    return InternalError;

  if (parameterObject["includesingles"].asBoolean())
    musicUrl.AddOption("show_singles", true);

  bool allroles = false;
  if (parameterObject["allroles"].isBoolean())
    allroles = parameterObject["allroles"].asBoolean();

  const CVariant &filter = parameterObject["filter"];

  if (allroles)
    musicUrl.AddOption("roleid", -1000); //All roles, override implicit roleid=1 filter required for backward compatibility
  else if (filter.isMember("roleid"))
    musicUrl.AddOption("roleid", static_cast<int>(filter["roleid"].asInteger()));
  else if (filter.isMember("role"))
    musicUrl.AddOption("role", filter["role"].asString());
  // Only one of genreid/genre, artistid/artist or rules type filter is allowed by filter syntax
  if (filter.isMember("artistid"))
    musicUrl.AddOption("artistid", static_cast<int>(filter["artistid"].asInteger()));
  else if (filter.isMember("artist"))
    musicUrl.AddOption("artist", filter["artist"].asString());
  else if (filter.isMember("genreid"))
    musicUrl.AddOption("genreid", static_cast<int>(filter["genreid"].asInteger()));
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  int total;
  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array();
         field != parameterObject["properties"].end_array(); ++field)
      fields.insert(field->asString());
  }

  if (!musicdatabase.GetAlbumsByWhereJSON(fields, musicUrl.ToString(), result, total, sorting))
    return InternalError;

  if (!result.isNull())
  {
    bool bFetchArt = fields.find("art") != fields.end();
    bool bFetchFanart = fields.find("fanart") != fields.end();
    if (bFetchArt || bFetchFanart)
    {
      CThumbLoader* thumbLoader = new CMusicThumbLoader();
      thumbLoader->OnLoaderStart();

      std::set<std::string> artfields;
      if (bFetchArt)
        artfields.insert("art");
      if (bFetchFanart)
        artfields.insert("fanart");

      for (unsigned int index = 0; index < result["albums"].size(); index++)
      {
        CFileItem item;
        item.GetMusicInfoTag()->SetDatabaseId(result["albums"][index]["albumid"].asInteger32(), MediaTypeAlbum);

        // Could use FillDetails, but it does unnecessary serialization of empty MusiInfoTag
        // CFileItemPtr itemptr(new CFileItem(item));
        // FillDetails(item.GetMusicInfoTag(), itemptr, artfields, result["albums"][index], thumbLoader);

        thumbLoader->FillLibraryArt(item);

        if (bFetchFanart)
        {
          if (item.HasArt("fanart"))
            result["albums"][index]["fanart"] = CTextureUtils::GetWrappedImageURL(item.GetArt("fanart"));
          else
            result["albums"][index]["fanart"] = "";
        }
        if (bFetchArt)
        {
          CGUIListItem::ArtMap artMap = item.GetArt();
          CVariant artObj(CVariant::VariantTypeObject);
          for (const auto& artIt : artMap)
          {
            if (!artIt.second.empty())
              artObj[artIt.first] = CTextureUtils::GetWrappedImageURL(artIt.second);
          }
          result["albums"][index]["art"] = artObj;
        }
      }

      delete thumbLoader;
    }
  }

  int start, end;
  HandleLimits(parameterObject, result, total, start, end);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAlbumDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int albumID = (int)parameterObject["albumid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  if (!musicdatabase.GetAlbum(albumID, album, false))
    return InvalidParams;

  std::string path = StringUtils::Format("musicdb://albums/{}/", albumID);

  CFileItemPtr albumItem;
  FillAlbumItem(album, path, albumItem);

  CFileItemList items;
  items.Add(albumItem);
  JSONRPC_STATUS ret = GetAdditionalAlbumDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItem("albumid", false, "albumdetails", items[0], parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString("musicdb://songs/"))
    return InternalError;

  if (parameterObject["singlesonly"].asBoolean())
    musicUrl.AddOption("singles", true);
  else if (!parameterObject["includesingles"].asBoolean())
    musicUrl.AddOption("singles", false);

  bool allroles = false;
  if (parameterObject["allroles"].isBoolean())
    allroles = parameterObject["allroles"].asBoolean();

  const CVariant &filter = parameterObject["filter"];

  if (allroles)
    musicUrl.AddOption("roleid", -1000); //All roles, override implicit roleid=1 filter required for backward compatibility
  else if (filter.isMember("roleid"))
    musicUrl.AddOption("roleid", static_cast<int>(filter["roleid"].asInteger()));
  else if (filter.isMember("role"))
    musicUrl.AddOption("role", filter["role"].asString());
  // Only one of genreid/genre, artistid/artist, albumid/album or rules type filter is allowed by filter syntax
  if (filter.isMember("artistid"))
    musicUrl.AddOption("artistid", static_cast<int>(filter["artistid"].asInteger()));
  else if (filter.isMember("artist"))
    musicUrl.AddOption("artist", filter["artist"].asString());
  else if (filter.isMember("genreid"))
    musicUrl.AddOption("genreid", static_cast<int>(filter["genreid"].asInteger()));
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("albumid"))
    musicUrl.AddOption("albumid", static_cast<int>(filter["albumid"].asInteger()));
  else if (filter.isMember("album"))
    musicUrl.AddOption("album", filter["album"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("songs", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  int total;
  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array();
         field != parameterObject["properties"].end_array(); ++field)
      fields.insert(field->asString());
  }

  if (!musicdatabase.GetSongsByWhereJSON(fields, musicUrl.ToString(), result, total, sorting))
    return InternalError;

  if (!result.isNull())
  {
    bool bFetchArt = fields.find("art") != fields.end();
    bool bFetchFanart = fields.find("fanart") != fields.end();
    bool bFetchThumb = fields.find("thumbnail") != fields.end();
    if (bFetchArt || bFetchFanart || bFetchThumb)
    {
      CThumbLoader* thumbLoader = new CMusicThumbLoader();
      thumbLoader->OnLoaderStart();

      std::set<std::string> artfields;
      if (bFetchArt)
        artfields.insert("art");
      if (bFetchFanart)
        artfields.insert("fanart");
      if (bFetchThumb)
        artfields.insert("thumbnail");

      for (unsigned int index = 0; index < result["songs"].size(); index++)
      {
        CFileItem item;
        // Only needs song and album id (if we have it) set to get art
        // Getting art is quicker if "albumid" has been fetched
        item.GetMusicInfoTag()->SetDatabaseId(result["songs"][index]["songid"].asInteger32(), MediaTypeSong);
        if (result["songs"][index].isMember("albumid"))
          item.GetMusicInfoTag()->SetAlbumId(result["songs"][index]["albumid"].asInteger32());
        else
          item.GetMusicInfoTag()->SetAlbumId(-1);

        // Could use FillDetails, but it does unnecessary serialization of empty MusiInfoTag
        // CFileItemPtr itemptr(new CFileItem(item));
        // FillDetails(item.GetMusicInfoTag(), itemptr, artfields, result["songs"][index], thumbLoader);

        thumbLoader->FillLibraryArt(item);

        if (bFetchThumb)
        {
          if (item.HasArt("thumb"))
            result["songs"][index]["thumbnail"] = CTextureUtils::GetWrappedImageURL(item.GetArt("thumb"));
          else
            result["songs"][index]["thumbnail"] = "";
        }
        if (bFetchFanart)
        {
          if (item.HasArt("fanart"))
            result["songs"][index]["fanart"] = CTextureUtils::GetWrappedImageURL(item.GetArt("fanart"));
          else
            result["songs"][index]["fanart"] = "";
        }
        if (bFetchArt)
        {
          CGUIListItem::ArtMap artMap = item.GetArt();
          CVariant artObj(CVariant::VariantTypeObject);
          for (const auto& artIt : artMap)
          {
            if (!artIt.second.empty())
              artObj[artIt.first] = CTextureUtils::GetWrappedImageURL(artIt.second);
          }
          result["songs"][index]["art"] = artObj;
        }
      }

      delete thumbLoader;
    }
  }

  int start, end;
  HandleLimits(parameterObject, result, total, start, end);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetSongDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int idSong = (int)parameterObject["songid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSong(idSong, song))
    return InvalidParams;

  CFileItemList items;
  CFileItemPtr item = std::make_shared<CFileItem>(song);
  FillItemArtistIDs(song.GetArtistIDArray(), item);
  items.Add(item);

  JSONRPC_STATUS ret = GetAdditionalSongDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItem("songid", true, "songdetails", items[0], parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyAddedAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyAddedAlbums(albums))
    return InternalError;

  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    std::string path =
        StringUtils::Format("musicdb://recentlyaddedalbums/{}/", albums[index].idAlbum);

    CFileItemPtr item;
    FillAlbumItem(albums[index], path, item);
    items.Add(item);
  }

  JSONRPC_STATUS ret = GetAdditionalAlbumDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyAddedSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int amount = (int)parameterObject["albumlimit"].asInteger();
  if (amount < 0)
    amount = 0;

  CFileItemList items;
  if (!musicdatabase.GetRecentlyAddedAlbumSongs("musicdb://songs/", items, (unsigned int)amount))
    return InternalError;

  JSONRPC_STATUS ret = GetAdditionalSongDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItemList("songid", true, "songs", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedAlbums(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyPlayedAlbums(albums))
    return InternalError;

  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    std::string path =
        StringUtils::Format("musicdb://recentlyplayedalbums/{}/", albums[index].idAlbum);

    CFileItemPtr item;
    FillAlbumItem(albums[index], path, item);
    items.Add(item);
  }

  JSONRPC_STATUS ret = GetAdditionalAlbumDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedSongs(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!musicdatabase.GetRecentlyPlayedAlbumSongs("musicdb://songs/", items))
    return InternalError;

  JSONRPC_STATUS ret = GetAdditionalSongDetails(parameterObject, items, musicdatabase);
  if (ret != OK)
    return ret;

  HandleFileItemList("songid", true, "songs", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetGenres(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  // Check if sources for genre wanted
  bool sourcesneeded(false);
  std::set<std::string> checkProperties;
  checkProperties.insert("sourceid");
  std::set<std::string> additionalProperties;
  if (CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    sourcesneeded = (additionalProperties.find("sourceid") != additionalProperties.end());

  CFileItemList items;
  if (!musicdatabase.GetGenresJSON(items, sourcesneeded))
    return InternalError;

  HandleFileItemList("genreid", false, "genres", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRoles(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!musicdatabase.GetRolesNav("musicdb://songs/", items))
    return InternalError;

  /* need to set strTitle in each item*/
  for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    items[i]->GetMusicInfoTag()->SetTitle(items[i]->GetLabel());

  HandleFileItemList("roleid", false, "roles", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS JSONRPC::CAudioLibrary::GetSources(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  // Add "file" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  if (!param["properties"].isMember("file"))
    param["properties"].append("file");

  CFileItemList items;
  if (!musicdatabase.GetSources(items))
    return InternalError;

  HandleFileItemList("sourceid", true, "sources", items, param, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAvailableArtTypes(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result)
{
  std::string mediaType;
  int mediaID = -1;
  if (parameterObject["item"].isMember("albumid"))
  {
    mediaType = MediaTypeAlbum;
    mediaID = parameterObject["item"]["albumid"].asInteger32();
  }
  if (parameterObject["item"].isMember("artistid"))
  {
    mediaType = MediaTypeArtist;
    mediaID = parameterObject["item"]["artistid"].asInteger32();
  }
  if (mediaID == -1)
    return InternalError;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CVariant availablearttypes = CVariant(CVariant::VariantTypeArray);
  for (const auto& artType : musicdatabase.GetAvailableArtTypesForItem(mediaID, mediaType))
  {
    availablearttypes.append(artType);
  }
  result = CVariant(CVariant::VariantTypeObject);
  result["availablearttypes"] = availablearttypes;

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAvailableArt(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result)
{
  std::string mediaType;
  int mediaID = -1;
  if (parameterObject["item"].isMember("albumid"))
  {
    mediaType = MediaTypeAlbum;
    mediaID = parameterObject["item"]["albumid"].asInteger32();
  }
  if (parameterObject["item"].isMember("artistid"))
  {
    mediaType = MediaTypeArtist;
    mediaID = parameterObject["item"]["artistid"].asInteger32();
  }
  if (mediaID == -1)
    return InternalError;

  std::string artType = parameterObject["arttype"].asString();
  StringUtils::ToLower(artType);

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CVariant availableart = CVariant(CVariant::VariantTypeArray);
  for (const auto& artentry : musicdatabase.GetAvailableArtForItem(mediaID, mediaType, artType))
  {
    CVariant item = CVariant(CVariant::VariantTypeObject);
    item["url"] = CTextureUtils::GetWrappedImageURL(artentry.m_url);
    item["arttype"] = artentry.m_aspect;
    if (!artentry.m_preview.empty())
      item["previewurl"] = CTextureUtils::GetWrappedImageURL(artentry.m_preview);
    availableart.append(item);
  }
  result = CVariant(CVariant::VariantTypeObject);
  result["availableart"] = availableart;

  return OK;
}

JSONRPC_STATUS CAudioLibrary::SetArtistDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["artistid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CArtist artist;
  if (!musicdatabase.GetArtist(id, artist) || artist.idArtist <= 0)
    return InvalidParams;

  if (ParameterNotNull(parameterObject, "artist"))
    artist.strArtist = parameterObject["artist"].asString();
  if (ParameterNotNull(parameterObject, "instrument"))
    CopyStringArray(parameterObject["instrument"], artist.instruments);
  if (ParameterNotNull(parameterObject, "style"))
    CopyStringArray(parameterObject["style"], artist.styles);
  if (ParameterNotNull(parameterObject, "mood"))
    CopyStringArray(parameterObject["mood"], artist.moods);
  if (ParameterNotNull(parameterObject, "born"))
    artist.strBorn = parameterObject["born"].asString();
  if (ParameterNotNull(parameterObject, "formed"))
    artist.strFormed = parameterObject["formed"].asString();
  if (ParameterNotNull(parameterObject, "description"))
    artist.strBiography = parameterObject["description"].asString();
  if (ParameterNotNull(parameterObject, "genre"))
    CopyStringArray(parameterObject["genre"], artist.genre);
  if (ParameterNotNull(parameterObject, "died"))
    artist.strDied = parameterObject["died"].asString();
  if (ParameterNotNull(parameterObject, "disbanded"))
    artist.strDisbanded = parameterObject["disbanded"].asString();
  if (ParameterNotNull(parameterObject, "yearsactive"))
    CopyStringArray(parameterObject["yearsactive"], artist.yearsActive);
  if (ParameterNotNull(parameterObject, "musicbrainzartistid"))
    artist.strMusicBrainzArtistID = parameterObject["musicbrainzartistid"].asString();
  if (ParameterNotNull(parameterObject, "sortname"))
    artist.strSortName = parameterObject["sortname"].asString();
  if (ParameterNotNull(parameterObject, "type"))
    artist.strType = parameterObject["type"].asString();
  if (ParameterNotNull(parameterObject, "gender"))
    artist.strGender = parameterObject["gender"].asString();
  if (ParameterNotNull(parameterObject, "disambiguation"))
    artist.strDisambiguation = parameterObject["disambiguation"].asString();

  // Update existing art. Any existing artwork that isn't specified in this request stays as is.
  // If the value is null then the existing art with that type is removed.
  if (ParameterNotNull(parameterObject, "art"))
  {
    // Get current artwork
    musicdatabase.GetArtForItem(artist.idArtist, MediaTypeArtist, artist.art);

    std::set<std::string> removedArtwork;
    CVariant art = parameterObject["art"];
    for (CVariant::const_iterator_map artIt = art.begin_map(); artIt != art.end_map(); ++artIt)
    {
      if (artIt->second.isString() && !artIt->second.asString().empty())
        artist.art[artIt->first] = CTextureUtils::UnwrapImageURL(artIt->second.asString());
      else if (artIt->second.isNull())
      {
        artist.art.erase(artIt->first);
        removedArtwork.insert(artIt->first);
      }
    }
    // Remove null art now, as not done by update
    if (!musicdatabase.RemoveArtForItem(artist.idArtist, MediaTypeArtist, removedArtwork))
      return InternalError;
  }

  // Update artist including adding or replacing (but not removing) art
  if (!musicdatabase.UpdateArtist(artist))
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::SetAlbumDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["albumid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  // Get current album details, but not songs as we do not want to update them here
  if (!musicdatabase.GetAlbum(id, album, false) || album.idAlbum <= 0)
    return InvalidParams;

  if (ParameterNotNull(parameterObject, "title"))
    album.strAlbum = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "displayartist"))
    album.strArtistDesc = parameterObject["displayartist"].asString();
  // Set album sort string before processing artist credits
  if (ParameterNotNull(parameterObject, "sortartist"))
    album.strArtistSort = parameterObject["sortartist"].asString();

  // Match up artist names and mbids to make new artist credits
  // Mbid values only apply if there are names
  if (ParameterNotNull(parameterObject, "artist"))
  {
    std::vector<std::string> artists;
    std::vector<std::string> mbids;
    CopyStringArray(parameterObject["artist"], artists);
    // Check for Musicbrainz ids
    if (ParameterNotNull(parameterObject, "musicbrainzalbumartistid"))
      CopyStringArray(parameterObject["musicbrainzalbumartistid"], mbids);
    // When display artist is not provided and yet artists is changing make by concatenation
    if (!ParameterNotNull(parameterObject, "displayartist"))
      album.strArtistDesc = StringUtils::Join(artists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    album.SetArtistCredits(artists, std::vector<std::string>(), mbids);
    // On updatealbum artists will be changed
    album.bArtistSongMerge = true;
  }

  if (ParameterNotNull(parameterObject, "description"))
    album.strReview = parameterObject["description"].asString();
  if (ParameterNotNull(parameterObject, "genre"))
    CopyStringArray(parameterObject["genre"], album.genre);
  if (ParameterNotNull(parameterObject, "theme"))
    CopyStringArray(parameterObject["theme"], album.themes);
  if (ParameterNotNull(parameterObject, "mood"))
    CopyStringArray(parameterObject["mood"], album.moods);
  if (ParameterNotNull(parameterObject, "style"))
    CopyStringArray(parameterObject["style"], album.styles);
  if (ParameterNotNull(parameterObject, "type"))
    album.strType = parameterObject["type"].asString();
  if (ParameterNotNull(parameterObject, "albumlabel"))
    album.strLabel = parameterObject["albumlabel"].asString();
  if (ParameterNotNull(parameterObject, "rating"))
    album.fRating = parameterObject["rating"].asFloat();
  if (ParameterNotNull(parameterObject, "userrating"))
    album.iUserrating = static_cast<int>(parameterObject["userrating"].asInteger());
  if (ParameterNotNull(parameterObject, "votes"))
    album.iVotes = static_cast<int>(parameterObject["votes"].asInteger());
  if (ParameterNotNull(parameterObject, "year"))
    album.strReleaseDate = parameterObject["year"].asString();
  if (ParameterNotNull(parameterObject, "musicbrainzalbumid"))
    album.strMusicBrainzAlbumID = parameterObject["musicbrainzalbumid"].asString();
  if (ParameterNotNull(parameterObject, "musicbrainzreleasegroupid"))
    album.strReleaseGroupMBID = parameterObject["musicbrainzreleasegroupid"].asString();
  if (ParameterNotNull(parameterObject, "isboxset"))
    album.bBoxedSet = parameterObject["isboxset"].asBoolean();
  if (ParameterNotNull(parameterObject, "originaldate"))
    album.strOrigReleaseDate = parameterObject["originaldate"].asString();
  if (ParameterNotNull(parameterObject, "releasedate"))
    album.strReleaseDate = parameterObject["releasedate"].asString();
  if (ParameterNotNull(parameterObject, "albumstatus"))
    album.strReleaseStatus = parameterObject["albumstatus"].asString();

  // Update existing art. Any existing artwork that isn't specified in this request stays as is.
  // If the value is null then the existing art with that type is removed.
  if (ParameterNotNull(parameterObject, "art"))
  {
    // Get current artwork
    musicdatabase.GetArtForItem(album.idAlbum, MediaTypeAlbum, album.art);

    std::set<std::string> removedArtwork;
    CVariant art = parameterObject["art"];
    for (CVariant::const_iterator_map artIt = art.begin_map(); artIt != art.end_map(); ++artIt)
    {
      if (artIt->second.isString() && !artIt->second.asString().empty())
        album.art[artIt->first] = CTextureUtils::UnwrapImageURL(artIt->second.asString());
      else if (artIt->second.isNull())
      {
        album.art.erase(artIt->first);
        removedArtwork.insert(artIt->first);
      }
    }
    // Remove null art now, as not done by update
    if (!musicdatabase.RemoveArtForItem(album.idAlbum, MediaTypeAlbum, removedArtwork))
      return InternalError;
  }

  // Update artist including adding or replacing (but not removing) art
  if (!musicdatabase.UpdateAlbum(album))
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::SetSongDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["songid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSong(id, song) || song.idSong != id)
    return InvalidParams;

  if (ParameterNotNull(parameterObject, "title"))
    song.strTitle = parameterObject["title"].asString();

  if (ParameterNotNull(parameterObject, "displayartist"))
    song.strArtistDesc = parameterObject["displayartist"].asString();
  // Set album sort string before processing artist credits
  if (ParameterNotNull(parameterObject, "sortartist"))
    song.strArtistSort = parameterObject["sortartist"].asString();

  // Match up artist names and mbids to make new artist credits
  // Mbid values only apply if there are names
  bool updateartists = false;
  if (ParameterNotNull(parameterObject, "artist"))
  {
    std::vector<std::string> artists, mbids;
    updateartists = true;
    CopyStringArray(parameterObject["artist"], artists);
    // Check for Musicbrainz ids
    if (ParameterNotNull(parameterObject, "musicbrainzartistid"))
      CopyStringArray(parameterObject["musicbrainzartistid"], mbids);
    // When display artist is not provided and yet artists is changing make by concatenation
    if (!ParameterNotNull(parameterObject, "displayartist"))
      song.strArtistDesc = StringUtils::Join(artists, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    song.SetArtistCredits(artists, std::vector<std::string>(), mbids);
  }

  if (ParameterNotNull(parameterObject, "genre"))
    CopyStringArray(parameterObject["genre"], song.genre);
  if (ParameterNotNull(parameterObject, "year"))
    song.strReleaseDate = parameterObject["year"].asString();
  if (ParameterNotNull(parameterObject, "rating"))
    song.rating = parameterObject["rating"].asFloat();
  if (ParameterNotNull(parameterObject, "userrating"))
    song.userrating = static_cast<int>(parameterObject["userrating"].asInteger());
  if (ParameterNotNull(parameterObject, "track"))
    song.iTrack = (song.iTrack & 0xffff0000) | ((int)parameterObject["track"].asInteger() & 0xffff);
  if (ParameterNotNull(parameterObject, "disc"))
    song.iTrack = (song.iTrack & 0xffff) | ((int)parameterObject["disc"].asInteger() << 16);
  if (ParameterNotNull(parameterObject, "duration"))
    song.iDuration = (int)parameterObject["duration"].asInteger();
  if (ParameterNotNull(parameterObject, "comment"))
    song.strComment = parameterObject["comment"].asString();
  if (ParameterNotNull(parameterObject, "musicbrainztrackid"))
    song.strMusicBrainzTrackID = parameterObject["musicbrainztrackid"].asString();
  if (ParameterNotNull(parameterObject, "playcount"))
    song.iTimesPlayed = static_cast<int>(parameterObject["playcount"].asInteger());
  if (ParameterNotNull(parameterObject, "lastplayed"))
    song.lastPlayed.SetFromDBDateTime(parameterObject["lastplayed"].asString());
  if (ParameterNotNull(parameterObject, "mood"))
    song.strMood = parameterObject["mood"].asString();
  if (ParameterNotNull(parameterObject, "disctitle"))
    song.strDiscSubtitle = parameterObject["disctitle"].asString();
  if (ParameterNotNull(parameterObject, "bpm"))
    song.iBPM = static_cast<int>(parameterObject["bpm"].asInteger());
  if (ParameterNotNull(parameterObject, "originaldate"))
    song.strOrigReleaseDate = parameterObject["originaldate"].asString();
  if (ParameterNotNull(parameterObject, "albumreleasedate"))
    song.strReleaseDate = parameterObject["albumreleasedate"].asString();
  if (ParameterNotNull(parameterObject, "songvideourl"))
    song.songVideoURL = parameterObject["songvideourl"].asString();

  // Update existing art. Any existing artwork that isn't specified in this request stays as is.
  // If the value is null then the existing art with that type is removed.
  if (ParameterNotNull(parameterObject, "art"))
  {
    // Get current artwork
    std::map<std::string, std::string> artwork;
    musicdatabase.GetArtForItem(song.idSong, MediaTypeSong, artwork);

    std::set<std::string> removedArtwork;
    CVariant art = parameterObject["art"];
    for (CVariant::const_iterator_map artIt = art.begin_map(); artIt != art.end_map(); ++artIt)
    {
      if (artIt->second.isString() && !artIt->second.asString().empty())
        artwork[artIt->first] = CTextureUtils::UnwrapImageURL(artIt->second.asString());
      else if (artIt->second.isNull())
      {
        artwork.erase(artIt->first);
        removedArtwork.insert(artIt->first);
      }
    }
    //Update artwork, not done in update song
    musicdatabase.SetArtForItem(song.idSong, MediaTypeSong, artwork);
    if (!musicdatabase.RemoveArtForItem(song.idSong, MediaTypeSong, removedArtwork))
      return InternalError;
  }

  // Update song (not including artwork)
  if (!musicdatabase.UpdateSong(song, updateartists))
    return InternalError;

  const auto item = std::make_shared<CFileItem>(song);
  CJSONRPCUtils::NotifyItemUpdated(item);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  std::string cmd =
      StringUtils::Format("updatelibrary(music, {}, {})", StringUtils::Paramify(directory),
                          parameterObject["showdialogs"].asBoolean() ? "true" : "false");

  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Export(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string cmd;
  if (parameterObject["options"].isMember("path"))
    cmd = StringUtils::Format("exportlibrary2(music, singlefile, {}, albums, albumartists)",
                              StringUtils::Paramify(parameterObject["options"]["path"].asString()));
  else
  {
    cmd = "exportlibrary2(music, library, dummy, albums, albumartists";
    if (parameterObject["options"]["images"].isBoolean() &&
        parameterObject["options"]["images"].asBoolean() == true)
      cmd += ", artwork";
    if (parameterObject["options"]["overwrite"].isBoolean() &&
        parameterObject["options"]["overwrite"].asBoolean() == true)
      cmd += ", overwrite";
    cmd += ")";
  }
  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Clean(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string cmd = StringUtils::Format(
      "cleanlibrary(music, {})", parameterObject["showdialogs"].asBoolean() ? "true" : "false");
  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

bool CAudioLibrary::FillFileItem(
    const std::string& strFilename,
    std::shared_ptr<CFileItem>& item,
    const CVariant& parameterObject /* = CVariant(CVariant::VariantTypeArray) */)
{
  CMusicDatabase musicdatabase;
  if (strFilename.empty())
    return false;

  bool filled = false;
  if (musicdatabase.Open())
  {
    if (CDirectory::Exists(strFilename))
    {
      CAlbum album;
      int albumid = musicdatabase.GetAlbumIdByPath(strFilename);
      if (musicdatabase.GetAlbum(albumid, album, false))
      {
        item->SetFromAlbum(album);
        FillItemArtistIDs(album.GetArtistIDArray(), item);

        CFileItemList items;
        items.Add(item);

        if (GetAdditionalAlbumDetails(parameterObject, items, musicdatabase) == OK)
          filled = true;
      }
    }
    else
    {
      CSong song;
      if (musicdatabase.GetSongByFileName(strFilename, song))
      {
        item->SetFromSong(song);
        FillItemArtistIDs(song.GetArtistIDArray(), item);

        CFileItemList items;
        items.Add(item);
        if (GetAdditionalSongDetails(parameterObject, items, musicdatabase) == OK)
          filled = true;
      }
    }
  }

  if (item->GetLabel().empty())
  {
    item->SetLabel(CUtil::GetTitleFromPath(strFilename, false));
    if (item->GetLabel().empty())
      item->SetLabel(URIUtils::GetFileName(strFilename));
  }

  return filled;
}

bool CAudioLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  std::string file = parameterObject["file"].asString();
  int artistID = (int)parameterObject["artistid"].asInteger(-1);
  int albumID = (int)parameterObject["albumid"].asInteger(-1);
  int genreID = (int)parameterObject["genreid"].asInteger(-1);

  bool success = false;
  CFileItemPtr fileItem(new CFileItem());
  if (FillFileItem(file, fileItem, parameterObject))
  {
    success = true;
    list.Add(fileItem);
  }

  if (artistID != -1 || albumID != -1 || genreID != -1)
    success |= musicdatabase.GetSongsNav("musicdb://songs/", list, genreID, artistID, albumID);

  int songID = (int)parameterObject["songid"].asInteger(-1);
  if (songID != -1)
  {
    CSong song;
    if (musicdatabase.GetSong(songID, song))
    {
      list.Add(std::make_shared<CFileItem>(song));
      success = true;
    }
  }

  if (success)
  {
    // If we retrieved the list of songs by "artistid"
    // we sort by album (and implicitly by track number)
    if (artistID != -1)
      list.Sort(SortByAlbum, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
    // If we retrieve the list of songs by "genreid"
    // we sort by artist (and implicitly by album and track number)
    else if (genreID != -1)
      list.Sort(SortByArtist, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
    // otherwise we sort by track number
    else
      list.Sort(SortByTrackNumber, SortOrderAscending);

  }

  return success;
}

void CAudioLibrary::FillItemArtistIDs(const std::vector<int>& artistids,
                                      std::shared_ptr<CFileItem>& item)
{
  // Add artistIds as separate property as not part of CMusicInfoTag
  CVariant artistidObj(CVariant::VariantTypeArray);
  for (const auto& artistid : artistids)
    artistidObj.push_back(artistid);

  item->SetProperty("artistid", artistidObj);
}

void CAudioLibrary::FillAlbumItem(const CAlbum& album,
                                  const std::string& path,
                                  std::shared_ptr<CFileItem>& item)
{
  item = std::make_shared<CFileItem>(path, album);
  // Add album artistIds as separate property as not part of CMusicInfoTag
  std::vector<int> artistids = album.GetArtistIDArray();
  FillItemArtistIDs(artistids, item);
}

JSONRPC_STATUS CAudioLibrary::GetAdditionalDetails(const CVariant &parameterObject, CFileItemList &items)
{
  if (items.IsEmpty())
    return OK;

  CMusicDatabase musicdb;
  if (CMediaTypes::IsMediaType(items.GetContent(), MediaTypeArtist))
    return GetAdditionalArtistDetails(parameterObject, items, musicdb);
  else if (CMediaTypes::IsMediaType(items.GetContent(), MediaTypeAlbum))
    return GetAdditionalAlbumDetails(parameterObject, items, musicdb);
  else if (CMediaTypes::IsMediaType(items.GetContent(), MediaTypeSong))
    return GetAdditionalSongDetails(parameterObject, items, musicdb);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAdditionalArtistDetails(const CVariant& parameterObject,
                                                         const CFileItemList& items,
                                                         CMusicDatabase& musicdatabase)
{
  if (!musicdatabase.Open())
    return InternalError;

  std::set<std::string> checkProperties;
  checkProperties.insert("roles");
  checkProperties.insert("songgenres");
  checkProperties.insert("isalbumartist");
  checkProperties.insert("sourceid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;

  if (additionalProperties.find("roles") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetRolesByArtist(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }
  if (additionalProperties.find("songgenres") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetGenresByArtist(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }
  if (additionalProperties.find("isalbumartist") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetIsAlbumArtist(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }
  if (additionalProperties.find("sourceid") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetSourcesByArtist(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAdditionalAlbumDetails(const CVariant& parameterObject,
                                                        const CFileItemList& items,
                                                        CMusicDatabase& musicdatabase)
{
  if (!musicdatabase.Open())
    return InternalError;

  std::set<std::string> checkProperties;
  checkProperties.insert("songgenres");
  checkProperties.insert("sourceid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;

  if (additionalProperties.find("songgenres") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetGenresByAlbum(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }
  if (additionalProperties.find("sourceid") != additionalProperties.end())
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      musicdatabase.GetSourcesByAlbum(item->GetMusicInfoTag()->GetDatabaseId(), item.get());
    }
  }

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAdditionalSongDetails(const CVariant& parameterObject,
                                                       const CFileItemList& items,
                                                       CMusicDatabase& musicdatabase)
{
  if (!musicdatabase.Open())
    return InternalError;

  std::set<std::string> checkProperties;
  checkProperties.insert("genreid");
  checkProperties.insert("sourceid");
  // Query (songview join songartistview) returns song.strAlbumArtists = CMusicInfoTag.m_strAlbumArtistDesc only
  // Actual album artist data, if required,  comes from album_artist and artist tables.
  // It may differ from just splitting album artist description string
  checkProperties.insert("albumartist");
  checkProperties.insert("albumartistid");
  checkProperties.insert("musicbrainzalbumartistid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    if (additionalProperties.find("genreid") != additionalProperties.end())
    {
      std::vector<int> genreids;
      if (musicdatabase.GetGenresBySong(item->GetMusicInfoTag()->GetDatabaseId(), genreids))
      {
        CVariant genreidObj(CVariant::VariantTypeArray);
        for (const auto& genreid : genreids)
          genreidObj.push_back(genreid);

        item->SetProperty("genreid", genreidObj);
      }
    }
    if (additionalProperties.find("sourceid") != additionalProperties.end())
    {
      musicdatabase.GetSourcesBySong(item->GetMusicInfoTag()->GetDatabaseId(), item->GetPath(), item.get());
    }
    if (item->GetMusicInfoTag()->GetAlbumId() > 0)
    {
      if (additionalProperties.find("albumartist") != additionalProperties.end() ||
          additionalProperties.find("albumartistid") != additionalProperties.end() ||
          additionalProperties.find("musicbrainzalbumartistid") != additionalProperties.end())
      {
        musicdatabase.GetArtistsByAlbum(item->GetMusicInfoTag()->GetAlbumId(), item.get());
      }
    }
  }

  return OK;
}

bool CAudioLibrary::CheckForAdditionalProperties(const CVariant &properties, const std::set<std::string> &checkProperties, std::set<std::string> &foundProperties)
{
  if (!properties.isArray() || properties.empty())
    return false;

  std::set<std::string> checkingProperties = checkProperties;
  for (CVariant::const_iterator_array itr = properties.begin_array();
       itr != properties.end_array() && !checkingProperties.empty(); ++itr)
  {
    if (!itr->isString())
      continue;

    std::string property = itr->asString();
    if (checkingProperties.find(property) != checkingProperties.end())
    {
      checkingProperties.erase(property);
      foundProperties.insert(property);
    }
  }

  return !foundProperties.empty();
}
