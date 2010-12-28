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

#include "FileItemHandler.h"
#include "PlaylistOperations.h"
#include "AudioLibrary.h"
#include "VideoLibrary.h"
#include "FileOperations.h"
#include "../Util.h"

using namespace MUSIC_INFO;
using namespace Json;
using namespace JSONRPC;

void CFileItemHandler::FillVideoDetails(const CVideoInfoTag *videoInfo, const CStdString &field, Value &result)
{
  if (videoInfo->IsEmpty())
    return;

  if (field.Equals("genre") && !videoInfo->m_strGenre.IsEmpty())
    result["genre"] = videoInfo->m_strGenre.c_str();
  if (field.Equals("director") && !videoInfo->m_strDirector.IsEmpty())
    result["director"] = videoInfo->m_strDirector.c_str();
  if (field.Equals("trailer") && !videoInfo->m_strTrailer.IsEmpty())
    result["trailer"] = videoInfo->m_strTrailer.c_str();
  if (field.Equals("tagline") && !videoInfo->m_strTagLine.IsEmpty())
    result["tagline"] = videoInfo->m_strTagLine.c_str();
  if (field.Equals("plot") && !videoInfo->m_strPlot.IsEmpty())
    result["plot"] = videoInfo->m_strPlot.c_str();
  if (field.Equals("plotoutline") && !videoInfo->m_strPlotOutline.IsEmpty())
    result["plotoutline"] = videoInfo->m_strPlotOutline.c_str();
  if (field.Equals("title") && !videoInfo->m_strTitle.IsEmpty())
    result["title"] = videoInfo->m_strTitle.c_str();
  if (field.Equals("originaltitle") && !videoInfo->m_strOriginalTitle.IsEmpty())
    result["originaltitle"] = videoInfo->m_strOriginalTitle.c_str();
  if (field.Equals("lastplayed") && !videoInfo->m_lastPlayed.IsEmpty())
    result["lastplayed"] = videoInfo->m_lastPlayed.c_str();
  if (field.Equals("showtitle") && !videoInfo->m_strShowTitle.IsEmpty())
    result["showtitle"] = videoInfo->m_strShowTitle.c_str();
  if (field.Equals("firstaired") && !videoInfo->m_strFirstAired.IsEmpty())
    result["firstaired"] = videoInfo->m_strFirstAired.c_str();
  if (field.Equals("duration"))
    result["duration"] = videoInfo->m_streamDetails.GetVideoDuration();
  if (field.Equals("season") && videoInfo->m_iSeason > 0)
    result["season"] = videoInfo->m_iSeason;
  if (field.Equals("episode") && videoInfo->m_iEpisode > 0)
    result["episode"] = videoInfo->m_iEpisode;
  if (field.Equals("runtime") && !videoInfo->m_strRuntime.IsEmpty())
    result["runtime"] = videoInfo->m_strRuntime.c_str();
  if (field.Equals("year") && videoInfo->m_iYear > 0)
    result["year"] = videoInfo->m_iYear;
  if (field.Equals("playcount") && videoInfo->m_playCount >= 0)
    result["playcount"] = videoInfo->m_playCount;
  if (field.Equals("rating"))
    result["rating"] = (double)videoInfo->m_fRating;
  if (field.Equals("writer") && !videoInfo->m_strWritingCredits.IsEmpty())
    result["writer"] = videoInfo->m_strWritingCredits.c_str();
  if (field.Equals("studio") && !videoInfo->m_strStudio.IsEmpty())
    result["studio"] = videoInfo->m_strStudio.c_str();
  if (field.Equals("mpaa") && !videoInfo->m_strMPAARating.IsEmpty())
    result["mpaa"] = videoInfo->m_strMPAARating.c_str();
  if (field.Equals("premiered") && !videoInfo->m_strPremiered.IsEmpty())
    result["premiered"] = videoInfo->m_strPremiered.c_str();
  if (field.Equals("album") && !videoInfo->m_strAlbum.IsEmpty())
    result["album"] = videoInfo->m_strAlbum.c_str();
  if (field.Equals("artist") && !videoInfo->m_strArtist.IsEmpty())
    result["artist"] = videoInfo->m_strArtist.c_str();
}

