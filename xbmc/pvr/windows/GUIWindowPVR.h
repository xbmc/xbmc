#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowPVRCommon.h"
#include "epg/GUIEPGGridContainer.h"
#include "utils/Stopwatch.h"
#include "threads/CriticalSection.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

namespace PVR
{
  class CGUIWindowPVRCommon;
  class CGUIWindowPVRChannels;
  class CGUIWindowPVRGuide;
  class CGUIWindowPVRRecordings;
  class CGUIWindowPVRSearch;
  class CGUIWindowPVRTimers;

  class CGUIWindowPVR : public CGUIMediaWindow
  {
    friend class CGUIWindowPVRCommon;
    friend class CGUIWindowPVRChannels;
    friend class CGUIWindowPVRGuide;
    friend class CGUIWindowPVRRecordings;
    friend class CGUIWindowPVRSearch;
    friend class CGUIWindowPVRTimers;

  public:
    virtual CGUIWindowPVRCommon *GetActiveView(void) const;
    virtual void SetActiveView(CGUIWindowPVRCommon *window);
    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
    virtual CGUIWindowPVRCommon *GetSavedView(void) const;
    virtual bool OnAction(const CAction &action);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    virtual void OnInitWindow(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnWindowLoaded(void);
    virtual void OnWindowUnload(void);
    virtual void FrameMove();
    virtual void Reset(void);
    virtual void Cleanup(void);
    virtual CPVRChannelGroupPtr GetSelectedGroup(void);
    virtual void SetSelectedGroup(CPVRChannelGroupPtr group);

    EPG::CGUIEPGGridContainer *m_guideGrid;

  protected:
    CGUIWindowPVR(int windowId, const char *xmlFile, bool bRadio);
    virtual ~CGUIWindowPVR(void);
    
    virtual void SetLabel(int iControl, const CStdString &strLabel);
    virtual void SetLabel(int iControl, int iLabel);
    virtual void UpdateButtons(void);
    virtual bool Update(const CStdString &strDirectory, bool updateFilterPath = true);
    virtual bool OnBack(int actionID);
    virtual bool IsRadio();
    virtual void SetRadio(bool bRadio);

  private:
    virtual bool OnMessageClick(CGUIMessage &message);
    virtual bool OpenGroupDialogSelect();
    virtual void CreateViews(void);

    CGUIWindowPVRCommon *    m_currentSubwindow;
    CGUIWindowPVRCommon *    m_savedSubwindow;

    CGUIWindowPVRChannels *  m_windowChannels;
    CGUIWindowPVRGuide    *  m_windowGuide;
    CGUIWindowPVRRecordings *m_windowRecordings;
    CGUIWindowPVRSearch *    m_windowSearch;
    CGUIWindowPVRTimers *    m_windowTimers;
    bool                     m_bWasReset;
    bool                     m_bRadio;
    CPVRChannelGroupPtr      m_selectedGroup;

    CCriticalSection         m_critSection;

    CStopWatch               m_refreshWatch;
  };
}
