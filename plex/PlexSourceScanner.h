#pragma once
/*
 *  PlexSourceScanner.h
 *  Plex
 *
 *  Created by Elan Feingold & James Clarke on 13/11/2009.
 *  Copyright 2009 Plex Development Team. All rights reserved.
 *
 */

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/timer.hpp>

#include "CocoaUtilsPlus.h"
#include "threads/CriticalSection.h"
#include "FileItem.h"
#include "threads/SingleLock.h"
#include "Settings.h"
#include "threads/Thread.h"
#include "URL.h"

#include <string>
#include <set>

class HostSources
{
 public:

  HostSources(const std::string& uuid, const std::string& host, const std::string& hostLabel, const std::string& url) 
    : uuid(uuid), host(host), hostLabel(hostLabel)
  {
    urls.insert(url);
  }
    
  void reset()
  {
    videoSources.clear();
    musicSources.clear();
    pictureSources.clear();
    applicationSources.clear();
    
    librarySections.Clear();
  }
  
  std::string url()
  {
    std::string ret;
    
    BOOST_FOREACH(std::string url, urls)
    {
      CURL theURL(url);
      ret = url;

      // If we found a local one, be happy.
      if (Cocoa_IsHostLocal(theURL.GetHostName()))
        return url;
    }
    
    // If we have a local one, prefer it.
    return ret;
  }
  
  std::string        uuid;
  std::string        host;
  std::string        hostLabel;
  std::set<std::string>   urls;
  bool          localConnection;
  VECSOURCES    videoSources;
  VECSOURCES    musicSources;
  VECSOURCES    pictureSources;
  VECSOURCES    applicationSources;
  CFileItemList librarySections;

  // This lock is used in CPlexSourceScanner::Process to protect the vectors defined above from concurrent use.
  boost::recursive_mutex lock;
  boost::timer m_lastScan;
};

typedef boost::shared_ptr<HostSources> HostSourcesPtr;
typedef std::pair<std::string, HostSourcesPtr> StringSourcesPair;

////////////////////////////////////////////
class CPlexSourceScanner : public CThread
{
public:
  
  virtual void Process();
  
  static void ScanHost(const std::string& uuid, const std::string& host, const std::string& hostLabel, const std::string& url);
  static void RemoveHost(const std::string& uuid, const std::string& url, bool force=false);
  
  static void MergeSourcesForWindow(int windowId);
  
  static void Lock() { g_lock.lock(); }
  static std::map<std::string, HostSourcesPtr>& GetMap() { return g_hostSourcesMap; }
  static void Unlock() { g_lock.unlock(); }

  static int GetActiveScannerCount() { return g_activeScannerCount; } 
  
  static void AutodetectPlexSources(CStdString strPlexPath, VECSOURCES& dstSources, CStdString strLabel = "", bool onlyShared = false);
  static void RemovePlexSources(CStdString strPlexPath, VECSOURCES& dstSources);
  
protected:
  
  static void MergeSource(VECSOURCES& sources, VECSOURCES& remoteSources);
  static void CheckForRemovedSources(VECSOURCES& sources, int windowId);
  
  CPlexSourceScanner(const HostSourcesPtr& sources)
    : CThread(CStdString("PlexSourceScanner: ") + CStdString(sources->host)), m_sources(sources)
  {
    Create(true);
  }
  
  virtual ~CPlexSourceScanner() {}
  
private:
  
  HostSourcesPtr m_sources;
  
  static std::map<std::string, HostSourcesPtr> g_hostSourcesMap;
  static boost::recursive_mutex g_lock;
  static int g_activeScannerCount;
};
