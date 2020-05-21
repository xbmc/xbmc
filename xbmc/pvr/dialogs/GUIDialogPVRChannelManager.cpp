/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelManager.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIMessage.h"
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
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/guilib/PVRGUIActions.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

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

using namespace PVR;
using namespace KODI::MESSAGING;

CGUIDialogPVRChannelManager::CGUIDialogPVRChannelManager() :
    CGUIDialog(WINDOW_DIALOG_PVR_CHANNEL_MANAGER, "DialogPVRChannelManager.xml"),
    m_channelItems(new CFileItemList)
{
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
        std::string strNumber;

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
          if (m_channelItems->Get(iNewSelect)->GetProperty("Number").asString() != "-")
          {
            strNumber = StringUtils::Format("%i", m_iSelected+1);
            m_channelItems->Get(iNewSelect)->SetProperty("Number", strNumber);
            strNumber = StringUtils::Format("%i", iNewSelect+1);
            m_channelItems->Get(m_iSelected)->SetProperty("Number", strNumber);
          }
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
  m_bIsRadio = false;
  m_bMovingMode = false;
  m_bContainsChanges = false;
  m_bAllowNewChannel = false;
  SetProperty("IsRadio", "");

  Update();

  if (m_initialSelection)
  {
    // set initial selection
    const std::shared_ptr<CPVRChannel> channel = m_initialSelection->GetPVRChannelInfoTag();
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

void CGUIDialogPVRChannelManager::Open(const std::shared_ptr<CFileItem>& initialSelection)
{
  m_initialSelection = initialSelection;
  CGUIDialog::Open();
}

bool CGUIDialogPVRChannelManager::OnClickListChannels(CGUIMessage& message)
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
      pItem->SetProperty("Changed", true);
      pItem->Select(false);
      m_bMovingMode = false;
      m_bContainsChanges = true;
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonOK(CGUIMessage& message)
{
  SaveList();
  Close();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonApply(CGUIMessage& message)
{
  SaveList();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonCancel(CGUIMessage& message)
{
  Close();
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioTV(CGUIMessage& message)
{
  if (m_bContainsChanges)
  {
    CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return true;

    pDialog->SetHeading(CVariant{20052});
    pDialog->SetLine(0, CVariant{""});
    pDialog->SetLine(1, CVariant{19212});
    pDialog->SetLine(2, CVariant{20103});
    pDialog->Open();

    if (pDialog->IsConfirmed())
      SaveList();
  }

  m_iSelected = 0;
  m_bMovingMode = false;
  m_bAllowNewChannel = false;
  m_bContainsChanges = false;
  m_bIsRadio = !m_bIsRadio;
  SetProperty("IsRadio", m_bIsRadio ? "true" : "");
  Update();
  SetData(m_iSelected);
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioActive(CGUIMessage& message)
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_ACTIVE);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      bool selected(msg.GetParam1() == 1);
      pItem->SetProperty("Changed", true);
      pItem->SetProperty("ActiveChannel", selected);
      m_bContainsChanges = true;
      Renumber();
      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonRadioParentalLocked(CGUIMessage& message)
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_PARENTAL_LOCK);
  if (!OnMessage(msg))
    return false;

  bool selected(msg.GetParam1() == 1);

  // ask for PIN first
  if (CServiceBroker::GetPVRManager().GUIActions()->CheckParentalPIN() != ParentalCheckResult::SUCCESS)
  { // failed - reset to previous
    SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_PARENTAL_LOCK, !selected);
    return false;
  }

  CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
  if (pItem)
  {
    pItem->SetProperty("Changed", true);
    pItem->SetProperty("ParentalLocked", selected);
    m_bContainsChanges = true;
    Renumber();
    return true;
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonEditName(CGUIMessage& message)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), EDIT_NAME);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      pItem->SetProperty("Changed", true);
      pItem->SetProperty("Name", msg.GetLabel());
      m_bContainsChanges = true;

      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickButtonChannelLogo(CGUIMessage& message)
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
  if (!pItem->GetProperty("Icon").asString().empty())
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

  pItem->SetProperty("Icon", strThumb);
  pItem->SetProperty("Changed", true);
  pItem->SetProperty("UserSetIcon", true);
  m_bContainsChanges = true;
  return true;
}

bool CGUIDialogPVRChannelManager::OnClickButtonUseEPG(CGUIMessage& message)
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), RADIOBUTTON_USEEPG);
  if (OnMessage(msg))
  {
    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
    if (pItem)
    {
      bool selected(msg.GetParam1() == 1);
      pItem->SetProperty("Changed", true);
      pItem->SetProperty("UseEPG", selected);
      m_bContainsChanges = true;

      return true;
    }
  }

  return false;
}

