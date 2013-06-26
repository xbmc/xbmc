/*
 *  Copyright (C) 2011 Plex, Inc.   
 *      Author: Elan Feingold
 */

#pragma once

#include <map>
#include <string>

#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "FileSystem/PlexDirectory.h"
#include "pictures/PictureThumbLoader.h"
#include "ThumbLoader.h"
#include "video/VideoThumbLoader.h"
#include "BackgroundMusicPlayer.h"
#include "music/MusicThumbLoader.h"

class PlexContentWorker;
typedef boost::shared_ptr<PlexContentWorker> PlexContentWorkerPtr;
typedef boost::shared_ptr<boost::thread> thread_ptr;

/////////////////////////////////////////////////////////////////////////////////////////
class PlexContentWorkerManager
{
 public:
  
  /// Constructor.
  PlexContentWorkerManager()
    : m_workerID(0) {}
  
  /// Number of pending workers.
  int pendingWorkers()
  {
    return m_pendingWorkers.size();
  }

  /// Queue a new worker.
  PlexContentWorkerPtr enqueue(int targetWindow, const string& url, int contextID);
  
  /// Find by ID.
  PlexContentWorkerPtr find(int id)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);

    if (m_pendingWorkers.find(id) != m_pendingWorkers.end())
      return m_pendingWorkers[id];

    return PlexContentWorkerPtr();
  }

  /// Destroy by ID.
  void destroy(int id)
  {
    boost::recursive_mutex::scoped_lock lk(m_mutex);

    if (m_pendingWorkers.find(id) != m_pendingWorkers.end())
      m_pendingWorkers.erase(id);
  }

  /// Cancel all pending workers.
  void cancelPending();

 private:
  
  /// Keeps track of the last worker ID.
  int m_workerID;

  /// Keeps track of pending workers.
  map<int, PlexContentWorkerPtr> m_pendingWorkers;
  
  /// Protects the map.
  boost::recursive_mutex m_mutex;
};

/////////////////////////////////////////////////////////////////////////////////////////
class PlexContentWorker
{
  friend class PlexContentWorkerManager;
  
 public:

  void run()
  {
    //printf("[%p] Processing content request in thread [%s]\n", this, m_url.c_str());

    if (m_cancelled == false)
    {
      // Get the results.
      XFILE::CPlexDirectory dir;
      dir.GetDirectory(m_url, *m_results.get());
    }

    // If we haven't been canceled, send them back.
    if (m_cancelled == false)
    {
      // Notify the main menu.
      CGUIMessage msg(GUI_MSG_SEARCH_HELPER_COMPLETE, m_targetWindow, 0, m_id, m_contextID);
      g_windowManager.SendThreadMessage(msg);
    }
    else
    {
      // Get rid of myself.
      m_manager->destroy(m_id);
    }
  }

  void cancel() { m_cancelled = true; }
  CFileItemListPtr getResults() { return m_results; }
  int getID() { return m_id; }
  void setThread(thread_ptr& t) { m_myThread = t; }

 protected:

  PlexContentWorker(PlexContentWorkerManager* manager, int id, int targetWindow, const string& url, int contextID)
    : m_manager(manager)
    , m_id(id)
    , m_targetWindow(targetWindow)
    , m_url(url)
    , m_cancelled(false)
    , m_contextID(contextID)
    , m_results(new CFileItemList())
  {}

 private:

  PlexContentWorkerManager* m_manager;
  
  int              m_id;
  int              m_targetWindow;
  string           m_url;
  bool             m_cancelled;
  int              m_contextID;
  CFileItemListPtr m_results;
  thread_ptr       m_myThread;
};

typedef boost::shared_ptr<CBackgroundInfoLoader> CBackgroundInfoLoaderPtr;

enum LoaderType { kVIDEO_LOADER, kPHOTO_LOADER, kMUSIC_LOADER };

struct Group
{
  Group() {}
  
  Group(LoaderType loaderType)
  {
    if (loaderType == kVIDEO_LOADER)
      loader = CBackgroundInfoLoaderPtr(new CVideoThumbLoader());
    else if (loaderType == kMUSIC_LOADER)
      loader = CBackgroundInfoLoaderPtr(new CMusicThumbLoader());
    else if (loaderType == kPHOTO_LOADER)
      loader = CBackgroundInfoLoaderPtr(new CPictureThumbLoader());
    
    list = CFileItemListPtr(new CFileItemList());
  }
  
  CFileItemListPtr         list;
  CBackgroundInfoLoaderPtr loader;
};

typedef std::pair<int, Group> int_list_pair;
