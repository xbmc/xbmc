/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelManager.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define BUTTON_OK                 4
#define BUTTON_APPLY              5
#define BUTTON_CANCEL             6
#define RADIOBUTTON_ACTIVE        7
#define EDIT_NAME                 8
#define BUTTON_CHANNEL_LOGO       9
#define IMAGE_CHANNEL_LOGO        10
#define RADIOBUTTON_USEEPG        12
#define SPIN_EPGSOURCE_SELECTION  13
#define RADIOBUTTON_PARENTAL_LOCK 14
#define CONTROL_LIST_CHANNELS     20
#define BUTTON_GROUP_MANAGER      30
#define BUTTON_NEW_CHANNEL        31
#define BUTTON_RADIO_TV           34
#define BUTTON_REFRESH_LOGOS 35

namespace
{
constexpr const char* LABEL_CHANNEL_DISABLED = "0";

// Note: strings must not be changed; they are part of the public skinning API for this dialog.
constexpr const char* PROPERTY_CHANNEL_NUMBER = "Number";
constexpr const char* PROPERTY_CHANNEL_ENABLED = "ActiveChannel";
constexpr const char* PROPERTY_CHANNEL_USER_SET_HIDDEN = "UserSetHidden";
constexpr const char* PROPERTY_CHANNEL_USER_SET_NAME = "UserSetName";
constexpr const char* PROPERTY_CHANNEL_LOCKED = "ParentalLocked";
constexpr const char* PROPERTY_CHANNEL_ICON = "Icon";
constexpr const char* PROPERTY_CHANNEL_CUSTOM_ICON = "UserSetIcon";
constexpr const char* PROPERTY_CHANNEL_NAME = "Name";
constexpr const char* PROPERTY_CHANNEL_EPG_ENABLED = "UseEPG";
constexpr const char* PROPERTY_CHANNEL_EPG_SOURCE = "EPGSource";
constexpr const char* PROPERTY_CLIENT_SUPPORTS_SETTINGS = "SupportsSettings";
constexpr const char* PROPERTY_CLIENT_NAME = "ClientName";
constexpr const char* PROPERTY_ITEM_CHANGED = "Changed";

} // namespace

using namespace PVR;
using namespace KODI::MESSAGING;

CGUIDialogPVRChannelManager::CGUIDialogPVRChannelManager() :
    CGUIDialog(WINDOW_DIALOG_PVR_CHANNEL_MANAGER, "DialogPVRChannelManager.xml"),
    m_channelItems(new CFileItemList)
{
  SetRadio(false);
}

CGUIDialogPVRChannelManager::~CGUIDialogPVRChannelManager()
{
  delete m_channelItems;
}

bool CGUIDialogPVRChannelManager::OnActionMove(const CAction& action)
{
  bool bReturn(false);
  int iActionId = action.GetID();

  if (GetFocusedControlID() == CONTROL_LIST_CHANNELS)
  {
    if (iActionId == ACTION_MOUSE_MOVE)
    {
      int iSelected = m_viewControl.GetSelectedItem();
      if (m_iSelected < iSelected)
      {
        iActionId = ACTION_MOVE_DOWN;
      }
      else if (m_iSelected > iSelected)
      {
        iActionId = ACTION_MOVE_UP;
      }
      else
      {
        return bReturn;
      }
    }

    if (iActionId == ACTION_MOVE_DOWN || iActionId == ACTION_MOVE_UP ||
        iActionId == ACTION_PAGE_DOWN || iActionId == ACTION_PAGE_UP ||
        iActionId == ACTION_FIRST_PAGE || iActionId == ACTION_LAST_PAGE)
    {
      CGUIDialog::OnAction(action);
      int iSelected = m_viewControl.GetSelectedItem();

      bReturn = true;
      if (!m_bMovingMode)
      {
        if (iSelected != m_iSelected)
        {
          m_iSelected = iSelected;
          SetData(m_iSelected);
        }
      }
      else
      {
        bool bMoveUp = iActionId == ACTION_PAGE_UP || iActionId == ACTION_MOVE_UP || iActionId == ACTION_FIRST_PAGE;
        unsigned int iLines = bMoveUp ? abs(m_iSelected - iSelected) : 1;
        bool bOutOfBounds = bMoveUp ? m_iSelected <= 0  : m_iSelected >= m_channelItems->Size() - 1;
        if (bOutOfBounds)
        {
          bMoveUp = !bMoveUp;
          iLines = m_channelItems->Size() - 1;
        }
        for (unsigned int iLine = 0; iLine < iLines; ++iLine)
        {
          unsigned int iNewSelect = bMoveUp ? m_iSelected - 1 : m_iSelected + 1;

          const CFileItemPtr newItem = m_channelItems->Get(iNewSelect);
          const std::string number = newItem->GetProperty(PROPERTY_CHANNEL_NUMBER).asString();
          if (number != LABEL_CHANNEL_DISABLED)
          {
            // Swap channel numbers
            const CFileItemPtr item = m_channelItems->Get(m_iSelected);
            newItem->SetProperty(PROPERTY_CHANNEL_NUMBER,
                                 item->GetProperty(PROPERTY_CHANNEL_NUMBER));
            SetItemChanged(newItem);
            item->SetProperty(PROPERTY_CHANNEL_NUMBER, number);
            SetItemChanged(item);
          }

          // swap items
          m_channelItems->Swap(iNewSelect, m_iSelected);
          m_iSelected = iNewSelect;
        }

        m_viewControl.SetItems(*m_channelItems);
        m_viewControl.SetSelectedItem(m_iSelected);
      }
    }
  }

  return bReturn;
}

