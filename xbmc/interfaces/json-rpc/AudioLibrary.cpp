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
#include "music/Artist.h"
#include "Application.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

JSON_STATUS CAudioLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::GetArtistDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int artistID = (int)parameterObject["artistid"].asInteger();

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CArtist artist;
  if (!musicdatabase.GetArtistInfo(artistID, artist))
  {
    musicdatabase.Close();
    return InvalidParams;
  }

  CFileItemPtr m_artistItem(new CFileItem(artist));
  m_artistItem->GetMusicInfoTag()->SetArtist(m_artistItem->GetLabel());
  m_artistItem->GetMusicInfoTag()->SetDatabaseId(artistID);
  CMusicDatabase::SetPropertiesFromArtist(*m_artistItem, artist);
  m_artistItem->SetCachedArtistThumb();
  HandleFileItem("artistid", false, "artistdetails", m_artistItem, parameterObject, parameterObject["properties"], result, false);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID  = (int)parameterObject["artistid"].asInteger();
  int genreID   = (int)parameterObject["genreid"].asInteger();

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("musicdb://3/", items, genreID, artistID, -1, -1))
    HandleFileItemList("albumid", false, "albums", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = (int)parameterObject["artistid"].asInteger();
  int albumID  = (int)parameterObject["albumid"].asInteger();
  int genreID  = (int)parameterObject["genreid"].asInteger();

  CFileItemList items;
  if (musicdatabase.GetSongsNav("musicdb://4/", items, genreID, artistID, albumID))
    HandleFileItemList("songid", true, "songs", items, parameterObject, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetSongDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::GetRecentlyAddedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::GetRecentlyAddedSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::GetGenres(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(music)");
  return ACK;
}

JSON_STATUS CAudioLibrary::Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CAudioLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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
  }

  return success;
}

void CAudioLibrary::FillAlbumItem(const CAlbum &album, const CStdString &path, CFileItemPtr &item)
{
  item = CFileItemPtr(new CFileItem(path, album));
  item->SetMusicThumb();
}
