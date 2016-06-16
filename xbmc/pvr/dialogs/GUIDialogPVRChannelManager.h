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

#include <vector>

#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include "pvr/channels/PVRChannelGroup.h"
#include "addons/PVRClient.h"

namespace PVR
{
  class CGUIDialogPVRChannelManager : public CGUIDialog
  {
  public:
    CGUIDialogPVRChannelManager(void);
    virtual ~CGUIDialogPVRChannelManager(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool OnAction(const CAction& action);
    virtual void OnWindowLoaded(void);
    virtual void OnWindowUnload(void);
    virtual bool HasListItems() const { return true; };
    virtual CFileItemPtr GetCurrentListItem(int offset = 0);

  protected:
    virtual void OnInitWindow();
    virtual void OnDeinitWindow(int nextWindowID);

    virtual bool OnPopupMenu(int iItem);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

    virtual bool OnActionMove(const CAction &action);

    virtual bool OnMessageClick(CGUIMessage &message);

    virtual bool OnClickListChannels(CGUIMessage &message);
    virtual bool OnClickButtonOK(CGUIMessage &message);
    virtual bool OnClickButtonApply(CGUIMessage &message);
    virtual bool OnClickButtonCancel(CGUIMessage &message);
    virtual bool OnClickButtonRadioTV(CGUIMessage &message);
    virtual bool OnClickButtonRadioActive(CGUIMessage &message);
    virtual bool OnClickButtonRadioParentalLocked(CGUIMessage &message);
    virtual bool OnClickButtonEditName(CGUIMessage &message);
    virtual bool OnClickButtonChannelLogo(CGUIMessage &message);
    virtual bool OnClickButtonUseEPG(CGUIMessage &message);
    virtual bool OnClickEPGSourceSpin(CGUIMessage &message);
    virtual bool OnClickButtonGroupManager(CGUIMessage &message);
    virtual bool OnClickButtonNewChannel();

    virtual bool PersistChannel(CFileItemPtr pItem, CPVRChannelGroupPtr group, unsigned int *iChannelNumber);
    virtual void SetItemsUnchanged(void);

  private:
    void Clear(void);
    void Update(void);
    void SaveList(void);
    void Renumber(void);
    void SetData(int iItem);
    void RenameChannel(CFileItemPtr pItem);
    bool m_bIsRadio;
    bool m_bMovingMode;
    bool m_bContainsChanges;
    bool m_bAllowNewChannel;

    int m_iSelected;
    CFileItemList* m_channelItems;
    CGUIViewControl m_viewControl;

    typedef std::vector<PVR_CLIENT>::iterator PVR_CLIENT_ITR;
    std::vector<PVR_CLIENT> m_clientsWithSettingsList;
  };
}