bool CGUIDialogPVRChannelManager::OnAction(const CAction& action)
{
  return OnActionMove(action) ||
         CGUIDialog::OnAction(action);
}

void CGUIDialogPVRChannelManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_iSelected = 0;
  m_bMovingMode = false;
  m_bAllowNewChannel = false;

  EnableChannelOptions(false);
  CONTROL_DISABLE(BUTTON_APPLY);

  // prevent resorting channels if backend channel numbers or backend channel order shall be used
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_bAllowRenumber = !settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS);
  m_bAllowReorder =
      m_bAllowRenumber && !settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);

  Update();

  if (m_initialSelection)
  {
    // set initial selection
    const std::shared_ptr<const CPVRChannel> channel = m_initialSelection->GetPVRChannelInfoTag();
    for (int i = 0; i < m_channelItems->Size(); ++i)
    {
      if (m_channelItems->Get(i)->GetPVRChannelInfoTag() == channel)
      {
        m_iSelected = i;
        m_viewControl.SetSelectedItem(m_iSelected);
        break;
      }
    }
    m_initialSelection.reset();
  }
  SetData(m_iSelected);
}

void CGUIDialogPVRChannelManager::OnDeinitWindow(int nextWindowID)
{
  Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIDialogPVRChannelManager::SetRadio(bool bIsRadio)
{
  m_bIsRadio = bIsRadio;
  SetProperty("IsRadio", m_bIsRadio ? "true" : "");
}

void CGUIDialogPVRChannelManager::Open(const std::shared_ptr<CFileItem>& initialSelection)
{
  m_initialSelection = initialSelection;
  CGUIDialog::Open();
}

bool CGUIDialogPVRChannelManager::OnClickListChannels(const CGUIMessage& message)
{
  if (!m_bMovingMode)
  {
    int iAction = message.GetParam1();
    int iItem = m_viewControl.GetSelectedItem();

    /* Check file item is in list range and get his pointer */
    if (iItem < 0 || iItem >= m_channelItems->Size()) return true;

    /* Process actions */
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
    {
      /* Show Contextmenu */
      OnPopupMenu(iItem);

      return true;
    }
  }
  else
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      pItem->Select(false);
      m_bMovingMode = false;
      SetItemChanged(pItem);
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonOK()
{
  SaveList();
  Close();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonApply()
{
  SaveList();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonCancel()
{
  Close();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioTV()
{
  PromptAndSaveList();

  m_iSelected = 0;
  m_bMovingMode = false;
  m_bAllowNewChannel = false;
  m_bIsRadio = !m_bIsRadio;
  SetProperty("IsRadio", m_bIsRadio ? "true" : "");
  Update();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioActive()
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_ACTIVE);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      const bool selected = (msg.GetParam1() == 1);
      if (pItem->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean() != selected)
      {
        pItem->SetProperty(PROPERTY_CHANNEL_ENABLED, selected);
        pItem->SetProperty(PROPERTY_CHANNEL_USER_SET_HIDDEN, true);
        SetItemChanged(pItem);
        Renumber();
      }
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioParentalLocked()
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_PARENTAL_LOCK);
  if (!OnMessage(msg))
    return false;

  bool selected(msg.GetParam1() == 1);

  // ask for PIN first
  if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalPIN() !=
      ParentalCheckResult::SUCCESS)
  { // failed - reset to previous
    SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_PARENTAL_LOCK, !selected);
    return false;
  }

  CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
  if (pItem)
  {
    if (pItem->GetProperty(PROPERTY_CHANNEL_LOCKED).asBoolean() != selected)
    {
      pItem->SetProperty(PROPERTY_CHANNEL_LOCKED, selected);
      SetItemChanged(pItem);
      Renumber();
    }
    return true;
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonEditName()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), EDIT_NAME);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      const std::string& label = msg.GetLabel();
      if (pItem->GetProperty(PROPERTY_CHANNEL_NAME).asString() != label)
      {
        pItem->SetProperty(PROPERTY_CHANNEL_NAME, label);
        pItem->SetProperty(PROPERTY_CHANNEL_USER_SET_NAME, true);
        SetItemChanged(pItem);
      }
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonChannelLogo()
{
  CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
  if (!pItem)
    return false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (profileManager->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
    return false;

  // setup our thumb list
  CFileItemList items;

  // add the current thumb, if available
  if (!pItem->GetProperty(PROPERTY_CHANNEL_ICON).asString().empty())
  {
    CFileItemPtr current(new CFileItem("thumb://Current", false));
    current->SetArt("thumb", pItem->GetPVRChannelInfoTag()->IconPath());
    current->SetLabel(g_localizeStrings.Get(19282));
    items.Add(current);
  }
  else if (pItem->HasArt("thumb"))
  { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
    CFileItemPtr current(new CFileItem("thumb://Current", false));
    current->SetArt("thumb", pItem->GetArt("thumb"));
    current->SetLabel(g_localizeStrings.Get(19282));
    items.Add(current);
  }

  // and add a "no thumb" entry as well
  CFileItemPtr nothumb(new CFileItem("thumb://None", false));
  nothumb->SetArt("icon", pItem->GetArt("icon"));
  nothumb->SetLabel(g_localizeStrings.Get(19283));
  items.Add(nothumb);

  std::string strThumb;
  VECSOURCES shares;
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetString(CSettings::SETTING_PVRMENU_ICONPATH) != "")
  {
    CMediaSource share1;
    share1.strPath = settings->GetString(CSettings::SETTING_PVRMENU_ICONPATH);
    share1.strName = g_localizeStrings.Get(19066);
    shares.push_back(share1);
  }
  CServiceBroker::GetMediaManager().GetLocalDrives(shares);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(19285), strThumb, NULL, 19285))
    return false;

  if (strThumb == "thumb://Current")
    return true;

  if (strThumb == "thumb://None")
    strThumb = "";

  if (pItem->GetProperty(PROPERTY_CHANNEL_ICON).asString() != strThumb)
  {
    pItem->SetProperty(PROPERTY_CHANNEL_ICON, strThumb);
    pItem->SetProperty(PROPERTY_CHANNEL_CUSTOM_ICON, true);
    SetItemChanged(pItem);
  }

  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonUseEPG()
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_USEEPG);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      const bool selected = (msg.GetParam1() == 1);
      if (pItem->GetProperty(PROPERTY_CHANNEL_EPG_ENABLED).asBoolean() != selected)
      {
        pItem->SetProperty(PROPERTY_CHANNEL_EPG_ENABLED, selected);
        SetItemChanged(pItem);
      }
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickEPGSourceSpin()
{
  //! @todo Add EPG scraper support
  return true;
  // CGUISpinControlEx* pSpin = static_cast<CGUISpinControlEx*>(GetControl(SPIN_EPGSOURCE_SELECTION));
  // if (pSpin)
  // {
  //   CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
  //   if (pItem)
  //   {
  //     if (pItem->GetProperty(PROPERTY_CHANNEL_EPG_SOURCE).asInteger() != 0)
  //     {
  //       pItem->SetProperty(PROPERTY_CHANNEL_EPG_SOURCE, static_cast<int>(0));
  //       SetItemChanged(pItem);
  //     }
  //     return true;
  //   }
  // }
}

