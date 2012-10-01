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

#include <string.h>
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
#include "pictures/PictureInfoTag.h"
#include "video/VideoDatabase.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "TextureCache.h"
#include "ThumbLoader.h"
#include "Util.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

void CFileItemHandler::FillDetails(ISerializable* info, CFileItemPtr item, const CVariant& fields, CVariant &result, CThumbLoader *thumbLoader /* = NULL */)
{
  if (info == NULL || fields.size() == 0)
    return;

  CVariant serialization;
  info->Serialize(serialization);

  bool fetchedArt = false;

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (item)
    {
      if (item->IsAlbum() && field.Equals("albumlabel"))
        field = "label";
      if (item->IsAlbum())
      {
        if (field == "label")
        {
          result["albumlabel"] = item->GetProperty("album_label");
          continue;
        }
        if (item->HasProperty("album_" + field + "_array"))
        {
          result[field] = item->GetProperty("album_" + field + "_array");
          continue;
        }
        if (item->HasProperty("album_" + field))
        {
          result[field] = item->GetProperty("album_" + field);
          continue;
        }
      }

      if (item->HasProperty("artist_" + field + "_array"))
      {
        result[field] = item->GetProperty("artist_" + field + "_array");
        continue;
      }
      if (item->HasProperty("artist_" + field))
      {
        result[field] = item->GetProperty("artist_" + field);
        continue;
      }

      if (field == "thumbnail")
      {
        if (thumbLoader != NULL && !item->HasThumbnail() && !fetchedArt &&
           ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) || (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
        {
          thumbLoader->FillLibraryArt(*item);
          fetchedArt = true;
        }
        else if (item->HasPictureInfoTag() && !item->HasThumbnail())
          item->SetThumbnailImage(CTextureCache::GetWrappedThumbURL(item->GetPath()));

        if (item->HasThumbnail())
          result["thumbnail"] = CTextureCache::GetWrappedImageURL(item->GetThumbnailImage());
        else
          result["thumbnail"] = "";
        continue;
      }

      if (field == "fanart")
      {
        if (thumbLoader != NULL && !item->HasProperty("fanart_image") && !fetchedArt &&
           ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) || (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
        {
          thumbLoader->FillLibraryArt(*item);
          fetchedArt = true;
        }

        if (item->HasProperty("fanart_image"))
          result["fanart"] = CTextureCache::GetWrappedImageURL(item->GetProperty("fanart_image").asString());
        else
          result["fanart"] = "";
        continue;
      }

      if (item->HasVideoInfoTag() && item->GetVideoContentType() == VIDEODB_CONTENT_TVSHOWS)
      {
        if (item->GetVideoInfoTag()->m_iSeason < 0 && field == "season")
        {
          result[field] = (int)item->GetProperty("totalseasons").asInteger();
          continue;
        }
        if (field == "watchedepisodes")
        {
          result[field] = (int)item->GetProperty("watchedepisodes").asInteger();
          continue;
        }
      }

      if (field == "lastmodified" && item->m_dateTime.IsValid())
      {
        result[field] = item->m_dateTime.GetAsLocalizedDateTime();
        continue;
      }
    }

    if (serialization.isMember(field) && !serialization[field].isNull() && (!result.isMember(field) || result[field].empty()))
      result[field] = serialization[field];
  }
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool sortLimit /* = true */)
{
  HandleFileItemList(ID, allowFile, resultname, items, parameterObject, result, items.Size(), sortLimit);
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, int size, bool sortLimit /* = true */)
{
  int start = (int)parameterObject["limits"]["start"].asInteger();
  int end   = (int)parameterObject["limits"]["end"].asInteger();
  end = (end <= 0 || end > size) ? size : end;
  start = start > end ? end : start;

  if (sortLimit)
    Sort(items, parameterObject);

  result["limits"]["start"] = start;
  result["limits"]["end"]   = end;
  result["limits"]["total"] = size;

  if (!sortLimit)
  {
    start = 0;
    end = items.Size();
  }

  CThumbLoader *thumbLoader = NULL;
  if (end - start > 0)
  {
    if (items.Get(start)->HasVideoInfoTag())
      thumbLoader = new CVideoThumbLoader();
    else if (items.Get(start)->HasMusicInfoTag())
      thumbLoader = new CMusicThumbLoader();

    if (thumbLoader != NULL)
      thumbLoader->Initialize();
  }

  for (int i = start; i < end; i++)
  {
    CVariant object;
    CFileItemPtr item = items.Get(i);
    HandleFileItem(ID, allowFile, resultname, item, parameterObject, parameterObject["properties"], result, true, thumbLoader);
  }

  delete thumbLoader;
}

void CFileItemHandler::HandleFileItem(const char *ID, bool allowFile, const char *resultname, CFileItemPtr item, const CVariant &parameterObject, const CVariant &validFields, CVariant &result, bool append /* = true */, CThumbLoader *thumbLoader /* = NULL */)
{
  CVariant object;
  bool hasFileField = false;

  if (item.get())
  {
    for (unsigned int i = 0; i < validFields.size(); i++)
    {
      CStdString field = validFields[i].asString();

      if (field == "file")
        hasFileField = true;
    }

    if (allowFile && hasFileField)
    {
      if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->GetPath().IsEmpty())
          object["file"] = item->GetVideoInfoTag()->GetPath().c_str();
      if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetURL().IsEmpty())
        object["file"] = item->GetMusicInfoTag()->GetURL().c_str();

      if (!object.isMember("file"))
        object["file"] = item->GetPath().c_str();
    }

    if (ID)
    {
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
        object[ID] = (int)item->GetMusicInfoTag()->GetDatabaseId();
      else if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > 0)
        object[ID] = item->GetVideoInfoTag()->m_iDbId;

      if (stricmp(ID, "id") == 0)
      {
        if (item->HasMusicInfoTag())
        {
          if (item->m_bIsFolder && item->IsAlbum())
            object["type"] = "album";
          else
            object["type"] = "song";
        }
        else if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_type.empty())
        {
          std::string type = item->GetVideoInfoTag()->m_type;
          if (type == "movie" || type == "tvshow" || type == "episode" || type == "musicvideo")
            object["type"] = type;
        }
        else if (item->HasPictureInfoTag())
          object["type"] = "picture";

        if (!object.isMember("type"))
          object["type"] = "unknown";
      }
    }

    FillDetails(item.get(), item, validFields, object, thumbLoader);

    if (item->HasVideoInfoTag())
    {
      if (thumbLoader == NULL)
      {
        thumbLoader = new CVideoThumbLoader();
        thumbLoader->Initialize();
      }
      FillDetails(item->GetVideoInfoTag(), item, validFields, object, thumbLoader);
    }
    if (item->HasMusicInfoTag())
    {
      if (thumbLoader == NULL)
      {
        thumbLoader = new CMusicThumbLoader();
        thumbLoader->Initialize();
      }
      FillDetails(item->GetMusicInfoTag(), item, validFields, object, thumbLoader);
    }
    if (item->HasPictureInfoTag())
      FillDetails(item->GetPictureInfoTag(), item, validFields, object, thumbLoader);

    object["label"] = item->GetLabel().c_str();
  }
  else
    object = CVariant(CVariant::VariantTypeNull);

  if (resultname)
  {
    if (append)
      result[resultname].append(object);
    else
      result[resultname] = object;
  }
}

