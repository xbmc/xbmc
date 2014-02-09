//
//  PlexRemoteSubscriberManager.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-19.
//
//

#include "PlexRemoteSubscriberManager.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include "Application.h"
#include "PlexJobs.h"

#include "Client/PlexMediaServerClient.h"
#include "PlexApplication.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "LocalizeStrings.h"

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriber::CPlexRemoteSubscriber(const std::string &uuid, int commandID, const std::string &ipaddress, int port, const std::string &protocol)
{
  if (!protocol.empty() && !ipaddress.empty())
  {
    m_url.SetProtocol(protocol);
    m_url.SetHostName(ipaddress);
    m_url.SetPort(port);
  }

  m_poller = false;
  m_commandID = commandID;
  m_uuid = uuid;
}

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexRemoteSubscriber::shouldRemove() const
{
  if (m_lastUpdated.elapsed() > PLEX_REMOTE_SUBSCRIBER_REMOVE_INTERVAL)
  {
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriber::shouldRemove removing %s because elapsed: %lld", m_uuid.c_str(), m_lastUpdated.elapsed());
    return true;
  }
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriber::shouldRemove will not remove because elapsed: %lld", m_lastUpdated.elapsed());
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriber::refresh(CPlexRemoteSubscriberPtr sub)
{
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriber::refresh %s", m_uuid.c_str());

  if (sub->getURL().Get() != getURL().Get())
  {
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriber::refresh new url %s", m_url.Get().c_str());
    m_url = sub->getURL();
  }

  if (sub->m_commandID != m_commandID)
    m_commandID = sub->m_commandID;

  m_lastUpdated.restart();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriberPtr CPlexRemoteSubscriberManager::addSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber) return CPlexRemoteSubscriberPtr();
  
  CSingleLock lk(m_crit);
  
  if (m_map.find(subscriber->getUUID()) != m_map.end())
  {
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::addSubscriber refreshed %s", subscriber->getUUID().c_str());
    m_map[subscriber->getUUID()]->refresh(subscriber);
  }
  else
  {
    m_map[subscriber->getUUID()] = subscriber;
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::addSubscriber added %s:%d [%s]",
              subscriber->getURL().GetHostName().c_str(), subscriber->getURL().GetPort(), subscriber->getUUID().c_str());

    if (!g_application.IsPlayingVideo())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(52500),
                                            subscriber->getName().empty() ? CStdString(subscriber->getURL().GetHostName()) : CStdString(subscriber->getName()),
                                            TOAST_DISPLAY_TIME, false);
  }
  
  g_plexApplication.timer.SetTimeout(PLEX_REMOTE_SUBSCRIBER_CHECK_INTERVAL * 1000, this);

  return m_map[subscriber->getUUID()];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::updateSubscriberCommandID(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber) return;

  CSingleLock lk(m_crit);
  if (m_map.find(subscriber->getUUID()) != m_map.end())
    m_map[subscriber->getUUID()]->setCommandID(subscriber->getCommandID());
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::removeSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber) return;
  
  CSingleLock lk(m_crit);
  
  if (m_map.find(subscriber->getUUID()) == m_map.end())
    return;
  
  m_map.erase(subscriber->getUUID());
  
  if (m_map.size() == 0)
    g_plexApplication.timer.RemoveTimeout(this);

  if (!g_application.IsPlayingVideo())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(52501),
                                          subscriber->getName().empty() ? CStdString(subscriber->getURL().GetHostName()) : CStdString(subscriber->getName()),
                                          TOAST_DISPLAY_TIME, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriberPtr CPlexRemoteSubscriberManager::findSubscriberByUUID(const std::string &uuid)
{
  if (uuid.empty())
    return CPlexRemoteSubscriberPtr();

  CSingleLock lk(m_crit);
  if (m_map.find(uuid) != m_map.end())
    return m_map[uuid];

  return CPlexRemoteSubscriberPtr();
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::OnTimeout()
{
  CSingleLock lk(m_crit);
  
  std::vector<std::string> subsToRemove;
  
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::OnTimeout starting with %lu clients", m_map.size());
  
  BOOST_FOREACH(SubscriberPair p, m_map)
  {
    if (p.second->shouldRemove())
      subsToRemove.push_back(p.first);
  }
  
  BOOST_FOREACH(std::string s, subsToRemove)
    m_map.erase(s);
  
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::OnTimeout %lu clients left after timeout", m_map.size());
  
  /* still clients to handle, restart the timer */
  if (m_map.size() > 0)
    g_plexApplication.timer.SetTimeout(PLEX_REMOTE_SUBSCRIBER_CHECK_INTERVAL * 1000, this);
}

////////////////////////////////////////////////////////////////////////////////////////
std::vector<CURL> CPlexRemoteSubscriberManager::getSubscriberURL() const
{
  std::vector<CURL> list;
  if (!hasSubscribers())
    return list;
  
  CSingleLock lk(m_crit);
  BOOST_FOREACH(SubscriberPair p, m_map)
    list.push_back(p.second->getURL());
  
  return list;
}

////////////////////////////////////////////////////////////////////////////////////////
std::vector<CPlexRemoteSubscriberPtr> CPlexRemoteSubscriberManager::getSubscribers() const
{
  std::vector<CPlexRemoteSubscriberPtr> list;
  if (!hasSubscribers())
    return list;

  CSingleLock lk(m_crit);
  BOOST_FOREACH(SubscriberPair p, m_map)
    list.push_back(p.second);

  return list;
}
