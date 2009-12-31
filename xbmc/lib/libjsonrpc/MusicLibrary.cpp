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
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
  {
    unsigned start, end;
    HandleFileItemList("artistid", "artists", items, start, end, parameterObject, result);
  }

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetAlbums(const CStdString &method, const Value& parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID))
  {
    unsigned start, end;
    HandleFileItemList("albumid", "albums", items, start, end, parameterObject, result);
  }

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongs(const CStdString &method, const Value& parameterObject, Value &result)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
  int albumID = parameterObject.get("albumid", -1).asInt();
  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
  {
    unsigned start, end;
    HandleFileItemList("songid", "songs", items, start, end, parameterObject, result);
  }

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongInfo(const CStdString &method, const Value& parameterObject, Value &result)
{
  int songID = parameterObject.get("songid", -1).asInt();

  if (songID < 0)
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong songInfo;
  if (musicdatabase.GetSongById(songID, songInfo))
  {
    if (!songInfo.strTitle.IsEmpty())
      result["title"] = songInfo.strTitle.c_str();
    if (!songInfo.strArtist.IsEmpty())
      result["artist"] = songInfo.strArtist.c_str();
    if (!songInfo.strAlbum.IsEmpty())
      result["album"] = songInfo.strAlbum.c_str();
    if (!songInfo.strAlbumArtist.IsEmpty())
      result["albumartist"] = songInfo.strAlbumArtist.c_str();
    if (!songInfo.strGenre.IsEmpty())
      result["genre"] = songInfo.strGenre.c_str();
    if (!songInfo.strThumb.IsEmpty())
      result["thumbnail"] = songInfo.strThumb.c_str();
    if (!songInfo.strComment.IsEmpty())
      result["comment"] = songInfo.strComment.c_str();

    if (!songInfo.iTrack > 0)
      result["track"] = songInfo.iTrack;
    if (!songInfo.iDuration > 0)
      result["duration"] = songInfo.iDuration;

    result["rating"]      = songInfo.rating;
    result["year"]        = songInfo.iYear;
    result["timesplayed"] = songInfo.iTimesPlayed;
  }

  musicdatabase.Close();
  return OK;
}
