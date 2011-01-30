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
#include "music/tags/MusicInfoTag.h"
#include "music/Song.h"
#include "Application.h"

using namespace MUSIC_INFO;
using namespace Json;
using namespace JSONRPC;

JSON_STATUS CAudioLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  //if (!ParameterIntOrNull(parameterObject, "genreid"))
  //  return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
    HandleFileItemList("artistid", false, "artists", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;
    
  //if (!(ParameterIntOrNull(parameterObject, "artistid") || ParameterIntOrNull(parameterObject, "albumid")))
  //  return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
  int genreID  = parameterObject.get("genreid", -1).asInt();
  int start = parameterObject.get("start", -1).asInt();
  int end = parameterObject.get("end", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID, start, end))
    HandleFileItemList("albumid", false, "albums", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  //if (!(ParameterIntOrNull(parameterObject, "albumid")))
  //  return InvalidParams;

  int albumID = parameterObject.get("albumid", -1).asInt();
  if (albumID <= 0)
    return InvalidParams;

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

  CFileItemPtr m_albumItem( new CFileItem(path, true) );
  m_albumItem->SetLabel(album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbum(album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetYear(album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating('0' + (album.iRating + 1) / 2);
  m_albumItem->GetMusicInfoTag()->SetGenre(album.strGenre);
  m_albumItem->GetMusicInfoTag()->SetDatabaseId(albumID);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem, album);
  m_albumItem->SetMusicThumb();
  HandleFileItem("albumid", false, "albumdetails", m_albumItem, parameterObject, validFields, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  //if (!(ParameterIntOrNull(param, "artistid") || ParameterIntOrNull(param, "albumid") || ParameterIntOrNull(param, "genreid")))
  //  return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
  int albumID  = parameterObject.get("albumid", -1).asInt();
  int genreID  = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
    HandleFileItemList("songid", true, "songs", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  //if (!(ParameterIntOrNull(parameterObject, "songid")))
  //  return InvalidParams;

  int idSong = parameterObject.get("songid", -1).asInt();
  if (idSong <= 0)
    return InvalidParams;

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
  HandleFileItem("songid", false, "songdetails", CFileItemPtr( new CFileItem(song) ), parameterObject, validFields, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  if (musicdatabase.GetGenresNav("", items))
    HandleFileItemList("genreid", true, "genres", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(music)");
  return ACK;
}

bool CAudioLibrary::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  bool success = false;

  if (musicdatabase.Open())
  {
    if (parameterObject["artistid"].isInt() || parameterObject["albumid"].isInt() || parameterObject["genreid"].isInt())
    {
      int artistID = parameterObject.get("artistid", -1).asInt();
      int albumID  = parameterObject.get("albumid", -1).asInt();
      int genreID  = parameterObject.get("genreid", -1).asInt();

      success = musicdatabase.GetSongsNav("", list, genreID, artistID, albumID);
    }
    if (parameterObject["songid"].isInt())
    {
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
    }

    musicdatabase.Close();
  }

  return success;
}
