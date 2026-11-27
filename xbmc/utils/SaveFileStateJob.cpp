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
#include "URIUtils.h"
#include "URL.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "log.h"
#include "media/MediaType.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "network/upnp/UPnP.h"
#include "utils/ItemProperties.h"
#include "utils/Variant.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoManagerTypes.h"

using namespace KODI;
using namespace KODI::VIDEO;

std::string CSaveFileState::ProgressTrackingFileOf(const CFileItem& item)
{
  // A disc in a drive is known by its label and serial, which playback reads its state back by
  if (item.HasVideoInfoTag() &&
      item.GetVideoInfoTag()->m_strFileNameAndPath.starts_with("removable://"))
    return item.GetVideoInfoTag()->m_strFileNameAndPath;

  // An add-on's item is listed, and so read back, by its own url, whatever that resolves to
  if (const std::string original{
          item.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString()};
      URIUtils::IsPlugin(original) && !IsVideoDb(item))
    return original;

  std::string progressTrackingFile = item.GetPath();
  if (CUtil::UseDynPathForAddOrUpdate(item))
  {
    progressTrackingFile = item.GetDynPath();
  }
  else if (item.HasVideoInfoTag() && IsVideoDb(item))
  {
    progressTrackingFile =
        item.GetVideoInfoTag()
            ->m_strFileNameAndPath; // we need the file url of the video db item to create the bookmark
  }
  else if (item.HasProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL))
  {
    // only use original_listitem_url for Python, UPnP and Bluray sources
    std::string original = item.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString();
    if (URIUtils::IsPlugin(original) || URIUtils::IsUPnP(original) ||
        URIUtils::IsBlurayPath(item.GetPath()))
      progressTrackingFile = original;
  }
  return progressTrackingFile;
}

