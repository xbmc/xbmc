#include "stdafx.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIWindowFileManager.h"
#include "Profile.h"
#include "../guilib/GUIListControl.h"
#include "util.h"
#include "application.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"
#include "xbox/network.h"
#include "utils/guiinfomanager.h"

#define CONTROL_PROFILES 2
#define CONTROL_LASTLOADED_PROFILE 3
#define CONTROL_LOGINSCREEN 4

CGUIWindowSettingsProfile::CGUIWindowSettingsProfile(void)
    : CGUIWindow(WINDOW_SETTINGS_PROFILES, "SettingsProfile.xml")
{
}

CGUIWindowSettingsProfile::~CGUIWindowSettingsProfile(void)
{}

bool CGUIWindowSettingsProfile::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

int CGUIWindowSettingsProfile::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  return msg.GetParam1();
}

void CGUIWindowSettingsProfile::OnPopupMenu(int iItem)
{
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_PROFILES);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();
  if (iItem == (int)g_settings.m_vecProfiles.size())
    return;

  // add the needed buttons  
  int btnLoad = pMenu->AddButton(20092); // load profile
  int btnDelete=0;
  if (iItem > 0)
    btnDelete = pMenu->AddButton(117); // Delete

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal();
  int iButton = pMenu->GetButton();
  if (iButton == btnLoad)
  {
    unsigned iCtrlID = GetFocusedControl();
    g_application.StopPlaying();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, m_gWindowManager.GetActiveWindow(), iCtrlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
    g_network.NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_network.Deinitialize();
    bool bOldMaster = g_passwordManager.bMasterUser;
    g_passwordManager.bMasterUser = true;
    g_settings.LoadProfile(iItem);
    
    CStdString strDate = g_infoManager.GetDate(true);
    CStdString strTime = g_infoManager.GetTime();
    if (strDate.IsEmpty() || strTime.IsEmpty())
      g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate("-");
    else
      g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate(strDate+" - "+strTime);
    g_settings.SaveProfiles("q:\\system\\profiles.xml"); // to set last loaded

    g_passwordManager.bMasterUser = bOldMaster;
    g_network.Initialize(g_guiSettings.GetInt("network.assignment"),
      g_guiSettings.GetString("network.ipaddress").c_str(),
      g_guiSettings.GetString("network.subnet").c_str(),
      g_guiSettings.GetString("network.gateway").c_str(),
      g_guiSettings.GetString("network.dns").c_str());    
    CGUIMessage msg3(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), iCtrlID, 0);
    OnMessage(msg3);
    CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, m_gWindowManager.GetActiveWindow(), iCtrlID, msg2.GetParam1(), msg2.GetParam2());
    OnMessage(msgSelect);
  }

  if (iButton == btnDelete)
  {
    if (g_settings.DeleteProfile(iItem))
      iItem--;
  }

  LoadList();
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(),CONTROL_PROFILES,iItem);
  OnMessage(msg);
}

bool CGUIWindowSettingsProfile::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearListItems();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PROFILES)
      {
        int iAction = message.GetParam1();
        if (
          iAction == ACTION_SELECT_ITEM ||
          iAction == ACTION_MOUSE_LEFT_CLICK ||
          iAction == ACTION_CONTEXT_MENU ||
          iAction == ACTION_MOUSE_RIGHT_CLICK
        )
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES, 0, 0, NULL);
          g_graphicsContext.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
          {
            //contextmenu
            if (iItem <= (int)g_settings.m_vecProfiles.size() - 1)
            {
              OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem < (int)g_settings.m_vecProfiles.size())
          {
            if (CGUIDialogProfileSettings::ShowForProfile(iItem))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              m_gWindowManager.SendMessage(msg);

              return true;
            }

            return false;
          }
          else if (iItem > (int)g_settings.m_vecProfiles.size() - 1)
          {
            CDirectory::Create(g_settings.GetUserDataFolder()+"\\profiles");
            if (CGUIDialogProfileSettings::ShowForProfile(g_settings.m_vecProfiles.size()))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              m_gWindowManager.SendMessage(msg);
              return true;
            }

            return false;
          }
        }
      }
      else if (iControl == CONTROL_LOGINSCREEN)
      {
        g_settings.bUseLoginScreen = !g_settings.bUseLoginScreen;
        g_settings.SaveProfiles("q:\\system\\profiles.xml");
        return true;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsProfile::LoadList()
{
  ClearListItems();

  for (UCHAR i = 0; i < g_settings.m_vecProfiles.size(); i++)
  {
    CProfile& profile = g_settings.m_vecProfiles.at(i);
    CGUIListItem* item = new CGUIListItem(profile.getName());
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PROFILES, 0, 0, (void*)item);
    g_graphicsContext.SendMessage(msg);
    m_vecListItems.push_back(item);
  }
  {
    CGUIListItem* item = new CGUIListItem(g_localizeStrings.Get(20058));
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PROFILES, 0, 0, (void*)item);
    g_graphicsContext.SendMessage(msg);
    m_vecListItems.push_back(item);
  }

  if (g_settings.bUseLoginScreen)
  {
    CONTROL_SELECT(CONTROL_LOGINSCREEN);
  }
  else
  {
    CONTROL_DESELECT(CONTROL_LOGINSCREEN);
  }
}

void CGUIWindowSettingsProfile::ClearListItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PROFILES, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  for (int i = 0;i < (int)m_vecListItems.size();++i)
  {
    CGUIListItem* pListItem = m_vecListItems[i];
    delete pListItem;
  }

  m_vecListItems.erase(m_vecListItems.begin(), m_vecListItems.end());
}

void CGUIWindowSettingsProfile::OnInitWindow()
{
  LoadList();
  CGUIWindow::OnInitWindow();
}