/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContentSettings.h"
#include "GUIWindowVideoFiles.h"
#include "application.h"
#include "GUIPassword.h"
#include "util.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogLockSettings.h"
#include "MediaManager.h"
#include "GUIWindowMusicBase.h"

#define BACKGROUND_IMAGE 999
#define BACKGROUND_BOTTOM 998
#define BUTTON_TEMPLATE 1000
#define SPACE_BETWEEN_BUTTONS 2

CGUIDialogContextMenu::CGUIDialogContextMenu(void):CGUIDialog(WINDOW_DIALOG_CONTEXT_MENU, "DialogContextMenu.xml")
{
  m_iClickedButton = -1;
  m_iNumButtons = 0;
}
CGUIDialogContextMenu::~CGUIDialogContextMenu(void)
{}
bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  { // someone has been clicked - deinit...
    m_iClickedButton = message.GetSenderId() - BUTTON_TEMPLATE;
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogContextMenu::OnInitWindow()
{ // disable the template button control
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE);
  if (pControl)
  {
    pControl->SetVisible(false);
  }
  m_iClickedButton = -1;
  // set initial control focus
  m_lastControlID = BUTTON_TEMPLATE + 1;
  CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::ClearButtons()
{ // destroy our buttons (if we have them from a previous viewing)
  for (int i = 1; i <= m_iNumButtons; i++)
  {
    // get the button to remove...
    CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + i);
    if (pControl)
    {
      // remove the control from our list
      Remove(BUTTON_TEMPLATE + i);
      // kill the button
      pControl->FreeResources();
      delete pControl;
    }
  }
  m_iNumButtons = 0;
}

int CGUIDialogContextMenu::AddButton(int iLabel)
{
  return AddButton(g_localizeStrings.Get(iLabel));
}

int CGUIDialogContextMenu::AddButton(const CStdString &strLabel)
{ // add a button to our control
  CGUIButtonControl *pButtonTemplate = (CGUIButtonControl *)GetControl(BUTTON_TEMPLATE);
  if (!pButtonTemplate) return 0;
  CGUIButtonControl *pButton = new CGUIButtonControl(*pButtonTemplate);
  if (!pButton) return 0;
  // set the button's ID and position
  m_iNumButtons++;
  DWORD dwID = BUTTON_TEMPLATE + m_iNumButtons;
  pButton->SetID(dwID);
  pButton->SetPosition(pButtonTemplate->GetXPosition(), (m_iNumButtons - 1)*(pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));
  pButton->SetVisible(true);
  pButton->SetNavigation(dwID - 1, dwID + 1, dwID, dwID);
  pButton->SetLabel(strLabel);
  Add(pButton);
  // and update the size of our menu
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
  {
    pControl->SetHeight(m_iNumButtons*(pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));
    CGUIControl *pControl2 = (CGUIControl *)GetControl(BACKGROUND_BOTTOM);
    if (pControl2)
      pControl2->SetPosition(pControl2->GetXPosition(), pControl->GetYPosition() + pControl->GetHeight());
  }
  return m_iNumButtons;
}
void CGUIDialogContextMenu::DoModal(int iWindowID /*= WINDOW_INVALID */)
{
  // update the navigation of the first and last buttons
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + 1);
  if (pControl)
    pControl->SetNavigation(BUTTON_TEMPLATE + m_iNumButtons, pControl->GetControlIdDown(), pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + m_iNumButtons);
  if (pControl)
    pControl->SetNavigation(pControl->GetControlIdUp(), BUTTON_TEMPLATE + 1, pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  // update our default control
  if (m_dwDefaultFocusControlID <= BUTTON_TEMPLATE || m_dwDefaultFocusControlID > (DWORD)(BUTTON_TEMPLATE + m_iNumButtons))
    m_dwDefaultFocusControlID = BUTTON_TEMPLATE + 1;
  // check the default control has focus...
  while (m_dwDefaultFocusControlID <= (DWORD)(BUTTON_TEMPLATE + m_iNumButtons) && !(GetControl(m_dwDefaultFocusControlID)->CanFocus()))
    m_dwDefaultFocusControlID++;
  CGUIDialog::DoModal();
}

int CGUIDialogContextMenu::GetButton()
{
  return m_iClickedButton;
}

