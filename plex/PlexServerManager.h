#pragma once

//
//  PlexServerManager.h
//
//  Created by Elan Feingold on 10/24/11.
//  Copyright (c) 2011 Blue Mandrill Design. All rights reserved.
//

#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include "CocoaUtilsPlus.h"
#include "filesystem/CurlFile.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "PlexSourceScanner.h"
#include "PlexLog.h"
#include "PlexTypes.h"

using namespace std;
using namespace XFILE;

class PlexServer;
typedef boost::shared_ptr<PlexServer> PlexServerPtr;
typedef pair<string, PlexServerPtr> key_server_pair;

#define PMS_LIVE_SCORE 50

////////////////////////////////////////////////////////////////////
class PlexServer
{
 public:

  /// Constructor.
  PlexServer(const string& uuid, const string& name, const string& addr, unsigned short port, const string& token)
    : uuid(uuid), name(name), address(addr), port(port), token(token), updatedAt(0), m_count(1)
  {
    // See if it's running on this machine.
    if (token.empty())
      local = Cocoa_IsHostLocal(addr);
    
    // Default to live if we detected it.
    live = detected();
    
    // Compute the key for the server.
    m_key = uuid + "-" + address + "-" + boost::lexical_cast<string>(port);
  }
  
  /// Is it alive? Blocks, can take time.
  bool reachable()
  {
    CCurlFile  http;

    /* set short timeout */
    http.SetTimeout(1);

    CStdString resp;
    live = http.Get(url(), resp);
    
    return live;
  }
  
  /// Return the root URL.
  string url()
  {
    string ret = "http://" + address + ":" + boost::lexical_cast<string>(port);
    if (token.empty() == false)
      ret += "?X-Plex-Token=" + token;
    
    return ret;
  }

  /// The score for the server.
  int score()
  {
    int ret = 0;
    
    // Bonus for being alive, being localhost, and being detected.
    if (live) ret += PMS_LIVE_SCORE;
    if (local) ret += 10;
    if (detected()) ret += 10;
    
    return ret;
  }
  
  /// The server key for hashing.
  string key() const
  {
    return m_key;
  }
  
  /// Was it auto-detected?
  bool detected()
  {
    return token.empty();
  }
  
  /// Equality operator.
  bool equals(const PlexServerPtr& rhs)
  {
    if (!rhs)
      return false;
    
    return (uuid == rhs->uuid && address == rhs->address && port == rhs->port);
  }
  
  /// Ref counting.
  void incRef()
  {
    ++m_count;
  }
  
  int decRef()
  {
    return --m_count;
  }
  
  int refCount() const
  {
    return m_count;
  }
  
  bool live;
  bool local;
  string uuid;
  string name;
  string token;
  string address;
  unsigned short port;
  time_t updatedAt;
  
 private:
  
  string m_key;
  int    m_count;
};

////////////////////////////////////////////////////////////////////
class PlexServerManager
{
public:
  
  /// Singleton.
  static PlexServerManager& Get()
  {
    static PlexServerManager* instance = 0;
    if (instance == 0)
    {
      instance = new PlexServerManager();
      boost::thread t(boost::bind(&PlexServerManager::run, instance));
      t.detach();
    }
    
    return *instance;
  }
  
