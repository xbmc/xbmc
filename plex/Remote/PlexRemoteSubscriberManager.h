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
#include "PlexGlobalTimer.h"
#include "threads/Event.h"
#include "URL.h"
#include "FileItem.h"
#include "Utility/PlexTimer.h"
#include "FileSystem/PlexFile.h"
#include "XBMCTinyXML.h"
#include "Client/PlexTimeline.h"
#include "PlexQueue.h"

class CPlexRemoteSubscriber;
typedef boost::shared_ptr<CPlexRemoteSubscriber> CPlexRemoteSubscriberPtr;

/* give clients 90 seconds before we time them out */
#define PLEX_REMOTE_SUBSCRIBER_REMOVE_INTERVAL 90

/* check all subscribers every 10th second */
#define PLEX_REMOTE_SUBSCRIBER_CHECK_INTERVAL 10

class CPlexRemoteSubscriber : public CThread
{
  public:
    static CPlexRemoteSubscriberPtr NewSubscriber(const std::string &uuid, const std::string &ipaddress, int port, int commandID = -1, const std::string &protocol = "http")
    {
      return CPlexRemoteSubscriberPtr(new CPlexRemoteSubscriber(false, uuid, commandID, ipaddress, port, protocol));
    };

    static CPlexRemoteSubscriberPtr NewPollSubscriber(const std::string& uuid, int commandID = -1)
    {
      CPlexRemoteSubscriberPtr sub =  CPlexRemoteSubscriberPtr(new CPlexRemoteSubscriber(true, uuid, commandID));
      return sub;
    }

    CPlexRemoteSubscriber(bool poller, const std::string &uuid, int commandID, const std::string &ipaddress="", int port=32400, const std::string& protocol="http");

    void Process();
    void Stop();

    CXBMCTinyXML waitForTimeline();

    void refresh(CPlexRemoteSubscriberPtr sub);
    bool shouldRemove() const;

    bool isPoller() const { return m_poller; }
  
    CURL getURL() const { return m_url; }
    std::string getUUID() const { return m_uuid; }

    int getCommandID() const { return m_commandID; }
    void setCommandID(int commandID) { m_commandID = commandID; }

    void setName(const std::string& name) { m_name = name; }
    std::string getName() const { return m_name; }

    bool queueTimeline(const CPlexTimelineCollectionPtr& timeline);

  
  private:
    bool sendTimeline(const CPlexTimelineCollectionPtr& timelines);
    CPlexQueue<CPlexTimelineCollectionPtr> m_outgoingTimelines;

    int m_commandID;
    CURL m_url;
    CPlexTimer m_lastUpdated;
    std::string m_uuid;
    bool m_poller;
    std::string m_name;
    XFILE::CPlexFile m_file;
};

typedef std::map<std::string, CPlexRemoteSubscriberPtr> SubscriberMap;
typedef std::pair<std::string, CPlexRemoteSubscriberPtr> SubscriberPair;

class CPlexRemoteSubscriberManager : public IPlexGlobalTimeout
{
  public:
    CPlexRemoteSubscriberManager() : m_stopped(false) {}
    CPlexRemoteSubscriberPtr addSubscriber(CPlexRemoteSubscriberPtr subscriber);
    void updateSubscriberCommandID(CPlexRemoteSubscriberPtr subscriber);
    void removeSubscriber(CPlexRemoteSubscriberPtr subscriber);
    CPlexRemoteSubscriberPtr findSubscriberByUUID(const std::string& uuid);
    std::vector<CURL> getSubscriberURL() const;

    std::vector<CPlexRemoteSubscriberPtr> getSubscribers() const;
  
    bool hasSubscribers() const { CSingleLock lk(m_crit); return m_map.size(); }
    CStdString TimerName() const { return "remoteSubscriberManager"; }
    void Stop();

  private:
    void OnTimeout();
  
    CCriticalSection m_crit;
    SubscriberMap m_map;
    bool m_stopped;
};

#endif /* defined(__Plex_Home_Theater__PlexRemoteSubscriberManager__) */