bool CGUIDialogPVRChannelManager::OnClickEPGSourceSpin(CGUIMessage& message)
{
  //! @todo Add EPG scraper support
  return true;
//  CGUISpinControlEx* pSpin = (CGUISpinControlEx *)GetControl(SPIN_EPGSOURCE_SELECTION);
//  if (pSpin)
//  {
//    CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
//    if (pItem)
//    {
//      pItem->SetProperty("EPGSource", (int)0);
//      pItem->SetProperty("Changed", true);
//      m_bContainsChanges = true;
//      return true;
//    }
//  }
}

bool CGUIDialogPVRChannelManager::OnClickButtonGroupManager(CGUIMessage& message)
{
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
  int iSelection = 0;
  if (CServiceBroker::GetPVRManager().Clients()->CreatedClientAmount() > 1)
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
      Update();
    else if (ret == PVR_ERROR_NOT_IMPLEMENTED)
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19038}); // "Information", "Not supported by the PVR backend."
    else
      HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // "Add-on error", "Check the log for more information about this message."
  }
  return true;
}

bool CGUIDialogPVRChannelManager::OnMessageClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
  case CONTROL_LIST_CHANNELS:
    return OnClickListChannels(message);
  case BUTTON_OK:
    return OnClickButtonOK(message);
  case BUTTON_APPLY:
    return OnClickButtonApply(message);
  case BUTTON_CANCEL:
    return OnClickButtonCancel(message);
  case BUTTON_RADIO_TV:
    return OnClickButtonRadioTV(message);
  case RADIOBUTTON_ACTIVE:
    return OnClickButtonRadioActive(message);
  case RADIOBUTTON_PARENTAL_LOCK:
    return OnClickButtonRadioParentalLocked(message);
  case EDIT_NAME:
    return OnClickButtonEditName(message);
  case BUTTON_CHANNEL_LOGO:
    return OnClickButtonChannelLogo(message);
  case RADIOBUTTON_USEEPG:
    return OnClickButtonUseEPG(message);
  case SPIN_EPGSOURCE_SELECTION:
    return OnClickEPGSourceSpin(message);
  case BUTTON_GROUP_MANAGER:
    return OnClickButtonGroupManager(message);
  case BUTTON_NEW_CHANNEL:
    return OnClickButtonNewChannel();
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

  buttons.Add(CONTEXT_BUTTON_MOVE, 116); /* Move channel up or down */

  if (pItem->GetProperty("SupportsSettings").asBoolean())
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
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*pItem);
    PVR_ERROR ret = PVR_ERROR_UNKNOWN;
    if (client)
      ret = client->OpenDialogChannelSettings(pItem->GetPVRChannelInfoTag());

    if (ret == PVR_ERROR_NOT_IMPLEMENTED)
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19038}); // "Information", "Not supported by the PVR backend."
    else if (ret != PVR_ERROR_NO_ERROR)
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
        const std::shared_ptr<CPVRChannel> channel = pItem->GetPVRChannelInfoTag();
        PVR_ERROR ret = client->DeleteChannel(channel);
        if (ret == PVR_ERROR_NO_ERROR)
        {
          CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel);
          m_channelItems->Remove(m_iSelected);
          m_viewControl.SetItems(*m_channelItems);
          Renumber();
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
  /* Check file item is in list range and get his pointer */
  if (iItem < 0 || iItem >= m_channelItems->Size()) return;

  CFileItemPtr pItem = m_channelItems->Get(iItem);
  if (!pItem)
    return;

  SET_CONTROL_LABEL2(EDIT_NAME, pItem->GetProperty("Name").asString());
  CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), EDIT_NAME, CGUIEditControl::INPUT_TYPE_TEXT, 19208);
  OnMessage(msg);

  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_ACTIVE, pItem->GetProperty("ActiveChannel").asBoolean());
  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_USEEPG, pItem->GetProperty("UseEPG").asBoolean());
  SET_CONTROL_SELECTED(GetID(), RADIOBUTTON_PARENTAL_LOCK, pItem->GetProperty("ParentalLocked").asBoolean());
}

