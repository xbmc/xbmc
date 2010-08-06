
#ifndef SAVE_FILE_STATE_H__
#define SAVE_FILE_STATE_H__

#include "Job.h"
#include "FileItem.h"

class CSaveFileStateJob : public CJob
{
  CFileItem m_item;
  CBookmark m_bookmark;
  bool      m_updatePlayCount;
public:
                CSaveFileStateJob(const CFileItem& item,
                                  const CBookmark& bookmark,
                                  bool updatePlayCount)
                  : m_item(item),
                    m_bookmark(bookmark),
                    m_updatePlayCount(updatePlayCount) {}
  virtual       ~CSaveFileStateJob() {}
  virtual bool  DoWork();
};

bool CSaveFileStateJob::DoWork()
{
  CStdString progressTrackingFile = m_item.m_strPath;

  if (progressTrackingFile != "")
  {
    if (m_item.IsVideo())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for video item %s", __FUNCTION__, progressTrackingFile.c_str());

      CVideoDatabase videodatabase;
      if (videodatabase.Open())
      {
        // No resume & watched status for livetv
        if (!m_item.IsLiveTV())
        {
          if (m_updatePlayCount)
          {
            CLog::Log(LOGDEBUG, "%s - Marking video item %s as watched", __FUNCTION__, progressTrackingFile.c_str());

            // consider this item as played
            videodatabase.IncrementPlayCount(m_item);
            CUtil::DeleteVideoDatabaseDirectoryCache();
            CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
            g_windowManager.SendMessage(message);
          }

          if (m_bookmark.timeInSeconds < 0.0f)
          {
            videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
          }
          else if (m_bookmark.timeInSeconds > 0.0f)
          {
            videodatabase.AddBookMarkToFile(progressTrackingFile, m_bookmark, CBookmark::RESUME);
          }
        }

        if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings)
        {
          videodatabase.SetVideoSettings(progressTrackingFile, g_settings.m_currentVideoSettings);
        }

        if ((m_item.IsDVDImage() ||
             m_item.IsDVDFile()    ) &&
             m_item.HasVideoInfoTag() &&
             m_item.GetVideoInfoTag()->HasStreamDetails())
        {
          videodatabase.SetStreamDetailsForFile(m_item.GetVideoInfoTag()->m_streamDetails,progressTrackingFile);
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CGUIMessage message(GUI_MSG_NOTIFY_ALL, g_windowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
          g_windowManager.SendMessage(message);
        }

        videodatabase.Close();
      }
    }

    if (m_item.IsAudio())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio item %s", __FUNCTION__, progressTrackingFile.c_str());

      if (m_updatePlayCount)
      {
#if 0
        // Can't write to the musicdatabase while scanning for music info
        CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
        if (dialog && !dialog->IsDialogRunning())
#endif
        {
          // consider this item as played
          CLog::Log(LOGDEBUG, "%s - Marking audio item %s as listened", __FUNCTION__, progressTrackingFile.c_str());

          CMusicDatabase musicdatabase;
          if (musicdatabase.Open())
          {
            musicdatabase.IncrTop100CounterByFileName(progressTrackingFile);
            musicdatabase.Close();
          }
        }
      }
    }
  }
  return true;
}

#endif // SAVE_FILE_STATE_H__