bool CGUIDialogPVRChannelManager::OnClickButtonGroupManager()
{
  PromptAndSaveList();

  /* Load group manager dialog */
  CGUIDialogPVRGroupManager* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRGroupManager>(WINDOW_DIALOG_PVR_GROUP_MANAGER);
  if (!pDlgInfo)
    return false;

  pDlgInfo->SetRadio(m_bIsRadio);

  /* Open dialog window */
  pDlgInfo->Open();

  Update();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonNewChannel()
{
  PromptAndSaveList();

  int iSelection = 0;
  if (m_clientsWithSettingsList.size() > 1)
  {
    CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (!pDlgSelect)
      return false;

    pDlgSelect->SetHeading(CVariant{19213}); // Select Client

    for (const auto& client : m_clientsWithSettingsList)
      pDlgSelect->Add(client->Name());
    pDlgSelect->Open();

    iSelection = pDlgSelect->GetSelectedItem();
  }

  if (iSelection >= 0 && iSelection < static_cast<int>(m_clientsWithSettingsList.size()))
  {
    int iClientID = m_clientsWithSettingsList[iSelection]->GetID();

    std::shared_ptr<CPVRChannel> channel(new CPVRChannel(m_bIsRadio));
    channel->SetChannelName(g_localizeStrings.Get(19204)); // New channel
    channel->SetClientID(iClientID);

    PVR_ERROR ret = PVR_ERROR_UNKNOWN;
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(iClientID);
    if (client)
    {
      channel->SetEPGEnabled(client->GetClientCapabilities().SupportsEPG());
      ret = client->OpenDialogChannelAdd(channel);
    }

    if (ret == PVR_ERROR_NO_ERROR)
    {
      CFileItemList prevChannelItems;
      prevChannelItems.Assign(*m_channelItems);

      Update();

      for (int index = 0; index < m_channelItems->Size(); ++index)
      {
        if (!prevChannelItems.Contains(m_channelItems->Get(index)->GetPath()))
        {
          m_iSelected = index;
          m_viewControl.SetSelectedItem(m_iSelected);
          SetData(m_iSelected);
          break;
        }
      }
    }
    else if (ret == PVR_ERROR_NOT_IMPLEMENTED)
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19038}); // "Information", "Not supported by the PVR backend."
    else
      HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // "Add-on error", "Check the log for more information about this message."
  }
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRefreshChannelLogos()
{
  for (const auto& item : *m_channelItems)
  {
    const std::string thumb = item->GetArt("thumb");
    if (!thumb.empty())
    {
      // clear current cached image
      CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
      item->SetArt("thumb", "");
    }
  }

  m_iSelected = 0;
  Update();

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return true;
}

