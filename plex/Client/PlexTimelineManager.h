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
#include <queue>
#include <boost/shared_ptr.hpp>

class CPlexTimelineContext
{
  public:
    CPlexTimelineContext(ePlexMediaType type = PLEX_MEDIA_TYPE_UNKNOWN) : state(PLEX_MEDIA_STATE_STOPPED), continuing(false), type(type) {}

    CPlexTimelineContext(const CPlexTimelineContext& other)
    {
      *this = other;
    }

    operator bool() const
    {
      return (bool)item;
    }

    bool operator==(const CPlexTimelineContext& other) const
    {
      if ((item && !other.item) ||
          (!item && other.item))
        return false;

      if (type != other.type)
        return false;

      if (state == other.state &&
          continuing == other.continuing &&
          item->GetPath() == other.item->GetPath())
        return true;

      return false;
    }

    const CPlexTimelineContext& operator=(const CPlexTimelineContext& other)
    {
      type = other.type;
      state = other.state;
      continuing = other.continuing;
      item = other.item;
      currentPosition = other.currentPosition;
      return *this;
    }

    ePlexMediaState state;
    ePlexMediaType type;
    bool continuing;
    CFileItemPtr item;
    int64_t currentPosition;
};

typedef std::map<ePlexMediaType, CPlexTimelineContext> PlexTimelineContextMap;

class CPlexTimelineManager : public IPlexGlobalTimeout
{
  public:
    CPlexTimelineManager();

    void ReportProgress(const CFileItemPtr &newItem, ePlexMediaState state, uint64_t currentPosition=0, bool force=false);
    std::vector<CUrlOptions> GetCurrentTimeLines(int commandID = -1);
    CXBMCTinyXML GetCurrentTimeLinesXML(CPlexRemoteSubscriberPtr subscriber);
    CUrlOptions GetCurrentTimeline(const CPlexTimelineContext& context, bool forServer=true);

    CXBMCTinyXML WaitForTimeline(CPlexRemoteSubscriberPtr subscriber);
    uint64_t GetItemDuration(CFileItemPtr item);

    void SendTimelineToSubscriber(CPlexRemoteSubscriberPtr subscriber);
    void SendTimelineToSubscribers(bool delay = false);

    void SetTextFieldFocused(bool focused, const CStdString &name="field", const CStdString &contents=CStdString(), bool isSecure=false);
    void UpdateLocation();

    void Stop();

    std::string GetCurrentFocusedTextField() const { return m_textFieldName; }
    bool IsTextFieldFocused() const { return m_textFieldFocused; }

    CStdString TimerName() const { return "timelineManager"; }

  private:
    void OnTimeout();
    void ReportProgress(const CPlexTimelineContext &context, bool force);

    CCriticalSection m_timelineLock;
    PlexTimelineContextMap m_contexts;

    CPlexTimer m_subTimer;
    CPlexTimer m_serverTimer;

    bool m_stopped;
    bool m_textFieldFocused;
    CStdString m_textFieldName;
    CStdString m_textFieldContents;
    bool m_textFieldSecure;

    void NotifyPollers();

    CCriticalSection m_pollerTimelineLock;
    std::queue<PlexTimelineContextMap> m_pollerContexts;
};

typedef boost::shared_ptr<CPlexTimelineManager> CPlexTimelineManagerPtr;

#endif // PLEXTIMELINEMANAGER_H
