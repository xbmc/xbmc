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

#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"
#include "utils/Observer.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include <map>

class CFileItemList;

namespace PVR
{
  class CGUIDialogPVRChannelsOSD : public CGUIDialog, public Observer
  {
  public:
    CGUIDialogPVRChannelsOSD(void);
    virtual ~CGUIDialogPVRChannelsOSD(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool OnAction(const CAction &action);
    virtual void OnWindowLoaded();
    virtual void OnWindowUnload();
    virtual void Notify(const Observable &obs, const ObservableMessage msg);

  protected:
    virtual void OnInitWindow();
    virtual void OnDeinitWindow(int nextWindowID);
    virtual void RestoreControlStates();
    virtual void SaveControlStates();

    void CloseOrSelect(unsigned int iItem);
    void GotoChannel(int iItem);
    void ShowInfo(int item);
    void Clear();
    void Update();
    CPVRChannelGroupPtr GetPlayingGroup();
    CGUIControl *GetFirstFocusableControl(int id);

    CFileItemList    *m_vecItems;
    CGUIViewControl   m_viewControl;

  private:
    CPVRChannelGroupPtr m_group;
    std::map<int, std::string> m_groupSelectedItemPaths;
    void SaveSelectedItemPath(int iGroupID);
    std::string GetLastSelectedItemPath(int iGroupID) const;
  };
}