void CFileItemHandler::FillMusicDetails(const CMusicInfoTag *musicInfo, const CStdString &field, Value &result)
{
  if (field.Equals("title") && !musicInfo->GetTitle().IsEmpty())
    result["title"] =  musicInfo->GetTitle().c_str();
  if (field.Equals("album") && !musicInfo->GetAlbum().IsEmpty())
    result["album"] =  musicInfo->GetAlbum().c_str();
  if (field.Equals("artist") && !musicInfo->GetArtist().IsEmpty())
    result["artist"] =  musicInfo->GetArtist().c_str();
  if (field.Equals("albumartist") && !musicInfo->GetAlbumArtist().IsEmpty())
    result["albumartist"] =  musicInfo->GetAlbumArtist().c_str();

  if (field.Equals("genre") && !musicInfo->GetGenre().IsEmpty())
    result["genre"] =  musicInfo->GetGenre().c_str();

  if (field.Equals("tracknumber"))
    result["tracknumber"] = (int)musicInfo->GetTrackNumber();
  if (field.Equals("discnumber"))
    result["discnumber"] = (int)musicInfo->GetDiscNumber();
  if (field.Equals("trackanddiscnumber"))
    result["trackanddiscnumber"] = (int)musicInfo->GetTrackAndDiskNumber();
  if (field.Equals("duration"))
    result["duration"] = (int)musicInfo->GetDuration();
  if (field.Equals("year"))
    result["year"] = (int)musicInfo->GetYear();

//  void GetReleaseDate(SYSTEMTIME& dateTime) const;

  if (field.Equals("musicbrainztrackid") && !musicInfo->GetMusicBrainzTrackID().IsEmpty())
    result["musicbrainztrackid"] =  musicInfo->GetMusicBrainzTrackID().c_str();
  if (field.Equals("musicbrainzartistid") && !musicInfo->GetMusicBrainzArtistID().IsEmpty())
    result["musicbrainzartistid"] =  musicInfo->GetMusicBrainzArtistID().c_str();
  if (field.Equals("musicbrainzalbumid") && !musicInfo->GetMusicBrainzAlbumID().IsEmpty())
    result["musicbrainzalbumid"] =  musicInfo->GetMusicBrainzAlbumID().c_str();
  if (field.Equals("musicbrainzalbumartistid") && !musicInfo->GetMusicBrainzAlbumArtistID().IsEmpty())
    result["musicbrainzalbumartistid"] =  musicInfo->GetMusicBrainzAlbumArtistID().c_str();
  if (field.Equals("musicbrainztrmidid") && !musicInfo->GetMusicBrainzTRMID().IsEmpty())
    result["musicbrainztrmidid"] =  musicInfo->GetMusicBrainzTRMID().c_str();

  if (field.Equals("comment") && !musicInfo->GetComment().IsEmpty())
    result["comment"] =  musicInfo->GetComment().c_str();
  if (field.Equals("lyrics") && !musicInfo->GetLyrics().IsEmpty())
    result["lyrics"] =  musicInfo->GetLyrics().c_str();

  if (field.Equals("rating"))
    result["rating"] = (int)(musicInfo->GetRating() - '0');
}

void CFileItemHandler::HandleFileItemList(const char *id, bool allowFile, const char *resultname, CFileItemList &items, const Value &parameterObject, Value &result)
{
  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);

  int size  = items.Size();
  int start = param.get("start", 0).asInt(); 
  int end   = param.get("end", size).asInt(); 
  end = end < 0 ? 0 : end > size ? size : end;
  start = start < 0 ? 0 : start > end ? end : start;

  Sort(items, param);

  result["start"] = start;
  result["end"]   = end;
  result["total"] = size;

  for (unsigned int i = start; i < end; i++)
  {
    Value object;
    CFileItemPtr item = items.Get(i);

    if (allowFile)
    {
      if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
        object["file"] = item->GetVideoInfoTag()->m_strFileNameAndPath.c_str();
      if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetURL().IsEmpty())
        object["file"] = item->GetMusicInfoTag()->GetURL().c_str();

      if (!object.isMember("file"))
        object["file"] = item->m_strPath.c_str();
    }

    if (id)
    {
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
        object[id] = (int)item->GetMusicInfoTag()->GetDatabaseId();
      else if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > 0)
        object[id] = item->GetVideoInfoTag()->m_iDbId;
    }

    if (!item->GetThumbnailImage().IsEmpty())
      object["thumbnail"] = item->GetThumbnailImage().c_str();
    if (!item->GetCachedFanart().IsEmpty())
      object["fanart"] = item->GetCachedFanart().c_str();

    const Json::Value fields = parameterObject.isMember("fields") && parameterObject["fields"].isArray() ? parameterObject["fields"] : Value(arrayValue);

    for (unsigned int i = 0; i < fields.size(); i++)
    {
      if (!fields[i].isString())
        continue;

      CStdString field = fields[i].asString();

      if (item->HasVideoInfoTag())
        FillVideoDetails(item->GetVideoInfoTag(), field, object);
      if (item->HasMusicInfoTag())
        FillMusicDetails(item->GetMusicInfoTag(), field, object);
      if (item->IsAlbum() && item->HasProperty(field))
      {
        if (field == "album_rating")
          object[field] = item->GetPropertyInt(field);
        else
          object[field] = item->GetProperty(field);
      }
    }

    object["label"] = item->GetLabel().c_str();

    if (resultname)
      result[resultname].append(object);
  }
}

