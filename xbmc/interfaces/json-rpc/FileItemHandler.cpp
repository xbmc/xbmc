/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemHandler.h"

#include "AudioLibrary.h"
#include "FileOperations.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "Util.h"
#include "VideoLibrary.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h" // EPG_TAG_INVALID_UID
#include "filesystem/Directory.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/FileUtils.h"
#include "utils/ISerializable.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"

#include <map>
#include <memory>
#include <string.h>

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

bool CFileItemHandler::GetField(const std::string& field,
                                const CVariant& info,
                                const std::shared_ptr<CFileItem>& item,
                                CVariant& result,
                                bool& fetchedArt,
                                CThumbLoader* thumbLoader /* = NULL */)
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

    if (item->HasPVRChannelInfoTag())
    {
      // Translate PVR.Details.Broadcast -> List.Item.Base format
      if (field == "cast")
      {
        // string -> Video.Cast
        const std::vector<std::string> actors =
            StringUtils::Split(info[field].asString(), EPG_STRING_TOKEN_SEPARATOR);

        result[field] = CVariant(CVariant::VariantTypeArray);
        for (const auto& actor : actors)
        {
          CVariant actorVar;
          actorVar["name"] = actor;
          result[field].push_back(actorVar);
        }
        return true;
      }
      else if (field == "director" || field == "writer")
      {
        // string -> Array.String
        result[field] = StringUtils::Split(info[field].asString(), EPG_STRING_TOKEN_SEPARATOR);
        return true;
      }
      else if (field == "isrecording")
      {
        result[field] = CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(
            *item->GetPVRChannelInfoTag());
        return true;
      }
    }

    if (item->HasEPGInfoTag())
    {
      if (field == "hastimer")
      {
        const std::shared_ptr<PVR::CPVRTimerInfoTag> timer =
            CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(item->GetEPGInfoTag());
        result[field] = (timer != nullptr);
        return true;
      }
      else if (field == "hasreminder")
      {
        const std::shared_ptr<PVR::CPVRTimerInfoTag> timer =
            CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(item->GetEPGInfoTag());
        result[field] = (timer && timer->IsReminder());
        return true;
      }
      else if (field == "hastimerrule")
      {
        const std::shared_ptr<PVR::CPVRTimerInfoTag> timer =
            CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(item->GetEPGInfoTag());
        result[field] = (timer && timer->HasParent());
        return true;
      }
      else if (field == "hasrecording")
      {
        const std::shared_ptr<PVR::CPVRRecording> recording =
            CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(
                item->GetEPGInfoTag());
        result[field] = (recording != nullptr);
        return true;
      }
      else if (field == "recording")
      {
        const std::shared_ptr<PVR::CPVRRecording> recording =
            CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(
                item->GetEPGInfoTag());
        result[field] = recording ? recording->m_strFileNameAndPath : "";
        return true;
      }
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
      if (thumbLoader && !item->GetProperty("libraryartfilled").asBoolean() && !fetchedArt &&
          ((item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > -1) ||
           (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > -1)))
      {
        thumbLoader->FillLibraryArt(*item);
        fetchedArt = true;
      }

      CGUIListItem::ArtMap artMap = item->GetArt();
      CVariant artObj(CVariant::VariantTypeObject);
      for (const auto& artIt : artMap)
      {
        if (!artIt.second.empty())
          artObj[artIt.first] = CTextureUtils::GetWrappedImageURL(artIt.second);
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

    if (item->HasVideoInfoTag() && item->GetVideoContentType() == VideoDbContentType::TVSHOWS)
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

void CFileItemHandler::FillDetails(const ISerializable* info,
                                   const std::shared_ptr<CFileItem>& item,
                                   std::set<std::string>& fields,
                                   CVariant& result,
                                   CThumbLoader* thumbLoader /* = NULL */)
{
  if (info == NULL || fields.empty())
    return;

  CVariant serialization;
  info->Serialize(serialization);

  bool fetchedArt = false;

  std::set<std::string> originalFields = fields;

  for (const auto& fieldIt : originalFields)
  {
    if (GetField(fieldIt, serialization, item, result, fetchedArt, thumbLoader) &&
        result.isMember(fieldIt) && !result[fieldIt].empty())
      fields.erase(fieldIt);
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
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array();
         field != parameterObject["properties"].end_array(); ++field)
      fields.insert(field->asString());
  }

  result[resultname].reserve(static_cast<size_t>(end - start));
  for (int i = start; i < end; i++)
  {
    CFileItemPtr item = items.Get(i);
    HandleFileItem(ID, allowFile, resultname, item, parameterObject, fields, result, true, thumbLoader);
  }

  delete thumbLoader;
}

void CFileItemHandler::HandleFileItem(const char* ID,
                                      bool allowFile,
                                      const char* resultname,
                                      const std::shared_ptr<CFileItem>& item,
                                      const CVariant& parameterObject,
                                      const CVariant& validFields,
                                      CVariant& result,
                                      bool append /* = true */,
                                      CThumbLoader* thumbLoader /* = NULL */)
{
  std::set<std::string> fields;
  if (parameterObject.isMember("properties") && parameterObject["properties"].isArray())
  {
    for (CVariant::const_iterator_array field = parameterObject["properties"].begin_array();
         field != parameterObject["properties"].end_array(); ++field)
      fields.insert(field->asString());
  }

  HandleFileItem(ID, allowFile, resultname, item, parameterObject, fields, result, append, thumbLoader);
}

void CFileItemHandler::HandleFileItem(const char* ID,
                                      bool allowFile,
                                      const char* resultname,
                                      const std::shared_ptr<CFileItem>& item,
                                      const CVariant& parameterObject,
                                      const std::set<std::string>& validFields,
                                      CVariant& result,
                                      bool append /* = true */,
                                      CThumbLoader* thumbLoader /* = NULL */)
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
        if (item->HasPVRTimerInfoTag() && !item->GetPVRTimerInfoTag()->Path().empty())
          object["file"] = item->GetPVRTimerInfoTag()->Path().c_str();

        if (!object.isMember("file"))
          object["file"] = item->GetDynPath().c_str();
      }
      fields.erase(fileField);
    }

    fileField = fields.find("mediapath");
    if (fileField != fields.end())
    {
      object["mediapath"] = item->GetPath().c_str();
      fields.erase(fileField);
    }

    fileField = fields.find("dynpath");
    if (fileField != fields.end())
    {
      object["dynpath"] = item->GetDynPath().c_str();
      fields.erase(fileField);
    }

    if (ID)
    {
      if (item->HasPVRChannelInfoTag() && item->GetPVRChannelInfoTag()->ChannelID() > 0)
         object[ID] = item->GetPVRChannelInfoTag()->ChannelID();
      else if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->DatabaseID() > 0)
        object[ID] = item->GetEPGInfoTag()->DatabaseID();
      else if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag()->RecordingID() > 0)
        object[ID] = item->GetPVRRecordingInfoTag()->RecordingID();
      else if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->TimerID() > 0)
        object[ID] = item->GetPVRTimerInfoTag()->TimerID();
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
        object[ID] = item->GetMusicInfoTag()->GetDatabaseId();
      else if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > 0)
        object[ID] = item->GetVideoInfoTag()->m_iDbId;

      if (StringUtils::CompareNoCase(ID, "id") == 0)
      {
        if (item->HasPVRChannelInfoTag())
          object["type"] = "channel";
        else if (item->HasPVRRecordingInfoTag())
          object["type"] = "recording";
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
    if (item->HasPVRChannelGroupMemberInfoTag())
      FillDetails(item->GetPVRChannelGroupMemberInfoTag().get(), item, fields, object, thumbLoader);
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
  if (!file.empty() &&
      (URIUtils::IsURL(file) || (CFileUtils::Exists(file) && !CDirectory::Exists(file))))
  {
    bool added = false;
    for (int index = 0; index < list.Size(); index++)
    {
      if (list[index]->GetDynPath() == file ||
          list[index]->GetMusicInfoTag()->GetURL() == file || list[index]->GetVideoInfoTag()->GetPath() == file)
      {
        added = true;
        break;
      }
    }

    if (!added)
    {
      CFileItemPtr item = std::make_shared<CFileItem>(file, false);
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
