#pragma once

/*
 *  Copyright 2011 Plex Development Team. All rights reserved.
 *
 */

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <vector>
#include <tinyxml.h>

#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/GUISettings.h"
#include "CocoaUtils.h"
#include "CocoaUtilsPlus.h"
#include "log.h"
#include "filesystem/CurlFile.h"
#include "FileSystem/PlexDirectory.h"
#include "PlexLibrarySectionManager.h"
#include "PlexServerManager.h"

using namespace std;
using namespace XFILE;

const string cMyPlexURL = "my.plexapp.com";

#define kPlaylistCacheTime 10

class MyPlexManager
{
 public:
  
  /// Singleton.
  static MyPlexManager& Get()
  {
    static MyPlexManager* instance = 0;
    if (instance == 0)
      instance = new MyPlexManager();
    
    return *instance;
  }
  
  /// Sign in.
  bool signIn()
  {
    if (g_guiSettings.GetString("myplex.email").empty() || g_guiSettings.GetString("myplex.password").empty())
      return false;
    
    CCurlFile http;
    SetupRequestHeaders(http);
    
    // Issue the sign-in request.
    CStdString res;
    string request = GetBaseUrl(true, g_guiSettings.GetString("myplex.email"), g_guiSettings.GetString("myplex.password")) + "/users/sign_in.xml";
    bool success = http.Post(request, "", res);
    
    if (success && res.empty() == false)
    {
      // Parse returned xml.
      TiXmlDocument doc;
      if (doc.Parse(res.c_str()))
      {
        TiXmlElement* root = doc.RootElement();
        if (root && root->ValueStr() == "user")
        {
          TiXmlNode* token = root->FirstChild("authentication-token");
          if (token)
          {
            const char* strToken = token->FirstChild()->Value();
            
            // Save the token.
            g_guiSettings.SetString("myplex.token", strToken);
            g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44011));
            
            return true;
          }
        }
      }
    }
    
    // Reset things.
    signOut();
    
    return false;
  }
  
  /// Sign out.
  void signOut()
  {
    // Clear out the token.
    g_guiSettings.SetString("myplex.token", "");
    g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(19010));
    
    // Remove all the myPlex sections, clear the queue.
    PlexLibrarySectionManager::Get().removeRemoteSections();
    vector<PlexServerPtr> servers;
    PlexServerManager::Get().setRemoteServers(servers);
    m_playlistCache.clear();
    
    // Notify.
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    g_windowManager.SendThreadMessage(msg2);
  }
  
  /// Get the contents of a playlist.
  bool getPlaylist(CFileItemList& list, const string& playlist, bool cacheOnly=false)
  {
    bool   success = true;
    string request = GetBaseUrl(true) + "/pms/playlists/" + playlist;
    
    // See if it's in the cache, and fresh.
    PlaylistCacheEntryPtr entry = m_playlistCache[request];
    if (!entry)
    {
      entry = PlaylistCacheEntryPtr(new PlaylistCacheEntry());
      m_playlistCache[request] = entry;
    }
    
    // Refresh if needed.
    bool cached = (time(0) - entry->addedAt < kPlaylistCacheTime);
    if (cached == false && cacheOnly == false)
    {
      if (fetchList(request, entry->list))
        entry->touch();
      else
        success = false;
    }

    // Assign over the list.
    dprintf("Playlist Cache: Success: %d Cached: %d List has %d items.", success, cached, entry->list.Size());
    if (success)
      list.Assign(entry->list);
    
    return success;
  }
  
  /// Get a playlist URL.
  string getPlaylistUrl(const string& playlist)
  {
    return GetBaseUrl(true) + "/pms/playlists/" + playlist + "?X-Plex-Token=" + string(g_guiSettings.GetString("myplex.token"));
  }
  
  /// Get all sections.
  bool getSections(CFileItemList& list)
  {
    string request = GetBaseUrl(true) + "/pms/system/library/sections";
    return fetchList(request, list);
  }
  
  /// Do a background scan.
  void scanAsync()
  {
    if (g_guiSettings.GetString("myplex.token").empty() == false)
    {
      boost::thread t(boost::bind(&MyPlexManager::scan, this));
      t.detach();
    }
  }
  
  /// myPlex section scanner.
  void scan()
  {
    dprintf("Scanning myPlex.");
    
    // If this is the first time through, sign in again, just to make sure everything is still valid.
    if (m_firstRun)
    {
      signIn();
      m_firstRun = false;
    }
    
    vector<CFileItemPtr>  sharedSections;
    vector<CFileItemPtr>  ownedSections;
    CFileItemList         sections;
    set<string>           uuids;
    vector<PlexServerPtr> servers;
    vector<PlexServerPtr> sharedServers;
    
    // Get the list of sections.
    if (getSections(sections))
    {
      for (int i=0; i<sections.Size(); i++)
      {
        // Separate into owned and shared.
        CFileItemPtr section = sections[i];
        
        // Make sure it has the token.
        bool owned = false;
        string token = section->GetProperty("accessToken").asString();
        if (token.empty())
        {
          token = g_guiSettings.GetString("myplex.token");
          owned = true;
        }
        
        // Add token to path and to fanart.
        section->SetPath(addArgument(section->GetPath(), "X-Plex-Token=" + token));
        //section->SetQuickFanart(addArgument(section->GetQuickFanart(), "X-Plex-Token=" + token));
        
        // Separate 'em into shared and owned.
        if (section->GetProperty("owned") == "1")
        {
          ownedSections.push_back(section);
          section->SetLabel2(section->GetProperty("serverName").asString());
        }
        else
        {
          sharedSections.push_back(section);
          section->SetLabel2(section->GetProperty("sourceTitle").asString());
        }
        
        // If we own it and the server hasn't been added, do so now.
        if (uuids.count(section->GetProperty("machineIdentifier").asString()) == 0)
        {
          string uuid = section->GetProperty("machineIdentifier").asString();
          string name = section->GetProperty("serverName").asString();
          string address = section->GetProperty("address").asString();
          unsigned short port = boost::lexical_cast<unsigned short>(section->GetProperty("port").asString());
          
          PlexServerPtr server = PlexServerPtr(new PlexServer(uuid, name, address, port, token));
          
          if (owned == true)
            servers.push_back(server);
          else
            sharedServers.push_back(server);
          
          uuids.insert(uuid);
        }
      }
      
      PlexServerManager::Get().setRemoteServers(servers);
      PlexServerManager::Get().setSharedServers(sharedServers);
    }
    
    // Get the queue.
    CFileItemList queue;
    getPlaylist(queue, "queue");
    
    // Add the sections.
    PlexLibrarySectionManager::Get().addRemoteOwnedSections(ownedSections);
    PlexLibrarySectionManager::Get().addSharedSections(sharedSections);
    
    // Notify.
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    g_windowManager.SendThreadMessage(msg2);
  }
  
 protected:
  
  /// Constructor.
  MyPlexManager()
    : m_firstRun(true)
  {
  }
  
  /// Smartly append an arg to a URL.
  string addArgument(const string& url, const string& arg)
  {
    if (url.find("?") != string::npos)
      return url + "&" + arg;
    
    return url + "?" + arg;
  }                              
                                
  /// Fetch a list from myPlex.
  bool fetchList(const string& url, CFileItemList& list)
  {
    if (g_guiSettings.GetString("myplex.token").empty() == false)
    {
      // Add the token to the request.
      string request = url;
      request += "?X-Plex-Token=" + g_guiSettings.GetString("myplex.token");
      
      CPlexDirectory plexDir(true, false);
      return plexDir.GetDirectory(request, list);
    }
    
    return false;
  }

  /// Utility method for myPlex to setup request headers.
  static void SetupRequestHeaders(CCurlFile& http)
  {
    // Initialize headers.
    http.SetRequestHeader("Content-Type", "application/xml");
    http.SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));
    http.SetRequestHeader("X-Plex-Product", "Plex Media Center");
    http.SetRequestHeader("X-Plex-Version", Cocoa_GetAppVersion());
    http.SetRequestHeader("X-Plex-Provides", "player");
    http.SetRequestHeader("X-Plex-Platform", Cocoa_GetMachinePlatform());
    http.SetRequestHeader("X-Plex-Platform-Version", Cocoa_GetMachinePlatformVersion());
  }
  
  /// Utility method to retrieve the myPlex URL.
  static string GetBaseUrl(bool secure=true, string username=string(), string password=string())
  {
    string ret;
    string unamepassword;

    if (!username.empty() && !password.empty())
      unamepassword = CURL::Encode(username) + ":" + CURL::Encode(password) + "@";
    
    // Allow the environment variable to override the default, useful for debugging.
    if (getenv("MYPLEX_URL"))
      ret = getenv("MYPLEX_URL");
    else
      ret = (secure ? "https://" : "http://") + unamepassword + cMyPlexURL;
    
    return ret;
  }
  
 private:
  
  class PlaylistCacheEntry
  {
   public:
    
    PlaylistCacheEntry()
      : addedAt(0) {}
    
    void touch() { addedAt = time(0); }
    
    time_t        addedAt;
    CFileItemList list;
  };
  
  typedef boost::shared_ptr<PlaylistCacheEntry> PlaylistCacheEntryPtr;
  
  bool          m_firstRun;
  map<string, PlaylistCacheEntryPtr> m_playlistCache;
  boost::mutex  m_mutex;
};