float CGUIDialogContextMenu::GetHeight()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetHeight();
  else
    return CGUIDialog::GetHeight();
}

float CGUIDialogContextMenu::GetWidth()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetWidth();
  else
    return CGUIDialog::GetWidth();
}
int CGUIDialogContextMenu::GetNumButtons()
{
  return m_iNumButtons;
}
void CGUIDialogContextMenu::EnableButton(int iButton, bool bEnable)
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + iButton);
  if (pControl) pControl->SetEnabled(bEnable);
}

bool CGUIDialogContextMenu::BookmarksMenu(const CStdString &strType, const CFileItem *item, float posX, float posY)
{
  // TODO: This should be callable even if we don't have any valid items
  if (!item)
    return false;

  bool bMaxRetryExceeded = false;
  if (g_guiSettings.GetInt("masterlock.maxretries") != 0)
  bMaxRetryExceeded = !(item->m_iBadPwdCount < g_guiSettings.GetInt("masterlock.maxretries"));

  // Get the share object from our file object
  VECSHARES *shares = g_settings.GetSharesFromType(strType);
  if (!shares) return false;
  CShare *share = NULL;
  for (unsigned int i = 0; i < shares->size(); i++)
  {
    CShare &testShare = shares->at(i);
    if (CUtil::IsDVD(testShare.strPath))
    {
      if (!item->IsDVD())
        continue;
    }
    else
    {
      if (!testShare.strPath.Equals(item->m_strPath))
        continue;
    }
    // paths match, what about share name - only match the leftmost
    // characters as the label may contain other info (status for instance)
    if (item->GetLabel().Left(testShare.strName.size()).Equals(testShare.strName))
    {
      share = &testShare;
      break;
    }
  }

  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    // check where we are
    bool bMyProgramsMenu = ("myprograms" == strType);
    // For DVD Drive Context menu stuff, we can also check where we are, so playin dvd can be dependet to the section!
    bool bIsDVDContextMenu  = false;
    bool bIsDVDMediaPresent = false;

    // load our menu
    pMenu->Initialize();

    // DVD Drive Context menu stuff
    int btn_PlayDisc = 0;
    int btn_Eject = 0;
    int btn_Rip=0;
    if (item->IsDVD() || item->IsCDDA())
    {
      // We need to check if there is a detected is inserted!
      if ( CDetectDVDMedia::IsDiscInDrive() )
      {
        btn_PlayDisc = pMenu->AddButton(341); // Play CD/DVD!

        CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
        if ( pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1) )
          btn_Rip = pMenu->AddButton(600);

        bIsDVDMediaPresent = true;
      }
      btn_Eject = pMenu->AddButton(13391);  // Eject/Load CD/DVD!
      bIsDVDContextMenu = true;
    }

    CStdString strDefault = GetDefaultShareNameByType(strType);

    // add the needed buttons
    int btn_AddShare=0;
    int btn_EditPath=0;
    int btn_Default=0;
    int btn_Delete=0;
    int btn_setThumb=0;
    int btn_RemoveThumb=0;
    int btn_ClearDefault=0;
    int btn_SetContent=0;
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser)
    {
      if (share)
      {
        btn_EditPath = pMenu->AddButton(1027); // Edit Source
        btn_Default = pMenu->AddButton(13335); // Set as Default
        btn_Delete = pMenu->AddButton(522); // Remove Source

        btn_setThumb = pMenu->AddButton(20019);
        if (share->m_strThumbnailImage != "")
          btn_RemoveThumb = pMenu->AddButton(20057);
      }
      if (!strDefault.IsEmpty())
        btn_ClearDefault = pMenu->AddButton(13403); // Clear Default

      if (strType == "video" && share && !CUtil::IsDVD(share->strPath))
        btn_SetContent = pMenu->AddButton(20333);

      btn_AddShare = pMenu->AddButton(1026); // Add Source
    }

    // This if statement should always be the *last* one to add buttons
    // Only show share lock stuff if masterlock isn't disabled
    int btn_LockShare = 0; // Lock Share
    int btn_ResetLock = 0; // Reset Share Lock
    int btn_RemoveLock = 0; // Remove Share Lock
    int btn_ReactivateLock = 0; // Reactivate Share Lock
    int btn_ChangeLock = 0; // Change Share Lock;

    if (share && LOCK_MODE_EVERYONE != g_settings.m_vecProfiles[0].getLockMode())
    {
      if (share->m_iHasLock == 0 && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser))
        btn_LockShare = pMenu->AddButton(12332);
      else if (share->m_iHasLock == 1)
      {
        btn_RemoveLock = pMenu->AddButton(12335);
      }
      else if (share->m_iHasLock == 2)
      {
        btn_RemoveLock = pMenu->AddButton(12335);
        if (!bMaxRetryExceeded)
          btn_ChangeLock = pMenu->AddButton(12356);
        if (bMaxRetryExceeded)
          btn_ResetLock = pMenu->AddButton(12334);
      }
    }
    if (share && !g_passwordManager.bMasterUser && share->m_iHasLock == 1)
      btn_ReactivateLock = pMenu->AddButton(12353);

    int btn_Settings = pMenu->AddButton(5);         // Settings

    // set the correct position
    pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
    pMenu->DoModal();

    int btn = pMenu->GetButton();
    if (btn > 0)
    {
      if (btn == btn_EditPath)
      {
        if (g_settings.m_iLastLoadedProfileIndex == 0)
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;
        }
        else
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;

        return CGUIDialogMediaSource::ShowAndEditMediaSource(strType, *share);
      }
      else if (btn == btn_Delete)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;
