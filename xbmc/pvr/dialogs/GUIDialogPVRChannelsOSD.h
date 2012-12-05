#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#include "GUIViewControl.h"
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
    virtual void OnWindowLoaded();
    virtual void OnWindowUnload();
    virtual void Notify(const Observable &obs, const ObservableMessage msg);

  protected:
    void CloseOrSelect(unsigned int iItem);
    void GotoChannel(int iItem);
    void ShowInfo(int item);
    void Clear();
    void Update();
    void Update(bool selectPlayingChannel);
    CPVRChannelGroupPtr GetPlayingGroup();
    CGUIControl *GetFirstFocusableControl(int id);

    CFileItemList    *m_vecItems;
    CGUIViewControl   m_viewControl;

  private:
    CPVRChannelGroupPtr m_group;
    std::map<int,int> m_groupSelectedItems;
    void SetLastSelectedItem(int iGroupID);
    int GetLastSelectedItem(int iGroupID) const;
  };
}

