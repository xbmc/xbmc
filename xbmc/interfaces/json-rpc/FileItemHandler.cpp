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
#include "utils/URIUtils.h"
#include "utils/ISerializable.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"


using namespace MUSIC_INFO;
using namespace Json;
using namespace JSONRPC;

void CFileItemHandler::FillDetails(ISerializable* info, CFileItemPtr item, const Value& fields, Value &result)
{
  if (info == NULL || fields.size() == 0)
    return;

  CVariant data;
  info->Serialize(data);

  Value serialization;
  data.toJsonValue(serialization);

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (item)
    {
      if (item->IsAlbum() && item->HasProperty(field))
      {
        if (field == "album_rating")
          result[field] = item->GetPropertyInt(field);
        else
          result[field] = item->GetProperty(field);

        continue;
      }

      if (field == "fanart")
      {
        CStdString cachedFanArt = item->GetCachedFanart();
        if (!cachedFanArt.IsEmpty())
        {
          result["fanart"] = cachedFanArt.c_str();
          continue;
        }
      }
    }

    if (serialization.isMember(field))
    {
      Value value = serialization[field];
      if (!value.isString() || (value.isString() && !value.asString().empty()))
        result[field] = value;
    }
  }
}

void CFileItemHandler::MakeFieldsList(const Json::Value &parameterObject, Json::Value &validFields)
{
  const Json::Value fields = parameterObject.isMember("fields") && parameterObject["fields"].isArray() ? parameterObject["fields"] : Value(arrayValue);

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    if (fields[i].isString())
      validFields.append(fields[i]);
  }
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

  Json::Value validFields = Value(arrayValue);
  MakeFieldsList(parameterObject, validFields);

  for (int i = start; i < end; i++)
  {
    Value object;
    CFileItemPtr item = items.Get(i);
    HandleFileItem(id, allowFile, resultname, item, parameterObject, validFields, result);
  }
}

void CFileItemHandler::HandleFileItem(const char *id, bool allowFile, const char *resultname, CFileItemPtr item, const Json::Value &parameterObject, const Json::Value &validFields, Json::Value &result)
{
  Value object;
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

  if (item->HasVideoInfoTag())
    FillDetails(item->GetVideoInfoTag(), item, validFields, object);
  if (item->HasMusicInfoTag())
    FillDetails(item->GetMusicInfoTag(), item, validFields, object);

  object["label"] = item->GetLabel().c_str();

  if (resultname)
    result[resultname].append(object);
}

bool CFileItemHandler::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  if (parameterObject["file"].isString())
  {
    CStdString file = parameterObject["file"].asString();
    CFileItemPtr item = CFileItemPtr(new CFileItem(file, URIUtils::HasSlashAtEnd(file)));
    list.Add(item);
  }

  CPlaylistOperations::FillFileItemList(parameterObject, list);
  CAudioLibrary::FillFileItemList(parameterObject, list);
  CVideoLibrary::FillFileItemList(parameterObject, list);
  CFileOperations::FillFileItemList(parameterObject, list);

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
  else if (method.Equals("playcount"))
    sortmethod = SORT_METHOD_PLAYCOUNT;
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
