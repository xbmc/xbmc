//
//  PlexRemoteSubscriberManager.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-19.
//
//

#ifndef __Plex_Home_Theater__PlexRemoteSubscriberManager__
#define __Plex_Home_Theater__PlexRemoteSubscriberManager__

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/timer.hpp>
#include "threads/Timer.h"
#include "URL.h"
#include "FileItem.h"
#include "Utility/PlexTimer.h"

class CPlexRemoteSubscriber;
typedef boost::shared_ptr<CPlexRemoteSubscriber> CPlexRemoteSubscriberPtr;

/* give clients 90 seconds before we time them out */
#define PLEX_REMOTE_SUBSCRIBER_REMOVE_INTERVAL 90

/* check all subscribers every 10th second */
#define PLEX_REMOTE_SUBSCRIBER_CHECK_INTERVAL 10

class CPlexRemoteSubscriber
{
  public:
    static CPlexRemoteSubscriberPtr NewSubscriber(const std::string &uuid, const std::string &ipaddress, int port)
    {
      return CPlexRemoteSubscriberPtr(new CPlexRemoteSubscriber(uuid, ipaddress, port));
    };
    CPlexRemoteSubscriber(const std::string &uuid, const std::string &ipaddress, int port=32400);
    void refresh();
    bool shouldRemove() const;
  
    CURL getURL() const { return m_url; }
    std::string getUUID() const { return m_uuid; }
  
  private:
    CURL m_url;
    CPlexTimer m_lastUpdated;
    std::string m_uuid;
};

typedef std::map<std::string, CPlexRemoteSubscriberPtr> SubscriberMap;
typedef std::pair<std::string, CPlexRemoteSubscriberPtr> SubscriberPair;

class CPlexRemoteSubscriberManager : public ITimerCallback
{
  public:
    CPlexRemoteSubscriberManager() : m_refreshTimer(this) {}
    void addSubscriber(CPlexRemoteSubscriberPtr subscriber);
    void removeSubscriber(CPlexRemoteSubscriberPtr subscriber);
    std::vector<CURL> getSubscriberURL() const;
  
    bool hasSubscribers() const { CSingleLock lk(m_crit); return m_map.size(); }
  
  private:
    void OnTimeout();
  
    CCriticalSection m_crit;
    CTimer m_refreshTimer;
    SubscriberMap m_map;
};

#endif /* defined(__Plex_Home_Theater__PlexRemoteSubscriberManager__) */
