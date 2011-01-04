/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "GUIDialogPVRChannelManager.h"
#include "Application.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "MediaManager.h"
#include "Picture.h"
#include "Settings.h"
#include "utils/log.h"
#include "GUISpinControlEx.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"
#include "GUIImage.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogPVRGroupManager.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogOK.h"
#include "GUIDialogKeyboard.h"

#include "utils/PVRChannelGroups.h"
#include "utils/PVRChannelGroup.h"
#include "utils/PVRChannels.h"
#include "utils/PVREpg.h"
#include "PVRManager.h"
#include "TVDatabase.h"

#define BUTTON_OK                 4
#define BUTTON_APPLY              5
#define BUTTON_CANCEL             6
#define RADIOBUTTON_ACTIVE        7
#define EDIT_NAME                 8
#define BUTTON_CHANNEL_LOGO       9
#define IMAGE_CHANNEL_LOGO        10
#define SPIN_GROUP_SELECTION      11
#define RADIOBUTTON_USEEPG        12
#define SPIN_EPGSOURCE_SELECTION  13
#define CONTROL_LIST_CHANNELS     20
#define BUTTON_GROUP_MANAGER      30
#define BUTTON_EDIT_CHANNEL       31
#define BUTTON_DELETE_CHANNEL     32
#define BUTTON_NEW_CHANNEL        33
#define BUTTON_RADIO_TV           34

using namespace std;

CGUIDialogPVRChannelManager::CGUIDialogPVRChannelManager()
    : CGUIDialog(WINDOW_DIALOG_PVR_CHANNEL_MANAGER, "DialogPVRChannelManager.xml")
{
  m_channelItems  = new CFileItemList;
  m_bIsRadio      = false;
}

CGUIDialogPVRChannelManager::~CGUIDialogPVRChannelManager()
{
  delete m_channelItems;
}

