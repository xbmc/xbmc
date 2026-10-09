/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogContextMenu.h"

#include "FileItem.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogYesNo.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "Util.h"
#include "addons/Scraper.h"
#include "dialogs/ImageChoices.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "media/MediaLockState.h"
#include "music/MusicFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/ArtTypes.h"
#include "utils/ArtUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using KODI::MEDIA::MediaSection;

#define BACKGROUND_IMAGE       999
#define GROUP_LIST             996
#define BUTTON_TEMPLATE       1000
#define BUTTON_START          1001
#define BUTTON_END            (BUTTON_START + (int)m_buttons.size() - 1)

void CContextButtons::Add(unsigned int button, const std::string &label)
{
  for (const auto& i : *this)
    if (i.first == button)
      return; // already have this button
  push_back(std::pair<unsigned int, std::string>(button, label));
}

void CContextButtons::Add(unsigned int button, int label)
{
  for (const auto& i : *this)
    if (i.first == button)
      return; // already have added this button
  push_back(std::pair<unsigned int, std::string>(
      button, CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(label)));
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

CGUIDialogContextMenu::~CGUIDialogContextMenu(void) = default;

bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  { // someone has been clicked - deinit...
    if (message.GetSenderId() >= BUTTON_START && message.GetSenderId() <= BUTTON_END)
      m_clickedButton = message.GetSenderId() - BUTTON_START;
    Close();
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_PLAYBACK_AVSTARTED)
  {
    // playback was just started from elsewhere - close the dialog
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
  m_lastControlID = m_initiallyFocusedButtonIdx + BUTTON_START;
  CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::SetupButtons()
{
  if (m_buttons.empty())
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
  if (pGroupList)
  {
    for (unsigned int i = 0; i < m_buttons.size(); i++)
    {
      CGUIButtonControl* pButton = new CGUIButtonControl(*pButtonTemplate);
      if (pButton)
      { // set the button's ID and position
        int id = BUTTON_START + i;
        pButton->SetID(id);
        pButton->SetVisible(true);
        pButton->SetLabel(m_buttons[i].second);
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
  CGUIControl *pControl = GetControl(BACKGROUND_IMAGE);
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
  if (m_backgroundImage)
    return m_backgroundImage->GetHeight();
  else
    return CGUIDialog::GetHeight();
}

float CGUIDialogContextMenu::GetWidth() const
{
  if (m_backgroundImage)
    return m_backgroundImage->GetWidth();
  else
    return CGUIDialog::GetWidth();
}

bool CGUIDialogContextMenu::SourcesMenu(MediaSection section,
                                        const CFileItemPtr& item,
                                        float posX,
                                        float posY)
{
  //! @todo This should be callable even if we don't have any valid items
  if (!item)
    return false;

  // grab our context menu
  CContextButtons buttons;
  GetContextButtons(section, item, buttons);

  int button = ShowAndGetChoice(buttons);
  if (button >= 0)
    return OnContextButton(section, item, (CONTEXT_BUTTON)button);
  return false;
}

namespace
{
bool ShowAndGetLock(CMediaSource& share, MediaSection section, MediaLockState state)
{
  KODI::UTILS::CLockInfo& lockInfo{share.GetLockInfo()};

  LockMode newLockMode{lockInfo.GetMode()};
  std::string newPassword;
  if (CGUIDialogLockSettings::ShowAndGetLock(newLockMode, newPassword))
  {
    lockInfo.SetState(state);
    lockInfo.SetMode(newLockMode);
  }
  else
    return false;

  // password entry and re-entry succeeded, write out the lock data
  CMediaSourceSettings& settings{CMediaSourceSettings::GetInstance()};
  settings.UpdateSource(section, share.strName, "lockcode", newPassword);
  settings.UpdateSource(section, share.strName, "lockmode",
                        std::to_string(static_cast<int>(newLockMode)));
  settings.UpdateSource(section, share.strName, "badpwdcount", "0");
  settings.Save();

  return true;
}
} // unnamed namespace

void CGUIDialogContextMenu::GetContextButtons(MediaSection section,
                                              const CFileItemPtr& item,
                                              CContextButtons& buttons)
{
  // Add buttons to the ContextMenu that should be visible for both sources and autosourced items
  // Optical removable drives automatically have the static Eject button added (see CEjectDisk).
  // Here we only add the eject button to HDD drives
  if (item && item->IsRemovable() && !item->IsDVD() && !MUSIC::IsCDDA(*item))
  {
    buttons.Add(CONTEXT_BUTTON_EJECT_DRIVE, 13420); // Remove safely
  }

  // Next, Add buttons to the ContextMenu that should ONLY be visible for sources and not autosourced items
  CMediaSource *share = GetShare(section, item.get());

  if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser)
  {
    if (share)
    {
      // Note. from now on, remove source & disable plugin should mean the same thing
      //! @todo might be smart to also combine editing source & plugin settings into one concept/dialog
      // Note. Temporarily disabled ability to remove plugin sources until installer is operational

      CURL url(share->strPath);
      bool isAddon = ADDON::TranslateContent(url.GetProtocol()) != ADDON::ContentType::NONE;
      if (!share->m_ignore && !isAddon)
        buttons.Add(CONTEXT_BUTTON_EDIT_SOURCE, 1027); // Edit Source
      if (CMediaSourceSettings::HasDefaultSource(section))
        buttons.Add(CONTEXT_BUTTON_SET_DEFAULT, 13335); // Set as Default
      if (!share->m_ignore && !isAddon)
        buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 522); // Remove Source

      buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);
    }
    if (!GetDefaultShareNameByType(section).empty())
      buttons.Add(CONTEXT_BUTTON_CLEAR_DEFAULT, 13403); // Clear Default
  }

  if (share)
  {
    const KODI::UTILS::CLockInfo& lockInfo{share->GetLockInfo()};
    const std::shared_ptr<const CSettingsComponent> settings{
        CServiceBroker::GetSettingsComponent()};
    const std::shared_ptr<const CProfileManager> profileMgr{settings->GetProfileManager()};

    if (profileMgr->GetMasterProfile().getLockMode() != LockMode::EVERYONE)
    {
      if (lockInfo.GetState() == LOCK_STATE_NO_LOCK &&
          (profileMgr->GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser))
        buttons.Add(CONTEXT_BUTTON_ADD_LOCK, 12332);
      else if (lockInfo.GetState() == LOCK_STATE_LOCK_BUT_UNLOCKED)
        buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);
      else if (lockInfo.GetState() == LOCK_STATE_LOCKED)
      {
        buttons.Add(CONTEXT_BUTTON_REMOVE_LOCK, 12335);

        bool maxRetryExceeded = false;
        if (settings->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES) != 0)
          maxRetryExceeded =
              (lockInfo.GetBadPasswordCount() >=
               settings->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES));

        if (maxRetryExceeded)
          buttons.Add(CONTEXT_BUTTON_RESET_LOCK, 12334);
        else
          buttons.Add(CONTEXT_BUTTON_CHANGE_LOCK, 12356);
      }
    }

    if (!g_passwordManager.bMasterUser && lockInfo.GetState() == LOCK_STATE_LOCK_BUT_UNLOCKED)
      buttons.Add(CONTEXT_BUTTON_REACTIVATE_LOCK, 12353);
  }
}

