/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogMediaSource.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogYesNo.h"
#include "PasswordManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/PVRDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/ActionIDs.h"
#include "music/windows/GUIWindowMusicBase.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/windows/GUIWindowVideoBase.h"

#if defined(TARGET_ANDROID)
#include "utils/FileUtils.h"

#include "platform/android/activity/XBMCApp.h"
#endif

#ifdef TARGET_WINDOWS_STORE
#include "platform/win10/filesystem/WinLibraryDirectory.h"
#endif

using namespace XFILE;

#define CONTROL_HEADING          2
#define CONTROL_PATH            10
#define CONTROL_PATH_BROWSE     11
#define CONTROL_NAME            12
#define CONTROL_PATH_ADD        13
#define CONTROL_PATH_REMOVE     14
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_CONTENT         20

CGUIDialogMediaSource::CGUIDialogMediaSource(void)
    : CGUIDialog(WINDOW_DIALOG_MEDIA_SOURCE, "DialogMediaSource.xml")
{
  m_paths = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMediaSource::~CGUIDialogMediaSource()
{
  delete m_paths;
}

bool CGUIDialogMediaSource::OnBack(int actionID)
{
  m_confirmed = false;
  return CGUIDialog::OnBack(actionID);
}

bool CGUIDialogMediaSource::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    int iAction = message.GetParam1();
    if (iControl == CONTROL_PATH && (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK))
      OnPath(GetSelectedItem());
    else if (iControl == CONTROL_PATH_BROWSE)
      OnPathBrowse(GetSelectedItem());
    else if (iControl == CONTROL_PATH_ADD)
      OnPathAdd();
    else if (iControl == CONTROL_PATH_REMOVE)
      OnPathRemove(GetSelectedItem());
    else if (iControl == CONTROL_NAME)
    {
      OnEditChanged(iControl, m_name);
      UpdateButtons();
    }
    else if (iControl == CONTROL_OK)
      OnOK();
    else if (iControl == CONTROL_CANCEL)
      OnCancel();
    else
      break;
    return true;
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    UpdateButtons();
  }
  break;
  case GUI_MSG_SETFOCUS:
    if (message.GetControlId() == CONTROL_PATH_BROWSE ||
      message.GetControlId() == CONTROL_PATH_ADD ||
      message.GetControlId() == CONTROL_PATH_REMOVE)
    {
      HighlightItem(GetSelectedItem());
    }
    else
      HighlightItem(-1);
    break;
  }
  return CGUIDialog::OnMessage(message);
}

// \brief Show CGUIDialogMediaSource dialog and prompt for a new media source.
// \return True if the media source is added, false otherwise.
bool CGUIDialogMediaSource::ShowAndAddMediaSource(const std::string &type)
{
  CGUIDialogMediaSource *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogMediaSource>(WINDOW_DIALOG_MEDIA_SOURCE);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetShare(CMediaSource());
  dialog->SetTypeOfMedia(type);
  dialog->Open();
  bool confirmed(dialog->IsConfirmed());
  if (confirmed)
  {
    // Add this media source
    // Get unique source name
    std::string strName = dialog->GetUniqueMediaSourceName();

    CMediaSource share;
    share.FromNameAndPaths(type, strName, dialog->GetPaths());
    if (dialog->m_paths->Size() > 0)
      share.m_strThumbnailImage = dialog->m_paths->Get(0)->GetArt("thumb");
    CMediaSourceSettings::GetInstance().AddShare(type, share);
    OnMediaSourceChanged(type, "", share);
  }
  dialog->m_paths->Clear();
  return confirmed;
}

bool CGUIDialogMediaSource::ShowAndEditMediaSource(const std::string &type, const std::string&share)
{
  VECSOURCES* pShares = CMediaSourceSettings::GetInstance().GetSources(type);
  if (pShares)
  {
    for (unsigned int i = 0;i<pShares->size();++i)
    {
      if (StringUtils::EqualsNoCase((*pShares)[i].strName, share))
        return ShowAndEditMediaSource(type, (*pShares)[i]);
    }
  }
  return false;
}

bool CGUIDialogMediaSource::ShowAndEditMediaSource(const std::string &type, const CMediaSource &share)
{
  std::string strOldName = share.strName;
  CGUIDialogMediaSource *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogMediaSource>(WINDOW_DIALOG_MEDIA_SOURCE);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetShare(share);
  dialog->SetTypeOfMedia(type, true);
  dialog->Open();
  bool confirmed(dialog->IsConfirmed());
  if (confirmed)
  {
    // Update media source
    // Get unique new source name when changed
    std::string strName(dialog->m_name);
    if (!StringUtils::EqualsNoCase(dialog->m_name, strOldName))
      strName = dialog->GetUniqueMediaSourceName();

    CMediaSource newShare;
    newShare.FromNameAndPaths(type, strName, dialog->GetPaths());
    CMediaSourceSettings::GetInstance().UpdateShare(type, strOldName, newShare);

    OnMediaSourceChanged(type, strOldName, newShare);
  }
  dialog->m_paths->Clear();
  return confirmed;
}

