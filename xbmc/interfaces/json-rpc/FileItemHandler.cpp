/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <map>
#include <string.h>

#include "FileItemHandler.h"
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
#include "TextureDatabase.h"
#include "video/VideoThumbLoader.h"
#include "music/MusicThumbLoader.h"
#include "Util.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "epg/Epg.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

bool CFileItemHandler::GetField(const std::string &field, const CVariant &info, const CFileItemPtr &item, CVariant &result, bool &fetchedArt, CThumbLoader *thumbLoader /* = NULL */)
{
  if (result.isMember(field) && !result[field].empty())
    return true;

  // overwrite serialized values
  if (item)
  {
    if (field == "mimetype" && item->GetMimeType().empty())
    {
      item->FillInMimeType(false);
      result[field] = item->GetMimeType();
      return true;
    }
  }

  // check for serialized values
  if (info.isMember(field) && !info[field].isNull())
  {
    result[field] = info[field];
    return true;
  }

  // check if the field requires special handling
  if (item)
  {
    if (item->IsAlbum())
    {
      if (field == "albumlabel")
      {
        result[field] = item->GetProperty("album_label");
        return true;
      }
      if (item->HasProperty("album_" + field + "_array"))
      {
        result[field] = item->GetProperty("album_" + field + "_array");
        return true;
      }
      if (item->HasProperty("album_" + field))
      {
        result[field] = item->GetProperty("album_" + field);
        return true;
      }
    }
    
    if (item->HasProperty("artist_" + field + "_array"))
    {
      result[field] = item->GetProperty("artist_" + field + "_array");
      return true;
    }
    if (item->HasProperty("artist_" + field))
    {
      result[field] = item->GetProperty("artist_" + field);
      return true;
    }

    if (field == "art")
    {
      if (thumbLoader != NULL && item->GetArt().size() <= 0 && !fetchedArt &&
        ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) || (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
      {
        thumbLoader->FillLibraryArt(*item);
        fetchedArt = true;
      }

      CGUIListItem::ArtMap artMap = item->GetArt();
      CVariant artObj(CVariant::VariantTypeObject);
      for (CGUIListItem::ArtMap::const_iterator artIt = artMap.begin(); artIt != artMap.end(); ++artIt)
      {
        if (!artIt->second.empty())
          artObj[artIt->first] = CTextureUtils::GetWrappedImageURL(artIt->second);
      }

      result["art"] = artObj;
      return true;
    }
    
    if (field == "thumbnail")
    {
      if (thumbLoader != NULL && !item->HasArt("thumb") && !fetchedArt &&
        ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) || (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
      {
        thumbLoader->FillLibraryArt(*item);
        fetchedArt = true;
      }
      else if (item->HasPictureInfoTag() && !item->HasArt("thumb"))
        item->SetArt("thumb", CTextureUtils::GetWrappedThumbURL(item->GetPath()));
      
      if (item->HasArt("thumb"))
        result["thumbnail"] = CTextureUtils::GetWrappedImageURL(item->GetArt("thumb"));
      else
        result["thumbnail"] = "";
      
      return true;
    }
    
    if (field == "fanart")
    {
      if (thumbLoader != NULL && !item->HasArt("fanart") && !fetchedArt &&
        ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) || (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
      {
        thumbLoader->FillLibraryArt(*item);
        fetchedArt = true;
      }
      
      if (item->HasArt("fanart"))
        result["fanart"] = CTextureUtils::GetWrappedImageURL(item->GetArt("fanart"));
      else
        result["fanart"] = "";
      
      return true;
    }
    
    if (item->HasVideoInfoTag() && item->GetVideoContentType() == VIDEODB_CONTENT_TVSHOWS)
    {
      if (item->GetVideoInfoTag()->m_iSeason < 0 && field == "season")
      {
        result[field] = (int)item->GetProperty("totalseasons").asInteger();
        return true;
      }
      if (field == "watchedepisodes")
      {
        result[field] = (int)item->GetProperty("watchedepisodes").asInteger();
        return true;
      }
    }

    if (item->HasProperty(field))
    {
      result[field] = item->GetProperty(field);
      return true;
    }
  }

  return false;
}

void CFileItemHandler::FillDetails(const ISerializable *info, const CFileItemPtr &item, std::set<std::string> &fields, CVariant &result, CThumbLoader *thumbLoader /* = NULL */)
{
  if (info == NULL || fields.size() == 0)
    return;

  CVariant serialization;
  info->Serialize(serialization);

  bool fetchedArt = false;

  std::set<std::string> originalFields = fields;

  for (std::set<std::string>::const_iterator fieldIt = originalFields.begin(); fieldIt != originalFields.end(); ++fieldIt)
  {
    if (GetField(*fieldIt, serialization, item, result, fetchedArt, thumbLoader) && result.isMember(*fieldIt) && !result[*fieldIt].empty())
      fields.erase(*fieldIt);
  }
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool sortLimit /* = true */)
{
  HandleFileItemList(ID, allowFile, resultname, items, parameterObject, result, items.Size(), sortLimit);
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, int size, bool sortLimit /* = true */)
{
  int start, end;
  HandleLimits(parameterObject, result, size, start, end);

  if (sortLimit)
    Sort(items, parameterObject);
  else
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
      thumbLoader->OnLoaderStart();
  }

  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array(); field != parameterObject["properties"].end_array(); field++)
      fields.insert(field->asString());
  }

  for (int i = start; i < end; i++)
  {
    CFileItemPtr item = items.Get(i);
    HandleFileItem(ID, allowFile, resultname, item, parameterObject, fields, result, true, thumbLoader);
  }

  delete thumbLoader;
}

