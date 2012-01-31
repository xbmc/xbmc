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

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

void CFileItemHandler::FillDetails(ISerializable* info, CFileItemPtr item, const CVariant& fields, CVariant &result)
{
  if (info == NULL || fields.size() == 0)
    return;

  CVariant serialization;
  info->Serialize(serialization);

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (item)
    {
      if (item->IsAlbum() && field.Equals("albumlabel"))
        field = "label";
      if (item->IsAlbum() && item->HasProperty("album_" + field))
      {
        if (field == "label")
          result["albumlabel"] = item->GetProperty("album_label");
        else
          result[field] = item->GetProperty("album_" + field);

        continue;
      }

      if (item->HasProperty("artist_" + field))
      {
        result[field] = item->GetProperty("artist_" + field);
        continue;
      }

      if (field == "fanart" && !item->HasPictureInfoTag())
      {
        CStdString fanart;
        if (item->HasProperty("fanart_image"))
          fanart = item->GetProperty("fanart_image").asString();
        if (fanart.empty())
          fanart = item->GetCachedFanart();
        if (!fanart.empty())
          result["fanart"] = fanart.c_str();

        continue;
      }
    }

    if (serialization.isMember(field) && !result.isMember(field))
      result[field] = serialization[field];
  }
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result)
{
  int size  = items.Size();
  int start = (int)parameterObject["limits"]["start"].asInteger();
  int end   = (int)parameterObject["limits"]["end"].asInteger();
  end = (end <= 0 || end > size) ? size : end;
  start = start > end ? end : start;

  Sort(items, parameterObject["sort"]);

  result["limits"]["start"] = start;
  result["limits"]["end"]   = end;
  result["limits"]["total"] = size;

  for (int i = start; i < end; i++)
  {
    CVariant object;
    CFileItemPtr item = items.Get(i);
    HandleFileItem(ID, allowFile, resultname, item, parameterObject, parameterObject["properties"], result);
  }
}

void CFileItemHandler::HandleFileItem(const char *ID, bool allowFile, const char *resultname, CFileItemPtr item, const CVariant &parameterObject, const CVariant &validFields, CVariant &result, bool append /* = true */)
{
  CVariant object;
  bool hasFileField = false;
  bool hasThumbnailField = false;

  if (item.get())
  {
    for (unsigned int i = 0; i < validFields.size(); i++)
    {
      CStdString field = validFields[i].asString();

      if (field == "file")
        hasFileField = true;
      if (field == "thumbnail")
        hasThumbnailField = true;
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
        else if (item->HasVideoInfoTag())
        {
          switch (item->GetVideoContentType())
          {
            case VIDEODB_CONTENT_EPISODES:
              object["type"] = "episode";
              break;

            case VIDEODB_CONTENT_MUSICVIDEOS:
              object["type"] = "musicvideo";
              break;

            case VIDEODB_CONTENT_MOVIES:
              object["type"] = "movie";
              break;

            case VIDEODB_CONTENT_TVSHOWS:
              object["type"] = "tvshow";
              break;

            default:
              break;
          }
        }
        else if (item->HasPictureInfoTag())
          object["type"] = "picture";

        if (!object.isMember("type"))
          object["type"] = "unknown";
      }
    }

    if (hasThumbnailField)
    {
      if (item->HasThumbnail())
        object["thumbnail"] = item->GetThumbnailImage().c_str();
      else if (item->HasVideoInfoTag())
      {
        CStdString strPath, strFileName;
        URIUtils::Split(item->GetCachedVideoThumb(), strPath, strFileName);
        CStdString cachedThumb = strPath + "auto-" + strFileName;

        if (CFile::Exists(cachedThumb))
          object["thumbnail"] = cachedThumb;
      }
      else if (item->HasPictureInfoTag())
      {
        CStdString thumb = CTextureCache::Get().CheckAndCacheImage(CTextureCache::GetWrappedThumbURL(item->GetPath()));
        if (!thumb.empty())
          object["thumbnail"] = thumb;
      }

      if (!object.isMember("thumbnail"))
        object["thumbnail"] = "";
    }

    if (item->HasVideoInfoTag())
      FillDetails(item->GetVideoInfoTag(), item, validFields, object);
    if (item->HasMusicInfoTag())
      FillDetails(item->GetMusicInfoTag(), item, validFields, object);
    if (item->HasPictureInfoTag())
      FillDetails(item->GetPictureInfoTag(), item, validFields, object);

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
        if (picture.Load(item->GetPath()))
          *item->GetPictureInfoTag() = picture;
      }
      if (item->GetLabel().IsEmpty())
        item->SetLabel(CUtil::GetTitleFromPath(file, false));
      list.Add(item);
    }
  }

  return (list.Size() > 0);
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
  else
    return false;

  return true;
}

void CFileItemHandler::Sort(CFileItemList &items, const CVariant &parameterObject)
{
  CStdString method = parameterObject["method"].asString();
  CStdString order  = parameterObject["order"].asString();

  method = method.ToLower();
  order  = order.ToLower();

  SORT_METHOD sortmethod = SORT_METHOD_NONE;
  SORT_ORDER  sortorder  = SORT_ORDER_ASC;

  if (ParseSortMethods(method, parameterObject["ignorearticle"].asBoolean(), order, sortmethod, sortorder))
    items.Sort(sortmethod, sortorder);
}
