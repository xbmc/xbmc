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
#include "utils/Observer.h"

namespace PVR
{
  class CPVRChannelGroup;
  class CGUIWindowPVR;

  class CGUIWindowPVRChannels : public CGUIWindowPVRCommon, private Observer
  {
    friend class CGUIWindowPVR;

  public:
    CGUIWindowPVRChannels(CGUIWindowPVR *parent, bool bRadio);
    virtual ~CGUIWindowPVRChannels(void) {};

    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) const;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    virtual const CPVRChannelGroup *SelectedGroup(void);
    virtual void SetSelectedGroup(CPVRChannelGroup *group);
    virtual CPVRChannelGroup *SelectNextGroup(void);
    virtual void UpdateData(void);
    virtual void Notify(const Observable &obs, const CStdString& msg);
    virtual void ResetObservers(void);
    virtual void UnregisterObservers(void);

  private:
    virtual bool OnClickButton(CGUIMessage &message);
    virtual bool OnClickList(CGUIMessage &message);

    virtual bool OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonGroupManager(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonHide(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonMove(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSetThumb(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonShowHidden(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonFilter(CFileItem *item, CONTEXT_BUTTON button);

    virtual void ShowGroupManager(void);

    CPVRChannelGroup *m_selectedGroup;
    bool              m_bShowHiddenChannels;
    bool              m_bRadio;
  };
}