std::string CGUIDialogMediaSource::GetUniqueMediaSourceName()
{
  // Get unique source name for this media type
  unsigned int i, j = 2;
  bool bConfirmed = false;
  VECSOURCES* pShares = CMediaSourceSettings::GetInstance().GetSources(m_type);
  std::string strName = m_name;
  while (!bConfirmed)
  {
    for (i = 0; i<pShares->size(); ++i)
    {
      if (StringUtils::EqualsNoCase((*pShares)[i].strName, strName))
        break;
    }
    if (i < pShares->size())
      // found a match -  try next
      strName = StringUtils::Format("{} ({})", m_name, j++);
    else
      bConfirmed = true;
  }
  return strName;
}

void CGUIDialogMediaSource::OnMediaSourceChanged(const std::string& type, const std::string& oldName, const CMediaSource& share)
{
  // Processing once media source added/edited - library scraping and scanning
  if (!StringUtils::StartsWithNoCase(share.strPath, "rss://") &&
    !StringUtils::StartsWithNoCase(share.strPath, "rsss://") &&
    !StringUtils::StartsWithNoCase(share.strPath, "upnp://"))
  {
    if (type == "video" && !URIUtils::IsLiveTV(share.strPath))
      // Assign content to a path, refresh scraper information optionally start a scan
      CGUIWindowVideoBase::OnAssignContent(share.strPath);
    else if (type == "music")
      CGUIWindowMusicBase::OnAssignContent(oldName, share);
  }
}

void CGUIDialogMediaSource::OnPathBrowse(int item)
{
  if (item < 0 || item >= m_paths->Size()) return;
  // Browse is called.  Open the filebrowser dialog.
  // Ignore current path is best at this stage??
  std::string path = m_paths->Get(item)->GetPath();
  bool allowNetworkShares(m_type != "programs");
  VECSOURCES extraShares;

  if (m_name != CUtil::GetTitleFromPath(path))
    m_bNameChanged = true;
  path.clear();

  if (m_type == "music")
  {
    CMediaSource share1;
#if defined(TARGET_ANDROID)
    // add the default android music directory
    std::string path;
    if (CXBMCApp::GetExternalStorage(path, "music") && !path.empty() && CDirectory::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20240);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif

#if defined(TARGET_WINDOWS_STORE)
    // add the default UWP music directory
    std::string path;
    if (XFILE::CWinLibraryDirectory::GetStoragePath(m_type, path) && !path.empty() && CDirectory::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20245);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif

    // add the music playlist location
    share1.strPath = "special://musicplaylists/";
    share1.strName = g_localizeStrings.Get(20011);
    share1.m_ignore = true;
    extraShares.push_back(share1);

    // add the recordings dir as needed
    if (CPVRDirectory::HasRadioRecordings())
    {
      share1.strPath = PVR::CPVRRecordingsPath::PATH_ACTIVE_RADIO_RECORDINGS;
      share1.strName = g_localizeStrings.Get(19017); // Recordings
      extraShares.push_back(share1);
    }
    if (CPVRDirectory::HasDeletedRadioRecordings())
    {
      share1.strPath = PVR::CPVRRecordingsPath::PATH_DELETED_RADIO_RECORDINGS;
      share1.strName = g_localizeStrings.Get(19184); // Deleted recordings
      extraShares.push_back(share1);
    }

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH) != "")
    {
      share1.strPath = "special://recordings/";
      share1.strName = g_localizeStrings.Get(21883);
      extraShares.push_back(share1);
    }
  }
  else if (m_type == "video")
  {
    CMediaSource share1;
#if defined(TARGET_ANDROID)
    // add the default android video directory
    std::string path;
    if (CXBMCApp::GetExternalStorage(path, "videos") && !path.empty() && CFileUtils::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20241);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif
#if defined(TARGET_WINDOWS_STORE)
    // add the default UWP music directory
    std::string path;
    if (XFILE::CWinLibraryDirectory::GetStoragePath(m_type, path) && !path.empty() && CDirectory::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20246);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif

    // add the video playlist location
    share1.m_ignore = true;
    share1.strPath = "special://videoplaylists/";
    share1.strName = g_localizeStrings.Get(20012);
    extraShares.push_back(share1);

    // add the recordings dir as needed
    if (CPVRDirectory::HasTVRecordings())
    {
      share1.strPath = PVR::CPVRRecordingsPath::PATH_ACTIVE_TV_RECORDINGS;
      share1.strName = g_localizeStrings.Get(19017); // Recordings
      extraShares.push_back(share1);
    }
    if (CPVRDirectory::HasDeletedTVRecordings())
    {
      share1.strPath = PVR::CPVRRecordingsPath::PATH_DELETED_TV_RECORDINGS;
      share1.strName = g_localizeStrings.Get(19184); // Deleted recordings
      extraShares.push_back(share1);
    }
  }
  else if (m_type == "pictures")
  {
    CMediaSource share1;
#if defined(TARGET_ANDROID)
    // add the default android music directory
    std::string path;
    if (CXBMCApp::GetExternalStorage(path, "pictures") && !path.empty() && CFileUtils::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20242);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }

    path.clear();
    if (CXBMCApp::GetExternalStorage(path, "photos") && !path.empty() && CFileUtils::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20243);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif
#if defined(TARGET_WINDOWS_STORE)
    // add the default UWP music directory
    std::string path;
    if (XFILE::CWinLibraryDirectory::GetStoragePath(m_type, path) && !path.empty() && CDirectory::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20247);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
    path.clear();
    if (XFILE::CWinLibraryDirectory::GetStoragePath("photos", path) && !path.empty() && CDirectory::Exists(path))
    {
      share1.strPath = path;
      share1.strName = g_localizeStrings.Get(20248);
      share1.m_ignore = true;
      extraShares.push_back(share1);
    }
#endif

    share1.m_ignore = true;
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH) != "")
    {
      share1.strPath = "special://screenshots/";
      share1.strName = g_localizeStrings.Get(20008);
      extraShares.push_back(share1);
    }
  }
  else if (m_type == "games")
  {
    // nothing to add
  }
  else if (m_type == "programs")
  {
    // nothing to add
  }
  if (CGUIDialogFileBrowser::ShowAndGetSource(path, allowNetworkShares, extraShares.size() == 0 ? NULL : &extraShares))
  {
    if (item < m_paths->Size()) // if the skin does funky things, m_paths may have been cleared
      m_paths->Get(item)->SetPath(path);
    if (!m_bNameChanged || m_name.empty())
    {
      CURL url(path);
      m_name = url.GetWithoutUserDetails();
      URIUtils::RemoveSlashAtEnd(m_name);
      m_name = CUtil::GetTitleFromPath(m_name);
    }
    UpdateButtons();
  }
}

void CGUIDialogMediaSource::OnPath(int item)
{
  if (item < 0 || item >= m_paths->Size()) return;

  std::string path(m_paths->Get(item)->GetPath());
  if (m_name != CUtil::GetTitleFromPath(path))
    m_bNameChanged = true;

  CGUIKeyboardFactory::ShowAndGetInput(path, CVariant{ g_localizeStrings.Get(1021) }, false);
  m_paths->Get(item)->SetPath(path);

  if (!m_bNameChanged || m_name.empty())
  {
    CURL url(m_paths->Get(item)->GetPath());
    m_name = url.GetWithoutUserDetails();
    URIUtils::RemoveSlashAtEnd(m_name);
    m_name = CUtil::GetTitleFromPath(m_name);
  }
  UpdateButtons();
}

void CGUIDialogMediaSource::OnOK()
{
  // Verify the paths by doing a GetDirectory.
  CFileItemList items;

  // Create temp media source to encode path urls as multipath
  // Name of actual source may need to be made unique when saved in sources
  CMediaSource share;
  share.FromNameAndPaths(m_type, m_name, GetPaths());

  if (StringUtils::StartsWithNoCase(share.strPath, "plugin://") ||
    CDirectory::GetDirectory(share.strPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_ALLOW_PROMPT) ||
    CGUIDialogYesNo::ShowAndGetInput(CVariant{ 1001 }, CVariant{ 1025 }))
  {
    m_confirmed = true;
    Close();
  }
}

void CGUIDialogMediaSource::OnCancel()
{
  m_confirmed = false;
  Close();
}