bool CGUIDialogPVRChannelManager::OnMessageClick(const CGUIMessage& message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
  case CONTROL_LIST_CHANNELS:
    return OnClickListChannels(message);
  case BUTTON_OK:
    return OnClickButtonOK();
  case BUTTON_APPLY:
    return OnClickButtonApply();
  case BUTTON_CANCEL:
    return OnClickButtonCancel();
  case BUTTON_RADIO_TV:
    return OnClickButtonRadioTV();
  case RADIOBUTTON_ACTIVE:
    return OnClickButtonRadioActive();
  case RADIOBUTTON_PARENTAL_LOCK:
    return OnClickButtonRadioParentalLocked();
  case EDIT_NAME:
    return OnClickButtonEditName();
  case BUTTON_CHANNEL_LOGO:
    return OnClickButtonChannelLogo();
  case RADIOBUTTON_USEEPG:
    return OnClickButtonUseEPG();
  case SPIN_EPGSOURCE_SELECTION:
    return OnClickEPGSourceSpin();
  case BUTTON_GROUP_MANAGER:
    return OnClickButtonGroupManager();
  case BUTTON_NEW_CHANNEL:
    return OnClickButtonNewChannel();
  case BUTTON_REFRESH_LOGOS:
    return OnClickButtonRefreshChannelLogos();
  default:
    return false;
  }
}

bool CGUIDialogPVRChannelManager::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_CLICKED:
      return OnMessageClick(message);
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRChannelManager::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS));
}

void CGUIDialogPVRChannelManager::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CFileItemPtr CGUIDialogPVRChannelManager::GetCurrentListItem(int offset)
{
  return m_channelItems->Get(m_iSelected);
}

