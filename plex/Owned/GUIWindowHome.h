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
#include "PlexContentPlayerMixin.h"
#include "Job.h"
#include <boost/timer.hpp>

#include "VideoThumbLoader.h"
#include "MusicThumbLoader.h"
#include "PictureThumbLoader.h"


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

class CGUIWindowHome;

class CAuxFanLoadThread : public CThread
{
  public:
    CAuxFanLoadThread() : CThread("Aux Fan Load Thread"), m_numSeconds(5) {}
    void Process();

    int m_numSeconds;
};

typedef std::pair<int, CFileItemListPtr> contentListPair;

class CPlexSectionLoadJob : public CJob
{
public:
  CPlexSectionLoadJob(const CStdString& url, int contentType) :
  CJob(), m_url(url), m_contentType(contentType), m_list(new CFileItemList) {}
  
  bool DoWork()
  {
    
    CPlexDirectory dir(true, false);
    m_list->Clear();
    return dir.GetDirectory(m_url, *m_list.get());
  }
  
  int GetContentType() const { return m_contentType; }
  CFileItemListPtr GetFileItemList() const { return m_list; }
  CStdString GetUrl() const { return m_url; }
  
  virtual const char *GetType() const { return "sectionloader"; };
  
  /*
  virtual bool operator==(const CJob* job) const
  {
    CPlexSectionLoadJob* ljob = (CPlexSectionLoadJob*)job;
    if (ljob->m_url == m_url) return true;
    return false;
  }
   */
  
  
private:
  CStdString m_url;
  CFileItemListPtr m_list;
  int m_contentType;
};

class CPlexSectionFanout : public IJobCallback
{
  public:
    CPlexSectionFanout(const CStdString& url, int sectionType);

    std::vector<contentListPair> GetContentLists();
    CFileItemListPtr GetContentList(int type);
    void Refresh();
    void Show();

    bool NeedsRefresh();
    static CStdString GetBestServerUrl(const CStdString& extraUrl="");

  private:
    int LoadSection(const CStdString& url, int contentType);
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job) {}

    std::map<int, CFileItemListPtr> m_fileLists;
    CStdString m_url;
    boost::timer m_age;
    CCriticalSection m_critical;
    int m_sectionType;
    std::vector<int> m_outstandingJobs;
  
    /* Thumb loaders, we pre-cache posters to make the fanouts quick and nice */
    CVideoThumbLoader m_videoThumb;
};

class CGUIWindowHome : public CGUIWindow,
                       public PlexContentPlayerMixin
{
public:
  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void) {}
  virtual bool OnMessage(CGUIMessage& message);


private:
  virtual bool OnAction(const CAction &action);
  virtual bool OnPopupMenu();
  virtual bool CheckTimer(const CStdString& strExisting, const CStdString& strNew, int title, int line1, int line2);
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);

  void HideAllLists();
  void RestoreSection();
  void RefreshSection(const CStdString& url, int type);
  void RefreshAllSections(bool force = true);
  void AddSection(const CStdString& url, int sectionType);
  void RemoveSection(const CStdString& url);
  bool ShowSection(const CStdString& url);
  bool ShowCurrentSection();
  std::vector<contentListPair> GetContentListsFromSection(const CStdString& url);
  CFileItemListPtr GetContentListFromSection(const CStdString& url, int contentType);

  CStdString GetCurrentItemName(bool onlySections=false);
  CFileItem* GetCurrentFileItem();

  void UpdateSections();
    
  bool                       m_globalArt;
  
  CGUIListItemPtr            m_videoChannelItem;
  CGUIListItemPtr            m_musicChannelItem;
  CGUIListItemPtr            m_photoChannelItem;
  CGUIListItemPtr            m_applicationChannelItem;

  std::map<CStdString, CPlexSectionFanout*> m_sections;
  
  CAuxFanLoadThread*         m_auxLoadingThread;
  CStdString                 m_lastSelectedItem;
};