void CFileItemHandler::HandleFileItem(const char *ID, bool allowFile, const char *resultname, CFileItemPtr item, const CVariant &parameterObject, const CVariant &validFields, CVariant &result, bool append /* = true */, CThumbLoader *thumbLoader /* = NULL */)
{
  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array(); field != parameterObject["properties"].end_array(); field++)
      fields.insert(field->asString());
  }

  HandleFileItem(ID, allowFile, resultname, item, parameterObject, fields, result, append, thumbLoader);
}

void CFileItemHandler::HandleFileItem(const char *ID, bool allowFile, const char *resultname, CFileItemPtr item, const CVariant &parameterObject, const std::set<std::string> &validFields, CVariant &result, bool append /* = true */, CThumbLoader *thumbLoader /* = NULL */)
{
  CVariant object;
  std::set<std::string> fields(validFields.begin(), validFields.end());

  if (item.get())
  {
    std::set<std::string>::const_iterator fileField = fields.find("file");
    if (fileField != fields.end())
    {
      if (allowFile)
      {
        if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->GetPath().empty())
          object["file"] = item->GetVideoInfoTag()->GetPath().c_str();
        if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetURL().empty())
          object["file"] = item->GetMusicInfoTag()->GetURL().c_str();
        if (item->HasPVRRecordingInfoTag() && !item->GetPVRRecordingInfoTag()->GetPath().empty())
          object["file"] = item->GetPVRRecordingInfoTag()->GetPath().c_str();
        if (item->HasPVRTimerInfoTag() && !item->GetPVRTimerInfoTag()->m_strFileNameAndPath.empty())
          object["file"] = item->GetPVRTimerInfoTag()->m_strFileNameAndPath.c_str();

        if (!object.isMember("file"))
          object["file"] = item->GetPath().c_str();
      }
      fields.erase(fileField);
    }

    if (ID)
    {
      if (item->HasPVRChannelInfoTag() && item->GetPVRChannelInfoTag()->ChannelID() > 0)
         object[ID] = item->GetPVRChannelInfoTag()->ChannelID();
      else if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->UniqueBroadcastID() > 0)
         object[ID] = item->GetEPGInfoTag()->UniqueBroadcastID();
      else if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag()->m_iRecordingId > 0)
         object[ID] = item->GetPVRRecordingInfoTag()->m_iRecordingId;
      else if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->m_iTimerId > 0)
         object[ID] = item->GetPVRTimerInfoTag()->m_iTimerId;
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
        object[ID] = (int)item->GetMusicInfoTag()->GetDatabaseId();
      else if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > 0)
        object[ID] = item->GetVideoInfoTag()->m_iDbId;

      if (stricmp(ID, "id") == 0)
      {
        if (item->HasPVRChannelInfoTag())
          object["type"] = "channel";
        else if (item->HasMusicInfoTag())
        {
          std::string type = item->GetMusicInfoTag()->GetType();
          if (type == MediaTypeAlbum || type == MediaTypeSong || type == MediaTypeArtist)
            object["type"] = type;
          else if (!item->m_bIsFolder)
            object["type"] = MediaTypeSong;
        }
        else if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_type.empty())
        {
          std::string type = item->GetVideoInfoTag()->m_type;
          if (type == MediaTypeMovie || type == MediaTypeTvShow || type == MediaTypeEpisode || type == MediaTypeMusicVideo)
            object["type"] = type;
        }
        else if (item->HasPictureInfoTag())
          object["type"] = "picture";

        if (!object.isMember("type"))
          object["type"] = "unknown";

        if (fields.find("filetype") != fields.end())
        {
          if (item->m_bIsFolder)
            object["filetype"] = "directory";
          else 
            object["filetype"] = "file";
        }
      }
    }

    bool deleteThumbloader = false;
    if (thumbLoader == NULL)
    {
      if (item->HasVideoInfoTag())
        thumbLoader = new CVideoThumbLoader();
      else if (item->HasMusicInfoTag())
        thumbLoader = new CMusicThumbLoader();

      if (thumbLoader != NULL)
      {
        deleteThumbloader = true;
        thumbLoader->OnLoaderStart();
      }
    }

    if (item->HasPVRChannelInfoTag())
      FillDetails(item->GetPVRChannelInfoTag().get(), item, fields, object, thumbLoader);
    if (item->HasEPGInfoTag())
      FillDetails(item->GetEPGInfoTag().get(), item, fields, object, thumbLoader);
    if (item->HasPVRRecordingInfoTag())
      FillDetails(item->GetPVRRecordingInfoTag().get(), item, fields, object, thumbLoader);
    if (item->HasPVRTimerInfoTag())
      FillDetails(item->GetPVRTimerInfoTag().get(), item, fields, object, thumbLoader);
    if (item->HasVideoInfoTag())
      FillDetails(item->GetVideoInfoTag(), item, fields, object, thumbLoader);
    if (item->HasMusicInfoTag())
      FillDetails(item->GetMusicInfoTag(), item, fields, object, thumbLoader);
    if (item->HasPictureInfoTag())
      FillDetails(item->GetPictureInfoTag(), item, fields, object, thumbLoader);
    
    FillDetails(item.get(), item, fields, object, thumbLoader);

    if (deleteThumbloader)
      delete thumbLoader;

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

  std::string file = parameterObject["file"].asString();
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
      if (item->GetLabel().empty())
      {
        item->SetLabel(CUtil::GetTitleFromPath(file, false));
        if (item->GetLabel().empty())
          item->SetLabel(URIUtils::GetFileName(file));
      }
      list.Add(item);
    }
  }

  return (list.Size() > 0);
}

void CFileItemHandler::Sort(CFileItemList &items, const CVariant &parameterObject)
{
  SortDescription sorting;
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return;

  items.Sort(sorting);
}
