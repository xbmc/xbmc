/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "GUIDialogContextMenu.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "GUIDialogFileBrowser.h"
#include "GUIUserMessages.h"
#include "Autorun.h"
#include "GUIPassword.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "GUIDialogMediaSource.h"
#include "profiles/ProfilesManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "storage/MediaManager.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "GUIDialogYesNo.h"
#include "addons/AddonManager.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "TextureCache.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "addons/Scraper.h"

#define BACKGROUND_IMAGE       999
#define GROUP_LIST             996
#define BUTTON_TEMPLATE       1000
#define BUTTON_START          1001
#define BUTTON_END            (BUTTON_START + (int)m_buttons.size() - 1)

void CContextButtons::Add(unsigned int button, const std::string &label)
{
  for (const_iterator i = begin(); i != end(); ++i)
    if (i->first == button)
      return; // already have this button
  push_back(std::pair<unsigned int, std::string>(button, label));
}

void CContextButtons::Add(unsigned int button, int label)
{
  for (const_iterator i = begin(); i != end(); ++i)
    if (i->first == button)
      return; // already have added this button
  push_back(std::pair<unsigned int, std::string>(button, g_localizeStrings.Get(label)));
}

CGUIDialogContextMenu::CGUIDialogContextMenu(void)
  : CGUIDialog(WINDOW_DIALOG_CONTEXT_MENU, "DialogContextMenu.xml")
{
  m_clickedButton = -1;
  m_backgroundImageSize = 0;
  m_loadType = KEEP_IN_MEMORY;
  m_coordX = 0.0f;
  m_coordY = 0.0f;
}

CGUIDialogContextMenu::~CGUIDialogContextMenu(void)
{
}

bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  { // someone has been clicked - deinit...
    if (message.GetSenderId() >= BUTTON_START && message.GetSenderId() <= BUTTON_END)
      m_clickedButton = message.GetSenderId() - BUTTON_START;
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogContextMenu::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_CONTEXT_MENU ||
      action.GetID() == ACTION_SWITCH_PLAYER)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogContextMenu::OnInitWindow()
{
  m_clickedButton = -1;
  // set initial control focus
  m_lastControlID = BUTTON_START;
  CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::SetupButtons()
{
  if (!m_buttons.size())
    return;

  // disable the template button control
  CGUIButtonControl *pButtonTemplate = dynamic_cast<CGUIButtonControl *>(GetFirstFocusableControl(BUTTON_TEMPLATE));
  if (!pButtonTemplate)
    pButtonTemplate = dynamic_cast<CGUIButtonControl *>(GetControl(BUTTON_TEMPLATE));
  if (!pButtonTemplate)
    return;
  pButtonTemplate->SetVisible(false);

  CGUIControlGroupList* pGroupList = dynamic_cast<CGUIControlGroupList *>(GetControl(GROUP_LIST));

  // add our buttons
  for (unsigned int i = 0; i < m_buttons.size(); i++)
  {
    CGUIButtonControl *pButton = new CGUIButtonControl(*pButtonTemplate);
    if (pButton)
    { // set the button's ID and position
      int id = BUTTON_START + i;
      pButton->SetID(id);
      pButton->SetVisible(true);
      pButton->SetLabel(m_buttons[i].second);
      if (pGroupList)
      {
        pButton->SetPosition(pButtonTemplate->GetXPosition(), pButtonTemplate->GetYPosition());
        // try inserting context buttons at position specified by template
        // button, if template button is not in grouplist fallback to adding
        // new buttons at the end of grouplist
        if (!pGroupList->InsertControl(pButton, pButtonTemplate))
          pGroupList->AddControl(pButton);
      }
    }
  }

  // fix up background images placement and size
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
  {
    // first set size of background image
    if (pGroupList)
    {
      if (pGroupList->GetOrientation() == VERTICAL)
      {
        // keep gap between bottom edges of grouplist and background image
        pControl->SetHeight(m_backgroundImageSize - pGroupList->Size() + pGroupList->GetHeight());
      }
      else
      {
        // keep gap between right edges of grouplist and background image
        pControl->SetWidth(m_backgroundImageSize - pGroupList->Size() + pGroupList->GetWidth());
      }
    }
  }

  // update our default control
  if (pGroupList)
    m_defaultControl = pGroupList->GetID();
}

void CGUIDialogContextMenu::SetPosition(float posX, float posY)
{
  if (posY + GetHeight() > m_coordsRes.iHeight)
    posY = m_coordsRes.iHeight - GetHeight();
  if (posY < 0) posY = 0;
  if (posX + GetWidth() > m_coordsRes.iWidth)
    posX = m_coordsRes.iWidth - GetWidth();
  if (posX < 0) posX = 0;
  CGUIDialog::SetPosition(posX, posY);
}

float CGUIDialogContextMenu::GetHeight() const
{
  const CGUIControl *backMain = GetControl(BACKGROUND_IMAGE);
  if (backMain)
    return backMain->GetHeight();
  else
    return CGUIDialog::GetHeight();
}

float CGUIDialogContextMenu::GetWidth() const
{
  const CGUIControl *pControl = GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetWidth();
  else
    return CGUIDialog::GetWidth();
}

bool CGUIDialogContextMenu::SourcesMenu(const std::string &strType, const CFileItemPtr& item, float posX, float posY)
{
  //! @todo This should be callable even if we don't have any valid items
  if (!item)
    return false;

  // grab our context menu
  CContextButtons buttons;
  GetContextButtons(strType, item, buttons);

  int button = ShowAndGetChoice(buttons);
  if (button >= 0)
    return OnContextButton(strType, item, (CONTEXT_BUTTON)button);
  return false;
}

void CGUIDialogContextMenu::GetContextButtons(const std::string &type, const CFileItemPtr& item, CContextButtons &buttons)
{
  // Add buttons to the ContextMenu that should be visible for both sources and autosourced items
  if (item && item->IsRemovable())
  {
    if (item->IsDVD() || item->IsCDDA())
    {
      buttons.Add(CONTEXT_BUTTON_EJECT_DISC, 13391);  // Eject/Load CD/DVD!
    }
    else // Must be HDD
    {
      buttons.Add(CONTEXT_BUTTON_EJECT_DRIVE, 13420);  // Eject Removable HDD!
    }
  }


  // Next, Add buttons to the ContextMenu that should ONLY be visible for sources and not autosourced items
  CMediaSource *share = GetShare(type, item.get());

  if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser)
  {
    if (share)
    {
      // Note. from now on, remove source & disable plugin should mean the same thing
      //! @todo might be smart to also combine editing source & plugin settings into one concept/dialog
      // Note. Temporarily disabled ability to remove plugin sources until installer is operational

      CURL url(share->strPath);
      bool isAddon = ADDON::TranslateContent(url.GetProtocol()) != CONTENT_NONE;
      if (!share->m_ignore && !isAddon)
        buttons.Add(CONTEXT_BUTTON_EDIT_SOURCE, 1027); // Edit Source
      if (type != "video")
        buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // Set as Default
      if (!share->m_ignore && !isAddon)
        buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 522); // Remove Source

      buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);
    }
    if (!GetDefaultShareNameByType(type).empty())
      buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // Clear Default
  }
  if (share && LOCK_MODE_EVERYONE != CProfilesManager::GetInstance().GetMasterProfile().getLockMode())
  {
    if (share->m_iHasLock == 0 && (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser))
      buttons.Add(CONTEXT_BUTTON_ADD_LOCK, 12332);
    else if (share->m_iHasLock == 1)
      buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);
    else if (share->m_iHasLock == 2)
    {
      buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);

      bool maxRetryExceeded = false;
      if (CSettings::GetInstance().GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES) != 0)
        maxRetryExceeded = (share->m_iBadPwdCount >= CSettings::GetInstance().GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES));

      if (maxRetryExceeded)
        buttons.Add(CONTEXT_BUTTON_RESET_LOCK, 12334);
      else
        buttons.Add(CONTEXT_BUTTON_CHANGE_LOCK, 12356);
    }
  }
  if (share && !g_passwordManager.bMasterUser && item->m_iHasLock == 1)
    buttons.Add(CONTEXT_BUTTON_REACTIVATE_LOCK, 12353);
}

