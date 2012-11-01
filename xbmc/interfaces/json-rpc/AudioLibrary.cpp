/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "AudioLibrary.h"
#include "music/MusicDatabase.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "music/Song.h"
#include "music/Artist.h"
#include "ApplicationMessenger.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

JSONRPC_STATUS CAudioLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  musicUrl.FromString("musicdb://2/");
  int genreID = -1, albumID = -1, songID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("albumid"))
    albumID = (int)filter["albumid"].asInteger();
  else if (filter.isMember("album"))
    musicUrl.AddOption("album", filter["album"].asString());
  else if (filter.isMember("songid"))
    songID = (int)filter["songid"].asInteger();
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("artists", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  bool albumArtistsOnly = !g_guiSettings.GetBool("musiclibrary.showcompilationartists");
  if (parameterObject["albumartistsonly"].isBoolean())
    albumArtistsOnly = parameterObject["albumartistsonly"].asBoolean();

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CFileItemList items;
  if (!musicdatabase.GetArtistsNav(musicUrl.ToString(), items, albumArtistsOnly, genreID, albumID, songID, CDatabase::Filter(), sorting))
    return InternalError;

  // Add "artist" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  param["properties"].append("artist");

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("artistid", false, "artists", items, param, result, size, false);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetArtistDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString("musicdb://2/"))
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

  HandleFileItem("artistid", false, "artistdetails", items[0], parameterObject, param["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  musicUrl.FromString("musicdb://3/");
  int artistID = -1, genreID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("artistid"))
    artistID = (int)filter["artistid"].asInteger();
  else if (filter.isMember("artist"))
    musicUrl.AddOption("artist", filter["artist"].asString());
  else if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CFileItemList items;
  if (!musicdatabase.GetAlbumsNav(musicUrl.ToString(), items, genreID, artistID, CDatabase::Filter(), sorting))
    return InternalError;

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result, size, false);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int albumID = (int)parameterObject["albumid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  if (!musicdatabase.GetAlbumInfo(albumID, album, NULL))
    return InvalidParams;

  CStdString path;
  if (!musicdatabase.GetAlbumPath(albumID, path))
    return InternalError;

  CFileItemPtr m_albumItem;
  FillAlbumItem(album, path, m_albumItem);
  HandleFileItem("albumid", false, "albumdetails", m_albumItem, parameterObject, parameterObject["properties"], result, false);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CMusicDbUrl musicUrl;
  musicUrl.FromString("musicdb://4/");
  int genreID = -1, albumID = -1, artistID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("artistid"))
    artistID = (int)filter["artistid"].asInteger();
  else if (filter.isMember("artist"))
    musicUrl.AddOption("artist", filter["artist"].asString());
  else if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    musicUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("albumid"))
    albumID = (int)filter["albumid"].asInteger();
  else if (filter.isMember("album"))
    musicUrl.AddOption("album", filter["album"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("songs", filter, xsp))
      return InvalidParams;

    musicUrl.AddOption("xsp", xsp);
  }

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CFileItemList items;
  if (!musicdatabase.GetSongsNav(musicUrl.ToString(), items, genreID, artistID, albumID, sorting))
    return InternalError;

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("songid", true, "songs", items, parameterObject, result, size, false);

  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int idSong = (int)parameterObject["songid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSongById(idSong, song))
    return InvalidParams;

  HandleFileItem("songid", false, "songdetails", CFileItemPtr( new CFileItem(song) ), parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyAddedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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
    CStdString path;
    path.Format("musicdb://6/%i/", albums[index].idAlbum);

    CFileItemPtr item;
    FillAlbumItem(albums[index], path, item);
    items.Add(item);
  }

  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyAddedSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int amount = (int)parameterObject["albumlimit"].asInteger();
  if (amount < 0)
    amount = 0;

  CFileItemList items;
  if (!musicdatabase.GetRecentlyAddedAlbumSongs("musicdb://", items, (unsigned int)amount))
    return InternalError;

  HandleFileItemList("songid", true, "songs", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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
    CStdString path;
    path.Format("musicdb://7/%i/", albums[index].idAlbum);

    CFileItemPtr item;
    FillAlbumItem(albums[index], path, item);
    items.Add(item);
  }

  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!musicdatabase.GetRecentlyPlayedAlbumSongs("musicdb://", items))
    return InternalError;

  HandleFileItemList("songid", true, "songs", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!musicdatabase.GetGenresNav("musicdb://1/", items))
    return InternalError;

  /* need to set strTitle in each item*/
  for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    items[i]->GetMusicInfoTag()->SetTitle(items[i]->GetLabel());

  HandleFileItemList("genreid", false, "genres", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CAudioLibrary::SetArtistDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["artistid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CArtist artist;
  if (!musicdatabase.GetArtistInfo(id, artist) || artist.idArtist <= 0)
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

  if (musicdatabase.SetArtistInfo(id, artist) <= 0)
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::SetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["albumid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  VECSONGS songs;
  if (!musicdatabase.GetAlbumInfo(id, album, &songs) || album.idAlbum <= 0)
    return InvalidParams;

  if (ParameterNotNull(parameterObject, "title"))
    album.strAlbum = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "artist"))
    CopyStringArray(parameterObject["artist"], album.artist);
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
    album.iRating = (int)parameterObject["rating"].asInteger();
  if (ParameterNotNull(parameterObject, "year"))
    album.iYear = (int)parameterObject["year"].asInteger();

  if (musicdatabase.SetAlbumInfo(id, album, songs) <= 0)
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::SetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["songid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSongById(id, song) || song.idSong != id)
    return InvalidParams;

  if (ParameterNotNull(parameterObject, "title"))
    song.strTitle = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "artist"))
    CopyStringArray(parameterObject["artist"], song.artist);
  if (ParameterNotNull(parameterObject, "albumartist"))
    CopyStringArray(parameterObject["albumartist"], song.albumArtist);
  if (ParameterNotNull(parameterObject, "genre"))
    CopyStringArray(parameterObject["genre"], song.genre);
  if (ParameterNotNull(parameterObject, "year"))
    song.iYear = (int)parameterObject["year"].asInteger();
  if (ParameterNotNull(parameterObject, "rating"))
    song.rating = '0' + (char)parameterObject["rating"].asInteger();
  if (ParameterNotNull(parameterObject, "album"))
    song.strAlbum = parameterObject["album"].asString();
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
  if (ParameterNotNull(parameterObject, "musicbrainzartistid"))
    song.strMusicBrainzArtistID = parameterObject["musicbrainzartistid"].asString();
  if (ParameterNotNull(parameterObject, "musicbrainzalbumid"))
    song.strMusicBrainzAlbumID = parameterObject["musicbrainzalbumid"].asString();
  if (ParameterNotNull(parameterObject, "musicbrainzalbumartistid"))
    song.strMusicBrainzAlbumArtistID = parameterObject["musicbrainzalbumartistid"].asString();

  if (musicdatabase.UpdateSong(song, id) <= 0)
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  CStdString cmd;
  if (directory.empty())
    cmd = "updatelibrary(music)";
  else
    cmd.Format("updatelibrary(music, %s)", directory.c_str());

  CApplicationMessenger::Get().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString cmd;
  if (parameterObject["options"].isMember("path"))
    cmd.Format("exportlibrary(music, false, %s)", parameterObject["options"]["path"].asString());
  else
    cmd.Format("exportlibrary(music, true, %s, %s)",
      parameterObject["options"]["images"].asBoolean() ? "true" : "false",
      parameterObject["options"]["overwrite"].asBoolean() ? "true" : "false");

  CApplicationMessenger::Get().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CApplicationMessenger::Get().ExecBuiltIn("cleanlibrary(music)");
  return ACK;
}

