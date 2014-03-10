#ifndef PLEXTIMELINE_H
#define PLEXTIMELINE_H

#include "PlexTypes.h"
#include "URL.h"
#include "UrlOptions.h"

#include "XBMCTinyXML.h"
#include "FileItem.h"

class CPlexTimeline
{
  public:
    CPlexTimeline(ePlexMediaType type = PLEX_MEDIA_TYPE_UNKNOWN) : m_state(PLEX_MEDIA_STATE_STOPPED), m_continuing(false), m_type(type) {}

    CPlexTimeline(const CPlexTimeline& other)
    {
      *this = other;
    }

    operator bool() const
    {
      return (bool)m_item;
    }

    bool compare(const CPlexTimeline& other)
    {
      if (!other)
        return false;

      if ((m_item && !other.m_item) ||
          (!m_item && other.m_item))
        return false;

      if (m_type != other.m_type)
        return false;

      if (m_state == other.m_state &&
          m_continuing == other.m_continuing &&
          m_item->GetPath() == other.m_item->GetPath())
        return true;

      return false;
    }

    CUrlOptions getTimeline(bool forServer=true);

    void setState(const ePlexMediaState& state) { m_state = state; }
    ePlexMediaState getState() const { return m_state; }

    ePlexMediaType getType() const { return m_type; }
    void setType(const ePlexMediaType &type) { m_type = type; }

    bool getContinuing() const { return m_continuing; }
    void setContinuing(bool continuing) { m_continuing = continuing; }

    CFileItemPtr getItem() const { return m_item; }
    void setItem(const CFileItemPtr &item) { m_item = item; }

    int64_t getCurrentPosition() const { return m_currentPosition; }
    void setCurrentPosition(const int64_t &currentPosition) { m_currentPosition = currentPosition; }

  private:
    ePlexMediaState m_state;
    ePlexMediaType m_type;
    bool m_continuing;
    CFileItemPtr m_item;
    int64_t m_currentPosition;
};

typedef boost::shared_ptr<CPlexTimeline> CPlexTimelinePtr;
typedef std::map<ePlexMediaType, CPlexTimelinePtr> CPlexTimelineMap;

class CPlexTimelineCollection
{
  public:
    CPlexTimelineCollection()
    {
      m_timelines[PLEX_MEDIA_TYPE_MUSIC] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_MUSIC));
      m_timelines[PLEX_MEDIA_TYPE_VIDEO] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_VIDEO));
      m_timelines[PLEX_MEDIA_TYPE_PHOTO] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_PHOTO));
    }

    CPlexTimelineCollection(const CPlexTimelineMap& timelines)
    {
      m_timelines = timelines;
    }

    void setTimelines(const CPlexTimelineMap& timelines) { m_timelines = timelines; }
    void setTimeline(const ePlexMediaType& type, const CPlexTimelinePtr& timeline)
    {
      m_timelines[type] = timeline;
    }

    CXBMCTinyXML getTimelinesXML(int commandID = 0);

  private:
    CPlexTimelineMap m_timelines;
};

typedef boost::shared_ptr<CPlexTimelineCollection> CPlexTimelineCollectionPtr;


#endif // PLEXTIMELINE_H