bool CGUIDialogContextMenu::OnContextButton(const std::string &type, const CFileItemPtr& item, CONTEXT_BUTTON button)
{
  // buttons that are available on both sources and autosourced items
  if (!item) return false;

  switch (button)
  {
  case CONTEXT_BUTTON_EJECT_DRIVE:
    return g_mediaManager.Eject(item->GetPath());

#ifdef HAS_DVD_DRIVE
  case CONTEXT_BUTTON_EJECT_DISC:
    g_mediaManager.ToggleTray(g_mediaManager.TranslateDevicePath(item->GetPath())[0]);
#endif
    return true;
  default:
    break;
  }

  // the rest of the operations require a valid share
  CMediaSource *share = GetShare(type, item.get());
  if (!share) return false;
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT_SOURCE:
    if (CProfilesManager::GetInstance().IsMasterProfile())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else if (!g_passwordManager.IsProfileLockUnlocked())
      return false;

    return CGUIDialogMediaSource::ShowAndEditMediaSource(type, *share);

  case CONTEXT_BUTTON_REMOVE_SOURCE:
  {
    if (CProfilesManager::GetInstance().IsMasterProfile())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else
    {
      if (!CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsMasterLockUnlocked(false))
        return false;
      if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
    }
    // prompt user if they want to really delete the source
    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{751}, CVariant{750}))
      return false;

    // check default before we delete, as deletion will kill the share object
    std::string defaultSource(GetDefaultShareNameByType(type));
    if (!defaultSource.empty())
    {
      if (share->strName == defaultSource)
        ClearDefault(type);
    }
    CMediaSourceSettings::GetInstance().DeleteSource(type, share->strName, share->strPath);
    return true;
  }
  case CONTEXT_BUTTON_SET_DEFAULT:
    if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

    // make share default
    SetDefault(type, share->strName);
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;
    // remove share default
    ClearDefault(type);
    return true;

  case CONTEXT_BUTTON_SET_THUMB:
    {
      if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
      else if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      // setup our thumb list
      CFileItemList items;

      // add the current thumb, if available
      if (!share->m_strThumbnailImage.empty())
      {
        CFileItemPtr current(new CFileItem("thumb://Current", false));
        current->SetArt("thumb", share->m_strThumbnailImage);
        current->SetLabel(g_localizeStrings.Get(20016));
        items.Add(current);
      }
      else if (item->HasArt("thumb"))
      { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
        CFileItemPtr current(new CFileItem("thumb://Current", false));
        current->SetArt("thumb", item->GetArt("thumb"));
        current->SetLabel(g_localizeStrings.Get(20016));
        items.Add(current);
      }
      // see if there's a local thumb for this item
      std::string folderThumb = item->GetFolderThumb();
      if (XFILE::CFile::Exists(folderThumb))
      {
        CFileItemPtr local(new CFileItem("thumb://Local", false));
        local->SetArt("thumb", folderThumb);
        local->SetLabel(g_localizeStrings.Get(20017));
        items.Add(local);
      }
      // and add a "no thumb" entry as well
      CFileItemPtr nothumb(new CFileItem("thumb://None", false));
      nothumb->SetIconImage(item->GetIconImage());
      nothumb->SetLabel(g_localizeStrings.Get(20018));
      items.Add(nothumb);

      std::string strThumb;
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares);
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), strThumb))
        return false;

      if (strThumb == "thumb://Current")
        return true;

      if (strThumb == "thumb://Local")
        strThumb = folderThumb;

      if (strThumb == "thumb://None")
        strThumb = "";

      if (!share->m_ignore)
      {
        CMediaSourceSettings::GetInstance().UpdateSource(type,share->strName,"thumbnail",strThumb);
        CMediaSourceSettings::GetInstance().Save();
      }
      else if (!strThumb.empty())
      { // this is some sort of an auto-share, so store in the texture database
        CTextureDatabase db;
        if (db.Open())
          db.SetTextureForPath(item->GetPath(), "thumb", strThumb);
      }

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }

  case CONTEXT_BUTTON_ADD_LOCK:
    {
      // prompt user for mastercode when changing lock settings) only for default user
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      std::string strNewPassword = "";
      if (!CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPassword))
        return false;
      // password entry and re-entry succeeded, write out the lock data
      share->m_iHasLock = 2;
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockcode", strNewPassword);
      strNewPassword = StringUtils::Format("%i", share->m_iLockMode);
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockmode", strNewPassword);
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_RESET_LOCK:
    {
      // prompt user for profile lock when changing lock settings
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REMOVE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{12335}, CVariant{750}))
        return false;

      share->m_iHasLock = 0;
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockmode", "0");
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockcode", "0");
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REACTIVATE_LOCK:
    {
      bool maxRetryExceeded = false;
      if (CSettings::GetInstance().GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES) != 0)
        maxRetryExceeded = (share->m_iBadPwdCount >= CSettings::GetInstance().GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES));
      if (!maxRetryExceeded)
      {
        // don't prompt user for mastercode when reactivating a lock
        g_passwordManager.LockSource(type, share->strName, true);
        return true;
      }
      return false;
    }
  case CONTEXT_BUTTON_CHANGE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      std::string strNewPW;
      std::string strNewLockMode;
      if (CGUIDialogLockSettings::ShowAndGetLock(share->m_iLockMode,strNewPW))
        strNewLockMode = StringUtils::Format("%i",share->m_iLockMode);
      else
        return false;
      // password ReSet and re-entry succeeded, write out the lock data
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockcode", strNewPW);
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "lockmode", strNewLockMode);
      CMediaSourceSettings::GetInstance().UpdateSource(type, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      g_windowManager.SendThreadMessage(msg);
      return true;
    }
  default:
    break;
  }
  return false;
}