bool CGUIDialogContextMenu::OnContextButton(MediaSection section,
                                            const CFileItemPtr& item,
                                            CONTEXT_BUTTON button)
{
  // buttons that are available on both sources and autosourced items
  if (!item)
    return false;

  switch (button)
  {
    case CONTEXT_BUTTON_EJECT_DRIVE:
      return CServiceBroker::GetMediaManager().Eject(item->GetPath());
    default:
      break;
  }

  // the rest of the operations require a valid share
  CMediaSource *share = GetShare(section, item.get());
  if (!share)
    return false;

  switch (button)
  {
  case CONTEXT_BUTTON_EDIT_SOURCE:
    if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->IsMasterProfile())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else if (!g_passwordManager.IsProfileLockUnlocked())
      return false;

    return CGUIDialogMediaSource::ShowAndEditMediaSource(section, *share);

  case CONTEXT_BUTTON_REMOVE_SOURCE:
  {
    if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->IsMasterProfile())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else
    {
      if (!CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsMasterLockUnlocked(false))
        return false;
      if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
    }
    // prompt user if they want to really delete the source
    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{751}, CVariant{750}))
      return false;

    // check default before we delete, as deletion will kill the share object
    std::string defaultSource(GetDefaultShareNameByType(section));
    if (!defaultSource.empty())
    {
      if (share->strName == defaultSource)
        ClearDefault(section);
    }
    CMediaSourceSettings::GetInstance().DeleteSource(section, share->strName, share->strPath);
    return true;
  }
  case CONTEXT_BUTTON_SET_DEFAULT:
    if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

    // make share default
    SetDefault(section, share->strName);
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;
    // remove share default
    ClearDefault(section);
    return true;

  case CONTEXT_BUTTON_SET_THUMB:
    {
      if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
        return false;
      else if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      // setup our thumb list
      CFileItemList items;

      // add the current thumb, if available
      if (!share->m_strThumbnailImage.empty())
      {
        CFileItemPtr current(new CFileItem(IMAGE_CHOICE::CURRENT, false));
        current->SetArt(ART::TYPE::THUMB, share->m_strThumbnailImage);
        current->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20016));
        items.Add(current);
      }
      else if (item->HasArt(ART::TYPE::THUMB))
      { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
        CFileItemPtr current(new CFileItem(IMAGE_CHOICE::CURRENT, false));
        current->SetArt(ART::TYPE::THUMB, item->GetArt(ART::TYPE::THUMB));
        current->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20016));
        items.Add(current);
      }
      // see if there's a local thumb for this item
      std::string folderThumb = ART::GetFolderThumb(*item);
      if (CFileUtils::Exists(folderThumb))
      {
        CFileItemPtr local(new CFileItem(IMAGE_CHOICE::LOCAL, false));
        local->SetArt(ART::TYPE::THUMB, folderThumb);
        local->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20017));
        items.Add(local);
      }
      // and add a "no thumb" entry as well
      CFileItemPtr nothumb(new CFileItem(IMAGE_CHOICE::NONE, false));
      nothumb->SetArt(ART::TYPE::ICON, item->GetArt(ART::TYPE::ICON));
      nothumb->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20018));
      items.Add(nothumb);

      std::string strThumb;
      std::vector<CMediaSource> shares;
      CServiceBroker::GetMediaManager().GetLocalDrives(shares);
      if (!CGUIDialogFileBrowser::ShowAndGetImage(
              items, shares, CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(1030),
              strThumb))
        return false;

      if (strThumb == IMAGE_CHOICE::CURRENT)
        return true;

      if (strThumb == IMAGE_CHOICE::LOCAL)
        strThumb = folderThumb;

      if (strThumb == IMAGE_CHOICE::NONE)
        strThumb = "";

      if (!share->m_ignore)
      {
        CMediaSourceSettings::GetInstance().UpdateSource(section, share->strName, "thumbnail",
                                                         strThumb);
        CMediaSourceSettings::GetInstance().Save();
      }
      else if (!strThumb.empty())
      { // this is some sort of an auto-share, so store in the texture database
        CTextureDatabase db;
        if (db.Open())
          db.SetTextureForPath(item->GetPath(), ART::TYPE::THUMB, strThumb);
      }

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      return true;
    }

  case CONTEXT_BUTTON_ADD_LOCK:
    {
      // prompt user for mastercode when changing lock settings) only for default user
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      if (!ShowAndGetLock(*share, section, LOCK_STATE_LOCKED))
        return false;

      // lock of a mediasource has been added
      // => refresh favourites due to possible visibility changes
      CServiceBroker::GetFavouritesService().RefreshFavourites();

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_RESET_LOCK:
    {
      // prompt user for profile lock when changing lock settings
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      CMediaSourceSettings::GetInstance().UpdateSource(section, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REMOVE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      // prompt user if they want to really remove the lock
      if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{12335}, CVariant{750}))
        return false;

      KODI::UTILS::CLockInfo& lockInfo{share->GetLockInfo()};
      lockInfo.SetState(LOCK_STATE_NO_LOCK);
      CMediaSourceSettings::GetInstance().UpdateSource(section, share->strName, "lockmode", "0");
      CMediaSourceSettings::GetInstance().UpdateSource(section, share->strName, "lockcode", "0");
      CMediaSourceSettings::GetInstance().UpdateSource(section, share->strName, "badpwdcount", "0");
      CMediaSourceSettings::GetInstance().Save();

      // lock of a mediasource has been removed
      // => refresh favourites due to possible visibility changes
      CServiceBroker::GetFavouritesService().RefreshFavourites();

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      return true;
    }
  case CONTEXT_BUTTON_REACTIVATE_LOCK:
    {
      bool maxRetryExceeded = false;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES) != 0)
        maxRetryExceeded = (share->GetLockInfo().GetBadPasswordCount() >=
                            CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                CSettings::SETTING_MASTERLOCK_MAXRETRIES));
      if (!maxRetryExceeded)
      {
        // don't prompt user for mastercode when reactivating a lock
        g_passwordManager.LockSource(section, share->strName, true);

        // lock of a mediasource has been reactivated
        // => refresh favourites due to possible visibility changes
        CServiceBroker::GetFavouritesService().RefreshFavourites();
        return true;
      }
      return false;
    }
  case CONTEXT_BUTTON_CHANGE_LOCK:
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

      if (!ShowAndGetLock(*share, section, share->GetLockInfo().GetState()))
        return false;

      // lock of a mediasource has been changed
      // => refresh favourites due to possible visibility changes
      CServiceBroker::GetFavouritesService().RefreshFavourites();

      CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      return true;
    }
  default:
    break;
  }
  return false;
}