// prompt user if they want to really delete the bookmark
        if (CGUIDialogYesNo::ShowAndGetInput(bMyProgramsMenu ? 758 : 751, 0, 750, 0))
        {
          // check default before we delete, as deletion will kill the share object
          if (!strDefault.IsEmpty())
          {
            if (share->strName.Equals(strDefault))
              ClearDefault(strType);
          }

          // delete this share
          g_settings.DeleteBookmark(strType, share->strName, share->strPath);
          return true;
        }
      }
      else if (btn == btn_AddShare)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        return CGUIDialogMediaSource::ShowAndAddMediaSource(strType);
      }
      else if (btn == btn_Default)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        // make share default
        SetDefault(strType, share->strName);
        return true;
      }
      else if (btn == btn_ClearDefault)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;
        // remove share default
        ClearDefault(strType);
        return true;
      }
      else if (btn == btn_setThumb)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        CStdString strThumb;
        VECSHARES shares;
        g_mediaManager.GetLocalDrives(shares);

        if (CGUIDialogFileBrowser::ShowAndGetImage(shares,g_localizeStrings.Get(20056),strThumb))
        {
          g_settings.UpdateBookmark(strType,share->strName,"thumbnail",strThumb);
          g_settings.SaveSources();

          CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_BOOKMARKS);
          m_gWindowManager.SendThreadMessage(msg);
          return true;
        }

        return false;
      }
      else if (btn == btn_RemoveThumb)
      {
        if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources())
        {
          if (!g_passwordManager.IsProfileLockUnlocked())
            return false;
        }
        else if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        g_settings.UpdateBookmark(strType,share->strName,"thumbnail","");
        g_settings.SaveSources();
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_BOOKMARKS);
        m_gWindowManager.SendThreadMessage(msg);
        return true;
      }
      else if (btn == btn_SetContent)
      {
        bool bRunScan=false, bScanRecursively=true, bUseDirNames=false;
        SScraperInfo info;
        if (CGUIDialogContentSettings::ShowForDirectory(share->strPath,info,bRunScan,bScanRecursively,bUseDirNames))
        {
          if (bRunScan)
          {
            CGUIWindowVideoFiles* pWindow = (CGUIWindowVideoFiles*)m_gWindowManager.GetWindow(WINDOW_VIDEO_FILES);
            pWindow->OnScan(share->strPath,info,bUseDirNames?1:0,bScanRecursively?1:0);
          }
        }
      }
      else if (btn == btn_PlayDisc)
      {
        // Ok Play now the Media CD/DVD!
        return CAutorun::PlayDisc();
      }
      else if (btn == btn_Eject)
      {
        if (CIoSupport::GetTrayState() == TRAY_OPEN)
        {
          CIoSupport::CloseTray();
          return true;
        }
        else
        {
          CIoSupport::EjectTray();
          return true;
        }
      }
      else if (btn == btn_Rip)
      {
        CGUIWindowMusicBase::OnRipCD();
        return true;
      }
      else if (btn == btn_LockShare)
      {
        bool bResult = false;
        CStdString strNewPassword = "";
        int iButton=0;
        // prompt user for mastercode when changing lock settings) only for default user
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        CGUIDialogLockSettings* pDialog = (CGUIDialogLockSettings*)m_gWindowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
        if (pDialog)
          bResult = CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPassword);
        else // OLD DEPRECATED
        {

          // popup the context menu
          CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
          if (pMenu)
          {
            // load our menu
            pMenu->Initialize();
            // add the needed buttons
            pMenu->AddButton(12337); // 1: Numeric Password
            pMenu->AddButton(12338); // 2: XBOX gamepad button combo
            pMenu->AddButton(12339); // 3: Full-text Password

            // set the correct position
            pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
            pMenu->DoModal();

            iButton = pMenu->GetButton();
            switch (pMenu->GetButton())
            {
            case 1:  // 1: Numeric Password
              if (!CGUIDialogNumeric::ShowAndVerifyNewPassword(strNewPassword))
                return false;
              break;
            case 2:  // 2: Gamepad Password
              if (!CGUIDialogGamepad::ShowAndVerifyNewPassword(strNewPassword))
                return false;
              break;
            case 3:  // 3: Fulltext Password
              if (!CGUIDialogKeyboard::ShowAndVerifyNewPassword(strNewPassword))
                return false;
              break;
            default:  // Not supported, abort
              return false;
              break;
            }
          }
          share->m_iLockMode = iButton;
          bResult = true;
        }
        if (!bResult)
          return false;
        // password entry and re-entry succeeded, write out the lock data
        share->m_iHasLock = 2;
        g_settings.UpdateBookmark(strType, share->strName, "lockcode", strNewPassword);
        strNewPassword.Format("%i",share->m_iLockMode);
        g_settings.UpdateBookmark(strType, share->strName, "lockmode", strNewPassword);
        g_settings.UpdateBookmark(strType, share->strName, "badpwdcount", "0");
        g_settings.SaveSources();

        CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_BOOKMARKS);
        m_gWindowManager.SendThreadMessage(msg);
        return true;

      }
      else if (btn == btn_ResetLock)
      {
        // prompt user for profile lock when changing lock settings
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        g_settings.UpdateBookmark(strType, share->strName, "badpwdcount", "0");
        g_settings.SaveSources();
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_BOOKMARKS);
        m_gWindowManager.SendThreadMessage(msg);
        return true;
      }
      else if (btn == btn_RemoveLock)
      {
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        if (!CGUIDialogYesNo::ShowAndGetInput(12335, 0, 750, 0))
          return false;

        share->m_iHasLock = 0;
        g_settings.UpdateBookmark(strType, share->strName, "lockmode", "0");
        g_settings.UpdateBookmark(strType, share->strName, "lockcode", "0");
        g_settings.UpdateBookmark(strType, share->strName, "badpwdcount", "0");
        g_settings.SaveSources();
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_BOOKMARKS);
        m_gWindowManager.SendThreadMessage(msg);

        return true;
      }
      else if (btn == btn_ReactivateLock)
      {
        if (!bMaxRetryExceeded)
        {
          // don't prompt user for mastercode when reactivating a lock
          g_passwordManager.LockBookmark(strType, share->strName,true);
          return true;
        }
        else  // this should never happen, but if it does, don't perform any action
          return false;
      }
      else if (btn == btn_ChangeLock)
      {
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        CStdString strNewPW;
	      CStdString strNewLockMode;
        bool bResult=false;
        CGUIDialogLockSettings* pDialog = (CGUIDialogLockSettings*)m_gWindowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
        if (pDialog)
        {
          bResult = CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPW);
          if (bResult)
            strNewLockMode.Format("%i",share->m_iLockMode);
        }
        else // OLD DEPRECATED
        {
          switch (item->m_iLockMode)
          {
          case 1:  // 1: Numeric Password
            if (!CGUIDialogNumeric::ShowAndVerifyNewPassword(strNewPW))
              return false;
            else strNewLockMode = "1";
            break;
          case 2:  // 2: Gamepad Password
            if (!CGUIDialogGamepad::ShowAndVerifyNewPassword(strNewPW))
              return false;
            else strNewLockMode = "2";
            break;
          case 3:  // 3: Fulltext Password
            if (!CGUIDialogKeyboard::ShowAndVerifyNewPassword(strNewPW))
              return false;
            else strNewLockMode = "3";
            break;
          default:  // Not supported, abort
            return false;
            break;
          }
          bResult = true;
        }
        if (!bResult)
          return false;
        // password ReSet and re-entry succeeded, write out the lock data
        g_settings.UpdateBookmark(strType, share->strName, "lockcode", strNewPW);
        g_settings.UpdateBookmark(strType, share->strName, "lockmode", strNewLockMode);
        g_settings.UpdateBookmark(strType, share->strName, "badpwdcount", "0");
        g_settings.SaveSources();
        return true;
      }
      else if (btn == btn_Settings)
      {
        if (strType == "video")
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
        else if (strType == "music")
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
        else if (strType == "myprograms")
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS);
        else if (strType == "pictures")
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
        else if (strType == "files")
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MENU);
        return true;
      }
    }
  }
  return false;
}