CMediaSource *CGUIDialogContextMenu::GetShare(const std::string &type, const CFileItem *item)
{
  VECSOURCES *shares = CMediaSourceSettings::GetInstance().GetSources(type);
  if (!shares || !item) return NULL;
  for (unsigned int i = 0; i < shares->size(); i++)
  {
    CMediaSource &testShare = shares->at(i);
    if (URIUtils::IsDVD(testShare.strPath))
    {
      if (!item->IsDVD())
        continue;
    }
    else
    {
      if (!URIUtils::CompareWithoutSlashAtEnd(testShare.strPath, item->GetPath()))
        continue;
    }
    // paths match, what about share name - only match the leftmost
    // characters as the label may contain other info (status for instance)
    if (StringUtils::StartsWithNoCase(item->GetLabel(), testShare.strName))
    {
      return &testShare;
    }
  }
  return NULL;
}

void CGUIDialogContextMenu::OnWindowLoaded()
{
  m_coordX = m_posX;
  m_coordY = m_posY;
  
  const CGUIControlGroupList* pGroupList = dynamic_cast<const CGUIControlGroupList *>(GetControl(GROUP_LIST));
  const CGUIControl *pControl = GetControl(BACKGROUND_IMAGE);
  if (pControl && pGroupList)
  {
    if (pGroupList->GetOrientation() == VERTICAL)
      m_backgroundImageSize = pControl->GetHeight();
    else
      m_backgroundImageSize = pControl->GetWidth();
  }

  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogContextMenu::OnDeinitWindow(int nextWindowID)
{
  //we can't be sure that controls are removed on window unload
  //we have to remove them to be sure that they won't stay for next use of context menu
  for (unsigned int i = 0; i < m_buttons.size(); i++)
  {
    const CGUIControl *control = GetControl(BUTTON_START + i);
    if (control)
      RemoveControl(control);
  }

  m_buttons.clear();
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

std::string CGUIDialogContextMenu::GetDefaultShareNameByType(const std::string &strType)
{
  VECSOURCES *pShares = CMediaSourceSettings::GetInstance().GetSources(strType);
  std::string strDefault = CMediaSourceSettings::GetInstance().GetDefaultSource(strType);

  if (!pShares) return "";

  bool bIsSourceName(false);
  int iIndex = CUtil::GetMatchingSource(strDefault, *pShares, bIsSourceName);
  if (iIndex < 0 || iIndex >= (int)pShares->size())
    return "";

  return pShares->at(iIndex).strName;
}

void CGUIDialogContextMenu::SetDefault(const std::string &strType, const std::string &strDefault)
{
  CMediaSourceSettings::GetInstance().SetDefaultSource(strType, strDefault);
  CMediaSourceSettings::GetInstance().Save();
}

void CGUIDialogContextMenu::ClearDefault(const std::string &strType)
{
  SetDefault(strType, "");
}

void CGUIDialogContextMenu::SwitchMedia(const std::string& strType, const std::string& strPath)
{
  // create menu
  CContextButtons choices;
  if (strType != "music")
    choices.Add(WINDOW_MUSIC_NAV, 2);
  if (strType != "video")
    choices.Add(WINDOW_VIDEO_NAV, 3);
  if (strType != "pictures")
    choices.Add(WINDOW_PICTURES, 1);
  if (strType != "files")
    choices.Add(WINDOW_FILES, 7);

  int window = ShowAndGetChoice(choices);
  if (window >= 0)
  {
    CUtil::DeleteDirectoryCache();
    g_windowManager.ChangeActiveWindow(window, strPath);
  }
}

int CGUIDialogContextMenu::Show(const CContextButtons& choices)
{
  auto dialog = static_cast<CGUIDialogContextMenu*>(g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU));
  if (!dialog)
    return -1;

  dialog->m_buttons = choices;
  dialog->Initialize();
  dialog->SetInitialVisibility();
  dialog->SetupButtons();
  dialog->PositionAtCurrentFocus();
  dialog->Open();
  return dialog->m_clickedButton;
}

int CGUIDialogContextMenu::ShowAndGetChoice(const CContextButtons &choices)
{
  if (choices.empty())
    return -1;

  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    pMenu->m_buttons = choices;
    pMenu->Initialize();
    pMenu->SetInitialVisibility();
    pMenu->SetupButtons();
    pMenu->PositionAtCurrentFocus();
    pMenu->Open();

    int idx = pMenu->m_clickedButton;
    if (idx != -1)
      return choices[idx].first;
  }
  return -1;
}

void CGUIDialogContextMenu::PositionAtCurrentFocus()
{
  CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
  if (window)
  {
    const CGUIControl *focusedControl = window->GetFocusedControl();
    if (focusedControl)
    {
      CPoint pos = focusedControl->GetRenderPosition() + CPoint(focusedControl->GetWidth() * 0.5f, focusedControl->GetHeight() * 0.5f)
                   + window->GetRenderPosition();
      SetPosition(m_coordX + pos.x - GetWidth() * 0.5f, m_coordY + pos.y - GetHeight() * 0.5f);
      return;
    }
  }
  // no control to center at, so just center the window
  CenterWindow();
}
