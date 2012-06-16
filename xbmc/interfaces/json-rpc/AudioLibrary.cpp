/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#include "Application.h"
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

  int genreID = (int)parameterObject["genreid"].asInteger();

  // Add "artist" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  param["properties"].append("artist");

  bool albumArtistsOnly = !g_guiSettings.GetBool("musiclibrary.showcompilationartists");
  if (parameterObject["albumartistsonly"].isBoolean())
    albumArtistsOnly = parameterObject["albumartistsonly"].asBoolean();

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("musicdb://2/", items, genreID, albumArtistsOnly))
    HandleFileItemList("artistid", false, "artists", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetArtistDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  CStdString where;
  where.Format("idArtist = %d", artistID);
  if (!musicdatabase.GetArtistsByWhere("musicdb://2/", where, items) || items.Size() != 1)
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  HandleFileItem("artistid", false, "artistdetails", items[0], parameterObject, parameterObject["properties"], result, false);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID  = (int)parameterObject["artistid"].asInteger();
  int genreID   = (int)parameterObject["genreid"].asInteger();

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CFileItemList items;
  if (!musicdatabase.GetAlbumsNav("musicdb://3/", items, genreID, artistID, -1, -1, sorting))
    return InternalError;

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result, size, false);

  musicdatabase.Close();
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
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  CStdString path;
  musicdatabase.GetAlbumPath(albumID, path);

  CFileItemPtr m_albumItem;
  FillAlbumItem(album, path, m_albumItem);
  HandleFileItem("albumid", false, "albumdetails", m_albumItem, parameterObject, parameterObject["properties"], result, false);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = (int)parameterObject["artistid"].asInteger();
  int albumID  = (int)parameterObject["albumid"].asInteger();
  int genreID  = (int)parameterObject["genreid"].asInteger();

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CFileItemList items;
  if (!musicdatabase.GetSongsNav("musicdb://4/", items, genreID, artistID, albumID, sorting))
    InternalError;

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("songid", true, "songs", items, parameterObject, result, size, false);

  musicdatabase.Close();
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
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  HandleFileItem("songid", false, "songdetails", CFileItemPtr( new CFileItem(song) ), parameterObject, parameterObject["properties"], result, false);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyAddedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  VECALBUMS albums;
  if (musicdatabase.GetRecentlyAddedAlbums(albums))
  {
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
  }

  musicdatabase.Close();
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
  if (musicdatabase.GetRecentlyAddedAlbumSongs("musicdb://", items, (unsigned int)amount))
    HandleFileItemList("songid", true, "songs", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  VECALBUMS albums;
  if (musicdatabase.GetRecentlyPlayedAlbums(albums))
  {
    CFileItemList items;

    for (unsigned int index = 0; index < albums.size(); index++)
    {
      CStdString path;
      path.Format("musicdb://8/%i/", albums[index].idAlbum);

      CFileItemPtr item;
      FillAlbumItem(albums[index], path, item);
      items.Add(item);
    }

    HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  }

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetRecentlyPlayedSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (musicdatabase.GetRecentlyPlayedAlbumSongs("musicdb://", items))
    HandleFileItemList("songid", true, "songs", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (musicdatabase.GetGenresNav("musicdb://1/", items))
  {
    /* need to set strTitle in each item*/
    for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
      items[i]->GetMusicInfoTag()->SetTitle(items[i]->GetLabel());

    HandleFileItemList("genreid", false, "genres", items, parameterObject, result);
  }

  musicdatabase.Close();
  return OK;
}

JSONRPC_STATUS CAudioLibrary::SetArtistDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["artistid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CArtist artist;
  musicdatabase.GetArtistInfo(id, artist);
  if (artist.idArtist <= 0)
  {
    musicdatabase.Close();
    return InvalidParams;
  }

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

  JSONRPC_STATUS status;
  if (musicdatabase.SetArtistInfo(id, artist) > 0)
    status = ACK;
  else
    status = InternalError;

  musicdatabase.Close();
  return status;
}

JSONRPC_STATUS CAudioLibrary::SetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["albumid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  VECSONGS songs;
  musicdatabase.GetAlbumInfo(id, album, &songs);
  if (album.idAlbum <= 0)
  {
    musicdatabase.Close();
    return InvalidParams;
  }

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

  JSONRPC_STATUS status;
  if (musicdatabase.SetAlbumInfo(id, album, songs) > 0)
    status = ACK;
  else
    status = InternalError;

  musicdatabase.Close();
  return status;
}

JSONRPC_STATUS CAudioLibrary::SetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["songid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSongById(id, song) || song.idSong != id)
  {
    musicdatabase.Close();
    return InvalidParams;
  }

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
    song.rating = (char)parameterObject["rating"].asInteger();
  if (ParameterNotNull(parameterObject, "album"))
    song.strAlbum = parameterObject["album"].asString();
  if (ParameterNotNull(parameterObject, "track"))
    song.iTrack = (int)parameterObject["track"].asInteger();
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

  JSONRPC_STATUS status;
  if (musicdatabase.UpdateSong(song, id) > 0)
    status = ACK;
  else
    status = InternalError;

  musicdatabase.Close();
  return status;
}

JSONRPC_STATUS CAudioLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  CStdString cmd;
  if (directory.empty())
    cmd = "updatelibrary(music)";
  else
    cmd.Format("updatelibrary(music, %s)", directory.c_str());

  g_application.getApplicationMessenger().ExecBuiltIn(cmd);
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

  g_application.getApplicationMessenger().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CAudioLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("cleanlibrary(music)");
  return ACK;
}

bool CAudioLibrary::FillFileItem(const CStdString &strFilename, CFileItem &item)
{
  CMusicDatabase musicdatabase;
  bool status = false;
  if (!strFilename.empty() && musicdatabase.Open())
  {
    if (CDirectory::Exists(strFilename))
    {
      CAlbum album;
      int albumid = musicdatabase.GetAlbumIdByPath(strFilename);
      if (musicdatabase.GetAlbumInfo(albumid, album, NULL))
      {
        item = CFileItem(strFilename, album);
        item.SetMusicThumb();
        status = true;
      }
    }
    else
    {
      CSong song;
      if (musicdatabase.GetSongByFileName(strFilename, song))
      {
        item = CFileItem(song);
        status = true;
      }
    }

    musicdatabase.Close();
  }

  return status;
}

bool CAudioLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  bool success = false;

  if (musicdatabase.Open())
  {
    CStdString file = parameterObject["file"].asString();
    int artistID = (int)parameterObject["artistid"].asInteger(-1);
    int albumID = (int)parameterObject["albumid"].asInteger(-1);
    int genreID = (int)parameterObject["genreid"].asInteger(-1);

    CFileItem fileItem;
    if (FillFileItem(file, fileItem))
    {
      success = true;
      list.Add(CFileItemPtr(new CFileItem(fileItem)));
    }

    if (artistID != -1 || albumID != -1 || genreID != -1)
      success |= musicdatabase.GetSongsNav("", list, genreID, artistID, albumID);

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

    musicdatabase.Close();

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
  }

  return success;
}

void CAudioLibrary::FillAlbumItem(const CAlbum &album, const CStdString &path, CFileItemPtr &item)
{
  item = CFileItemPtr(new CFileItem(path, album));
  item->SetMusicThumb();
}
