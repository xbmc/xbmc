#ifndef PLEXNAVIGATIONHELPER_H
#define PLEXNAVIGATIONHELPER_H

#include "FileItem.h"
#include "guilib/Key.h"
#include "threads/Event.h"
#include "JobManager.h"
#include "URL.h"

class CPlexNavigationHelper : public IJobCallback
{
  public:
    CStdString navigateToItem(CFileItemPtr item, const CURL& parentURL = CURL(), int windowId = WINDOW_INVALID, bool swap = false);
    bool CacheUrl(const std::string& url, bool& cancel);
    static void navigateToNowPlaying();

private:
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    CStdString ShowPluginSearch(CFileItemPtr item);
    void ShowPluginSettings(CFileItemPtr item);
    bool m_cacheSuccess;
};

#endif // PLEXNAVIGATIONHELPER_H