bool CGUIDialogPVRChannelManager::OnPopupMenu(int iItem)
{
  // popup the context menu
  // grab our context menu
  CContextButtons buttons;

  // mark the item
  if (iItem >= 0 && iItem < m_channelItems->Size())
    m_channelItems->Get(iItem)->Select(true);
  else
    return false;

  CFileItemPtr pItem = m_channelItems->Get(iItem);
  if (!pItem)
    return false;

  if (m_bAllowReorder &&
      pItem->GetProperty(PROPERTY_CHANNEL_NUMBER).asString() != LABEL_CHANNEL_DISABLED)
    buttons.Add(CONTEXT_BUTTON_MOVE, 116); /* Move channel up or down */

  if (pItem->GetProperty(PROPERTY_CLIENT_SUPPORTS_SETTINGS).asBoolean())
  {
    buttons.Add(CONTEXT_BUTTON_SETTINGS, 10004); /* Open add-on channel settings dialog */
    buttons.Add(CONTEXT_BUTTON_DELETE, 117); /* Delete add-on channel */
  }

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

  // deselect our item
  if (iItem >= 0 && iItem < m_channelItems->Size())
    m_channelItems->Get(iItem)->Select(false);

  if (choice < 0)
    return false;

  return OnContextButton(iItem, (CONTEXT_BUTTON)choice);
}

bool CGUIDialogPVRChannelManager::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  /* Check file item is in list range and get his pointer */
  if (itemNumber < 0 || itemNumber >= m_channelItems->Size()) return false;

  CFileItemPtr pItem = m_channelItems->Get(itemNumber);
  if (!pItem)
    return false;

  if (button == CONTEXT_BUTTON_MOVE)
  {
    m_bMovingMode = true;
    pItem->Select(true);
  }
  else if (button == CONTEXT_BUTTON_SETTINGS)
  {
    PromptAndSaveList();

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*pItem);
    PVR_ERROR ret = PVR_ERROR_UNKNOWN;
    if (client)
      ret = client->OpenDialogChannelSettings(pItem->GetPVRChannelInfoTag());

    if (ret == PVR_ERROR_NO_ERROR)
    {
      Update();
      SetData(m_iSelected);
    }
    else if (ret == PVR_ERROR_NOT_IMPLEMENTED)
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19038}); // "Information", "Not supported by the PVR backend."
    else
      HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // "Add-on error", "Check the log for more information about this message."
  }
  else if (button == CONTEXT_BUTTON_DELETE)
  {
    CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return true;

    pDialog->SetHeading(CVariant{19211}); // Delete channel
    pDialog->SetText(CVariant{750}); // Are you sure?
    pDialog->Open();

    if (pDialog->IsConfirmed())
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*pItem);
      if (client)
      {
        const std::shared_ptr<const CPVRChannel> channel = pItem->GetPVRChannelInfoTag();
        PVR_ERROR ret = client->DeleteChannel(channel);
        if (ret == PVR_ERROR_NO_ERROR)
        {
          CPVRChannelGroups* groups =
              CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
          if (groups)
          {
            groups->UpdateFromClients({});
            Update();
          }
        }
        else if (ret == PVR_ERROR_NOT_IMPLEMENTED)
          HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19038}); // "Information", "Not supported by the PVR backend."
        else
          HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // "Add-on error", "Check the log for more information about this message."
      }
    }
  }
  return true;
}

void CGUIDialogPVRChannelManager::SetData(int iItem)
{
  if (iItem < 0 || iItem >= m_channelItems->Size())
  {
    ClearChannelOptions();
    EnableChannelOptions(false);
    return;
  }

  CFileItemPtr pItem = m_channelItems->Get(iItem);
  if (!pItem)
    return;

  SET_CONTROL_LABEL2(EDIT_NAME, pItem->GetProperty(PROPERTY_CHANNEL_NAME).asString());
  CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), EDIT_NAME, CGUIEditControl::INPUT_TYPE_TEXT, 19208);
  OnMessage(msg);

  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_ACTIVE,
                       pItem->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean());
  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_USEEPG,
                       pItem->GetProperty(PROPERTY_CHANNEL_EPG_ENABLED).asBoolean());
  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_PARENTAL_LOCK,
                       pItem->GetProperty(PROPERTY_CHANNEL_LOCKED).asBoolean());

  EnableChannelOptions(true);
}

