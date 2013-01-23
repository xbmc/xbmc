#pragma once

//
//  Copyright (c) 2011 Plex, Inc. All rights reserved.
//

#include <string>
#include <set>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include "FileSystem/PlexDirectory.h"
#include "PlexServerManager.h"

using namespace std;
using namespace XFILE;

class ManualServer
{
 public:
  
  ManualServer(const string& address)
    : deleted(false)
    , address(address)
    , updatedAt(0) {}
  
  string url()
  {
    return "http://" + address + ":32400";
  }
  
  bool   deleted;
  string uuid;
  string name;
  string address;
  time_t updatedAt;
};

typedef boost::shared_ptr<ManualServer> ManualServerPtr;
typedef pair<string, ManualServerPtr> address_server_pair;

class ManualServerScanner
{
 public:
  
  /// Singleton.
  static ManualServerScanner& Get()
  {
    static ManualServerScanner* g_instance;
    if (g_instance == 0)
    {
      g_instance = new ManualServerScanner();
      boost::thread t(boost::bind(&ManualServerScanner::run, g_instance));
    }
    
    return *g_instance;
  }
  
  /// Add a new manual server.
  void addServer(const string& address, bool removeOthers=false)
  {
    boost::mutex::scoped_lock lk(m_condMutex);
    
    if (m_serverMap.find(address) == m_serverMap.end())
      m_serverMap[address] = ManualServerPtr(new ManualServer(address));
    
    if (removeOthers)
    {
      // Mark the others as deleted.
      BOOST_FOREACH(address_server_pair pair, m_serverMap)
        if (pair.first != address && pair.first != "127.0.0.1")
          pair.second->deleted = true;
    }
    
    m_condition.notify_one();
  }
    
  /// Remove all servers except local.
  void removeAllServersButLocal()
  {
    boost::mutex::scoped_lock lk(m_condMutex);
    
    // Mark non-local as deleted.
    BOOST_FOREACH(address_server_pair pair, m_serverMap)
    if (pair.first != "127.0.0.1")
      pair.second->deleted = true;
  }
  
 private:
  
  void run()
  {
    boost::mutex::scoped_lock lk(m_condMutex);
    boost::posix_time::seconds delay(10);
    
    while (true)
    {
      // Wait to be signaled or for the timeout to occur.
      m_condition.timed_wait(lk, delay);
      
      // Get a copy of the servers.
      map<string, ManualServerPtr> servers(m_serverMap);
      
      // Whack deleted servers.
      BOOST_FOREACH(address_server_pair pair, servers)
        if (pair.second->deleted)
          m_serverMap.erase(pair.first);
      
      // See if they're alive.
      BOOST_FOREACH(address_server_pair pair, servers)
      {
        // Make sure we don't replace localhost when we ask for listing. Only wait
        // five seconds as well, otherwise if we hit some dead server we end up taking
        // 5 whole MINUTES to timeout.
        //
        CPlexDirectory dir(true, false, false, 5);
        CFileItemList  list;
        CCurlFile http;
        struct __stat64 st;

        dprintf("Manual Server Scanner: About to manually test server %s (deleted: %d)", pair.second->address.c_str(), pair.second->deleted);

        if (pair.second->deleted == false &&
            http.Stat(CURL(pair.second->url()), &st) == 0 &&
            dir.GetDirectory(pair.second->url(), list) &&
            list.GetProperty("updatedAt").asString().empty() == false)
        {
          // Update name and UUID.
          pair.second->name = list.GetProperty("friendlyName").asString();
          pair.second->uuid = list.GetProperty("machineIdentifier").asString();
          
          time_t updatedAt = boost::lexical_cast<int>(list.GetProperty("updatedAt").asString());
          if (pair.second->updatedAt == 0)
          {
            dprintf("Manual Server Scanner: NEW SERVER: %s", pair.second->address.c_str());
            PlexServerManager::Get().addServer(pair.second->uuid, pair.second->name, pair.second->address, 32400);
          }
          else if (pair.second->updatedAt != updatedAt)
          {
            dprintf("Manual Server Scanner: UPDATED SERVER: %s", pair.second->address.c_str());
            PlexServerManager::Get().updateServer(pair.second->uuid, pair.second->name, pair.second->address, 32400, updatedAt);
          }
          
          pair.second->updatedAt = updatedAt;
        }
        else
        {
          if (pair.second->updatedAt != 0)
          {
            dprintf("Manual Server Scanner: DEAD SERVER: %s", pair.second->address.c_str());
            PlexServerManager::Get().removeServer(pair.second->uuid, pair.second->name, pair.second->address, 32400);
          }
          
          pair.second->updatedAt = 0;
        }
      }
    }
  }
  
  ManualServerScanner() {}
  ~ManualServerScanner() {}

  map<string, ManualServerPtr> m_serverMap;
  
  boost::condition m_condition;
  boost::mutex     m_condMutex;
};
