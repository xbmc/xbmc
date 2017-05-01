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
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction& action) override;
    void OnWindowLoaded(void) override;
    void OnWindowUnload(void) override;
    bool HasListItems() const override{ return true; };
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void Clear(void);
    void Update(void);
    void SaveList(void);
    void Renumber(void);
    void SetData(int iItem);
    void RenameChannel(const CFileItemPtr &pItem);

    bool OnPopupMenu(int iItem);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool OnActionMove(const CAction &action);
    bool OnMessageClick(CGUIMessage &message);
    bool OnClickListChannels(CGUIMessage &message);
    bool OnClickButtonOK(CGUIMessage &message);
    bool OnClickButtonApply(CGUIMessage &message);
    bool OnClickButtonCancel(CGUIMessage &message);
    bool OnClickButtonRadioTV(CGUIMessage &message);
    bool OnClickButtonRadioActive(CGUIMessage &message);
    bool OnClickButtonRadioParentalLocked(CGUIMessage &message);
    bool OnClickButtonEditName(CGUIMessage &message);
    bool OnClickButtonChannelLogo(CGUIMessage &message);
    bool OnClickButtonUseEPG(CGUIMessage &message);
    bool OnClickEPGSourceSpin(CGUIMessage &message);
    bool OnClickButtonGroupManager(CGUIMessage &message);
    bool OnClickButtonNewChannel();

    bool PersistChannel(const CFileItemPtr &pItem, const CPVRChannelGroupPtr &group, unsigned int *iChannelNumber);
    void SetItemsUnchanged(void);

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
