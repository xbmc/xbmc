/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SaveFileStateJob.h"
#include "network/upnp/UPnP.h"
#include "StringUtils.h"
#include "utils/Variant.h"
#include "URIUtils.h"
#include "URL.h"
#include "log.h"
#include "video/VideoDatabase.h"
#include "interfaces/AnnouncementManager.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "music/MusicDatabase.h"
#include "xbmc/music/tags/MusicInfoTag.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "FileItem.h"
#include "video/Bookmark.h"

void CSaveFileState::DoWork(CFileItem& item,
                            CBookmark& bookmark,
                            bool updatePlayCount)
{
  std::string progressTrackingFile = item.GetPath();

  if (item.HasVideoInfoTag() && StringUtils::StartsWith(item.GetVideoInfoTag()->m_strFileNameAndPath, "removable://"))
    progressTrackingFile = item.GetVideoInfoTag()->m_strFileNameAndPath; // this variable contains removable:// suffixed by disc label+uniqueid or is empty if label not uniquely identified
  else if (item.HasVideoInfoTag() && item.IsVideoDb())
    progressTrackingFile = item.GetVideoInfoTag()->m_strFileNameAndPath; // we need the file url of the video db item to create the bookmark
  else if (item.HasProperty("original_listitem_url"))
  {
    // only use original_listitem_url for Python, UPnP and Bluray sources
    std::string original = item.GetProperty("original_listitem_url").asString();
    if (URIUtils::IsPlugin(original) || URIUtils::IsUPnP(original) || URIUtils::IsBluray(item.GetPath()))
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
    if (item.IsVideo())
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "%s - Saving file state for video item %s", __FUNCTION__, redactPath.c_str());

      CVideoDatabase videodatabase;
      if (!videodatabase.Open())
      {
        CLog::Log(LOGWARNING, "%s - Unable to open video database. Can not save file state!", __FUNCTION__);
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
              CLog::Log(LOGDEBUG, "%s - Marking video item %s as watched", __FUNCTION__, redactPath.c_str());

              // consider this item as played
              videodatabase.IncrementPlayCount(item);
              item.GetVideoInfoTag()->IncrementPlayCount();

              item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, true);
              updateListing = true;

              if (item.HasVideoInfoTag())
              {
                CVariant data;
                data["id"] = item.GetVideoInfoTag()->m_iDbId;
                data["type"] = item.GetVideoInfoTag()->m_type;
                CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", data);
              }
            }
          }
          else
            videodatabase.UpdateLastPlayed(item);

          if (!item.HasVideoInfoTag() ||
              item.GetVideoInfoTag()->GetResumePoint().timeInSeconds != bookmark.timeInSeconds)
          {
            if (bookmark.timeInSeconds <= 0.0f)
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
              CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", data);
            }

            updateListing = true;
          }
        }

        if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->HasStreamDetails())
        {
          CFileItem dbItem(item);

          // Check whether the item's db streamdetails need updating
          if (!videodatabase.GetStreamDetails(dbItem) ||
              dbItem.GetVideoInfoTag()->m_streamDetails != item.GetVideoInfoTag()->m_streamDetails)
          {
            videodatabase.SetStreamDetailsForFile(item.GetVideoInfoTag()->m_streamDetails, progressTrackingFile);
            updateListing = true;
          }
        }

        // Could be part of an ISO stack. In this case the bookmark is saved onto the part.
        // In order to properly update the list, we need to refresh the stack's resume point
        CApplicationStackHelper& stackHelper = g_application.GetAppStackHelper();
        if (stackHelper.HasRegisteredStack(item) && stackHelper.GetRegisteredStackTotalTimeMs(item) == 0)
          videodatabase.GetResumePoint(*(stackHelper.GetRegisteredStack(item)->GetVideoInfoTag()));

        videodatabase.Close();

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CFileItemPtr msgItem(new CFileItem(item));
          if (item.HasProperty("original_listitem_url"))
            msgItem->SetPath(item.GetProperty("original_listitem_url").asString());
          CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE_ITEM, 0, msgItem);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
        }
      }
    }

    if (item.IsAudio())
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio item %s", __FUNCTION__, redactPath.c_str());

      CMusicDatabase musicdatabase;
      if (updatePlayCount)
      {
        if (!musicdatabase.Open())
        {
          CLog::Log(LOGWARNING, "%s - Unable to open music database. Can not save file state!", __FUNCTION__);
        }
        else
        {
          // consider this item as played
          CLog::Log(LOGDEBUG, "%s - Marking audio item %s as listened", __FUNCTION__, redactPath.c_str());

          musicdatabase.IncrementPlayCount(item);
          musicdatabase.Close();

          // UPnP announce resume point changes to clients
          // however not if playcount is modified as that already announces
          if (item.IsMusicDb())
          {
            CVariant data;
            data["id"] = item.GetMusicInfoTag()->GetDatabaseId();
            data["type"] = item.GetMusicInfoTag()->GetType();
            CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
          }
        }
      }

      if (item.IsAudioBook())
      {
        musicdatabase.Open();
        musicdatabase.SetResumeBookmarkForAudioBook(item, item.m_lStartOffset + CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));
        musicdatabase.Close();
      }
    }
  }
}