bool CGUIDialogPVRChannelManager::OnAction(const CAction& action)
{
  int actionID = action.GetID();
  if (actionID == ACTION_PREVIOUS_MENU || actionID == ACTION_CLOSE_DIALOG)
  {
    Close();
    return true;
  }
  else if (actionID == ACTION_MOVE_DOWN || actionID == ACTION_MOVE_UP ||
           actionID == ACTION_PAGE_DOWN || actionID == ACTION_PAGE_UP)
  {
    if (GetFocusedControlID() == CONTROL_LIST_CHANNELS)
    {
      if (!m_bMovingMode)
      {
        CGUIDialog::OnAction(action);
        int iSelected = m_viewControl.GetSelectedItem();
        if (iSelected != m_iSelected)
        {
          m_iSelected = iSelected;
          SetData(m_iSelected);
        }
        return true;
      }
      else
      {
        CStdString number;
        CGUIDialog::OnAction(action);
        if (actionID == ACTION_MOVE_UP)
        {
          unsigned int newSelect = m_iSelected == 0 ? m_channelItems->Size()-1 : m_iSelected == 0 ? m_iSelected : m_iSelected-1;
          if (m_channelItems->Get(newSelect)->GetProperty("Number") != "-")
          {
            number.Format("%i", m_iSelected+1);
            m_channelItems->Get(newSelect)->SetProperty("Number", number);
            number.Format("%i", newSelect+1);
            m_channelItems->Get(m_iSelected)->SetProperty("Number", number);
          }
          m_channelItems->Swap(newSelect, m_iSelected);
          m_iSelected = newSelect;
        }
        else if (actionID == ACTION_MOVE_DOWN)
        {
          int newSelect = m_iSelected >= m_channelItems->Size()-1 ? 0 : m_iSelected+1;
          if (m_channelItems->Get(newSelect)->GetProperty("Number") != "-")
          {
            number.Format("%i", m_iSelected+1);
            m_channelItems->Get(newSelect)->SetProperty("Number", number);
            number.Format("%i", newSelect+1);
            m_channelItems->Get(m_iSelected)->SetProperty("Number", number);
          }
          m_channelItems->Swap(newSelect, m_iSelected);
          m_iSelected = newSelect;
        }
        else if (actionID == ACTION_PAGE_UP)
        {
          unsigned int lines = m_iSelected-m_viewControl.GetSelectedItem();
          for (unsigned int i = 0; i < lines; i++)
          {
            unsigned int newSelect = m_iSelected == 0 ? m_channelItems->Size()-1 : m_iSelected == 0 ? m_iSelected : m_iSelected-1;
            if (m_channelItems->Get(newSelect)->GetProperty("Number") != "-")
            {
              number.Format("%i", m_iSelected+1);
              m_channelItems->Get(newSelect)->SetProperty("Number", number);
              number.Format("%i", newSelect+1);
              m_channelItems->Get(m_iSelected)->SetProperty("Number", number);
            }
            m_channelItems->Swap(newSelect, m_iSelected);
            m_iSelected = newSelect;
          }
        }
        else if (actionID == ACTION_PAGE_DOWN)
        {
          unsigned int lines = m_viewControl.GetSelectedItem()-m_iSelected;
          for (unsigned int i = 0; i < lines; i++)
          {
            int newSelect = m_iSelected >= m_channelItems->Size()-1 ? 0 : m_iSelected+1;
            if (m_channelItems->Get(newSelect)->GetProperty("Number") != "-")
            {
              number.Format("%i", m_iSelected+1);
              m_channelItems->Get(newSelect)->SetProperty("Number", number);
              number.Format("%i", newSelect+1);
              m_channelItems->Get(m_iSelected)->SetProperty("Number", number);
            }
            m_channelItems->Swap(newSelect, m_iSelected);
            m_iSelected = newSelect;
          }
        }
        m_viewControl.SetItems(*m_channelItems);
        m_viewControl.SetSelectedItem(m_iSelected);
        return true;
      }
    }
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogPVRChannelManager::OnMessage(CGUIMessage& message)
{
  unsigned int iControl = 0;
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_iSelected = 0;
      m_bIsRadio = false;
      m_bMovingMode = false;
      m_bContainsChanges = false;
      SetProperty("IsRadio", "");
      Update();
      SetData(m_iSelected);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      iControl = message.GetSenderId();
      if (iControl == CONTROL_LIST_CHANNELS)
      {
        if (!m_bMovingMode)
        {
          int iAction = message.GetParam1();
          int iItem = m_viewControl.GetSelectedItem();

          /* Check file item is in list range and get his pointer */
          if (iItem < 0 || iItem >= (int)m_channelItems->Size()) return true;

          CFileItemPtr pItem = m_channelItems->Get(iItem);

          /* Process actions */
          if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
          {
            /* Show Contextmenu */
            OnPopupMenu(iItem);
          }
        }
        else
        {
          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("Changed", true);
          pItem->Select(false);
          m_bMovingMode = false;
          m_bContainsChanges = true;
          return true;
        }
      }
      else if (iControl == BUTTON_OK)
      {
        SaveList();
        Close();
        return true;
      }
      else if (iControl == BUTTON_APPLY)
      {
        SaveList();
        return true;
      }
      else if (iControl == BUTTON_CANCEL)
      {
        Close();
        return true;
      }
      else if (iControl == BUTTON_RADIO_TV)
      {
        if (m_bContainsChanges)
        {
          // prompt user for confirmation of channel record
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
          if (!pDialog)
            return true;

          pDialog->SetHeading(20052);
          pDialog->SetLine(0, "");
          pDialog->SetLine(1, 19212);
          pDialog->SetLine(2, 20103);
          pDialog->DoModal();

          if (pDialog->IsConfirmed())
            SaveList();
        }

        m_iSelected = 0;
        m_bMovingMode = false;
        m_bContainsChanges = false;
        m_bIsRadio = !m_bIsRadio;
        SetProperty("IsRadio", m_bIsRadio ? "true" : "");
        Update();
        SetData(m_iSelected);
        return true;
      }
      else if (iControl == RADIOBUTTON_ACTIVE)
      {
        CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl *)GetControl(RADIOBUTTON_ACTIVE);
        if (pRadioButton)
        {
          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("Changed", true);
          pItem->SetProperty("ActiveChannel", pRadioButton->IsSelected());
          m_bContainsChanges = true;
          Renumber();
        }
      }
      else if (iControl == EDIT_NAME)
      {
        CGUIEditControl *pEdit = (CGUIEditControl *)GetControl(EDIT_NAME);
        if (pEdit)
        {
          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("Changed", true);
          pItem->SetProperty("Name", pEdit->GetLabel2());
          m_bContainsChanges = true;
        }
      }
      else if (iControl == BUTTON_CHANNEL_LOGO)
      {
        CFileItemPtr pItem = m_channelItems->Get(m_iSelected);

        if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
          return false;
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        // setup our thumb list
        CFileItemList items;

        // add the current thumb, if available
        if (!pItem->GetProperty("Icon").IsEmpty())
        {
          CFileItemPtr current(new CFileItem("thumb://Current", false));
          current->SetThumbnailImage(pItem->GetPVRChannelInfoTag()->IconPath());
          current->SetLabel(g_localizeStrings.Get(20016));
          items.Add(current);
        }
        else if (pItem->HasThumbnail())
        { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
          CFileItemPtr current(new CFileItem("thumb://Current", false));
          current->SetThumbnailImage(pItem->GetThumbnailImage());
          current->SetLabel(g_localizeStrings.Get(20016));
          items.Add(current);
        }

        // and add a "no thumb" entry as well
        CFileItemPtr nothumb(new CFileItem("thumb://None", false));
        nothumb->SetIconImage(pItem->GetIconImage());
        nothumb->SetLabel(g_localizeStrings.Get(20018));
        items.Add(nothumb);

        CStdString strThumb;
        VECSOURCES shares;
        if (g_guiSettings.GetString("pvrmenu.iconpath") != "")
        {
          CMediaSource share1;
          share1.strPath = g_guiSettings.GetString("pvrmenu.iconpath");
          share1.strName = g_localizeStrings.Get(19018);
          shares.push_back(share1);
        }
        g_mediaManager.GetLocalDrives(shares);
        if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), strThumb))
          return false;

        if (strThumb == "thumb://Current")
          return true;

        if (strThumb == "thumb://None")
          strThumb = "";

        pItem->SetProperty("Icon", strThumb);
        pItem->SetProperty("Changed", true);
        m_bContainsChanges = true;
        return true;
      }
      else if (iControl == SPIN_GROUP_SELECTION)
      {
        CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
        if (pSpin)
        {
          if (!m_bIsRadio && PVRChannelGroupsTV.size() == 0)
            return true;
          else if (m_bIsRadio && PVRChannelGroupsRadio.size() == 0)
            return true;

          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("GroupId", (int)pSpin->GetValue());
          pItem->SetProperty("Changed", true);
          m_bContainsChanges = true;
          return true;
        }
      }
      else if (iControl == RADIOBUTTON_USEEPG)
      {
        CGUIRadioButtonControl *pRadioButton = (CGUIRadioButtonControl *)GetControl(RADIOBUTTON_USEEPG);
        if (pRadioButton)
        {
          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("Changed", true);
          pItem->SetProperty("UseEPG", pRadioButton->IsSelected());
          m_bContainsChanges = true;
        }
      }
      else if (iControl == SPIN_EPGSOURCE_SELECTION)
      {
        /// TODO: Add EPG scraper support
        return true;
        CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(SPIN_EPGSOURCE_SELECTION);
        if (pSpin)
        {
          CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
          pItem->SetProperty("EPGSource", (int)0);
          pItem->SetProperty("Changed", true);
          m_bContainsChanges = true;
          return true;
        }
      }
      else if (iControl == BUTTON_GROUP_MANAGER)
      {
        /* Load group manager dialog */
        CGUIDialogPVRGroupManager* pDlgInfo = (CGUIDialogPVRGroupManager*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
        if (!pDlgInfo)
          return false;

        pDlgInfo->SetRadio(m_bIsRadio);

        /* Open dialog window */
        pDlgInfo->DoModal();

        CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
        if (pSpin)
        {
          pSpin->Clear();
          pSpin->AddLabel(g_localizeStrings.Get(19140), -1);
          if (!m_bIsRadio)
          {
            for (unsigned int i = 0; i < PVRChannelGroupsTV.size(); i++)
              pSpin->AddLabel(PVRChannelGroupsTV[i].GroupName(), PVRChannelGroupsTV[i].GroupID());
          }
          else
          {
            for (unsigned int i = 0; i < PVRChannelGroupsRadio.size(); i++)
              pSpin->AddLabel(PVRChannelGroupsRadio[i].GroupName(), PVRChannelGroupsTV[i].GroupID());
          }
          pSpin->SetValue(m_channelItems->Get(m_iSelected)->GetPropertyInt("GroupId"));
        }

        return true;
      }
      else if (iControl == BUTTON_EDIT_CHANNEL)
      {
        CFileItemPtr pItem = m_channelItems->Get(m_iSelected);
        if (pItem->GetPropertyBOOL("Virtual"))
        {
          CStdString strURL = pItem->GetProperty("StreamURL");
          if (CGUIDialogKeyboard::ShowAndGetInput(strURL, g_localizeStrings.Get(19214), false))
            pItem->SetProperty("StreamURL", strURL);
          return true;
        }

        CGUIDialogOK::ShowAndGetInput(19033,19038,0,0);
        return true;
      }
      else if (iControl == BUTTON_DELETE_CHANNEL)
      {
        CFileItemPtr pItem = m_channelItems->Get(m_iSelected);

        // prompt user for confirmation of channel record
        CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
        if (!pDialog)
          return true;

        pDialog->SetHeading(19211);
        pDialog->SetLine(0, "");
        pDialog->SetLine(1, 750);
        pDialog->SetLine(2, "");
        pDialog->DoModal();

        if (pDialog->IsConfirmed())
        {
          if (pItem->GetPropertyBOOL("Virtual"))
          {
            CTVDatabase *database = g_PVRManager.GetTVDatabase();
            database->Open();
            database->RemoveDBChannel(*pItem->GetPVRChannelInfoTag());
            database->Close();

            m_channelItems->Remove(m_iSelected);
            m_viewControl.SetItems(*m_channelItems);
            Renumber();
            return true;
          }
          CGUIDialogOK::ShowAndGetInput(19033,19038,0,0);
        }
        return true;
      }
      else if (iControl == BUTTON_NEW_CHANNEL)
      {
        std::vector<long> clients;

        CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
        if (!pDlgSelect)
          return false;

        pDlgSelect->SetHeading(19213); // Select Client
        pDlgSelect->Add(g_localizeStrings.Get(19209));
        clients.push_back(999);
        CLIENTMAPITR itr;
        for (itr = g_PVRManager.Clients()->begin() ; itr != g_PVRManager.Clients()->end(); itr++)
        {
          CStdString strClient = (*itr).second->GetBackendName() + ":" + (*itr).second->GetConnectionString();
          clients.push_back((*itr).first);
          pDlgSelect->Add(strClient);
        }

        pDlgSelect->DoModal();

        int selection = pDlgSelect->GetSelectedLabel();
        if (selection >= 0 && selection <= (int) clients.size())
        {
          int clientID = clients[selection];
          if (clientID == 999)
          {
            CStdString strURL = "";
            if (CGUIDialogKeyboard::ShowAndGetInput(strURL, g_localizeStrings.Get(19214), false))
            {
              if (!strURL.IsEmpty())
              {
                CPVRChannel newchannel;
                newchannel.SetChannelName(g_localizeStrings.Get(19204));
                newchannel.SetRadio(m_bIsRadio);
                newchannel.SetEPGEnabled(false);
                newchannel.SetVirtual(true);
                newchannel.SetStreamURL(strURL);
                newchannel.SetClientID(999);

                CTVDatabase *database = g_PVRManager.GetTVDatabase();
                database->Open();
                newchannel.SetChannelID(database->AddDBChannel(newchannel));
                database->Close();
                CFileItemPtr channel(new CFileItem(newchannel));

                channel->SetProperty("ActiveChannel", true);
                channel->SetProperty("Name", g_localizeStrings.Get(19204));
                channel->SetProperty("UseEPG", false);
                channel->SetProperty("GroupId", (int)newchannel.GroupID());
                channel->SetProperty("Icon", newchannel.IconPath());
                channel->SetProperty("EPGSource", (int)0);
                channel->SetProperty("ClientName", g_localizeStrings.Get(19209));

                m_channelItems->AddFront(channel, m_iSelected);
                m_viewControl.SetItems(*m_channelItems);
                Renumber();
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(19033,19038,0,0);
          }
        }
        return true;
      }
    }
    break;
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

  buttons.Add(CONTEXT_BUTTON_MOVE, 116);              /* Move channel up or down */
  if (pItem->GetPropertyBOOL("Virtual"))
    buttons.Add(CONTEXT_BUTTON_EDIT_SOURCE, 1027);    /* Edit virtual channel URL */

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
  if (itemNumber < 0 || itemNumber >= (int)m_channelItems->Size()) return false;

  CFileItemPtr pItem = m_channelItems->Get(itemNumber);

  if (button == CONTEXT_BUTTON_MOVE)
  {
    m_bMovingMode = true;
    pItem->Select(true);
  }
  else if (button == CONTEXT_BUTTON_EDIT_SOURCE)
  {
    CStdString strURL = pItem->GetProperty("StreamURL");
    if (CGUIDialogKeyboard::ShowAndGetInput(strURL, g_localizeStrings.Get(19214), false))
      pItem->SetProperty("StreamURL", strURL);
  }
  return true;
}

void CGUIDialogPVRChannelManager::SetData(int iItem)
{
  CGUISpinControlEx      *pSpin;
  CGUIEditControl        *pEdit;
  CGUIRadioButtonControl *pRadioButton;

  /* Check file item is in list range and get his pointer */
  if (iItem < 0 || iItem >= (int)m_channelItems->Size()) return;

  CFileItemPtr pItem = m_channelItems->Get(iItem);
//  CPVRChannel *infotag = pItem->GetPVRChannelInfoTag();

  pEdit = (CGUIEditControl *)GetControl(EDIT_NAME);
  if (pEdit)
  {
    pEdit->SetLabel2(pItem->GetProperty("Name"));
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TEXT, 19208);
  }

  pRadioButton = (CGUIRadioButtonControl *)GetControl(RADIOBUTTON_ACTIVE);
  if (pRadioButton) pRadioButton->SetSelected(pItem->GetPropertyBOOL("ActiveChannel"));

  pRadioButton = (CGUIRadioButtonControl *)GetControl(RADIOBUTTON_USEEPG);
  if (pRadioButton) pRadioButton->SetSelected(pItem->GetPropertyBOOL("UseEPG"));

  pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
  if (pSpin) pSpin->SetValue(pItem->GetPropertyInt("GroupId"));

  pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
  if (pSpin) pSpin->SetValue(pItem->GetPropertyInt("EPGSource"));

}

