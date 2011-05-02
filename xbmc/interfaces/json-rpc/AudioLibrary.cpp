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
#include "music/Song.h"
#include "Application.h"
#include "filesystem/Directory.h"

using namespace MUSIC_INFO;
using namespace Json;
using namespace JSONRPC;
using namespace XFILE;

JSON_STATUS CAudioLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = parameterObject["genreid"].asInt();

  // Add "artist" to "fields" array by default
  Value param = parameterObject;
  if (!param.isMember("fields"))
    param["fields"] = Value(arrayValue);
  param["fields"].append("artist");

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
    HandleFileItemList("artistid", false, "artists", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject["artistid"].asInt();
  int genreID  = parameterObject["genreid"].asInt();
  int start = parameterObject["limits"]["start"].asInt();
  int end = parameterObject["limits"]["end"].asInt();
  if (end == 0)
    end = -1;

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID, start, end))
    HandleFileItemList("albumid", false, "albums", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  int albumID = parameterObject["albumid"].asInt();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CAlbum album;
  if (!musicdatabase.GetAlbumInfo(albumID, album, NULL))
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields;
  MakeFieldsList(parameterObject, validFields);

  CStdString path;
  musicdatabase.GetAlbumPath(albumID, path);

  CFileItemPtr m_albumItem( new CFileItem(path, album) );
  m_albumItem->SetLabel(album.strAlbum);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem, album);
  m_albumItem->SetMusicThumb();
  HandleFileItem("albumid", false, "albumdetails", m_albumItem, parameterObject, validFields, result, false);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject["artistid"].asInt();
  int albumID  = parameterObject["albumid"].asInt();
  int genreID  = parameterObject["genreid"].asInt();

  CFileItemList items;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
    HandleFileItemList("songid", true, "songs", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  int idSong = parameterObject["songid"].asInt();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong song;
  if (!musicdatabase.GetSongById(idSong, song))
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  Json::Value validFields;
  MakeFieldsList(parameterObject, validFields);
  HandleFileItem("songid", false, "songdetails", CFileItemPtr( new CFileItem(song) ), parameterObject, validFields, result, false);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  // Add "genre" to "fields" array by default
  Value param = parameterObject;
  if (!param.isMember("fields"))
    param["fields"] = Value(arrayValue);
  param["fields"].append("genre");
  param["fields"].append("thumbnail");

  CFileItemList items;
  if (musicdatabase.GetGenresNav("", items))
    HandleFileItemList("genreid", false, "genres", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(music)");
  return ACK;
}

bool CAudioLibrary::FillFileItem(const CStdString &strFilename, CFileItem &item)
{
  CMusicDatabase musicdatabase;
  bool status = false;
  if (!strFilename.empty() && !CDirectory::Exists(strFilename) && musicdatabase.Open())
  {
    CSong song;
    if (musicdatabase.GetSongByFileName(strFilename, song))
    {
      item = CFileItem(song);
      status = true;
    }

    musicdatabase.Close();
  }

  return status;
}

bool CAudioLibrary::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  bool success = false;

  if (musicdatabase.Open())
  {
    CStdString file       = parameterObject["file"].asString();
    int artistID          = parameterObject["artistid"].asInt();
    int albumID           = parameterObject["albumid"].asInt();
    int genreID           = parameterObject["genreid"].asInt();

    CFileItem fileItem;
    if (FillFileItem(file, fileItem))
    {
      success = true;
      list.Add(CFileItemPtr(new CFileItem(fileItem)));
    }

    if (artistID != -1 || albumID != -1 || genreID != -1)
      success |= musicdatabase.GetSongsNav("", list, genreID, artistID, albumID);

    int songID = parameterObject.get("songid", -1).asInt();
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
  }

  return success;
}
