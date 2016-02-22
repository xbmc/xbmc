#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "guilib/GUIDialog.h"
#include "GUIViewControl.h"

class CFileItemList;

namespace PVR
{
  class CPVRChannelGroup;

  class CGUIDialogPVRGroupManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRGroupManager(void);
    virtual ~CGUIDialogPVRGroupManager(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnWindowLoaded();
    virtual void OnWindowUnload();
    void SetRadio(bool IsRadio) { m_bIsRadio = IsRadio; }

  protected:
    void Clear();
    void Update();

  private:
    bool PersistChanges(void);
    bool CancelChanges(void);
    bool ActionButtonOk(CGUIMessage &message);
    bool ActionButtonNewGroup(CGUIMessage &message);
    bool ActionButtonDeleteGroup(CGUIMessage &message);
    bool ActionButtonRenameGroup(CGUIMessage &message);
    bool ActionButtonUngroupedChannels(CGUIMessage &message);
    bool ActionButtonGroupMembers(CGUIMessage &message);
    bool ActionButtonChannelGroups(CGUIMessage &message);
    bool OnMessageClick(CGUIMessage &message);

    CPVRChannelGroup *m_selectedGroup;
    bool              m_bIsRadio;

    unsigned int      m_iSelectedUngroupedChannel;
    unsigned int      m_iSelectedGroupMember;
    unsigned int      m_iSelectedChannelGroup;

    CFileItemList *   m_ungroupedChannels;
    CFileItemList *   m_groupMembers;
    CFileItemList *   m_channelGroups;

    CGUIViewControl   m_viewUngroupedChannels;
    CGUIViewControl   m_viewGroupMembers;
    CGUIViewControl   m_viewChannelGroups;
  };
}