bool CFileItemHandler::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  Value param = ForceObject(parameterObject);

  if (parameterObject.isString())
    param["file"] = parameterObject.asString();

  if (param["file"].isString())
  {
    CStdString file = param["file"].asString();
    CFileItemPtr item = CFileItemPtr(new CFileItem(file, CUtil::HasSlashAtEnd(file)));
    list.Add(item);
  }

  CPlaylistOperations::FillFileItemList(param, list);
  CAudioLibrary::FillFileItemList(param, list);
  CVideoLibrary::FillFileItemList(param, list);
  CFileOperations::FillFileItemList(param, list);

  return true;
}

bool CFileItemHandler::ParseSortMethods(const CStdString &method, const bool &ignorethe, const CStdString &order, SORT_METHOD &sortmethod, SORT_ORDER &sortorder)
{
  if (order.Equals("ascending"))
    sortorder = SORT_ORDER_ASC;
  else if (order.Equals("descending"))
    sortorder = SORT_ORDER_DESC;
  else
    return false;

  if (method.Equals("none"))
    sortmethod = SORT_METHOD_NONE;
  else if (method.Equals("label"))
    sortmethod = ignorethe ? SORT_METHOD_LABEL_IGNORE_THE : SORT_METHOD_LABEL;
  else if (method.Equals("date"))
    sortmethod = SORT_METHOD_DATE;
  else if (method.Equals("size"))
    sortmethod = SORT_METHOD_SIZE;
  else if (method.Equals("file"))
    sortmethod = SORT_METHOD_FILE;
  else if (method.Equals("drivetype"))
    sortmethod = SORT_METHOD_DRIVE_TYPE;
  else if (method.Equals("track"))
    sortmethod = SORT_METHOD_TRACKNUM;
  else if (method.Equals("duration"))
    sortmethod = SORT_METHOD_DURATION;
  else if (method.Equals("title"))
    sortmethod = ignorethe ? SORT_METHOD_TITLE_IGNORE_THE : SORT_METHOD_TITLE;
  else if (method.Equals("artist"))
    sortmethod = ignorethe ? SORT_METHOD_ARTIST_IGNORE_THE : SORT_METHOD_ARTIST;
  else if (method.Equals("album"))
    sortmethod = ignorethe ? SORT_METHOD_ALBUM_IGNORE_THE : SORT_METHOD_ALBUM;
  else if (method.Equals("genre"))
    sortmethod = SORT_METHOD_GENRE;
  else if (method.Equals("year"))
    sortmethod = SORT_METHOD_YEAR;
  else if (method.Equals("videorating"))
    sortmethod = SORT_METHOD_VIDEO_RATING;
  else if (method.Equals("programcount"))
    sortmethod = SORT_METHOD_PROGRAM_COUNT;
  else if (method.Equals("playlist"))
    sortmethod = SORT_METHOD_PLAYLIST_ORDER;
  else if (method.Equals("episode"))
    sortmethod = SORT_METHOD_EPISODE;
  else if (method.Equals("videotitle"))
    sortmethod = SORT_METHOD_VIDEO_TITLE;
  else if (method.Equals("sorttitle"))
    sortmethod = ignorethe ? SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE : SORT_METHOD_VIDEO_SORT_TITLE;
  else if (method.Equals("productioncode"))
    sortmethod = SORT_METHOD_PRODUCTIONCODE;
  else if (method.Equals("songrating"))
    sortmethod = SORT_METHOD_SONG_RATING;
  else if (method.Equals("mpaarating"))
    sortmethod = SORT_METHOD_MPAA_RATING;
  else if (method.Equals("videoruntime"))
    sortmethod = SORT_METHOD_VIDEO_RUNTIME;
  else if (method.Equals("studio"))
    sortmethod = ignorethe ? SORT_METHOD_STUDIO_IGNORE_THE : SORT_METHOD_STUDIO;
  else if (method.Equals("fullpath"))
    sortmethod = SORT_METHOD_FULLPATH;
  else if (method.Equals("lastplayed"))
    sortmethod = SORT_METHOD_LASTPLAYED;
  else if (method.Equals("unsorted"))
    sortmethod = SORT_METHOD_UNSORTED;
  else if (method.Equals("max"))
    sortmethod = SORT_METHOD_MAX;
  else
    return false;

  return true;
}

void CFileItemHandler::Sort(CFileItemList &items, const Value &parameterObject)
{
  Value sort = parameterObject["sort"];

  if (sort.isObject())
  {
    CStdString method = sort["method"].isString() ? sort["method"].asString() : "none";
    CStdString order  = sort["order"].isString() ? sort["order"].asString() : "ascending";
    bool ignorethe    = sort["ignorethe"].isBool() ? sort["ignorethe"].asBool() : false;

    method = method.ToLower();
    order  = order.ToLower();

    SORT_METHOD sortmethod = SORT_METHOD_NONE;
    SORT_ORDER  sortorder  = SORT_ORDER_ASC;

    if (ParseSortMethods(method, ignorethe, order, sortmethod, sortorder))
      items.Sort(sortmethod, sortorder);
  }
}
