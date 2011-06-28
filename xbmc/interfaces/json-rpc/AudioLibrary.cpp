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

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

JSON_STATUS CAudioLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = (int)parameterObject["genreid"].asInteger();

  // Add "artist" to "fields" array by default
  CVariant param = parameterObject;
  if (!param.isMember("fields"))
    param["fields"] = CVariant(CVariant::VariantTypeArray);
  param["fields"].append("artist");

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
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
  HandleFileItem("artistid", false, "artistdetails", m_artistItem, parameterObject, parameterObject["fields"], result, false);

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
  int start     = (int)parameterObject["limits"]["start"].asInteger();
  int end       = (int)parameterObject["limits"]["end"].asInteger();
  if (end == 0)
    end = -1;

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID, start, end))
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
  HandleFileItem("albumid", false, "albumdetails", m_albumItem, parameterObject, parameterObject["fields"], result, false);

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
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
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

  HandleFileItem("songid", false, "songdetails", CFileItemPtr( new CFileItem(song) ), parameterObject, parameterObject["fields"], result, false);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CAudioLibrary::GetRecentlyAddedAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int amount = (int)parameterObject["albums"].asInteger();
  if (amount < 0)
    amount = 0;

  VECALBUMS albums;
  if (musicdatabase.GetRecentlyAddedAlbums(albums, (unsigned int)amount))
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

  int amount = (int)parameterObject["albums"].asInteger();
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
  if (musicdatabase.GetGenresNav("", items))
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
  CStdString path = parameterObject["path"].asString();
  bool singleFile = parameterObject["singlefile"].asBoolean();

  if (!singleFile && path.IsEmpty())
    return InvalidParams;

  CStdString cmd;
  if (singleFile)
    cmd.Format("exportlibrary(music, true, %s, %s, %s)",
      parameterObject["images"].asBoolean() ? "true" : "false",
      parameterObject["overwrite"].asBoolean() ? "true" : "false",
      parameterObject["actorthumbs"].asBoolean() ? "true" : "false");
  else
    cmd.Format("exportlibrary(music, false, %s)", path);

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

bool CAudioLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CMusicDatabase musicdatabase;
  bool success = false;

  if (musicdatabase.Open())
  {
    CStdString file       = parameterObject["file"].asString();
    int artistID          = (int)parameterObject["artistid"].asInteger();
    int albumID           = (int)parameterObject["albumid"].asInteger();
    int genreID           = (int)parameterObject["genreid"].asInteger();

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