  /// Get me the best server.
  PlexServerPtr bestServer()
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    return m_bestServer;
  }
  
  /// Server appeared.
  void addServer(const string& uuid, const string& name, const string& addr, unsigned short port, const string& token="")
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    PlexServerPtr server = PlexServerPtr(new PlexServer(uuid, name, addr, port, token));
    
    // Keep a reference count for servers in case we get updated.
    if (m_servers.find(server->key()) != m_servers.end())
    {
      // Same server, found a different way.
      m_servers[server->key()]->incRef();
      dprintf("Plex Server Manager: added existing server '%s' (%s) ref=%d.", name.c_str(), addr.c_str(), m_servers[server->key()]->refCount());
    }
    else
    {
      // Brand new server, add and scan.
      dprintf("Plex Server Manager: added new server '%s' (%s).", name.c_str(), addr.c_str());
      m_servers[server->key()] = server;
      CPlexSourceScanner::ScanHost(uuid, addr, name, server->url());
    }
    
    updateBestServer();
    dump();
  }
  
  /// Server disappeared.
  void removeServer(const string& uuid, const string& name, const string& addr, unsigned short port)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    PlexServerPtr server = PlexServerPtr(new PlexServer(uuid, name, addr, port, ""));
    
    // Decrease reference count.
    if (m_servers.find(server->key()) != m_servers.end())
    {
      if (m_servers[server->key()]->decRef() == 0)
      {
        dprintf("Plex Server Manager: removed existing server '%s' (%s).", name.c_str(), addr.c_str());
        m_servers.erase(server->key());
        CPlexSourceScanner::RemoveHost(uuid, server->url(), true);
      }
      else
      {
        dprintf("Plex Server Manager: lost a reference to server '%s' (%s) ref=%d.", name.c_str(), addr.c_str(), server->refCount());
      }
    }
    
    updateBestServer();
    dump();
  }

  /// Server was updated.
  void updateServer(const string& uuid, const string& name, const string& addr, unsigned short port, time_t updatedAt)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    
    PlexServerPtr server = PlexServerPtr(new PlexServer(uuid, name, addr, port, ""));
    if (m_servers.find(server->key()) != m_servers.end())
    {
      // See if anything actually changed.
      if (m_servers[server->key()]->updatedAt < updatedAt)
      {
        dprintf("Plex Server Manager: the server '%s' was updated, rescanning (updated at %ld)", name.c_str(), updatedAt);
        m_servers[server->key()]->updatedAt = updatedAt;
        CPlexSourceScanner::ScanHost(uuid, addr, name, server->url());
      }
      else
      {
        dprintf("Plex Server Manager: the server '%s' was already up to date, not scanning.", name.c_str());
      }
    }
  }
  
  /// Return if server is reachable
  bool checkServerReachability(const string& uuid, const string& addr, unsigned short port)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    PlexServerPtr server = PlexServerPtr(new PlexServer(uuid, "", addr, port, ""));
    if (m_servers.find(server->key()) != m_servers.end())
    {
      return m_servers[server->key()]->reachable();
    }
    return false;
  }

  /// Set the shared servers.
  void setSharedServers(const vector<PlexServerPtr>& sharedServers)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    m_sharedServers.assign(sharedServers.begin(), sharedServers.end());
  }
  
  /// Get shared servers.
  void getSharedServers(vector<PlexServerPtr>& sharedServers)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    sharedServers.assign(m_sharedServers.begin(), m_sharedServers.end());
  }

  /// Remove all non-detected servers.
  void setRemoteServers(const vector<PlexServerPtr>& remoteServers)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);

    // See which ones are actually new.
    set<string> addedServers;
    set<PlexServerPtr> deletedServers;
    computeAddedAndRemoved(remoteServers, addedServers, deletedServers);
    dprintf("Plex Server Manager: Setting %ld remote servers (%ld new, %ld deleted)", remoteServers.size(), addedServers.size(), deletedServers.size());
    
    // Whack existing detected servers.
    set<string> detected;
    BOOST_FOREACH(key_server_pair pair, m_servers)
      if (pair.second->detected() == false)
        detected.insert(pair.first);
    
    BOOST_FOREACH(string key, detected)
      m_servers.erase(key);
    
    // Add the new ones.
    BOOST_FOREACH(PlexServerPtr server, remoteServers)
      m_servers[server->key()] = server;
    
    // Notify the source scanner.
    BOOST_FOREACH(string s, addedServers)
    {
      PlexServerPtr server = m_servers[s];
      CPlexSourceScanner::ScanHost(server->uuid, server->address, server->name, server->url());
    }
    
    BOOST_FOREACH(PlexServerPtr server, deletedServers)
      CPlexSourceScanner::RemoveHost(server->uuid, server->url());
    
    updateBestServer();
    dump();
  }
  
 private:
  
  /// Compute added and removed sets.
  void computeAddedAndRemoved(const vector<PlexServerPtr>& remoteServers, set<string>& addedServers, set<PlexServerPtr>& deletedServers)
  {
    set<string> remoteServerKeys;
    
    BOOST_FOREACH(PlexServerPtr server, remoteServers)
    {
      if (m_servers.find(server->key()) == m_servers.end())
        addedServers.insert(server->key());
      remoteServerKeys.insert(server->key());
    }
    
    // Find out which ones are deleted.
    BOOST_FOREACH(key_server_pair pair, m_servers)
    if (pair.second->detected() == false && remoteServerKeys.find(pair.first) == remoteServerKeys.end())
      deletedServers.insert(pair.second);
  }
  
  /// Figure out what the best server is.
  void updateBestServer()
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    
    PlexServerPtr bestServer;
    int bestScore = 0;
    
    BOOST_FOREACH(key_server_pair pair, m_servers)
    {
      if (pair.second->score() > bestScore && pair.second->score() >= PMS_LIVE_SCORE)
      {
        // Compute the score.
        bestScore = pair.second->score();
        bestServer = pair.second;
        
        // Bonus if it's the current server.
        if (bestServer->equals(m_bestServer))
          bestScore++;
      }
    }
    
    if (bestServer)
      dprintf("Plex Server Manager: Computed best server to be [%s] (%s:%d) with score %d.", bestServer->name.c_str(), bestServer->address.c_str(), bestServer->port, bestScore);
    else
      dprintf("Plex Server Manager: There is no worthy server.");
    
    // If the server changed, notify the home screen, there may be repercussions.
    if ((!m_bestServer && bestServer) ||
        (m_bestServer && !bestServer) ||
        (m_bestServer && bestServer && m_bestServer->equals(bestServer) == false))
    {
      // Notify the main menu.
      dprintf("Plex Server Manager: Notifying home screen about change to best server.");
      CGUIMessage msg(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
      g_windowManager.SendThreadMessage(msg);
    }
    
    m_bestServer = bestServer;
  }
  
  void dump()
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);
    
    dprintf("SERVERS:");
    BOOST_FOREACH(key_server_pair pair, m_servers)
      dprintf("  * %s [%s:%d] local: %d live: %d (%s) count: %d", pair.second->name.c_str(), pair.second->address.c_str(), pair.second->port, pair.second->local, pair.second->live, pair.second->uuid.c_str(), pair.second->refCount());
  }
  
  void run()
  {
    map<string, PlexServerPtr> servers;
    
    while (true)
    {
      // Get servers.
      m_mutex.lock();
      servers = m_servers;
      m_mutex.unlock();
      
      // Run connectivity checks.
      dprintf("Plex Server Manager: Running connectivity check.");
      BOOST_FOREACH(key_server_pair pair, servers)
        pair.second->reachable();
      
      updateBestServer();
      dump();
      
      // Sleep.
      boost::this_thread::sleep(boost::posix_time::seconds(30));
    }
  }
  
  PlexServerPtr              m_bestServer;
  map<string, PlexServerPtr> m_servers;
  vector<PlexServerPtr>      m_sharedServers;
  boost::recursive_mutex     m_mutex;
};