void CGUIDialogContextMenu::OnWindowUnload()
{
  ClearButtons();
}

CStdString CGUIDialogContextMenu::GetDefaultShareNameByType(const CStdString &strType)
{
  VECSHARES *pShares = g_settings.GetSharesFromType(strType);
  CStdString strDefault = g_settings.GetDefaultShareFromType(strType);

  if (!pShares) return "";

  bool bIsBookmarkName(false);
  int iIndex = CUtil::GetMatchingShare(strDefault, *pShares, bIsBookmarkName);
  if (iIndex < 0)
    return "";

  return pShares->at(iIndex).strName;
}

void CGUIDialogContextMenu::SetDefault(const CStdString &strType, const CStdString &strDefault)
{
  if (strType == "myprograms")
    strcpy(g_stSettings.m_szDefaultPrograms, strDefault.c_str());
  else if (strType == "files")
    strcpy(g_stSettings.m_szDefaultFiles, strDefault.c_str());
  else if (strType == "music")
    strcpy(g_stSettings.m_szDefaultMusic, strDefault.c_str());
  else if (strType == "video")
    strcpy(g_stSettings.m_szDefaultVideos, strDefault.c_str());
  else if (strType == "pictures")
    strcpy(g_stSettings.m_szDefaultPictures, strDefault.c_str());
  g_settings.SaveSources();
}