void CGUIDialogMediaSource::UpdateButtons()
{
  if (!m_paths->Size()) // sanity
    return;

  CONTROL_ENABLE_ON_CONDITION(CONTROL_OK, !m_paths->Get(0)->GetPath().empty() && !m_name.empty());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PATH_ADD, !m_paths->Get(0)->GetPath().empty());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PATH_REMOVE, m_paths->Size() > 1);
  // name
  SET_CONTROL_LABEL2(CONTROL_NAME, m_name);
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_NAME, 0, 1022);

  int currentItem = GetSelectedItem();
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_PATH);
  for (int i = 0; i < m_paths->Size(); i++)
  {
    CFileItemPtr item = m_paths->Get(i);
    std::string path;
    CURL url(item->GetPath());
    path = url.GetWithoutUserDetails();
    if (path.empty()) path = "<" + g_localizeStrings.Get(231) + ">"; // <None>
    item->SetLabel(path);
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PATH, 0, 0, m_paths);
  OnMessage(msg);
  SendMessage(GUI_MSG_ITEM_SELECT, CONTROL_PATH, currentItem);

  SET_CONTROL_HIDDEN(CONTROL_CONTENT);
}

void CGUIDialogMediaSource::SetShare(const CMediaSource &share)
{
  m_paths->Clear();
  for (unsigned int i = 0; i < share.vecPaths.size(); i++)
  {
    CFileItemPtr item(new CFileItem(share.vecPaths[i], true));
    m_paths->Add(item);
  }
  if (0 == share.vecPaths.size())
  {
    CFileItemPtr item(new CFileItem("", true));
    m_paths->Add(item);
  }
  m_name = share.strName;
  UpdateButtons();
}

void CGUIDialogMediaSource::SetTypeOfMedia(const std::string &type, bool editNotAdd)
{
  m_type = type;
  std::string heading;
  if (editNotAdd)
  {
    if (type == "video")
      heading = g_localizeStrings.Get(10053);
    else if (type == "music")
      heading = g_localizeStrings.Get(10054);
    else if (type == "pictures")
      heading = g_localizeStrings.Get(10055);
    else if (type == "games")
      heading = g_localizeStrings.Get(35252); // "Edit game source"
    else if (type == "programs")
      heading = g_localizeStrings.Get(10056);
    else
      heading = g_localizeStrings.Get(10057);
  }
  else
  {
    if (type == "video")
      heading = g_localizeStrings.Get(10048);
    else if (type == "music")
      heading = g_localizeStrings.Get(10049);
    else if (type == "pictures")
      heading = g_localizeStrings.Get(13006);
    else if (type == "games")
      heading = g_localizeStrings.Get(35251); // "Add game source"
    else if (type == "programs")
      heading = g_localizeStrings.Get(10051);
    else
      heading = g_localizeStrings.Get(10052);
  }
  SET_CONTROL_LABEL(CONTROL_HEADING, heading);
}

int CGUIDialogMediaSource::GetSelectedItem()
{
  CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PATH);
  OnMessage(message);
  int value = message.GetParam1();
  if (value < 0 || value >= m_paths->Size()) return 0;
  return value;
}

void CGUIDialogMediaSource::HighlightItem(int item)
{
  for (int i = 0; i < m_paths->Size(); i++)
    m_paths->Get(i)->Select(false);
  if (item >= 0 && item < m_paths->Size())
    m_paths->Get(item)->Select(true);
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_PATH, item);
  OnMessage(msg);
}

void CGUIDialogMediaSource::OnPathRemove(int item)
{
  m_paths->Remove(item);
  UpdateButtons();
  if (item >= m_paths->Size())
    HighlightItem(m_paths->Size() - 1);
  else
    HighlightItem(item);
  if (m_paths->Size() <= 1)
  {
    SET_CONTROL_FOCUS(CONTROL_PATH_ADD, 0);
  }
}

void CGUIDialogMediaSource::OnPathAdd()
{
  // add a new item and select it as well
  CFileItemPtr item(new CFileItem("", true));
  m_paths->Add(item);
  UpdateButtons();
  HighlightItem(m_paths->Size() - 1);
}

std::vector<std::string> CGUIDialogMediaSource::GetPaths() const
{
  std::vector<std::string> paths;
  for (int i = 0; i < m_paths->Size(); i++)
  {
    if (!m_paths->Get(i)->GetPath().empty())
    { // strip off the user and password for supported paths (anything that the password manager can auth)
      // and add the user/pass to the password manager - note, we haven't confirmed that it works
      // at this point, but if it doesn't, the user will get prompted anyway in implementation.
      CURL url(m_paths->Get(i)->GetPath());
      if (CPasswordManager::GetInstance().IsURLSupported(url) && !url.GetUserName().empty())
      {
        CPasswordManager::GetInstance().SaveAuthenticatedURL(url);
        url.SetPassword("");
        url.SetUserName("");
        url.SetDomain("");
      }
      paths.push_back(url.Get());
    }
  }
  return paths;
}

void CGUIDialogMediaSource::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // clear paths container
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PATH, 0);
  OnMessage(msg);
}