void CGUIDialogPVRChannelManager::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS);

  // empty the lists ready for population
  Clear();

  if (!m_bIsRadio)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      CFileItemPtr channel(new CFileItem(PVRChannelsTV[i]));
      channel->SetProperty("ActiveChannel", (bool)!PVRChannelsTV[i].IsHidden());
      channel->SetProperty("Name", PVRChannelsTV[i].ChannelName());
      channel->SetProperty("UseEPG", PVRChannelsTV[i].EPGEnabled());
      channel->SetProperty("GroupId", (int)PVRChannelsTV[i].GroupID());
      channel->SetProperty("Icon", PVRChannelsTV[i].IconPath());
      channel->SetProperty("EPGSource", (int)0);
      CStdString number; number.Format("%i", PVRChannelsTV[i].ChannelNumber());
      channel->SetProperty("Number", number);

      if (PVRChannelsTV[i].IsVirtual())
      {
        channel->SetProperty("Virtual", true);
        channel->SetProperty("StreamURL", PVRChannelsTV[i].StreamURL());
      }

      CStdString clientName;
      if (PVRChannelsTV[i].ClientID() == 999) /* XBMC internal */
        clientName = g_localizeStrings.Get(19209);
      else
        clientName = g_PVRManager.Clients()->find(PVRChannelsTV[i].ClientID())->second->GetBackendName() + ":" + g_PVRManager.Clients()->find(PVRChannelsTV[i].ClientID())->second->GetConnectionString();
      channel->SetProperty("ClientName", clientName);

      m_channelItems->Add(channel);
    }

    CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
    if (pSpin)
    {
      pSpin->Clear();
      pSpin->AddLabel(g_localizeStrings.Get(19140), -1);
      for (unsigned int i = 0; i < PVRChannelGroupsTV.size(); i++)
      {
        pSpin->AddLabel(PVRChannelGroupsTV[i].GroupName(), PVRChannelGroupsTV[i].GroupID());
      }
    }

    pSpin = (CGUISpinControlEx *)GetControl(SPIN_EPGSOURCE_SELECTION);
    if (pSpin)
    {
      pSpin->Clear();
      pSpin->AddLabel(g_localizeStrings.Get(19210), 0);
      /// TODO: Add Labels for EPG scrapers here
    }
  }
  else
  {
    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      CFileItemPtr channel(new CFileItem(PVRChannelsRadio[i]));
      channel->SetProperty("ActiveChannel", (bool)!PVRChannelsRadio[i].IsHidden());
      channel->SetProperty("Name", PVRChannelsRadio[i].ChannelName());
      channel->SetProperty("UseEPG", PVRChannelsRadio[i].EPGEnabled());
      channel->SetProperty("GroupId", (int)PVRChannelsRadio[i].GroupID());
      channel->SetProperty("Icon", PVRChannelsRadio[i].IconPath());
      channel->SetProperty("EPGSource", (int)0);
      CStdString number; number.Format("%i", PVRChannelsRadio[i].ChannelNumber());
      channel->SetProperty("Number", number);

      if (PVRChannelsRadio[i].IsVirtual())
      {
        channel->SetProperty("Virtual", true);
        channel->SetProperty("StreamURL", PVRChannelsRadio[i].StreamURL());
      }

      CStdString clientName;
      if (PVRChannelsRadio[i].ClientID() == 999) /* XBMC internal */
        clientName = g_localizeStrings.Get(19209);
      else
        clientName = g_PVRManager.Clients()->find(PVRChannelsRadio[i].ClientID())->second->GetBackendName() + ":" + g_PVRManager.Clients()->find(PVRChannelsRadio[i].ClientID())->second->GetConnectionString();
      channel->SetProperty("ClientName", clientName);

      m_channelItems->Add(channel);
    }

    CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(SPIN_GROUP_SELECTION);
    if (pSpin)
    {
      pSpin->Clear();
      pSpin->AddLabel(g_localizeStrings.Get(19140), -1);
      for (unsigned int i = 0; i < PVRChannelGroupsRadio.size(); i++)
      {
        pSpin->AddLabel(PVRChannelGroupsRadio[i].GroupName(), PVRChannelGroupsRadio[i].GroupID());
      }
    }

    pSpin = (CGUISpinControlEx *)GetControl(SPIN_EPGSOURCE_SELECTION);
    if (pSpin)
    {
      pSpin->Clear();
      pSpin->AddLabel(g_localizeStrings.Get(19210), 0);
      /// TODO: Add Labels for EPG scrapers here
    }
  }

  Renumber();
  m_viewControl.SetItems(*m_channelItems);
  m_viewControl.SetSelectedItem(m_iSelected);

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRChannelManager::Clear()
{
  m_viewControl.Clear();
  m_channelItems->Clear();
}

