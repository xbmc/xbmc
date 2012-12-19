#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <map>
#include <string>

#include "guilib/GUIWindow.h"
#include "PlexContentWorker.h"
#include "PlexContentPlayerMixin.h"

// List IDs.
#define CONTENT_LIST_RECENTLY_ADDED    11000
#define CONTENT_LIST_ON_DECK           11001
#define CONTENT_LIST_RECENTLY_ACCESSED 11002
#define CONTENT_LIST_QUEUE             11003
#define CONTENT_LIST_RECOMMENDATIONS   11004

#define CONTENT_LIST_FANART            12000


#define CONTEXT_BUTTON_SLEEP 1
#define CONTEXT_BUTTON_QUIT 2
#define CONTEXT_BUTTON_SHUTDOWN 3

class PlexContentWorkerManager;
class CGUIWindowHome;

class CFanLoadingThread : public CThread
{
  public:
    CFanLoadingThread(CGUIWindowHome *window) : CThread("Fan Loading Thread") { m_window = window; }
    void Process();
    void LoadFanWithDelay(const CStdString& key, int delay = 300);
    void CancelCurrent();
    void StopThread(bool bWait);

  private:
    CStopWatch m_loadTimer;
    CStdString m_key;
    CGUIWindowHome *m_window;
    boost::mutex m_mutex;
    boost::condition_variable m_wakeMe;
    int m_delay;
};

class CGUIWindowHome : public CGUIWindow,
                       public PlexContentPlayerMixin
{
public:
  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void);
  virtual void UpdateContentForSelectedItem(const std::string& key);

private:
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnPopupMenu();
  virtual bool CheckTimer(const CStdString& strExisting, const CStdString& strNew, int title, int line1, int line2);
  void HideAllLists();
  virtual void SaveStateBeforePlay(CGUIBaseContainer* container);

  void UpdateSections();
  bool SaveSelectedMenuItem();
  void RestoreSelectedMenuItem();
  int  LookupIDFromKey(const std::string& key);
  bool KeyHaveFanout(const CStdString& key);
  CFileItem* CurrentFileItem() const;
  
  std::string m_lastSelectedItemKey;
  
  std::map<int, std::string> m_idToSectionUrlMap;
  std::map<int, int>         m_idToSectionTypeMap;
  std::map<int, Group>       m_contentLists;
  int                        m_selectedContainerID;
  bool                       m_globalArt;
  
  CGUIListItemPtr            m_videoChannelItem;
  CGUIListItemPtr            m_musicChannelItem;
  CGUIListItemPtr            m_photoChannelItem;
  CGUIListItemPtr            m_applicationChannelItem;
  
  PlexContentWorkerManager*  m_workerManager;
  CFanLoadingThread*         m_loadingThread;
};

