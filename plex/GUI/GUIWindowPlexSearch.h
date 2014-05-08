#pragma once

/*
 *      Copyright (C) 2013 Plex
 *      http://www.plexapp.com
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

#include "FileItem.h"
#include "guilib/GUIWindow.h"
#include "JobManager.h"
#include "guilib/GUIEditControl.h"
#include "PlexGlobalTimer.h"
#include "threads/CriticalSection.h"
#include "PlexNavigationHelper.h"

class CGUIWindowPlexSearch : public CGUIWindow, public IJobCallback, public IPlexGlobalTimeout
{
  public:

    CGUIWindowPlexSearch();
    virtual ~CGUIWindowPlexSearch();

    virtual bool OnAction(const CAction &action);
    virtual bool OnMessage(CGUIMessage& message);

    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    bool InProgress() const
    {
      CSingleLock lk(m_threadsSection);
      return (m_currentSearchId.size() > 0);
    }

    CStdString TimerName() const { return "plexSearch"; }

  private:
    void InitWindow();
    void UpdateSearch();
    void OnTimeout();
    CStdString GetString();
    void HideAllLists();
    void ProcessResults(CFileItemList *results);
    void Reset();
    CGUIEditControl *GetEditControl() const;

    std::vector<unsigned int> m_currentSearchId;
    CStdString m_currentSearchString;
    CCriticalSection m_threadsSection;

    std::map<int, int> m_resultMap;
    bool OnClick(int senderId, int action);

    CPlexNavigationHelper m_navHelper;

    int m_lastFocusedContainer;
    int m_lastFocusedItem;
};
