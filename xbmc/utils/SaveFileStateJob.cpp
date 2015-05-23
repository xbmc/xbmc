/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "SaveFileStateJob.h"
#include "pvr/PVRManager.h"
#include "settings/MediaSettings.h"
#include "network/upnp/UPnP.h"
#include "StringUtils.h"
#include "Variant.h"
#include "URIUtils.h"
#include "URL.h"
#include "log.h"
#include "video/VideoDatabase.h"
#include "interfaces/AnnouncementManager.h"
#include "Util.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "music/MusicDatabase.h"

bool CSaveFileStateJob::DoWork()
{
  std::string progressTrackingFile = m_item.GetPath();

  if (m_item.HasVideoInfoTag() && StringUtils::StartsWith(m_item.GetVideoInfoTag()->m_strFileNameAndPath, "removable://"))
    progressTrackingFile = m_item.GetVideoInfoTag()->m_strFileNameAndPath; // this variable contains removable:// suffixed by disc label+uniqueid or is empty if label not uniquely identified
  else if (m_item.HasProperty("original_listitem_url"))
  {
    // only use original_listitem_url for Python, UPnP and Bluray sources
    std::string original = m_item.GetProperty("original_listitem_url").asString();
    if (URIUtils::IsPlugin(original) || URIUtils::IsUPnP(original) || URIUtils::IsBluray(m_item.GetPath()))
      progressTrackingFile = original;
  }

  if (progressTrackingFile != "")
  {
#ifdef HAS_UPNP
    // checks if UPnP server of this file is available and supports updating
    if (URIUtils::IsUPnP(progressTrackingFile)
        && UPNP::CUPnP::SaveFileState(m_item, m_bookmark, m_updatePlayCount)) {
      return true;
    }
#endif
    if (m_item.IsVideo())
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
        bool updateListing = false;
        // No resume & watched status for livetv
        if (!m_item.IsLiveTV())
        {
          if (m_updatePlayCount)
          {
            CLog::Log(LOGDEBUG, "%s - Marking video item %s as watched", __FUNCTION__, redactPath.c_str());

            // consider this item as played
            videodatabase.IncrementPlayCount(m_item);
            m_item.GetVideoInfoTag()->m_playCount++;

            // PVR: Set recording's play count on the backend (if supported)
            if (m_item.HasPVRRecordingInfoTag())
              m_item.GetPVRRecordingInfoTag()->IncrementPlayCount();

            m_item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, true);
            updateListing = true;
          }
          else
            videodatabase.UpdateLastPlayed(m_item);

          if (!m_item.HasVideoInfoTag() || m_item.GetVideoInfoTag()->m_resumePoint.timeInSeconds != m_bookmark.timeInSeconds)
          {
            if (m_bookmark.timeInSeconds <= 0.0f)
              videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
            else
              videodatabase.AddBookMarkToFile(progressTrackingFile, m_bookmark, CBookmark::RESUME);
            if (m_item.HasVideoInfoTag())
              m_item.GetVideoInfoTag()->m_resumePoint = m_bookmark;

            // PVR: Set/clear recording's resume bookmark on the backend (if supported)
            if (m_item.HasPVRRecordingInfoTag())
            {
              PVR::CPVRRecordingPtr recording = m_item.GetPVRRecordingInfoTag();
              recording->SetLastPlayedPosition(m_bookmark.timeInSeconds <= 0.0f ? 0 : (int)m_bookmark.timeInSeconds);
              recording->m_resumePoint = m_bookmark;
            }

            // UPnP announce resume point changes to clients
            // however not if playcount is modified as that already announces
            if (m_item.IsVideoDb() && !m_updatePlayCount)
            {
              CVariant data;
              data["id"] = m_item.GetVideoInfoTag()->m_iDbId;
              data["type"] = m_item.GetVideoInfoTag()->m_type;
              ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", data);
            }

            updateListing = true;
          }
        }

        if (m_videoSettings != CMediaSettings::Get().GetDefaultVideoSettings())
        {
          videodatabase.SetVideoSettings(progressTrackingFile, m_videoSettings);
        }

        if (m_item.HasVideoInfoTag() && m_item.GetVideoInfoTag()->HasStreamDetails())
        {
          CFileItem dbItem(m_item);

          // Check whether the item's db streamdetails need updating
          if (!videodatabase.GetStreamDetails(dbItem) || dbItem.GetVideoInfoTag()->m_streamDetails != m_item.GetVideoInfoTag()->m_streamDetails)
          {
            videodatabase.SetStreamDetailsForFile(m_item.GetVideoInfoTag()->m_streamDetails, progressTrackingFile);
            updateListing = true;
          }
        }

        // in order to properly update the the list, we need to update the stack item which is held in g_application.m_stackFileItemToUpdate
        if (m_item.HasProperty("stackFileItemToUpdate"))
        {
          m_item = m_item_discstack; // as of now, the item is replaced by the discstack item
          videodatabase.GetResumePoint(*m_item.GetVideoInfoTag());
        }
        videodatabase.Close();

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CFileItemPtr msgItem(new CFileItem(m_item));
          if (m_item.HasProperty("original_listitem_url"))
            msgItem->SetPath(m_item.GetProperty("original_listitem_url").asString());
          CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE_ITEM, 1, msgItem); // 1 to update the listing as well
          g_windowManager.SendThreadMessage(message);
        }
      }
    }

    if (m_item.IsAudio())
    {
      std::string redactPath = CURL::GetRedacted(progressTrackingFile);
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio item %s", __FUNCTION__, redactPath.c_str());

      if (m_updatePlayCount)
      {
#if 0
        // Can't write to the musicdatabase while scanning for music info
        CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
        if (dialog && !dialog->IsDialogRunning())
#endif
        {
          CMusicDatabase musicdatabase;
          if (!musicdatabase.Open())
          {
            CLog::Log(LOGWARNING, "%s - Unable to open music database. Can not save file state!", __FUNCTION__);
          }
          else
          {
            // consider this item as played
            CLog::Log(LOGDEBUG, "%s - Marking audio item %s as listened", __FUNCTION__, redactPath.c_str());

            musicdatabase.IncrementPlayCount(m_item);
            musicdatabase.Close();
          }
        }
      }
    }
  }
  return true;
}
