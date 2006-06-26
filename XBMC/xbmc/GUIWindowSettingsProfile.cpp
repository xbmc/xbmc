#include "stdafx.h"
#include "GUIWindowSettingsProfile.h"
#include "Profile.h"
#include "../guilib/GUIListControl.h"
#include "util.h"
#include "application.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"

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
  int btnEdit = pMenu->AddButton(20067); // edit
  int btnDelete=0;
  if (iItem > 0)
    btnDelete = pMenu->AddButton(117); // Delete

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal();
  int iButton = pMenu->GetButton();
  if (iButton == btnEdit)
    CGUIDialogProfileSettings::ShowForProfile(iItem);
  if (iButton == btnDelete)
  {
    DoDelete(iItem);
    iItem--;
  }

  LoadList();
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
  m_gWindowManager.SendMessage(msg);

}

void CGUIWindowSettingsProfile::DoDelete(int iItem)
{
  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(13201);
    message.Format(str.c_str(), g_settings.m_vecProfiles.at(iItem).getName());
    dlgYesNo->SetHeading(13200);
    dlgYesNo->SetLine(0, message);
    dlgYesNo->SetLine(1, "");
    dlgYesNo->SetLine(2, "");
    dlgYesNo->DoModal();

    if (dlgYesNo->IsConfirmed())
    {
      //delete profile
      g_settings.DeleteProfile(iItem);
      LoadList();
    }
  }
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
            g_settings.LoadProfile(iItem);
            g_settings.Save();
            return true;
          }
          else if (iItem > (int)g_settings.m_vecProfiles.size() - 1)
          {
            CDirectory::Create(g_settings.GetUserDataFolder()+"\\profiles");
            if (CGUIDialogProfileSettings::ShowForProfile(g_settings.m_vecProfiles.size()))
              LoadList();
            return true;
          }
        }
      }
      else if (iControl == CONTROL_LOGINSCREEN)
      {
        g_settings.bUseLoginScreen = !g_settings.bUseLoginScreen;
        g_settings.SaveProfiles("q:\\system\\profiles.xml");
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

  SetLastLoaded();
}

void CGUIWindowSettingsProfile::SetLastLoaded()
{
  //last loaded
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LASTLOADED_PROFILE);
    g_graphicsContext.SendMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LASTLOADED_PROFILE);
    CStdString lbl = g_localizeStrings.Get(13204);
    CStdString lastLoaded;

    if ((g_settings.m_iLastLoadedProfileIndex < 0) || (g_settings.m_iLastLoadedProfileIndex >= (int)g_settings.m_vecProfiles.size()))
    {
      lastLoaded = g_localizeStrings.Get(13205); //unknown
    }
    else
    {
      CProfile& profile = g_settings.m_vecProfiles.at(g_settings.m_iLastLoadedProfileIndex);
      lastLoaded = profile.getName();
    }
    CStdString strItem;
    strItem.Format("%s %s", lbl.c_str(), lastLoaded.c_str());
    msg.SetLabel(strItem);
    g_graphicsContext.SendMessage(msg);
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