void CGUIDialogContextMenu::ClearDefault(const CStdString &strType)
{
  SetDefault(strType, "");
}

void CGUIDialogContextMenu::SwitchMedia(const CStdString& strType, const CStdString& strPath, float posX, float posY)
{
  // what should we display?
  vector <CStdString> vecTypes;
  if (!strType.Equals("music"))
    vecTypes.push_back(g_localizeStrings.Get(2));	// My Music
  if (!strType.Equals("video"))
    vecTypes.push_back(g_localizeStrings.Get(3));	// My Videos
  if (!strType.Equals("pictures"))
    vecTypes.push_back(g_localizeStrings.Get(1));	// My Pictures
  if (!strType.Equals("files"))
    vecTypes.push_back(g_localizeStrings.Get(7));	// My Files

  // something went wrong
  if (vecTypes.size() != 3)
    return;

  // create menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  pMenu->Initialize();

  // add buttons
  int btn_Type[3];
  for (int i=0; i<3; i++)
  {
    btn_Type[i] = pMenu->AddButton(vecTypes[i]);
  }

  // display menu
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  // check selection
  int btn = pMenu->GetButton();
  for (int i=0; i<3; i++)
  {
    if (btn == btn_Type[i])
    {
      // map back to correct window
      int iWindow = WINDOW_INVALID;
      if (vecTypes[i].Equals(g_localizeStrings.Get(2)))
        iWindow = WINDOW_MUSIC_FILES;
      else if (vecTypes[i].Equals(g_localizeStrings.Get(3)))
        iWindow = WINDOW_VIDEO_FILES;
      else if (vecTypes[i].Equals(g_localizeStrings.Get(1)))
        iWindow = WINDOW_PICTURES;
      else if (vecTypes[i].Equals(g_localizeStrings.Get(7)))
        iWindow = WINDOW_FILES;

      //m_gWindowManager.ActivateWindow(iWindow, strPath);
      m_gWindowManager.ChangeActiveWindow(iWindow, strPath);
      return;
    }
  }
  return;
}