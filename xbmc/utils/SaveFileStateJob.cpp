/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SaveFileStateJob.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "URL.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "log.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "network/upnp/UPnP.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "utils/Variant.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"

#include <chrono>

using namespace KODI;
using namespace KODI::VIDEO;
using namespace std::chrono_literals;

void CSaveFileState::DoWork(CFileItem& item,
                            CBookmark& bookmark,
                            bool updatePlayCount)
{
  std::string progressTrackingFile = item.GetPath();
  if (item.GetProperty("new_stack_path").asBoolean(false) ||
      (URIUtils::IsBlurayPath(item.GetDynPath()) &&
       (item.GetVideoContentType() == VideoDbContentType::MOVIES ||
        item.GetVideoContentType() == VideoDbContentType::EPISODES ||
        item.GetVideoContentType() == VideoDbContentType::UNKNOWN /* Removable bluray */)))
    progressTrackingFile = item.GetDynPath();
  else if (item.HasVideoInfoTag() && IsVideoDb(item))
    progressTrackingFile =
        item.GetVideoInfoTag()
            ->m_strFileNameAndPath; // we need the file url of the video db item to create the bookmark
  else if (item.HasProperty("original_listitem_url"))
  {
    // only use original_listitem_url for Python, UPnP and Bluray sources
    std::string original = item.GetProperty("original_listitem_url").asString();
    if (URIUtils::IsPlugin(original) || URIUtils::IsUPnP(original) ||
        URIUtils::IsBlurayPath(item.GetPath()))
      progressTrackingFile = original;
  }

  if (!progressTrackingFile.empty())
  {
#ifdef HAS_UPNP
    // checks if UPnP server of this file is available and supports updating
    if (URIUtils::IsUPnP(progressTrackingFile)
        && UPNP::CUPnP::SaveFileState(item, bookmark, updatePlayCount))
    {
      return;
    }
#endif
    if (IsVideo(item))
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "{} - Saving file state for video item {}", __FUNCTION__, redactPath);

      CVideoDatabase videodatabase;
      if (!videodatabase.Open())
      {
        CLog::Log(LOGWARNING, "{} - Unable to open video database. Can not save file state!",
                  __FUNCTION__);
      }
      else
      {
        if (URIUtils::IsPlugin(progressTrackingFile) && !(item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId >= 0))
        {
          // FileItem from plugin can lack information, make sure all needed fields are set
          CVideoInfoTag *tag = item.GetVideoInfoTag();
          CStreamDetails streams = tag->m_streamDetails;
          if (videodatabase.LoadVideoInfo(progressTrackingFile, *tag))
          {
            item.SetPath(progressTrackingFile);
            item.ClearProperty("original_listitem_url");
            tag->m_streamDetails = streams;
          }
        }

        bool updateListing = false;
        // No resume & watched status for livetv
        if (!item.IsLiveTV())
        {
          if (updatePlayCount)
          {
            // no watched for not yet finished pvr recordings
            if (!item.IsInProgressPVRRecording())
            {
              CLog::Log(LOGDEBUG, "{} - Marking video item {} as watched", __FUNCTION__,
                        redactPath);

              // consider this item as played
              videodatabase.BeginTransaction();
              const CDateTime newLastPlayed = videodatabase.IncrementPlayCount(item);
              if (newLastPlayed.IsValid())
                videodatabase.CommitTransaction();
              else
                videodatabase.RollbackTransaction();

              item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED);
              updateListing = true;

              if (item.HasVideoInfoTag())
              {
                if (item.IsPVRRecording())
                  CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().IncrementPlayCount(
                      item);
                else
                  item.GetVideoInfoTag()->IncrementPlayCount();

                if (newLastPlayed.IsValid())
                  item.GetVideoInfoTag()->m_lastPlayed = newLastPlayed;

                CVariant data;
                data["id"] = item.GetVideoInfoTag()->m_iDbId;
                data["type"] = item.GetVideoInfoTag()->m_type;
                CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary,
                                                                   "OnUpdate", data);
              }
            }
          }
          else
          {
            videodatabase.BeginTransaction();
            const CDateTime newLastPlayed = videodatabase.UpdateLastPlayed(item);
            if (newLastPlayed.IsValid())
              videodatabase.CommitTransaction();
            else
              videodatabase.RollbackTransaction();

            if (item.HasVideoInfoTag() && newLastPlayed.IsValid())
              item.GetVideoInfoTag()->m_lastPlayed = newLastPlayed;
          }

          if (!item.HasVideoInfoTag() ||
              item.GetVideoInfoTag()->GetResumePoint().timeInSeconds != bookmark.timeInSeconds)
          {
            videodatabase.BeginTransaction();
            bool success{true};
            if (bookmark.timeInSeconds <= 0.0)
              success = videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
            else
              success = videodatabase.AddBookMarkToFile(progressTrackingFile, bookmark,
                                                        CBookmark::RESUME);
            if (success)
              videodatabase.CommitTransaction();
            else
              videodatabase.RollbackTransaction();

            if (item.HasVideoInfoTag() && success)
              item.GetVideoInfoTag()->SetResumePoint(bookmark);

            // UPnP announce resume point changes to clients
            // however not if playcount is modified as that already announces
            if (item.HasVideoInfoTag() && !updatePlayCount)
            {
              CVariant data;
              data["id"] = item.GetVideoInfoTag()->m_iDbId;
              data["type"] = item.GetVideoInfoTag()->m_type;
              CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary,
                                                                 "OnUpdate", data);
            }

            updateListing = true;
          }
        }

        if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->HasStreamDetails() &&
            !item.IsLiveTV())
        {
          CFileItem dbItem(item);

          // Check whether the item's db streamdetails need updating
          if (!videodatabase.GetStreamDetails(dbItem) ||
              dbItem.GetVideoInfoTag()->m_streamDetails != item.GetVideoInfoTag()->m_streamDetails)
          {
            videodatabase.BeginTransaction();

            if (videodatabase.SetStreamDetailsForFile(item.GetVideoInfoTag()->m_streamDetails,
                                                      progressTrackingFile))
            {
              videodatabase.CommitTransaction();
              updateListing = true;
            }
            else
            {
              videodatabase.RollbackTransaction();
            }
          }
        }

        // See if idFile of library item needs updating
        const CVideoInfoTag* tag{item.HasVideoInfoTag() ? item.GetVideoInfoTag() : nullptr};

        const bool updateNeeded{
            [&item, &tag]
            {
              if (!tag || tag->m_iFileId < 0)
                return false; // No tag or file to update
              if (tag->m_iDbId < 0 && item.GetVideoContentType() != VideoDbContentType::UNKNOWN)
                return false; // No video db item to update
              if (URIUtils::IsBlurayPath(item.GetDynPath()) &&
                  !URIUtils::IsStack(tag->m_strFileNameAndPath) &&
                  tag->m_strFileNameAndPath != item.GetDynPath())
                return true; // Bluray path to update
              if (item.GetProperty("new_stack_path").asBoolean(false))
                return true; // Stack path to update
              return false;
            }()};

        if (updateNeeded)
        {
          videodatabase.BeginTransaction();
          // tag->m_iFileId contains the idFile originally played and may be different to the idFile
          // in the movie table entry if it's a non-default video version
          const int newFileId{videodatabase.SetFileForMedia(
              progressTrackingFile, item.GetVideoContentType(), tag->m_iDbId,
              CVideoDatabase::FileRecord{.m_idFile = tag->m_iFileId,
                                         .m_playCount = tag->GetPlayCount(),
                                         .m_lastPlayed = tag->m_lastPlayed,
                                         .m_dateAdded = tag->m_dateAdded})};
          if (newFileId > 0)
          {
            videodatabase.CommitTransaction();
            item.GetVideoInfoTag()->m_iFileId = newFileId;
          }
          else
            videodatabase.RollbackTransaction();
        }

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CFileItemPtr msgItem(new CFileItem(item));
          if (item.HasProperty("original_listitem_url"))
            msgItem->SetPath(item.GetProperty("original_listitem_url").asString());

          CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE_ITEM, 0, msgItem);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
        }

        videodatabase.Close();
      }
    }

    if (MUSIC::IsAudio(item))
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "{} - Saving file state for audio item {}", __FUNCTION__, redactPath);

      CMusicDatabase musicdatabase;
      if (updatePlayCount)
      {
        if (!musicdatabase.Open())
        {
          CLog::Log(LOGWARNING, "{} - Unable to open music database. Can not save file state!",
                    __FUNCTION__);
        }
        else
        {
          // consider this item as played
          CLog::Log(LOGDEBUG, "{} - Marking audio item {} as listened", __FUNCTION__, redactPath);

          musicdatabase.IncrementPlayCount(item);
          musicdatabase.Close();

          // UPnP announce resume point changes to clients
          // however not if playcount is modified as that already announces
          if (MUSIC::IsMusicDb(item))
          {
            CVariant data;
            data["id"] = item.GetMusicInfoTag()->GetDatabaseId();
            data["type"] = item.GetMusicInfoTag()->GetType();
            CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary,
                                                               "OnUpdate", data);
          }
        }
      }

      if (MUSIC::IsAudioBook(item))
      {
        musicdatabase.Open();
        musicdatabase.SetResumeBookmarkForAudioBook(
            item, item.GetStartOffset() + CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));
        musicdatabase.Close();
      }
    }
  }
}
