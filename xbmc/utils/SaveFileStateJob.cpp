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
#include "application/ApplicationStackHelper.h"
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

using namespace KODI;
using namespace KODI::VIDEO;

void CSaveFileState::DoWork(CFileItem& item,
                            CBookmark& bookmark,
                            bool updatePlayCount)
{
  std::string progressTrackingFile = item.GetPath();

  if (item.HasVideoInfoTag() &&
      StringUtils::StartsWith(item.GetVideoInfoTag()->m_strFileNameAndPath, "removable://"))
    progressTrackingFile =
        item.GetVideoInfoTag()
            ->m_strFileNameAndPath; // this variable contains removable:// suffixed by disc label+uniqueid or is empty if label not uniquely identified
  else if (URIUtils::IsBlurayPath(item.GetDynPath()) &&
           (item.GetVideoContentType() == VideoDbContentType::MOVIES ||
            item.GetVideoContentType() == VideoDbContentType::EPISODES))
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
        //! @todo check possible failure of BeginTransaction
        //! todo catch and handle errors and commit/rollback the transaction appropriately
        videodatabase.BeginTransaction();

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
        if (!item.IsLiveTV() && !item.m_multipleTitles)
        {
          //! @todo handle db failures to maintain data integrity
          if (updatePlayCount)
          {
            // no watched for not yet finished pvr recordings
            if (!item.IsInProgressPVRRecording())
            {
              CLog::Log(LOGDEBUG, "{} - Marking video item {} as watched", __FUNCTION__,
                        redactPath);

              // consider this item as played
              const CDateTime newLastPlayed = videodatabase.IncrementPlayCount(item);

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
            const CDateTime newLastPlayed = videodatabase.UpdateLastPlayed(item);

            if (item.HasVideoInfoTag() && newLastPlayed.IsValid())
              item.GetVideoInfoTag()->m_lastPlayed = newLastPlayed;
          }

          if (!item.HasVideoInfoTag() ||
              item.GetVideoInfoTag()->GetResumePoint().timeInSeconds != bookmark.timeInSeconds)
          {
            if (bookmark.timeInSeconds <= 0.0)
              videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
            else
              videodatabase.AddBookMarkToFile(progressTrackingFile, bookmark, CBookmark::RESUME);
            if (item.HasVideoInfoTag())
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
        videodatabase.CommitTransaction();

        if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->HasStreamDetails() &&
            !item.IsLiveTV() && !item.m_multipleTitles)
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

        // See if idFile needs updating
        const CVideoInfoTag* tag{item.HasVideoInfoTag() ? item.GetVideoInfoTag() : nullptr};

        if (tag && URIUtils::IsBlurayPath(item.GetDynPath()) &&
            tag->m_strFileNameAndPath != item.GetDynPath() && !item.m_multipleTitles)
        {
          videodatabase.BeginTransaction();
          // tag->m_iFileId contains the idFile originally played and may be different to the idFile
          // in the movie table entry if it's a non-default video version
          if (videodatabase.SetFileForMedia(item.GetDynPath(), item.GetVideoContentType(),
                                            tag->m_iDbId, tag->m_iFileId))
            videodatabase.CommitTransaction();
          else
            videodatabase.RollbackTransaction();
        }

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CFileItemPtr msgItem(new CFileItem(item));
          if (item.HasProperty("original_listitem_url"))
            msgItem->SetPath(item.GetProperty("original_listitem_url").asString());

          // Could be part of an ISO stack. In this case the bookmark is saved onto the part.
          // In order to properly update the list, we need to refresh the stack's resume point
          const auto& components = CServiceBroker::GetAppComponents();
          const auto stackHelper = components.GetComponent<CApplicationStackHelper>();
          if (stackHelper->HasRegisteredStack(item) &&
              stackHelper->GetRegisteredStackTotalTimeMs(item) == 0)
            videodatabase.GetResumePoint(*(msgItem->GetVideoInfoTag()));

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
