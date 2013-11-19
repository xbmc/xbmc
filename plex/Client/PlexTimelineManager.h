#ifndef PLEXTIMELINEMANAGER_H
#define PLEXTIMELINEMANAGER_H

#include "StdString.h"
#include "FileItem.h"
#include "Utility/PlexTimer.h"
#include "threads/Event.h"
#include "threads/Timer.h"
#include "UrlOptions.h"
#include "FileItem.h"
#include "Remote/PlexRemoteSubscriberManager.h"
#include "utils/XBMCTinyXML.h"

#include <map>
#include <boost/shared_ptr.hpp>

class CPlexTimelineManager : public IPlexGlobalTimeout
{
  public:
    enum MediaType {
      MUSIC,
      PHOTO,
      VIDEO,
      UNKNOWN
    };

    enum MediaState {
      MEDIA_STATE_STOPPED,
      MEDIA_STATE_PLAYING,
      MEDIA_STATE_BUFFERING,
      MEDIA_STATE_PAUSED
    };

    CPlexTimelineManager();

    void ReportProgress(const CFileItemPtr &currentItem, MediaState state, uint64_t currentPosition=0);
    std::vector<CUrlOptions> GetCurrentTimeLines(int commandID = -1);
    CXBMCTinyXML GetCurrentTimeLinesXML(CPlexRemoteSubscriberPtr subscriber);
    CUrlOptions GetCurrentTimeline(CPlexTimelineManager::MediaType type, bool forServer=true);

    static std::string StateToString(MediaState state);
    static std::string MediaTypeToString(CPlexTimelineManager::MediaType type);
    CPlexTimelineManager::MediaType GetMediaType(CFileItemPtr item);
    CPlexTimelineManager::MediaType GetMediaType(const CStdString &typestr);
    CXBMCTinyXML WaitForTimeline(CPlexRemoteSubscriberPtr subscriber);
    uint64_t GetItemDuration(CFileItemPtr item);

    void SendTimelineToSubscriber(CPlexRemoteSubscriberPtr subscriber);
    void SendTimelineToSubscribers();

    void SetTextFieldFocused(bool focused, const CStdString &name="field", const CStdString &contents=CStdString(), bool isSecure=false);
    void UpdateLocation();

    void Stop();

    std::string GetCurrentFocusedTextField() const { return m_textFieldName; }
    bool IsTextFieldFocused() const { return m_textFieldFocused; }

  private:
    void OnTimeout();

    std::map<MediaType, CFileItemPtr> m_currentItems;
    std::map<MediaType, MediaState> m_currentStates;

    CPlexTimer m_subTimer;
    CPlexTimer m_serverTimer;

    bool m_stopped;
    bool m_textFieldFocused;
    CStdString m_textFieldName;
    CStdString m_textFieldContents;
    bool m_textFieldSecure;

    void NotifyPollers();
};

typedef boost::shared_ptr<CPlexTimelineManager> CPlexTimelineManagerPtr;

#endif // PLEXTIMELINEMANAGER_H