void CSaveFileState::DoWork(CFileItem& item, CBookmark& bookmark, bool updatePlayCount)
{
  const std::string progressTrackingFile{ProgressTrackingFileOf(item)};
  if (!progressTrackingFile.empty())
  {
#ifdef HAS_UPNP
    // checks if UPnP server of this file is available and supports updating
    if (URIUtils::IsUPnP(progressTrackingFile)
        && UPNP::CUPnP::SaveFileState(item, bookmark, updatePlayCount))
    {
      if (auto* gui = CServiceBroker::GetGUI())
      {
        CFileItem updatedItem(item);
        if (updatedItem.HasVideoInfoTag())
          updatedItem.GetVideoInfoTag()->SetResumePoint(bookmark);
        if (updatedItem.HasProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL))
          updatedItem.SetPath(
              updatedItem.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString());
        else
          updatedItem.SetPath(
              progressTrackingFile); // fallback to progressTrackingFile which should be the upnp path

        CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0,
                            std::make_shared<CFileItem>(updatedItem));
        gui->GetWindowManager().SendThreadMessage(message);
      }
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
            item.ClearProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL);
            tag->m_streamDetails = streams;
          }
        }

        // Whether the item moves below to the file it was played from
        const bool updateNeeded{CVideoDatabase::PlayedItemMovesFile(item)};

        bool updateListing = false;
        // Whether a resume point was written, so that the item can move to the file below
        bool resumePointWritten{true};
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
                if (item.GetVideoInfoTag()->IncrementPlayCount())
                  item.SetProperty(ITEM::PROPERTY::PLAYCOUNT_INCREMENTED, CVariant{true});

                if (newLastPlayed.IsValid())
                  item.GetVideoInfoTag()->m_lastPlayed = newLastPlayed;

                const MEDIA::TYPE type{item.GetVideoInfoTag()->GetMediaType()};
                CServiceBroker::GetAnnouncementManager()->Announce(
                    ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::Update{
                        .type = type,
                        .otherType = type == MEDIA::TYPE::NONE ? item.GetVideoInfoTag()->m_type
                                                               : std::string{},
                        .id = item.GetVideoInfoTag()->m_iDbId}});
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

          // The item's resume point is that of the file it was played from. A file it moves to
          // below has none of its own yet, and the old file may be deleted with its resume point.
          if (!item.HasVideoInfoTag() || updateNeeded ||
              item.GetVideoInfoTag()->GetResumePoint().timeInSeconds != bookmark.timeInSeconds)
          {
            videodatabase.BeginTransaction();
            bool success{true};
            // A finished playback clears the resume point with a negative position
            if (bookmark.timeInSeconds <= 0.0)
              success = videodatabase.ClearBookMarksOfFile(
                  progressTrackingFile, CBookmark::RESUME,
                  item.HasVideoInfoTag() && item.GetVideoInfoTag()->GetResumePoint().IsPartWay());
            else
            {
              success = videodatabase.AddBookMarkToFile(progressTrackingFile, bookmark,
                                                        CBookmark::RESUME);
              resumePointWritten = success;
            }
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
              const MEDIA::TYPE type{item.GetVideoInfoTag()->GetMediaType()};
              CServiceBroker::GetAnnouncementManager()->Announce(
                  ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::Update{
                      .type = type,
                      .otherType = type == MEDIA::TYPE::NONE ? item.GetVideoInfoTag()->m_type
                                                             : std::string{},
                      .id = item.GetVideoInfoTag()->m_iDbId}});
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
              (dbItem.GetVideoInfoTag()->m_streamDetails !=
                   item.GetVideoInfoTag()->m_streamDetails &&
               dbItem.GetVideoInfoTag()->m_streamDetails.ShouldUpdateWithNewDetails(
                   item.GetVideoInfoTag()->m_streamDetails)))
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
        int replacedFileId{-1};
        MEDIA::TYPE mediaType{MEDIA::TYPE::NONE};
        int idMedia{-1};
        if (updateNeeded && !resumePointWritten)
        {
          // Moving could delete the old file, and with it the only resume point left
          CLog::LogF(LOGWARNING,
                     "{} {} stays on file {}, as its resume point could not be saved to {}",
                     item.GetVideoInfoTag()->GetMediaType(), item.GetVideoInfoTag()->m_iDbId,
                     item.GetVideoInfoTag()->m_iFileId, redactPath);
        }
        else if (const auto move{videodatabase.MoveFileOfPlayedItem(item, progressTrackingFile)})
        {
          item.GetVideoInfoTag()->m_iFileId = move->idFile;
          if (move->replacedIdFile > 0)
          {
            CLog::LogF(LOGDEBUG, "{} {} now uses file {} ({}) instead of file {}", move->mediaType,
                       move->idMedia, move->idFile, redactPath, move->replacedIdFile);
            replacedFileId = move->replacedIdFile;
            mediaType = move->mediaType;
            idMedia = move->idMedia;
            updateListing = true;
          }
        }

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CFileItemPtr msgItem(new CFileItem(item));
          if (item.HasProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL))
            msgItem->SetPath(item.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString());
          if (replacedFileId > 0)
            msgItem->SetProperty(ITEM::PROPERTY::REPLACED_FILE_ID, replacedFileId);

          int64_t updateFlags = 0;
          if (item.IsStrm() && !item.IsPlugin() && item.GetDynPath() != item.GetPath())
          {
            // Prevent replacing the library title with the remote resource name.
            updateFlags |= GUI_MSG_FLAG_KEEP_CURRENT_LABELS;
          }
          CGUIMessage message(GUI_MSG_NOTIFY_ALL,
                              CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0,
                              GUI_MSG_UPDATE_ITEM, updateFlags, msgItem);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);

          // Widgets reload on the announcement, which must follow the file change
          if (replacedFileId > 0)
            CVideoDatabase::AnnounceUpdate(mediaType, idMedia);
        }

        CLog::LogF(LOGDEBUG, "Finished saving file state for video item {} (listing update {})",
                   redactPath, updateListing ? "sent" : "not needed");

        videodatabase.Close();
      }
    }

    if (MUSIC::IsAudio(item))
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "{} - Saving file state for audio item {}", __FUNCTION__, redactPath);

      CMusicDatabase musicdatabase;
      if (!musicdatabase.Open())
      {
        CLog::LogF(LOGWARNING, "Unable to open music database. Can not save file state!");
      }
      else
      {
        bool updated{false};
        if (updatePlayCount)
        {
          // consider this item as played
          CLog::LogF(LOGDEBUG, "Marking audio item {} as listened", redactPath);
          musicdatabase.IncrementPlayCount(item);
          updated = true;
        }

        if (MUSIC::IsAudioBook(item) && item.GetEndOffset() > 0) // Audio file with chapters
        {
          int bookmarkMs;
          if (bookmark.timeInSeconds > 0.0)
            bookmarkMs = static_cast<int>(item.GetStartOffset() +
                                          CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));
          else
            bookmarkMs = 0; // bookmarkMs of <= 0 clears the bookmark

          CLog::LogF(LOGDEBUG, "Setting resume point at {} for audio item {}", bookmarkMs,
                     redactPath);
          musicdatabase.SetResumeBookmarkForAudioBook(item, bookmarkMs);
          updated = true;
        }

        musicdatabase.Close();

        // UPnP announce resume point changes to clients
        // however not if playcount is modified as that already announces
        if (updated && MUSIC::IsMusicDb(item))
        {
          const MEDIA::TYPE type{item.GetMusicInfoTag()->GetMediaType()};
          CServiceBroker::GetAnnouncementManager()->Announce(
              ANNOUNCEMENT::AudioLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::Update{
                  .type = type,
                  .otherType =
                      type == MEDIA::TYPE::NONE ? item.GetMusicInfoTag()->GetType() : std::string{},
                  .id = item.GetMusicInfoTag()->GetDatabaseId()}});
        }
      }
    }
  }
}
