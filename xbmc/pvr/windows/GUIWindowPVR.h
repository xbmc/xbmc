#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "GUIWindowPVRCommon.h"
#include "epg/GUIEPGGridContainer.h"
#include "threads/CriticalSection.h"

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
    CGUIWindowPVR(void);
    virtual ~CGUIWindowPVR(void);

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
    virtual void InitializeEpgCache(void);
    virtual void Reset(void);

    void UnlockWindow();

    EPG::CGUIEPGGridContainer *m_guideGrid;

  protected:
    virtual void SetLabel(int iControl, const CStdString &strLabel);
    virtual void SetLabel(int iControl, int iLabel);
    virtual void UpdateButtons(void);

  private:
    virtual bool OnMessageFocus(CGUIMessage &message);
    virtual bool OnMessageClick(CGUIMessage &message);

    virtual void CreateViews(void);

    CGUIWindowPVRCommon *    m_currentSubwindow;
    CGUIWindowPVRCommon *    m_savedSubwindow;

    CGUIWindowPVRChannels *  m_windowChannelsTV;
    CGUIWindowPVRChannels *  m_windowChannelsRadio;
    CGUIWindowPVRGuide    *  m_windowGuide;
    CGUIWindowPVRRecordings *m_windowRecordings;
    CGUIWindowPVRSearch *    m_windowSearch;
    CGUIWindowPVRTimers *    m_windowTimers;

    bool                     m_bViewsCreated;
    CCriticalSection         m_critSection;
    bool                     m_bDialogOKActive;
  };
}