void CGUIDialogPVRChannelManager::Update()
{
  m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS);

  // empty the lists ready for population
  Clear();

  std::shared_ptr<CPVRChannelGroup> channels = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);

  // No channels available, nothing to do.
  if(!channels)
    return;

  const std::vector<std::shared_ptr<PVRChannelGroupMember>> groupMembers = channels->GetMembers();
  std::shared_ptr<CFileItem> channelFile;
  for (const auto& member : groupMembers)
  {
    channelFile = std::make_shared<CFileItem>(member->channel);
    if (!channelFile || !channelFile->HasPVRChannelInfoTag())
      continue;
    const std::shared_ptr<CPVRChannel> channel(channelFile->GetPVRChannelInfoTag());

    channelFile->SetProperty("ActiveChannel", !channel->IsHidden());
    channelFile->SetProperty("Name", channel->ChannelName());
    channelFile->SetProperty("UseEPG", channel->EPGEnabled());
    channelFile->SetProperty("Icon", channel->IconPath());
    channelFile->SetProperty("EPGSource", 0);
    channelFile->SetProperty("ParentalLocked", channel->IsLocked());
    channelFile->SetProperty("Number", StringUtils::Format("%i", channel->ChannelNumber().GetChannelNumber()));

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*channelFile);
    if (client)
    {
      channelFile->SetProperty("ClientName", client->GetFriendlyName());
      channelFile->SetProperty("SupportsSettings", client->GetClientCapabilities().SupportsChannelSettings());
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
    SET_CONTROL_VISIBLE(BUTTON_NEW_CHANNEL);
  else
    SET_CONTROL_HIDDEN(BUTTON_NEW_CHANNEL);

  Renumber();
  m_viewControl.SetItems(*m_channelItems);
  m_viewControl.SetSelectedItem(m_iSelected);
}

void CGUIDialogPVRChannelManager::Clear()
{
  m_viewControl.Clear();
  m_channelItems->Clear();
}

void CGUIDialogPVRChannelManager::RenameChannel(const CFileItemPtr& pItem)
{
  std::string strChannelName = pItem->GetProperty("Name").asString();
  if (strChannelName != pItem->GetPVRChannelInfoTag()->ChannelName())
  {
    std::shared_ptr<CPVRChannel> channel = pItem->GetPVRChannelInfoTag();
    channel->SetChannelName(strChannelName);

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*pItem);
    if (!client || (client->RenameChannel(channel) != PVR_ERROR_NO_ERROR))
      HELPERS::ShowOKDialogText(CVariant{2103}, CVariant{16029}); // Add-on error;Check the log file for details.
  }
}

bool CGUIDialogPVRChannelManager::PersistChannel(const CFileItemPtr& pItem, const std::shared_ptr<CPVRChannelGroup>& group, unsigned int* iChannelNumber)
{
  if (!pItem || !pItem->HasPVRChannelInfoTag() || !group)
    return false;

  return group->UpdateChannel(pItem->GetPVRChannelInfoTag()->StorageId(),
                              pItem->GetProperty("Name").asString(),
                              pItem->GetProperty("Icon").asString(),
                              static_cast<int>(pItem->GetProperty("EPGSource").asInteger()),
                              ++(*iChannelNumber),
                              !pItem->GetProperty("ActiveChannel").asBoolean(), // hidden
                              pItem->GetProperty("UseEPG").asBoolean(),
                              pItem->GetProperty("ParentalLocked").asBoolean(),
                              pItem->GetProperty("UserSetIcon").asBoolean());
}

void CGUIDialogPVRChannelManager::SaveList()
{
  if (!m_bContainsChanges)
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
  unsigned int iNextChannelNumber(0);
  std::shared_ptr<CPVRChannelGroup> group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);
  if (!group)
    return;

  for (int iListPtr = 0; iListPtr < m_channelItems->Size(); ++iListPtr)
  {
    CFileItemPtr pItem = m_channelItems->Get(iListPtr);

    if (!pItem->HasPVRChannelInfoTag())
      continue;

    if (pItem->GetProperty("SupportsSettings").asBoolean())
      RenameChannel(pItem);

    PersistChannel(pItem, group, &iNextChannelNumber);

    pDlgProgress->SetPercentage(iListPtr * 100 / m_channelItems->Size());
  }

  group->SortAndRenumber();
  group->Persist();
  m_bContainsChanges = false;
  SetItemsUnchanged();
  auto channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
  channelGroups->PropagateChannelNumbersAndPersist();
  pDlgProgress->Close();
}

void CGUIDialogPVRChannelManager::SetItemsUnchanged()
{
  for (int iItemPtr = 0; iItemPtr < m_channelItems->Size(); ++iItemPtr)
  {
    CFileItemPtr pItem = m_channelItems->Get(iItemPtr);
    if (pItem)
      pItem->SetProperty("Changed", false);
  }
}

void CGUIDialogPVRChannelManager::Renumber()
{
  int iNextChannelNumber(0);
  std::string strNumber;
  CFileItemPtr pItem;
  for (int iChannelPtr = 0; iChannelPtr < m_channelItems->Size(); ++iChannelPtr)
  {
    pItem = m_channelItems->Get(iChannelPtr);
    if (pItem->GetProperty("ActiveChannel").asBoolean())
    {
      strNumber = StringUtils::Format("%i", ++iNextChannelNumber);
      pItem->SetProperty("Number", strNumber);
    }
    else
      pItem->SetProperty("Number", "-");
  }
}