bool CFileItemHandler::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CAudioLibrary::FillFileItemList(parameterObject, list);
  CVideoLibrary::FillFileItemList(parameterObject, list);
  CFileOperations::FillFileItemList(parameterObject, list);

  CStdString file = parameterObject["file"].asString();
  if (!file.empty() && (URIUtils::IsURL(file) || (CFile::Exists(file) && !CDirectory::Exists(file))))
  {
    bool added = false;
    for (int index = 0; index < list.Size(); index++)
    {
      if (list[index]->GetPath() == file)
      {
        added = true;
        break;
      }
    }

    if (!added)
    {
      CFileItemPtr item = CFileItemPtr(new CFileItem(file, false));
      if (item->IsPicture())
      {
        CPictureInfoTag picture;
        picture.Load(item->GetPath());
        *item->GetPictureInfoTag() = picture;
      }
      if (item->GetLabel().IsEmpty())
        item->SetLabel(CUtil::GetTitleFromPath(file, false));
      list.Add(item);
    }
  }

  return (list.Size() > 0);
}

bool CFileItemHandler::ParseSorting(const CVariant &parameterObject, SortBy &sortBy, SortOrder &sortOrder, SortAttribute &sortAttributes)
{
  CStdString method = parameterObject["sort"]["method"].asString();
  CStdString order = parameterObject["sort"]["order"].asString();
  method.ToLower();
  order.ToLower();

  sortAttributes = SortAttributeNone;
  if (parameterObject["sort"]["ignorearticle"].asBoolean())
    sortAttributes = SortAttributeIgnoreArticle;
  else
    sortAttributes = SortAttributeNone;

  if (order.Equals("ascending"))
    sortOrder = SortOrderAscending;
  else if (order.Equals("descending"))
    sortOrder = SortOrderDescending;
  else
    return false;

  if (method.Equals("none"))
    sortBy = SortByNone;
  else if (method.Equals("label"))
    sortBy = SortByLabel;
  else if (method.Equals("date"))
    sortBy = SortByDate;
  else if (method.Equals("size"))
    sortBy = SortBySize;
  else if (method.Equals("file"))
    sortBy = SortByFile;
  else if (method.Equals("path"))
    sortBy = SortByPath;
  else if (method.Equals("drivetype"))
    sortBy = SortByDriveType;
  else if (method.Equals("title"))
    sortBy = SortByTitle;
  else if (method.Equals("track"))
    sortBy = SortByTrackNumber;
  else if (method.Equals("time"))
    sortBy = SortByTime;
  else if (method.Equals("artist"))
    sortBy = SortByArtist;
  else if (method.Equals("album"))
    sortBy = SortByAlbum;
  else if (method.Equals("albumtype"))
    sortBy = SortByAlbumType;
  else if (method.Equals("genre"))
    sortBy = SortByGenre;
  else if (method.Equals("country"))
    sortBy = SortByCountry;
  else if (method.Equals("year"))
    sortBy = SortByYear;
  else if (method.Equals("rating"))
    sortBy = SortByRating;
  else if (method.Equals("votes"))
    sortBy = SortByVotes;
  else if (method.Equals("top250"))
    sortBy = SortByTop250;
  else if (method.Equals("programcount"))
    sortBy = SortByProgramCount;
  else if (method.Equals("playlist"))
    sortBy = SortByPlaylistOrder;
  else if (method.Equals("episode"))
    sortBy = SortByEpisodeNumber;
  else if (method.Equals("season"))
    sortBy = SortBySeason;
  else if (method.Equals("totalepisodes"))
    sortBy = SortByNumberOfEpisodes;
  else if (method.Equals("watchedepisodes"))
    sortBy = SortByNumberOfWatchedEpisodes;
  else if (method.Equals("tvshowstatus"))
    sortBy = SortByTvShowStatus;
  else if (method.Equals("tvshowtitle"))
    sortBy = SortByTvShowTitle;
  else if (method.Equals("sorttitle"))
    sortBy = SortBySortTitle;
  else if (method.Equals("productioncode"))
    sortBy = SortByProductionCode;
  else if (method.Equals("mpaa"))
    sortBy = SortByMPAA;
  else if (method.Equals("studio"))
    sortBy = SortByStudio;
  else if (method.Equals("dateadded"))
    sortBy = SortByDateAdded;
  else if (method.Equals("lastplayed"))
    sortBy = SortByLastPlayed;
  else if (method.Equals("playcount"))
    sortBy = SortByPlaycount;
  else if (method.Equals("listeners"))
    sortBy = SortByListeners;
  else if (method.Equals("bitrate"))
    sortBy = SortByBitrate;
  else if (method.Equals("random"))
    sortBy = SortByRandom;
  else
    return false;

  return true;
}

void CFileItemHandler::ParseLimits(const CVariant &parameterObject, int &limitStart, int &limitEnd)
{
  limitStart = (int)parameterObject["limits"]["start"].asInteger();
  limitEnd = (int)parameterObject["limits"]["end"].asInteger();
}

void CFileItemHandler::Sort(CFileItemList &items, const CVariant &parameterObject)
{
  SortDescription sorting;
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return;

  items.Sort(sorting);
}
