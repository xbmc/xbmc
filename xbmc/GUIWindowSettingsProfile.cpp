
#include "stdafx.h"
#include "GUIWindowSettingsProfile.h"
#include "Profile.h"
#include "Settings.h"
#include "../guilib/GUIListItem.h"
#include "util.h"
#include "application.h"

#define CONTROL_PROFILES 2
#define CONTROL_LASTLOADED_PROFILE 3

CGUIWindowSettingsProfile::CGUIWindowSettingsProfile(void)
:CGUIWindow(0)
{
	m_iLastControl=-1;
}

CGUIWindowSettingsProfile::~CGUIWindowSettingsProfile(void)
{
}

void CGUIWindowSettingsProfile::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindow::OnAction(action);
}

int CGUIWindowSettingsProfile::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_PROFILES,0,0,NULL);
  g_graphicsContext.SendMessage(msg);

	return msg.GetParam1();
}

void CGUIWindowSettingsProfile::OnPopupMenu(int iItem)
{
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_PROFILES);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (!pMenu) return;
	// clean any buttons not needed
	pMenu->ClearButtons();
	// add the needed buttons
	pMenu->AddButton(13206);// Overwrite
	pMenu->AddButton(118);	// Rename
	pMenu->AddButton(117);	// Delete

	// position it correctly
	pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
	pMenu->DoModal(GetID());
	switch (pMenu->GetButton())
	{
	case 1:	// Overwrite
		DoOverwrite(iItem);
		break;
	case 2:	// Rename
		DoRename(iItem);
		break;
	case 3:	// Delete
		DoDelete(iItem);
		break;
	}
}

void CGUIWindowSettingsProfile::DoRename(int iItem)
{
  if (iItem < (int)g_settings.m_vecProfiles.size())  // do nothing when <new profile> is selected
  {
    CStdString strProfileName;
    if (GetKeyboard(strProfileName)) {
      CProfile& profile = g_settings.m_vecProfiles.at(iItem);
      profile.setName(strProfileName);
      g_settings.Save();
      LoadList();
    }
  }
}

void CGUIWindowSettingsProfile::DoDelete(int iItem)
{
  if (iItem < (int)g_settings.m_vecProfiles.size())  // do nothing when <new profile> is selected
  {
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
        dlgYesNo->DoModal(GetID());

        if (dlgYesNo->IsConfirmed())
        {
          //delete profile
          g_settings.DeleteProfile(iItem);
          LoadList();
        }
      }
    }
  }
}

void CGUIWindowSettingsProfile::DoOverwrite(int iItem)
{
  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(13207);
    message.Format(str.c_str(), g_settings.m_vecProfiles.at(iItem).getName());
    dlgYesNo->SetHeading(13200);
    dlgYesNo->SetLine(0, message);
    dlgYesNo->SetLine(1, "");
    dlgYesNo->SetLine(2, "");
    dlgYesNo->DoModal(GetID());

    if (dlgYesNo->IsConfirmed())
    {
      g_settings.SaveSettingsToProfile(iItem);
    }
  }
}

