#include "MusicLibrary.h"
#include "../MusicDatabase.h"
#include "../FileItem.h"
#include "../Util.h"
#include "../MusicInfoTag.h"
#include "../Song.h"

using namespace MUSIC_INFO;
using namespace Json;

JSON_STATUS CMusicLibrary::GetArtists(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  std::vector<CJSONObject> artists;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
  {
    unsigned int size  = (unsigned int)items.Size();
    unsigned int start = parameterObject.has<long>("start") ? (unsigned int)parameterObject.get<long>("start") : 0;
    unsigned int end   = parameterObject.has<long>("end")   ? (unsigned int)parameterObject.get<long>("end")   : size;
    end = end > size ? size : end;

    result.Add("start", (long)start);
    result.Add("end",  (long)end);
    result.Add("total", (long)items.Size());
    for (unsigned int i = start; i < end; i++)
    {
      CJSONObject object;
      CFileItemPtr item = items.Get(i);
      CUtil::RemoveSlashAtEnd(item->m_strPath);
      object.Add("artistid", atol(item->m_strPath.c_str()));
      object.Add("artist", item->GetMusicInfoTag()->GetArtist());
      if (!item->GetThumbnailImage().IsEmpty())
        object.Add("thumbnail", item->GetThumbnailImage());
      artists.push_back(object);
    }
  }

  if (artists.size() > 0)
    result.Add("artists", artists);

  musicdatabase.Close();*/
  return OK;
}

JSON_STATUS CMusicLibrary::GetAlbums(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  int artistID = parameterObject.has<long>("artistid") ? (int)parameterObject.get<long>("artistid") : -1;
  int genreID  = parameterObject.has<long>("genreid")  ? (int)parameterObject.get<long>("genreid")  : -1;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  std::vector<CJSONObject> albums;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID))
  {
    unsigned int size   = (unsigned int)items.Size();
    unsigned int start = parameterObject.has<long>("start") ? (unsigned int)parameterObject.get<long>("start") : 0;
    unsigned int end   = parameterObject.has<long>("end")   ? (unsigned int)parameterObject.get<long>("end")   : size;
    end = end > size ? size : end;

    result.Add("start", (long)start);
    result.Add("end",  (long)end);
    result.Add("total", (long)items.Size());
    for (unsigned int i = start; i < end; i++)
    {
      CJSONObject object;
      CFileItemPtr item = items.Get(i);
      CUtil::RemoveSlashAtEnd(item->m_strPath);
      object.Add("albumid", atol(item->m_strPath.c_str()));
      object.Add("album", item->GetMusicInfoTag()->GetAlbum());
      if (!item->GetThumbnailImage().IsEmpty())
        object.Add("thumbnail", item->GetThumbnailImage());
      albums.push_back(object);
    }
  }

  if (albums.size() > 0)
    result.Add("albums", albums);

  musicdatabase.Close();*/
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongs(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  int artistID = parameterObject.has<long>("artistid") ? (int)parameterObject.get<long>("artistid") : -1;
  int albumID  = parameterObject.has<long>("albumid")  ? (int)parameterObject.get<long>("albumid")  : -1;
  int genreID  = parameterObject.has<long>("genreid")  ? (int)parameterObject.get<long>("genreid")  : -1;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CFileItemList items;
  std::vector<CJSONObject> songs;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
  {
    unsigned int size   = (unsigned int)items.Size();
    unsigned int start = parameterObject.has<long>("start") ? (unsigned int)parameterObject.get<long>("start") : 0;
    unsigned int end   = parameterObject.has<long>("end")   ? (unsigned int)parameterObject.get<long>("end")   : size;
    end = end > size ? size : end;

    result.Add("start", (long)start);
    result.Add("end",  (long)end);
    result.Add("total", (long)items.Size());
    for (unsigned int i = start; i < end; i++)
    {
      CJSONObject object;
      CFileItemPtr item = items.Get(i);
      CUtil::RemoveSlashAtEnd(item->m_strPath);
      object.Add("songid", atol(item->m_strPath.c_str()));
      object.Add("title", item->GetMusicInfoTag()->GetTitle());
      if (!item->GetThumbnailImage().IsEmpty())
        object.Add("thumbnail", item->GetThumbnailImage());
      songs.push_back(object);
    }
  }

  if (songs.size() > 0)
    result.Add("songs", songs);

  musicdatabase.Close();*/
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
 /* if (!parameterObject.has<long>("songid"))
    return InvalidParams;

  int songID = (int)parameterObject.get<long>("songid");

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong songInfo;
  if (musicdatabase.GetSongById(songID, songInfo))
  {
    if (!songInfo.strTitle.IsEmpty())
      result.Add("title", songInfo.strTitle);
    if (!songInfo.strArtist.IsEmpty())
      result.Add("artist", songInfo.strArtist);
    if (!songInfo.strAlbum.IsEmpty())
      result.Add("album", songInfo.strAlbum);
    if (!songInfo.strAlbumArtist.IsEmpty())
      result.Add("albumartist", songInfo.strAlbumArtist);
    if (!songInfo.strGenre.IsEmpty())
      result.Add("genre", songInfo.strGenre);
    if (!songInfo.strThumb.IsEmpty())
      result.Add("thumbnail", songInfo.strThumb);
    if (!songInfo.strComment.IsEmpty())
      result.Add("comment", songInfo.strComment);

    if (!songInfo.iTrack > 0)
      result.Add("track", (long)songInfo.iTrack);
    if (!songInfo.iDuration > 0)
      result.Add("duration", (long)songInfo.iDuration);

    result.Add("rating",      (long)songInfo.rating);
    result.Add("year",        (long)songInfo.iYear);
    result.Add("timesplayed", (long)songInfo.iTimesPlayed);
  }

  musicdatabase.Close();*/
  return OK;
}