void CGUIDialogPVRChannelManager::Update()
{
  m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS);

  // empty the lists ready for population
  Clear();

  std::shared_ptr<CPVRChannelGroup> channels = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);

  // No channels available, nothing to do.
  if (!channels)
    return;

  channels->UpdateFromClients({});

  const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers = channels->GetMembers();
  std::shared_ptr<CFileItem> channelFile;
  for (const auto& member : groupMembers)
  {
    channelFile = std::make_shared<CFileItem>(member);
    if (!channelFile)
      continue;
    const std::shared_ptr<const CPVRChannel> channel(channelFile->GetPVRChannelInfoTag());

    channelFile->SetProperty(PROPERTY_CHANNEL_ENABLED, !channel->IsHidden());
    channelFile->SetProperty(PROPERTY_CHANNEL_USER_SET_HIDDEN, channel->IsUserSetHidden());
    channelFile->SetProperty(PROPERTY_CHANNEL_USER_SET_NAME, channel->IsUserSetName());
    channelFile->SetProperty(PROPERTY_CHANNEL_NAME, channel->ChannelName());
    channelFile->SetProperty(PROPERTY_CHANNEL_EPG_ENABLED, channel->EPGEnabled());
    channelFile->SetProperty(PROPERTY_CHANNEL_ICON, channel->IconPath());
    channelFile->SetProperty(PROPERTY_CHANNEL_CUSTOM_ICON, channel->IsUserSetIcon());
    channelFile->SetProperty(PROPERTY_CHANNEL_EPG_SOURCE, 0);
    channelFile->SetProperty(PROPERTY_CHANNEL_LOCKED, channel->IsLocked());
    channelFile->SetProperty(PROPERTY_CHANNEL_NUMBER,
                             member->ChannelNumber().FormattedChannelNumber());

    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(*channelFile);
    if (client)
    {
      channelFile->SetProperty(PROPERTY_CLIENT_NAME, client->GetFriendlyName());
      channelFile->SetProperty(PROPERTY_CLIENT_SUPPORTS_SETTINGS,
                               client->GetClientCapabilities().SupportsChannelSettings());
    }

    m_channelItems->Add(channelFile);
  }

  {
    std::vector< std::pair<std::string, int> > labels;
    labels.emplace_back(g_localizeStrings.Get(19210), 0);
    //! @todo Add Labels for EPG scrapers here
    SET_CONTROL_LABELS(SPIN_EPGSOURCE_SELECTION, 0, &labels);
  }

  m_clientsWithSettingsList = CServiceBroker::GetPVRManager().Clients()->GetClientsSupportingChannelSettings(m_bIsRadio);
  if (!m_clientsWithSettingsList.empty())
    m_bAllowNewChannel = true;

  if (m_bAllowNewChannel)
    CONTROL_ENABLE(BUTTON_NEW_CHANNEL);
  else
    CONTROL_DISABLE(BUTTON_NEW_CHANNEL);

  Renumber();
  m_viewControl.SetItems(*m_channelItems);
  if (m_iSelected >= m_channelItems->Size())
    m_iSelected = m_channelItems->Size() - 1;
  m_viewControl.SetSelectedItem(m_iSelected);
  SetData(m_iSelected);
}

void CGUIDialogPVRChannelManager::Clear()
{
  m_viewControl.Clear();
  m_channelItems->Clear();

  ClearChannelOptions();
  EnableChannelOptions(false);

  CONTROL_DISABLE(BUTTON_APPLY);
}

void CGUIDialogPVRChannelManager::ClearChannelOptions()
{
  CONTROL_DESELECT(RADIOBUTTON_ACTIVE);
  SET_CONTROL_LABEL2(EDIT_NAME, "");
  SET_CONTROL_FILENAME(BUTTON_CHANNEL_LOGO, "");
  CONTROL_DESELECT(RADIOBUTTON_USEEPG);

  std::vector<std::pair<std::string, int>> labels = {{g_localizeStrings.Get(19210), 0}};
  SET_CONTROL_LABELS(SPIN_EPGSOURCE_SELECTION, 0, &labels);

  CONTROL_DESELECT(RADIOBUTTON_PARENTAL_LOCK);
}

void CGUIDialogPVRChannelManager::EnableChannelOptions(bool bEnable)
{
  if (bEnable)
  {
    CONTROL_ENABLE(RADIOBUTTON_ACTIVE);
    CONTROL_ENABLE(EDIT_NAME);
    CONTROL_ENABLE(BUTTON_CHANNEL_LOGO);
    CONTROL_ENABLE(IMAGE_CHANNEL_LOGO);
    CONTROL_ENABLE(RADIOBUTTON_USEEPG);
    CONTROL_ENABLE(SPIN_EPGSOURCE_SELECTION);
    CONTROL_ENABLE(RADIOBUTTON_PARENTAL_LOCK);
  }
  else
  {
    CONTROL_DISABLE(RADIOBUTTON_ACTIVE);
    CONTROL_DISABLE(EDIT_NAME);
    CONTROL_DISABLE(BUTTON_CHANNEL_LOGO);
    CONTROL_DISABLE(IMAGE_CHANNEL_LOGO);
    CONTROL_DISABLE(RADIOBUTTON_USEEPG);
    CONTROL_DISABLE(SPIN_EPGSOURCE_SELECTION);
    CONTROL_DISABLE(RADIOBUTTON_PARENTAL_LOCK);
  }
}

