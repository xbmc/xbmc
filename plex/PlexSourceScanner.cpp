/*
 *  PlexSourceScanner.cpp
 *  Plex
 *
 *  Created by Elan Feingold & James Clarke on 13/11/2009.
 *  Copyright 2009 Plex Development Team. All rights reserved.
 *
 */
#include "log.h"
#include "File.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "Key.h"
#include "Picture.h"
#include "PlexDirectory.h"
#include "PlexUtils.h"
#include "PlexSourceScanner.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "CocoaUtilsPlus.h"
#include "PlexLibrarySectionManager.h"
#include "URIUtils.h"

map<std::string, HostSourcesPtr> CPlexSourceScanner::g_hostSourcesMap;
boost::recursive_mutex CPlexSourceScanner::g_lock;
int CPlexSourceScanner::g_activeScannerCount = 0;

using namespace XFILE; 

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::Process()
{
  CStdString path;
  
  CLog::Log(LOGNOTICE, "Plex Source Scanner starting...(%s) (uuid: %s)", m_sources->host.c_str(), m_sources->uuid.c_str());

  {
    boost::recursive_mutex::scoped_lock lock(g_lock);
    g_activeScannerCount++;
  }

  // Take the per-HostSources instance lock. We want parallelism, so we don't use g_lock.
  // However, we may have threads scanning the same HostSources object, so we need to protect the vectors
  // Otherwise, extremely nasty heap corruption can ensue.
  boost::recursive_mutex::scoped_lock sources_lock(m_sources->lock);
  
  if (m_sources->host.find("members.mac.com") != std::string::npos)
  {
    CLog::Log(LOGWARNING, "Skipping MobileMe address: %s", m_sources->host.c_str());
  }
  else
  {
    // Compute the real host label (empty for local server).
    std::string realHostLabel = m_sources->hostLabel;
    bool onlyShared = false;
    string url = m_sources->url();
    dprintf("Plex Source Scanner using best URL %s", url.c_str());

    // Act a bit differently if we're talking to a local server.
    bool remoteOwned = false;
    if (g_guiSettings.GetString("myplex.token").empty() == false && 
        url.find(g_guiSettings.GetString("myplex.token")) != string::npos)
      remoteOwned = true;
    
    if (Cocoa_IsHostLocal(m_sources->host) == true || remoteOwned == true)
    {
      realHostLabel = "";
      onlyShared = false;
    }
    
    // Create a new entry.
    CLog::Log(LOGNOTICE, "Scanning remote server: %s (remote: %d)", m_sources->host.c_str(), remoteOwned);
    
    // Scan the server.
    path = PlexUtils::AppendPathToURL(url, "music");
    AutodetectPlexSources(path, m_sources->musicSources, realHostLabel, onlyShared);
    dprintf("Plex Source Scanner for %s: found %ld music channels.", m_sources->hostLabel.c_str(), m_sources->musicSources.size());
    
    path = PlexUtils::AppendPathToURL(url, "video");
    AutodetectPlexSources(path, m_sources->videoSources, realHostLabel, onlyShared);
    dprintf("Plex Source Scanner for %s: found %ld video channels.", m_sources->hostLabel.c_str(), m_sources->videoSources.size());
    
    path = PlexUtils::AppendPathToURL(url, "photos");
    AutodetectPlexSources(path, m_sources->pictureSources, realHostLabel, onlyShared);
    dprintf("Plex Source Scanner for %s: found %ld photo channels.", m_sources->hostLabel.c_str(), m_sources->pictureSources.size());
      
    path = PlexUtils::AppendPathToURL(url, "applications");
    AutodetectPlexSources(path, m_sources->applicationSources, realHostLabel, onlyShared);
    dprintf("Plex Source Scanner for %s: found %ld application channels.", m_sources->hostLabel.c_str(), m_sources->applicationSources.size());
    
    // Library sections.
    path = PlexUtils::AppendPathToURL(url, "library/sections");
    CPlexDirectory plexDir(true, false);
    plexDir.SetTimeout(5);
    m_sources->librarySections.ClearItems();
    bool sectionSuccess = plexDir.GetDirectory(path, m_sources->librarySections);
    dprintf("Plex Source Scanner for %s: found %d library sections, success: %d", m_sources->hostLabel.c_str(), m_sources->librarySections.Size(), sectionSuccess);
    
    // Edit for friendly name.
    vector<CFileItemPtr> sections;
    for (int i=0; i<m_sources->librarySections.Size(); i++)
    {
      CFileItemPtr item = m_sources->librarySections[i];
      item->SetLabel2(m_sources->hostLabel);
      item->SetProperty("machineIdentifier", m_sources->uuid);
      
      // Load and set fanart.
      item->CacheLocalFanart();
      if (CFile::Exists(item->GetCachedProgramFanart()))
        item->SetProperty("fanart_image", item->GetCachedProgramFanart());
      
      CLog::Log(LOGNOTICE, " -> Local section '%s' found.", item->GetLabel().c_str());
      sections.push_back(item);
    }
    
    // Add the sections, but only if they're local (be extra safe).
    if (remoteOwned == false && url.find("X-Plex-Token") == string::npos)
      PlexLibrarySectionManager::Get().addLocalSections(m_sources->uuid, sections);
    
    // Notify the UI.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
    g_windowManager.SendThreadMessage(msg);
    
    // Notify the main menu.
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    g_windowManager.SendThreadMessage(msg2);
  
    CLog::Log(LOGNOTICE, "Scanning host %s is complete.", m_sources->host.c_str());
  }
  
  boost::recursive_mutex::scoped_lock lock(g_lock);
  g_activeScannerCount--;
  
  CLog::Log(LOGNOTICE, "Plex Source Scanner finished for host %s (%d left)", m_sources->host.c_str(), g_activeScannerCount);
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::ScanHost(const std::string& uuid, const std::string& host, const std::string& hostLabel, const std::string& url)
{
  boost::recursive_mutex::scoped_lock lock(g_lock);
  
  // Find or create a new host sources.
  HostSourcesPtr sources;
  
  dprintf("Plex Source Scanner: asked to scan host %s (%s)", host.c_str(), uuid.c_str());
  if (g_hostSourcesMap.count(uuid) != 0)
  {
    // We have an addition source.
    sources = g_hostSourcesMap[uuid];
    sources->urls.insert(url);
    
    dprintf("Plex Source Scanner: got existing server %s (local: %d count: %ld)", host.c_str(), Cocoa_IsHostLocal(host), sources->urls.size());
  }
  else
  {
    // New one.
    sources = HostSourcesPtr(new HostSources(uuid, host, hostLabel, url));
    g_hostSourcesMap[uuid] = sources;
    dprintf("Plex Source Scanner: got new server %s (local: %d count: %ld)", host.c_str(), Cocoa_IsHostLocal(host), sources->urls.size());
  }
  
  new CPlexSourceScanner(sources);
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::RemoveHost(const std::string& uuid, const std::string& url, bool force)
{
  { // Remove the entry from the map in case it was remote.
    boost::recursive_mutex::scoped_lock lock(g_lock);
 
    dprintf("Removing host %s for sources (force: %d)", uuid.c_str(), force);
    HostSourcesPtr sources = g_hostSourcesMap[uuid];
    if (sources)
    {
      // Remove the URL, and if we still have routes to the sources, get out.
      sources->urls.erase(url);
      dprintf("Plex Source Scanner: removing server %s (url: %s), %ld urls left.", sources->hostLabel.c_str(), url.c_str(), sources->urls.size());
      if (sources->urls.size() > 0)
        return;
      
      // If the count went down to zero, whack it.
      g_hostSourcesMap.erase(uuid);
    }    
  }
  
  // Notify the library section manager.
  PlexLibrarySectionManager::Get().removeLocalSections(uuid);
    
  // Notify the UI.
  CLog::Log(LOGNOTICE, "Notifying remote remove host on %s", uuid.c_str());
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
  g_windowManager.SendThreadMessage(msg);
  
  CGUIMessage msg2(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage(msg2);
  
  CGUIMessage msg3(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
  g_windowManager.SendThreadMessage(msg3);
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::MergeSourcesForWindow(int windowId)
{
  boost::recursive_mutex::scoped_lock lock(g_lock);
  
  switch (windowId) 
  {
    case WINDOW_MUSIC_FILES:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_musicSources, pair.second->musicSources);
      CheckForRemovedSources(g_settings.m_musicSources, windowId);
      break;
      
    case WINDOW_PICTURES:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_pictureSources, pair.second->pictureSources);
      CheckForRemovedSources(g_settings.m_pictureSources, windowId);
      break;
      
    case WINDOW_VIDEO_NAV:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_videoSources, pair.second->videoSources);
      CheckForRemovedSources(g_settings.m_videoSources, windowId);
      break;
      
    case WINDOW_PROGRAMS:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_programSources, pair.second->applicationSources);
      CheckForRemovedSources(g_settings.m_programSources, windowId);
      break;
      
    default:
      break;   
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::MergeSource(VECSOURCES& sources, VECSOURCES& remoteSources)
{
  BOOST_FOREACH(CMediaSource source, remoteSources)
  {
    // If the source doesn't already exist, add it.
    bool bIsSourceName = true;
    if (CUtil::GetMatchingSource(source.strName, sources, bIsSourceName) < 0)
    {
      source.m_autoDetected = true;
      sources.push_back(source);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::CheckForRemovedSources(VECSOURCES& sources, int windowId)
{
  VECSOURCES::iterator iterSources = sources.begin();
  while (iterSources != sources.end()) 
  {
    CMediaSource source = *iterSources;
    bool bFound = true;
    
    if (source.m_autoDetected)
    {
      bool bIsSourceName = true;
      bFound = false;
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
      {
        VECSOURCES remoteSources;
        switch (windowId) 
        {
          case WINDOW_MUSIC_FILES:
            remoteSources = pair.second->musicSources;
            break;
          case WINDOW_PICTURES:
            remoteSources = pair.second->pictureSources;
            break;
          case WINDOW_VIDEO_NAV:
            remoteSources = pair.second->videoSources;
            break;
          case WINDOW_PROGRAMS:
            remoteSources = pair.second->applicationSources;
          default:
            return;
        }
        
        if (CUtil::GetMatchingSource(source.strName, remoteSources, bIsSourceName) >= 0)
          bFound = true;
      }
    }
    
    if (!bFound)
      sources.erase(iterSources);
    else
      ++iterSources;
  }  
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::AutodetectPlexSources(CStdString strPlexPath, VECSOURCES& dstSources, CStdString strLabel, bool onlyShared)
{
  bool bIsSourceName = true;
  bool bPerformRemove = true;
  
  // Auto-add PMS sources
  VECSOURCES pmsSources;
  CFileItemList* fileItems = new CFileItemList();
  CPlexDirectory plexDir;
  plexDir.SetTimeout(2);
  
  URIUtils::AddSlashAtEnd(strPlexPath);
  if (plexDir.GetDirectory(strPlexPath, *fileItems))
  {
    // Make sure all items in the PlexDirectory are added as sources
    for ( int i = 0; i < fileItems->Size(); i++ )
    {
      CFileItemPtr item = fileItems->Get(i);
      if ((!onlyShared) || item->HasProperty("share"))
      {
        CMediaSource share;
        share.strName = item->GetLabel();
        
        // Add the label (if provided).
        if (strLabel != "")
          share.strName.Format("%s (%s)", share.strName, strLabel);
        
        // Get special attributes for PMS sources
        if (item->HasProperty("hasPrefs"))
          share.m_hasPrefs = item->GetProperty("hasPrefs").asBoolean();
        
        if (item->HasProperty("pluginIdentifer"))
          share.m_strPluginIdentifier = item->GetProperty("pluginIdentifier").asString();
        
        if (item->HasProperty("hasStoreServices"))
          share.m_hasStoreServices = item->GetProperty("hasStoreServices").asBoolean();
        
        share.strPath = item->GetPath();
        share.m_strFanArtUrl = item->GetQuickFanart();
        share.m_ignore = true;
        
        // Download thumbnail if needed.
        CStdString cachedThumb(item->GetCachedPlexMediaServerThumb());
        CStdString thumb(item->GetThumbnailImage());
        
        if (CFile::Exists(cachedThumb))
        {
          item->SetThumbnailImage(cachedThumb);
        }
        else
        {
          if(CPicture::CreateThumbnail(thumb, cachedThumb))
            item->SetThumbnailImage(cachedThumb);
          else
            item->SetThumbnailImage("");
        }
        
        share.m_strThumbnailImage = cachedThumb;
        
        // Fanart.
        if (!item->HasProperty("fanart_image"))
        {
          item->CacheLocalFanart();
          if (CFile::Exists(item->GetCachedProgramFanart()))
            item->SetProperty("fanart_image", item->GetCachedProgramFanart());
        }
        
        pmsSources.push_back(share);
        if (CUtil::GetMatchingSource(share.strName, dstSources, bIsSourceName) < 0)
          dstSources.push_back(share);
      }
    }
    delete fileItems;
    
    // Remove any local PMS sources that don't exist in the PlexDirectory
    for (int i = dstSources.size() - 1; i >= 0; i--)
    {
      CMediaSource share = dstSources.at(i);
      if ((share.strPath.find(strPlexPath) != string::npos) && (share.strPath.find("/", strPlexPath.length()) == share.strPath.length()-1))
      {
        if (CUtil::GetMatchingSource(dstSources.at(i).strName, pmsSources, bIsSourceName) < 0)
          dstSources.erase(dstSources.begin()+i);
      }
    }
    
    // Everything ran successfully - don't remove PMS sources
    bPerformRemove = false;
  }
  
  // If there was a problem connecting to the local PMS, remove local root sources
  if (bPerformRemove)
  {
    RemovePlexSources(strPlexPath, dstSources);
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void CPlexSourceScanner::RemovePlexSources(CStdString strPlexPath, VECSOURCES& dstSources)
{
  for ( int i = dstSources.size() - 1; i >= 0; i--)
  {
    CMediaSource share = dstSources.at(i);
    if ((share.strPath.find(strPlexPath) != string::npos) && (share.strPath.find("/", strPlexPath.length()) == share.strPath.length()-1))
      dstSources.erase(dstSources.begin()+i);
  }
}