CMediaSource *CGUIDialogContextMenu::GetShare(MediaSection section, const CFileItem *item)
{
  if (!item)
    return nullptr;
  std::vector<CMediaSource>& shares = CMediaSourceSettings::GetInstance().GetSources(section);
  for (unsigned int i = 0; i < shares.size(); i++)
  {
    CMediaSource &testShare = shares.at(i);
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
  return nullptr;
}

void CGUIDialogContextMenu::OnWindowLoaded()
{
  m_coordX = m_posX;
  m_coordY = m_posY;

  const CGUIControlGroupList* pGroupList = dynamic_cast<const CGUIControlGroupList *>(GetControl(GROUP_LIST));
  m_backgroundImage = GetControl(BACKGROUND_IMAGE);
  if (m_backgroundImage && pGroupList)
  {
    if (pGroupList->GetOrientation() == VERTICAL)
      m_backgroundImageSize = m_backgroundImage->GetHeight();
    else
      m_backgroundImageSize = m_backgroundImage->GetWidth();
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
    {
      RemoveControl(control);
      delete control;
    }
  }

  m_buttons.clear();
  m_initiallyFocusedButtonIdx = 0;
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

std::string CGUIDialogContextMenu::GetDefaultShareNameByType(MediaSection section)
{
  std::vector<CMediaSource>& shares = CMediaSourceSettings::GetInstance().GetSources(section);
  std::string strDefault = CMediaSourceSettings::GetInstance().GetDefaultSource(section);

  bool bIsSourceName(false);
  int iIndex = CUtil::GetMatchingSource(strDefault, shares, bIsSourceName);
  if (iIndex < 0 || iIndex >= (int)shares.size())
    return "";

  return shares.at(iIndex).strName;
}

void CGUIDialogContextMenu::SetDefault(MediaSection section, const std::string &strDefault)
{
  CMediaSourceSettings::GetInstance().SetDefaultSource(section, strDefault);
  CMediaSourceSettings::GetInstance().Save();
}

void CGUIDialogContextMenu::ClearDefault(MediaSection section)
{
  SetDefault(section, "");
}

void CGUIDialogContextMenu::SwitchMedia(MediaSection section, const std::string& strPath)
{
  // create menu
  CContextButtons choices;
  if (section != MediaSection::MUSIC)
    choices.Add(WINDOW_MUSIC_NAV, 2);
  if (section != MediaSection::VIDEO)
    choices.Add(WINDOW_VIDEO_NAV, 3);
  if (section != MediaSection::PICTURES)
    choices.Add(WINDOW_PICTURES, 1);
  if (section != MediaSection::FILES)
    choices.Add(WINDOW_FILES, 7);

  int window = ShowAndGetChoice(choices);
  if (window >= 0)
  {
    CUtil::DeleteDirectoryCache();
    CServiceBroker::GetGUI()->GetWindowManager().ChangeActiveWindow(window, strPath);
  }
}

int CGUIDialogContextMenu::Show(const CContextButtons& choices, int focusedButtonIdx /* = 0 */)
{
  auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogContextMenu>(WINDOW_DIALOG_CONTEXT_MENU);
  if (!dialog)
    return -1;

  dialog->m_buttons = choices;
  dialog->Initialize();
  dialog->SetInitialVisibility();
  dialog->SetupButtons();
  dialog->PositionAtCurrentFocus();
  dialog->m_initiallyFocusedButtonIdx = focusedButtonIdx;
  dialog->Open();
  return dialog->m_clickedButton;
}

int CGUIDialogContextMenu::ShowAndGetChoice(const CContextButtons &choices)
{
  if (choices.empty())
    return -1;

  CGUIDialogContextMenu *pMenu = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogContextMenu>(WINDOW_DIALOG_CONTEXT_MENU);
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
  CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
  if (window)
  {
    const CGUIControl *focusedControl = window->GetFocusedControl();
    if (focusedControl)
    {
      CPoint pos = focusedControl->GetRenderPosition() + CPoint(focusedControl->GetWidth() * 0.5f, focusedControl->GetHeight() * 0.5f);
      SetPosition(m_coordX + pos.x - GetWidth() * 0.5f, m_coordY + pos.y - GetHeight() * 0.5f);
      return;
    }
  }
  // no control to center at, so just center the window
  CenterWindow();
}