void CGUIDialogPVRChannelManager::RenameChannel(const CFileItemPtr& pItem)
{
  std::string strChannelName = pItem->GetProperty(PROPERTY_CHANNEL_NAME).asString();
  if (strChannelName != pItem->GetPVRChannelInfoTag()->ChannelName())
  {
    const std::shared_ptr<CPVRChannel> channel = pItem->GetPVRChannelInfoTag();
    channel->SetChannelName(strChannelName);

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*pItem);
    if (!client || (client->RenameChannel(channel) != PVR_ERROR_NO_ERROR))
      HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // Add-on error;Check the log file for details.
  }
}

bool CGUIDialogPVRChannelManager::UpdateChannelData(const std::shared_ptr<CFileItem>& pItem,
                                                    const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (!pItem || !group)
    return false;

  const auto groupMember = pItem->GetPVRChannelGroupMemberInfoTag();

  if (pItem->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean()) // enabled == not hidden
  {
    if (m_bAllowRenumber)
    {
      groupMember->SetChannelNumber(CPVRChannelNumber(
          static_cast<unsigned int>(pItem->GetProperty(PROPERTY_CHANNEL_NUMBER).asInteger()), 0));
    }

    // make sure the channel is part of the group
    group->AppendToGroup(groupMember);
  }
  else
  {
    // remove the hidden channel from the group
    group->RemoveFromGroup(groupMember);
  }

  const auto channel = groupMember->Channel();

  channel->SetChannelName(pItem->GetProperty(PROPERTY_CHANNEL_NAME).asString(),
                          pItem->GetProperty(PROPERTY_CHANNEL_USER_SET_NAME).asBoolean());
  channel->SetHidden(!pItem->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean(),
                     pItem->GetProperty(PROPERTY_CHANNEL_USER_SET_HIDDEN).asBoolean());
  channel->SetLocked(pItem->GetProperty(PROPERTY_CHANNEL_LOCKED).asBoolean());
  channel->SetIconPath(pItem->GetProperty(PROPERTY_CHANNEL_ICON).asString(),
                       pItem->GetProperty(PROPERTY_CHANNEL_CUSTOM_ICON).asBoolean());

  //! @todo add other scrapers
  if (pItem->GetProperty(PROPERTY_CHANNEL_EPG_SOURCE).asInteger() == 0)
    channel->SetEPGScraper("client");

  channel->SetEPGEnabled(pItem->GetProperty(PROPERTY_CHANNEL_EPG_ENABLED).asBoolean());

  return true;
}

void CGUIDialogPVRChannelManager::PromptAndSaveList()
{
  if (!HasChangedItems())
    return;

  CGUIDialogYesNo* pDialogYesNo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (pDialogYesNo)
  {
    pDialogYesNo->SetHeading(CVariant{20052});
    pDialogYesNo->SetLine(0, CVariant{""});
    pDialogYesNo->SetLine(1, CVariant{19212});
    pDialogYesNo->SetLine(2, CVariant{20103});
    pDialogYesNo->Open();

    if (pDialogYesNo->IsConfirmed())
      SaveList();
    else
      Update();
  }
}