void CGUIDialogPVRChannelManager::SaveList()
{
  if (!m_bContainsChanges)
   return;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetHeading(190);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, 328);
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->Progress();
  pDlgProgress->SetPercentage(0);

  int activeChannels = 0;
  for (int i = 0; i < m_channelItems->Size(); i++)
  {
    if (m_channelItems->Get(i)->GetPropertyBOOL("ActiveChannel"))
      activeChannels++;
  }

  for (int i = 0; i < m_channelItems->Size(); i++)
  {
    CFileItemPtr pItem = m_channelItems->Get(i);
    CPVRChannel *tag = pItem->GetPVRChannelInfoTag();
    if (!pItem->GetPropertyBOOL("ActiveChannel"))
      tag->SetChannelNumber(1+activeChannels++);
    else
      tag->SetChannelNumber(atoi(pItem->GetProperty("Number")));
    tag->SetChannelName(pItem->GetProperty("Name"));
    tag->SetHidden(!pItem->GetPropertyBOOL("ActiveChannel"));
    tag->SetIconPath(pItem->GetProperty("Icon"));
    tag->SetGroupID(pItem->GetPropertyInt("GroupId"));

    if (pItem->GetPropertyBOOL("Virtual"))
    {
      tag->SetStreamURL(pItem->GetProperty("StreamURL"));
    }

    CStdString prevEPGSource = tag->EPGScraper();
    int epgSource = pItem->GetPropertyInt("EPGSource");
    if (epgSource == 0)
      tag->SetEPGScraper("client");

    if ((tag->EPGEnabled() && !pItem->GetPropertyBOOL("UseEPG")) || prevEPGSource != tag->EPGScraper())
    {
      ((CPVREpg *)tag->GetEPG())->Clear();
    }
    tag->SetEPGEnabled(pItem->GetPropertyBOOL("UseEPG"));

    database->UpdateDBChannel(*tag);
    pItem->SetProperty("Changed", false);
    pDlgProgress->SetPercentage(i * 100 / m_channelItems->Size());
  }

  database->Close();
  if (!m_bIsRadio)
  {
    PVRChannelsTV.Unload();
    PVRChannelsTV.Load();
  }
  else
  {
    PVRChannelsRadio.Unload();
    PVRChannelsRadio.Load();
  }
  m_bContainsChanges = false;
  pDlgProgress->Close();
}

void CGUIDialogPVRChannelManager::Renumber()
{
  int number = 1;
  CStdString strNumber;
  CFileItemPtr pItem;
  for (int i = 0; i < m_channelItems->Size(); i++)
  {
    pItem = m_channelItems->Get(i);
    if (pItem->GetPropertyBOOL("ActiveChannel"))
    {
      strNumber.Format("%i", number++);
      pItem->SetProperty("Number", strNumber);
    }
    else
      pItem->SetProperty("Number", "-");
  }
}