bool CGUIWindowSettingsProfile::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
		{
			m_iLastControl=GetFocusedControl();
			ClearListItems();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

      LoadList();
      
	    if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(m_iLastControl, 0);

			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
      if (iControl==CONTROL_PROFILES)
      {
        int iAction=message.GetParam1();
        if (
          iAction == ACTION_SELECT_ITEM || 
          iAction == ACTION_MOUSE_LEFT_CLICK || 
          iAction == ACTION_CONTEXT_MENU || 
          iAction == ACTION_MOUSE_RIGHT_CLICK
        ) {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_PROFILES,0,0,NULL);
          g_graphicsContext.SendMessage(msg);
          int iItem=msg.GetParam1();
          if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK) 
          {
            //contextmenu
            if (iItem <= (int)g_settings.m_vecProfiles.size() - 1) {
					    OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem > (int)g_settings.m_vecProfiles.size() - 1) {
            //new profile
            CStdString strProfileName;
            if (GetKeyboard(strProfileName)) {
              CProfile profile;
              profile.setName(strProfileName);
              CStdString str = "";
              int i = 0;
              str.Format("T:\\profile%i.xml", i);
              while (CUtil::FileExists(str)) {
                i++;
                str.Format("T:\\profile%i.xml", i);
              }
              str.Format("profile%i.xml", i);
              profile.setFileName(str);
              g_settings.m_vecProfiles.push_back(profile);
              g_settings.SaveSettingsToProfile(iItem);
              g_settings.Save();
              LoadList();
            }
						return true;
          }
          //load profile
					CStdString strPrevSkin = g_guiSettings.GetString("LookAndFeel.Skin");
					int iPrevResolution = g_guiSettings.m_LookAndFeelResolution;
          CSettings::stSettings prevSettings = g_stSettings;
          g_application.StopPlaying();
          g_application.StopServices();
          g_settings.LoadProfile(iItem);
          //reload stuff
	        CStdString strLanguagePath;
	        strLanguagePath.Format("Q:\\language\\%s\\strings.xml", g_guiSettings.GetString("LookAndFeel.Language"));
	        g_localizeStrings.Load(strLanguagePath);
          g_graphicsContext.SetD3DParameters(&g_application.m_d3dpp, g_settings.m_ResInfo);
          g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
          g_graphicsContext.SetOffset(g_guiSettings.GetInt("UIOffset.X"), g_guiSettings.GetInt("UIOffset.Y"));
          if (
            (iPrevResolution != g_guiSettings.m_LookAndFeelResolution) ||
            (CUtil::cmpnocase(strPrevSkin.c_str(), g_guiSettings.GetString("LookAndFeel.Skin").c_str()))
            )
          {
            g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));
            m_gWindowManager.ActivateWindow(GetID());
          }
          g_application.StartServices();
          SetLastLoaded();
          CGUIDialogOK* dlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          if (dlgOK)
          {
            CStdString message;
            CStdString str = g_localizeStrings.Get(13203);
            message.Format(str.c_str(), g_settings.m_vecProfiles.at(iItem).getName());
            dlgOK->SetHeading(13200);
            dlgOK->SetLine(0, message);
            dlgOK->SetLine(1, "");
            dlgOK->SetLine(2, "");
            dlgOK->DoModal(GetID());
          }
          return true;
        }
      }
		}
		break;
	}

	return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsProfile::LoadList()
{
	ClearListItems();

  for (UCHAR i = 0; i < g_settings.m_vecProfiles.size(); i++) {
    CProfile& profile = g_settings.m_vecProfiles.at(i);
    CGUIListItem* item = new CGUIListItem(profile.getName());
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_PROFILES,0,0,(void*)item);
    g_graphicsContext.SendMessage(msg);
		m_vecListItems.push_back(item);
  }
  {
  CGUIListItem* item = new CGUIListItem(g_localizeStrings.Get(13202));
  CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_PROFILES,0,0,(void*)item);
  g_graphicsContext.SendMessage(msg);
	m_vecListItems.push_back(item);
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
	  CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(), CONTROL_LASTLOADED_PROFILE);
    CStdString lbl = g_localizeStrings.Get(13204);
    CStdString lastLoaded;
    
    if ((g_settings.m_iLastLoadedProfileIndex < 0) || (g_settings.m_iLastLoadedProfileIndex >= (int)g_settings.m_vecProfiles.size()))
    {
      lastLoaded = g_localizeStrings.Get(13205);//unknown
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


bool CGUIWindowSettingsProfile::GetKeyboard(CStdString& strInput)
{
	CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
	if (!pKeyboard) return false;
	// setup keyboard
	pKeyboard->CenterWindow();
	pKeyboard->SetText(strInput);
	pKeyboard->DoModal(m_gWindowManager.GetActiveWindow());
	pKeyboard->Close();	

	if (pKeyboard->IsDirty())
	{	// have text - update this.
		strInput = pKeyboard->GetText();
		if (strInput.IsEmpty())
			return false;
		return true;
	}
	return false;
}

void CGUIWindowSettingsProfile::ClearListItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_PROFILES,0,0,NULL);
  g_graphicsContext.SendMessage(msg);

	for (int i=0;i<(int)m_vecListItems.size();++i)
	{
		CGUIListItem* pListItem=m_vecListItems[i];
		delete pListItem;
	}

	m_vecListItems.erase(m_vecListItems.begin(), m_vecListItems.end());
}
