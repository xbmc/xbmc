
#ifndef SAVE_FILE_STATE_H__
#define SAVE_FILE_STATE_H__

#include "Job.h"
#include "FileItem.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"

/* PLEX */
#include "Client/PlexMediaServerClient.h"
#include "Client/PlexTimelineManager.h"
#include "Remote/PlexRemoteSubscriberManager.h"
#include <boost/make_shared.hpp>
#include "PlexApplication.h"
/* END PLEX */

class CSaveFileStateJob : public CJob
{
  CFileItem m_item;
  CFileItem m_item_discstack;
  CBookmark m_bookmark;
  bool      m_updatePlayCount;
public:
                CSaveFileStateJob(const CFileItem& item,
                                  const CFileItem& item_discstack,
                                  const CBookmark& bookmark,
                                  bool updatePlayCount)
                  : m_item(item),
                    m_item_discstack(item_discstack),
                    m_bookmark(bookmark),
                    m_updatePlayCount(updatePlayCount) {}
  virtual       ~CSaveFileStateJob() {}
  virtual bool  DoWork();
};

bool CSaveFileStateJob::DoWork()
{
  CStdString progressTrackingFile = m_item.GetPath();
  if (m_item.HasVideoInfoTag() && m_item.GetVideoInfoTag()->m_strFileNameAndPath.Find("removable://") == 0)
    progressTrackingFile = m_item.GetVideoInfoTag()->m_strFileNameAndPath; // this variable contains removable:// suffixed by disc label+uniqueid or is empty if label not uniquely identified
  else if (m_item.HasProperty("original_listitem_url") && 
      (URIUtils::IsPlugin(m_item.GetProperty("original_listitem_url").asString()) || URIUtils::IsBluray(m_item.GetPath()) ) )
    progressTrackingFile = m_item.GetProperty("original_listitem_url").asString();

  if (progressTrackingFile != "")
  {
    if (m_item.IsVideo())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for video item %s", __FUNCTION__, progressTrackingFile.c_str());

#ifndef __PLEX__
      CVideoDatabase videodatabase;
      if (!videodatabase.Open())
      {
        CLog::Log(LOGWARNING, "%s - Unable to open video database. Can not save file state!", __FUNCTION__);
      }
      else
#endif
      {
        bool updateListing = false;
        // No resume & watched status for livetv
        if (!m_item.IsLiveTV())
        {
          if (m_updatePlayCount)
          {
            CLog::Log(LOGDEBUG, "%s - Marking video item %s as watched", __FUNCTION__, progressTrackingFile.c_str());

            // consider this item as played
#ifndef __PLEX__
            videodatabase.IncrementPlayCount(m_item);
#endif
            m_item.GetVideoInfoTag()->m_playCount++;

            // PVR: Set recording's play count on the backend (if supported)
            if (m_item.HasPVRRecordingInfoTag())
              m_item.GetPVRRecordingInfoTag()->IncrementPlayCount();

            m_item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, true);
            updateListing = true;
            /* PLEX */
            CFileItemPtr item = boost::make_shared<CFileItem>(m_item);
            g_plexApplication.mediaServerClient->SetItemWatched(item);
            g_plexApplication.timelineManager->ReportProgress(item, PLEX_MEDIA_STATE_STOPPED);
            /* END PLEX */
          }
#ifndef __PLEX__
          else
            videodatabase.UpdateLastPlayed(m_item);
#endif

          if (!m_item.HasVideoInfoTag() || m_item.GetVideoInfoTag()->m_resumePoint.timeInSeconds != m_bookmark.timeInSeconds)
          {
#ifndef __PLEX__
            if (m_bookmark.timeInSeconds <= 0.0f)
              videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
            else
              videodatabase.AddBookMarkToFile(progressTrackingFile, m_bookmark, CBookmark::RESUME);
#else
            if (m_bookmark.timeInSeconds < 0.0f)
            {
              g_plexApplication.timelineManager->ReportProgress(boost::make_shared<CFileItem>(m_item), PLEX_MEDIA_STATE_STOPPED);
            }
            else if (m_bookmark.timeInSeconds > 0.0f)
            {
              g_plexApplication.timelineManager->ReportProgress(boost::make_shared<CFileItem>(m_item), PLEX_MEDIA_STATE_STOPPED, m_bookmark.timeInSeconds*1000);
              m_item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);
            }
#endif
            if (m_item.HasVideoInfoTag())
              m_item.GetVideoInfoTag()->m_resumePoint = m_bookmark;

            // PVR: Set/clear recording's resume bookmark on the backend (if supported)
            if (m_item.HasPVRRecordingInfoTag())
            {
              PVR::CPVRRecording *recording = m_item.GetPVRRecordingInfoTag();
              recording->SetLastPlayedPosition(m_bookmark.timeInSeconds <= 0.0f ? 0 : (int)m_bookmark.timeInSeconds);
              recording->m_resumePoint = m_bookmark;
            }

            updateListing = true;
          }
        }

        if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings)
        {
#ifndef __PLEX__
          videodatabase.SetVideoSettings(progressTrackingFile, g_settings.m_currentVideoSettings);
#endif
        }

#ifndef __PLEX__
        if (m_item.HasVideoInfoTag() && m_item.GetVideoInfoTag()->HasStreamDetails())
        {
          CFileItem dbItem(m_item);
          videodatabase.GetStreamDetails(dbItem); // Fetch stream details from the db (if any)
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
#endif

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
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio item %s", __FUNCTION__, m_item.GetPath().c_str());

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
            CLog::Log(LOGDEBUG, "%s - Marking audio item %s as listened", __FUNCTION__, m_item.GetPath().c_str());

            musicdatabase.IncrementPlayCount(m_item);
            musicdatabase.Close();
          }
        }
      }
    }
  }
  return true;
}

#endif // SAVE_FILE_STATE_H__