bool CAudioLibrary::FillFileItem(const CStdString &strFilename, CFileItem &item)
{
  CMusicDatabase musicdatabase;
  if (strFilename.empty() || !musicdatabase.Open())
    return false;

  if (CDirectory::Exists(strFilename))
  {
    CAlbum album;
    int albumid = musicdatabase.GetAlbumIdByPath(strFilename);
    if (!musicdatabase.GetAlbumInfo(albumid, album, NULL))
      return false;

    item.SetFromAlbum(album);
  }
  else
  {
    CSong song;
    if (!musicdatabase.GetSongByFileName(strFilename, song))
      return false;

    item.SetFromSong(song);
  }

  return true;
}

bool CAudioLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CStdString file = parameterObject["file"].asString();
  int artistID = (int)parameterObject["artistid"].asInteger(-1);
  int albumID = (int)parameterObject["albumid"].asInteger(-1);
  int genreID = (int)parameterObject["genreid"].asInteger(-1);

  bool success = false;
  CFileItem fileItem;
  if (FillFileItem(file, fileItem))
  {
    success = true;
    list.Add(CFileItemPtr(new CFileItem(fileItem)));
  }

  if (artistID != -1 || albumID != -1 || genreID != -1)
    success |= musicdatabase.GetSongsNav("musicdb://4/", list, genreID, artistID, albumID);

  int songID = (int)parameterObject["songid"].asInteger(-1);
  if (songID != -1)
  {
    CSong song;
    if (musicdatabase.GetSongById(songID, song))
    {
      list.Add(CFileItemPtr(new CFileItem(song)));
      success = true;
    }
  }

  if (success)
  {
    // If we retrieved the list of songs by "artistid"
    // we sort by album (and implicitly by track number)
    if (artistID != -1)
      list.Sort(SORT_METHOD_ALBUM_IGNORE_THE, SortOrderAscending);
    // If we retrieve the list of songs by "genreid"
    // we sort by artist (and implicitly by album and track number)
    else if (genreID != -1)
      list.Sort(SORT_METHOD_ARTIST_IGNORE_THE, SortOrderAscending);
    // otherwise we sort by track number
    else
      list.Sort(SORT_METHOD_TRACKNUM, SortOrderAscending);

  }

  return success;
}

void CAudioLibrary::FillAlbumItem(const CAlbum &album, const CStdString &path, CFileItemPtr &item)
{
  item = CFileItemPtr(new CFileItem(path, album));
}