void CGUIDialogPVRChannelManager::SaveList()
{
  if (!HasChangedItems())
    return;

  /* display the progress dialog */
  CGUIDialogProgress* pDlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetHeading(CVariant{190});
  pDlgProgress->SetLine(0, CVariant{""});
  pDlgProgress->SetLine(1, CVariant{328});
  pDlgProgress->SetLine(2, CVariant{""});
  pDlgProgress->Open();
  pDlgProgress->Progress();
  pDlgProgress->SetPercentage(0);

  /* persist all channels */
  std::shared_ptr<CPVRChannelGroup> group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);
  if (!group)
    return;

  for (int iListPtr = 0; iListPtr < m_channelItems->Size(); ++iListPtr)
  {
    CFileItemPtr pItem = m_channelItems->Get(iListPtr);
    if (pItem && pItem->GetProperty(PROPERTY_ITEM_CHANGED).asBoolean())
    {
      if (pItem->GetProperty(PROPERTY_CLIENT_SUPPORTS_SETTINGS).asBoolean())
        RenameChannel(pItem);

      if (pItem->GetProperty(PROPERTY_CHANNEL_USER_SET_NAME).asBoolean() &&
          pItem->GetProperty(PROPERTY_CHANNEL_NAME).asString().empty())
      {
        // if the user changes the name manually to an empty string we reset the
        // userset name flag and change the name to the name from the client
        pItem->SetProperty(PROPERTY_CHANNEL_USER_SET_NAME, false);

        const std::string newName = pItem->GetPVRChannelInfoTag()->ClientChannelName();
        pItem->SetProperty(PROPERTY_CHANNEL_NAME, newName);
        SET_CONTROL_LABEL2(EDIT_NAME, newName);
      }

      // Write the changes made in this dialog back to the actual channel instance
      if (UpdateChannelData(pItem, group))
        pItem->SetProperty(PROPERTY_ITEM_CHANGED, false);
    }

    pDlgProgress->SetPercentage(iListPtr * 100 / m_channelItems->Size());
  }

  group->SortAndRenumber();

  auto channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
  channelGroups->UpdateChannelNumbersFromAllChannelsGroup();
  channelGroups->PersistAll();
  pDlgProgress->Close();

  CONTROL_DISABLE(BUTTON_APPLY);
}

bool CGUIDialogPVRChannelManager::HasChangedItems() const
{
  return std::any_of(m_channelItems->cbegin(), m_channelItems->cend(), [](const auto& item) {
    return item && item->GetProperty(PROPERTY_ITEM_CHANGED).asBoolean();
  });
}

namespace
{

bool IsItemChanged(const std::shared_ptr<CFileItem>& item)
{
  const std::shared_ptr<const CPVRChannelGroupMember> member =
      item->GetPVRChannelGroupMemberInfoTag();
  const std::shared_ptr<const CPVRChannel> channel = member->Channel();

  return item->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean() == channel->IsHidden() ||
         item->GetProperty(PROPERTY_CHANNEL_USER_SET_HIDDEN).asBoolean() !=
             channel->IsUserSetHidden() ||
         item->GetProperty(PROPERTY_CHANNEL_USER_SET_NAME).asBoolean() !=
             channel->IsUserSetName() ||
         item->GetProperty(PROPERTY_CHANNEL_NAME).asString() != channel->ChannelName() ||
         item->GetProperty(PROPERTY_CHANNEL_EPG_ENABLED).asBoolean() != channel->EPGEnabled() ||
         item->GetProperty(PROPERTY_CHANNEL_ICON).asString() != channel->IconPath() ||
         item->GetProperty(PROPERTY_CHANNEL_CUSTOM_ICON).asBoolean() != channel->IsUserSetIcon() ||
         item->GetProperty(PROPERTY_CHANNEL_EPG_SOURCE).asInteger() != 0 ||
         item->GetProperty(PROPERTY_CHANNEL_LOCKED).asBoolean() != channel->IsLocked() ||
         item->GetProperty(PROPERTY_CHANNEL_NUMBER).asString() !=
             member->ChannelNumber().FormattedChannelNumber();
}

} // namespace

void CGUIDialogPVRChannelManager::SetItemChanged(const CFileItemPtr& pItem)
{
  const bool changed = IsItemChanged(pItem);
  pItem->SetProperty(PROPERTY_ITEM_CHANGED, changed);

  if (changed || HasChangedItems())
    CONTROL_ENABLE(BUTTON_APPLY);
  else
    CONTROL_DISABLE(BUTTON_APPLY);
}

void CGUIDialogPVRChannelManager::Renumber()
{
  if (!m_bAllowRenumber)
    return;

  int iNextChannelNumber = 0;
  for (const auto& item : *m_channelItems)
  {
    const std::string number = item->GetProperty(PROPERTY_CHANNEL_ENABLED).asBoolean()
                                   ? std::to_string(++iNextChannelNumber)
                                   : LABEL_CHANNEL_DISABLED;

    if (item->GetProperty(PROPERTY_CHANNEL_NUMBER).asString() != number)
    {
      item->SetProperty(PROPERTY_CHANNEL_NUMBER, number);
      SetItemChanged(item);
    }
  }
}
