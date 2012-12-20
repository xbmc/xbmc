#pragma once

/*
 *  Copyright 2011 Plex Development Team. All rights reserved.
 *
 */

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <vector>
#include "utils/XBMCTinyXML.h"

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
#include "xbmc/Util.h"
#include "guilib/LocalizeStrings.h"

#include "GUIInfoManager.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"

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

  /// Sign out.
  void signOut()
  {
    boost::mutex::scoped_lock lk(m_mutex);

    // Clear out the token.
    g_guiSettings.SetString("myplex.token", "");
    g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44010));
    
    // Remove all the myPlex sections, clear the queue.
    PlexLibrarySectionManager::Get().removeRemoteSections();
    vector<PlexServerPtr> servers;
    PlexServerManager::Get().setRemoteServers(servers);
    m_playlistCache.clear();

    // clear some local variables
    m_remoteServers.clear();
    m_sectionThumbnails.clear();
    m_sharedSections.clear();
    
    // Notify.
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    g_windowManager.SendThreadMessage(msg2);
  }

  bool CreatePinRequest(int* pin, int* pageId)
  {
    CCurlFile http;
    http.SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));
    string url = GetBaseUrl(true) + "/pins.xml";

    CStdString returnData;
    if (http.Post(url, "", returnData))
    {
      TiXmlDocument doc;
      doc.Parse(returnData.c_str());
      if (doc.RootElement() != 0)
      {
        TiXmlElement* root=doc.RootElement();
        TiXmlElement* code=root->FirstChildElement("code");
        if (code && code->GetText())
          pin = code->GetText();
        else
          return false;

        TiXmlElement* id=root->FirstChildElement("id");
        if (id && id->GetText())
          *pageId = atoi(id->GetText());
        else
          return false;

        return true;
      }
    }
    return false;
  }

  bool TryGetTokenFromPin(int pageId)
  {
    CCurlFile http;
    http.SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));
    CStdString url;
    url.Format("%s/pins/%d.xml", GetBaseUrl(true), pageId);

    CStdString data;
    if (http.Get(url, data))
    {
      dprintf("Got pin data: %s", data.c_str());
      TiXmlDocument doc;
      doc.Parse(data.c_str());
      if (doc.RootElement() != 0)
      {
        TiXmlElement* root=doc.RootElement();
        TiXmlElement* token=root->FirstChildElement("auth_token");
        if(token && token->GetText())
        {
          g_guiSettings.SetString("myplex.token", token->GetText());
          return true;
        }
      }
    }

    return false;
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

  /// Return a list of all servers
  bool getServers(CFileItemList& list)
  {
    string request = GetBaseUrl(true) + "/pms/servers.xml";
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

  void scanSharedServer(PlexServerPtr server, CFileItemPtr serverItem)
  {
    server->incRef();
    dprintf("Scanning shared host %s", server->name.c_str());
    CStdString url;
    url.Format("http://%s:%d/library/sections", server->address, server->port);

    CFileItemList sections;
    if (fetchList(url, sections, server->token))
    {
      for (int i=0; i < sections.Size(); i++)
      {
        CFileItemPtr section = sections[i];
        section->SetProperty("machineIdentifier", server->uuid);
        section->SetLabel2(serverItem->GetProperty("sourceTitle").asString());
        section->SetProperty("sourceTitle", serverItem->GetProperty("sourceTitle").asString());
        section->SetProperty("serverName", server->name);

        /* Take the mutex so that we can get thumbnails and modify m_sharedSections here */
        boost::mutex::scoped_lock lk(m_mutex);

        CStdString path = CURL(section->GetPath()).GetUrlWithoutOptions() + "/";

        if (m_sectionThumbnails.find(path) != m_sectionThumbnails.end())
        {
          vector<CStdString> thumbs = m_sectionThumbnails[path];
          for (size_t i = 0; i < thumbs.size() ; i++)
          {
            dprintf("MyPlexManager: [%s] thumb%ld = %s", path.c_str(), i, thumbs[i].c_str());
            section->SetArt(PLEX_ART_THUMB, i, thumbs[i]);
          }
        }
        else
        {
          dprintf("MyPlexManager: no thumbnails for section %s", path.c_str());
        }
        m_sharedSections.push_back(section);
      }
    }
    server->decRef();

    vector<CFileItemPtr> sharedSections;
    {
      /* Take the mutex so that we can copy sharedSections here */
      boost::mutex::scoped_lock lk(m_mutex);
      sharedSections = m_sharedSections;
    }

    PlexLibrarySectionManager::Get().addSharedSections(sharedSections);

    /* Make sure the home window knows about it */
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    g_windowManager.SendThreadMessage(msg2);
  }

  void testLocalAddress(PlexServerPtr localServer)
  {
    if (localServer->reachable())
    {
      /* We default to 32400 and we shouldn't need a token */
      dprintf("MyPlexManager: Adding local address %s to server %s", localServer->address.c_str(), localServer->name.c_str());
      PlexServerManager::Get().addServer(localServer->uuid, localServer->name, localServer->address, 32400);
    }
    else
    {
      dprintf("MyPlexManager: Couldn't reach %s for server %s, will not use it.", localServer->address.c_str(), localServer->name.c_str());
    }
  }
  
  /// myPlex section scanner.
  void scan()
  {
    dprintf("Scanning myPlex.");
    
#if 0
    // If this is the first time through, sign in again, just to make sure everything is still valid.
    if (m_firstRun)
    {
      signIn();
      m_firstRun = false;
    }
#endif

    m_didFetchThumbs = false;
    CFileItemList servers;
    map<string, PlexServerPtr> newRemoteServers;
    vector<PlexServerPtr> sharedServers;

    if (getServers(servers))
    {
      {
        /* Clear the shared sections while holding the mutex */
        boost::mutex::scoped_lock lk(m_mutex);
        m_sharedSections.clear();
        m_sectionThumbnails.clear();
      }

      for (int i=0; i<servers.Size(); i++)
      {
        CFileItemPtr server = servers[i];
        dprintf("Server %s", server->GetLabel().c_str());

        // Get token
        bool owned = false;
        string token = server->GetProperty("accessToken").asString();
        if (token.empty())
        {
          // This is probably our own server, just add our token to it
          token = g_guiSettings.GetString("myplex.token");
          owned = true;
        }

        if (server->HasProperty("owned"))
          owned = server->GetProperty("owned").asBoolean();

        CStdString uuid = server->GetProperty("uuid").asString();
        CStdString address = server->GetProperty("address").asString();
        CStdString name = server->GetLabel();
        int port = (int)server->GetProperty("port").asInteger();

        PlexServerPtr serverPtr;
        if (owned)
        {
          if (!PlexServerManager::Get().getServerByKey(uuid, address, port, serverPtr))
          {
            dprintf("MyPlexManager: Adding new server %s", name.c_str());
            PlexServerManager::Get().addServer(uuid, name, address, port, token);
            PlexServerManager::Get().getServerByKey(uuid, address, port, serverPtr);
          }

          /* Check for localAddresses */
          if (server->HasProperty("localAddresses"))
          {
            CStdString localAddresses = server->GetProperty("localAddresses").asString();
            dprintf("MyPlexManager: localAddresses = %s", localAddresses.c_str());
            vector<CStdString> addresses;
            CUtil::SplitParams(localAddresses, addresses);
            PlexServerPtr localServerPtr;

            BOOST_FOREACH(CStdString address, addresses)
            {
              if (!PlexServerManager::Get().getServerByKey(uuid, address, 32400, localServerPtr))
              {
                localServerPtr = PlexServerPtr(new PlexServer(uuid, name, address, 32400, ""));
                boost::thread t(boost::bind(&MyPlexManager::testLocalAddress, this, localServerPtr));
                t.detach();
              }
            }
          }

          newRemoteServers[serverPtr->key()] = serverPtr;
        }
        else
        {
          serverPtr = PlexServerPtr(new PlexServer(uuid, name, address, port, token));
          sharedServers.push_back(serverPtr);

          /* We have a shared server, that means that we need to fetch the thumbs for the
           * shared server sections
           * This is a bit stupid, but hey...
           */
          CFileItemList sections;
          if (!m_didFetchThumbs && getSections(sections))
          {
            for (int i = 0; i < sections.Size(); i++)
            {
              CFileItemPtr section = sections[i];
              CStdString path = section->GetPath();

              vector<CStdString> thumbnails;
              for (int t=0; t < 4; t++)
              {
                if (section->HasArt(PLEX_ART_THUMB, t))
                  thumbnails.push_back(section->GetArt(PLEX_ART_THUMB, t));
              }

              if (thumbnails.size() > 0)
              {
                boost::mutex::scoped_lock lk(m_mutex);
                m_sectionThumbnails[path] = thumbnails;
                dprintf("Added %ld thumbnails for %s", thumbnails.size(), path.c_str());
              }
            }
          }

          m_didFetchThumbs = true;

          /* Scan this host async */
          boost::thread t(boost::bind(&MyPlexManager::scanSharedServer, this, serverPtr, server));
          t.detach();
        }
      }

      /* Check for servers that are removed */
      BOOST_FOREACH(key_server_pair serverPair, m_remoteServers)
      {
        if (newRemoteServers.find(serverPair.first) != newRemoteServers.end())
        {
          PlexServerPtr serv = serverPair.second;

          dprintf("MyPlexManager: Seems like we lost server %s", serv->name.c_str());
          PlexServerManager::Get().removeServer(serv->uuid, serv->name, serv->address, serv->port);
        }
      }

      m_remoteServers = newRemoteServers;
      PlexServerManager::Get().setSharedServers(sharedServers);
    }

    // Get the queue.
    CFileItemList queue;
    getPlaylist(queue, "queue");

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
  bool fetchList(const string& url, CFileItemList& list, const CStdString& token="")
  {
    CStdString ourToken = token;

    if (ourToken.empty() == true)
    {
      ourToken = g_guiSettings.GetString("myplex.token");
    }

    if (ourToken.empty() == true)
    {
      dprintf("MyPlexManager: no token for %s", url.c_str());
      return false;
    }

    // Add the token to the request.
    string request = url;
    request += "?X-Plex-Token=" + ourToken;

    CPlexDirectory plexDir(true, false);
    return plexDir.GetDirectory(request, list);
  }

  /// Utility method for myPlex to setup request headers.
  static void SetupRequestHeaders(CCurlFile& http)
  {
    // Initialize headers.
    http.SetRequestHeader("Content-Type", "application/xml");
    http.SetRequestHeader("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid"));
    http.SetRequestHeader("X-Plex-Product", "Plex Media Center");
    http.SetRequestHeader("X-Plex-Version", g_infoManager.GetVersion());
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

  /* thumbnails */
  map<string, vector<CStdString> > m_sectionThumbnails;
  bool m_didFetchThumbs;

  map<string, PlexServerPtr> m_remoteServers;
  vector<CFileItemPtr> m_sharedSections;
};

class MyPlexPinLogin : public CThread
{
  public:
    MyPlexPinLogin() : CThread("MyPlexPinLoginThread")
    {};

    CStdString m_pin;

    virtual void Process()
    {
      int pageId;
      if (MyPlexManager::Get().CreatePinRequest(m_pin, &pageId))
      {
        CGUIMessage msg(GUI_MSG_MYPLEX_GOT_PIN, 0, WINDOW_DIALOG_MYPLEX_PIN, 0, 0);
        g_windowManager.SendMessage(msg, WINDOW_DIALOG_MYPLEX_PIN);

        bool gotToken = false;
        while (!gotToken && !m_bStop)
        {
          if (MyPlexManager::Get().TryGetTokenFromPin(pageId))
          {
            CGUIMessage msg(GUI_MSG_MYPLEX_GOT_TOKEN, 0, WINDOW_DIALOG_MYPLEX_PIN, 0, 0);
            g_windowManager.SendMessage(msg, WINDOW_DIALOG_MYPLEX_PIN);

            g_guiSettings.SetString("myplex.status", g_localizeStrings.Get(44011));
            MyPlexManager::Get().scanAsync();

            return;
          }

          Sleep(5000);
        }
      }
    }
};
