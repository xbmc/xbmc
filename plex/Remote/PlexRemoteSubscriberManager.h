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
#include "threads/Event.h"
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
    static CPlexRemoteSubscriberPtr NewSubscriber(const std::string &uuid, const std::string &ipaddress, int port, int commandID = -1, const std::string &protocol = "http")
    {
      return CPlexRemoteSubscriberPtr(new CPlexRemoteSubscriber(uuid, commandID, ipaddress, port, protocol));
    };

    static CPlexRemoteSubscriberPtr NewPollSubscriber(const std::string& uuid, int commandID = -1)
    {
      CPlexRemoteSubscriberPtr sub =  CPlexRemoteSubscriberPtr(new CPlexRemoteSubscriber(uuid, commandID));
      sub->setPoller();
      return sub;
    }

    CPlexRemoteSubscriber(const std::string &uuid, int commandID, const std::string &ipaddress="", int port=32400, const std::string& protocol="http");

    void refresh(CPlexRemoteSubscriberPtr sub);
    bool shouldRemove() const;

    void setPoller() { m_poller = true; }
    bool isPoller() const { return m_poller; }
  
    CURL getURL() const { return m_url; }
    std::string getUUID() const { return m_uuid; }

    int getCommandID() const { return m_commandID; }
    void setCommandID(int commandID) { m_commandID = commandID; }

    CEvent m_pollEvent;
  
  private:
    int m_commandID;
    CURL m_url;
    CPlexTimer m_lastUpdated;
    std::string m_uuid;
    bool m_poller;
};

typedef std::map<std::string, CPlexRemoteSubscriberPtr> SubscriberMap;
typedef std::pair<std::string, CPlexRemoteSubscriberPtr> SubscriberPair;

class CPlexRemoteSubscriberManager : public ITimerCallback
{
  public:
    CPlexRemoteSubscriberManager() : m_refreshTimer(this) {}
    CPlexRemoteSubscriberPtr addSubscriber(CPlexRemoteSubscriberPtr subscriber);
    void updateSubscriberCommandID(CPlexRemoteSubscriberPtr subscriber);
    void removeSubscriber(CPlexRemoteSubscriberPtr subscriber);
    CPlexRemoteSubscriberPtr findSubscriberByUUID(const std::string& uuid);
    std::vector<CURL> getSubscriberURL() const;

    std::vector<CPlexRemoteSubscriberPtr> getSubscribers() const;
  
    bool hasSubscribers() const { CSingleLock lk(m_crit); return m_map.size(); }
  
  private:
    void OnTimeout();
  
    CCriticalSection m_crit;
    CTimer m_refreshTimer;
    SubscriberMap m_map;
};

#endif /* defined(__Plex_Home_Theater__PlexRemoteSubscriberManager__) */
