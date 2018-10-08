/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "ProfileManager.h"
#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIPassword.h"
#include "PasswordManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/Skin.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "events/EventLog.h"
#include "events/EventLogManager.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/InputManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#include "threads/SingleLock.h"
#include "utils/FileUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#include "addons/AddonManager.h" //! @todo Remove me
#include "addons/Service.h" //! @todo Remove me
#include "favourites/FavouritesService.h" //! @todo Remove me
#include "guilib/StereoscopicsManager.h" //! @todo Remove me
#include "interfaces/json-rpc/JSONRPC.h" //! @todo Remove me
#include "network/Network.h" //! @todo Remove me
#include "network/NetworkServices.h" //! @todo Remove me
#include "pvr/PVRManager.h" //! @todo Remove me
#include "video/VideoLibraryQueue.h"//! @todo Remove me
#include "weather/WeatherManager.h" //! @todo Remove me
#include "Application.h" //! @todo Remove me
#include "ContextMenuManager.h" //! @todo Remove me
#include "PlayListPlayer.h" //! @todo Remove me

//! @todo
//! eventually the profile should dictate where special://masterprofile/ is
//! but for now it makes sense to leave all the profile settings in a user
//! writeable location like special://masterprofile/
#define PROFILES_FILE     "special://masterprofile/profiles.xml"

#define XML_PROFILES      "profiles"
#define XML_AUTO_LOGIN    "autologin"
#define XML_LAST_LOADED   "lastloaded"
#define XML_LOGIN_SCREEN  "useloginscreen"
#define XML_NEXTID        "nextIdProfile"
#define XML_PROFILE       "profile"

using namespace XFILE;

static CProfile EmptyProfile;

CProfileManager::CProfileManager() :
    m_usingLoginScreen(false),
    m_profileLoadedForLogin(false),
    m_autoLoginProfile(-1),
    m_lastUsedProfile(0),
    m_currentProfile(0),
    m_nextProfileId(0),
    m_eventLogs(new CEventLogManager)
{
}

CProfileManager::~CProfileManager()
{
}

void CProfileManager::Initialize(const std::shared_ptr<CSettings>& settings)
{
  m_settings = settings;

  if (m_settings->IsLoaded())
    OnSettingsLoaded();

  m_settings->GetSettingsManager()->RegisterSettingsHandler(this);

  std::set<std::string> settingSet = {
    CSettings::SETTING_EVENTLOG_SHOW
  };

  m_settings->GetSettingsManager()->RegisterCallback(this, settingSet);
}

void CProfileManager::Uninitialize()
{
  m_settings->GetSettingsManager()->UnregisterCallback(this);
  m_settings->GetSettingsManager()->UnregisterSettingsHandler(this);
}

void CProfileManager::OnSettingsLoaded()
{
  // check them all
  std::string strDir = m_settings->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  if (strDir == "set default" || strDir.empty())
  {
    strDir = "special://profile/playlists/";
    m_settings->SetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH, strDir.c_str());
  }

  CDirectory::Create(strDir);
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"music"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"video"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"mixed"));
}

void CProfileManager::OnSettingsSaved() const
{
  // save mastercode
  Save();
}

void CProfileManager::OnSettingsCleared()
{
  Clear();
}

bool CProfileManager::Load()
{
  bool ret = true;
  const std::string file = PROFILES_FILE;

  CSingleLock lock(m_critical);

  // clear out our profiles
  m_profiles.clear();

  if (CFile::Exists(file))
  {
    CXBMCTinyXML profilesDoc;
    if (profilesDoc.LoadFile(file))
    {
      const TiXmlElement *rootElement = profilesDoc.RootElement();
      if (rootElement && StringUtils::EqualsNoCase(rootElement->Value(), XML_PROFILES))
      {
        XMLUtils::GetUInt(rootElement, XML_LAST_LOADED, m_lastUsedProfile);
        XMLUtils::GetBoolean(rootElement, XML_LOGIN_SCREEN, m_usingLoginScreen);
        XMLUtils::GetInt(rootElement, XML_AUTO_LOGIN, m_autoLoginProfile);
        XMLUtils::GetInt(rootElement, XML_NEXTID, m_nextProfileId);

        std::string defaultDir("special://home/userdata");
        if (!CDirectory::Exists(defaultDir))
          defaultDir = "special://xbmc/userdata";

        const TiXmlElement* pProfile = rootElement->FirstChildElement(XML_PROFILE);
        while (pProfile)
        {
          CProfile profile(defaultDir);
          profile.Load(pProfile, GetNextProfileId());
          AddProfile(profile);

          pProfile = pProfile->NextSiblingElement(XML_PROFILE);
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CProfileManager: error loading %s, no <profiles> node", file.c_str());
        ret = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CProfileManager: error loading %s, Line %d\n%s", file.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
      ret = false;
    }
  }

  if (m_profiles.empty())
  { // add the master user
    CProfile profile("special://masterprofile/", "Master user", 0);
    AddProfile(profile);
  }

  // check the validity of the previous profile index
  if (m_lastUsedProfile >= m_profiles.size())
    m_lastUsedProfile = 0;

  SetCurrentProfileId(m_lastUsedProfile);

  // check the validity of the auto login profile index
  if (m_autoLoginProfile < -1 || m_autoLoginProfile >= (int)m_profiles.size())
    m_autoLoginProfile = -1;
  else if (m_autoLoginProfile >= 0)
    SetCurrentProfileId(m_autoLoginProfile);

  // the login screen runs as the master profile, so if we're using this, we need to ensure
  // we switch to the master profile
  if (m_usingLoginScreen)
    SetCurrentProfileId(0);

  return ret;
}

bool CProfileManager::Save() const
{
  const std::string file = PROFILES_FILE;

  CSingleLock lock(m_critical);

  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement(XML_PROFILES);
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (pRoot == nullptr)
    return false;

  XMLUtils::SetInt(pRoot, XML_LAST_LOADED, m_currentProfile);
  XMLUtils::SetBoolean(pRoot, XML_LOGIN_SCREEN, m_usingLoginScreen);
  XMLUtils::SetInt(pRoot, XML_AUTO_LOGIN, m_autoLoginProfile);
  XMLUtils::SetInt(pRoot, XML_NEXTID, m_nextProfileId);

  for (const auto& profile : m_profiles)
    profile.Save(pRoot);

  // save the file
  return xmlDoc.SaveFile(file);
}

void CProfileManager::Clear()
{
  CSingleLock lock(m_critical);
  m_usingLoginScreen = false;
  m_profileLoadedForLogin = false;
  m_lastUsedProfile = 0;
  m_nextProfileId = 0;
  SetCurrentProfileId(0);
  m_profiles.clear();
}

void CProfileManager::PrepareLoadProfile(unsigned int profileIndex)
{
  CContextMenuManager &contextMenuManager = CServiceBroker::GetContextMenuManager();
  ADDON::CServiceAddonManager &serviceAddons = CServiceBroker::GetServiceAddons();
  PVR::CPVRManager &pvrManager = CServiceBroker::GetPVRManager();
  CNetworkBase &networkManager = CServiceBroker::GetNetwork();

  contextMenuManager.Deinit();

  serviceAddons.Stop();

  // stop PVR related services
  pvrManager.Stop();

  if (profileIndex != 0 || !IsMasterProfile())
    networkManager.NetworkMessage(CNetwork::SERVICES_DOWN, 1);
}

bool CProfileManager::LoadProfile(unsigned int index)
{
  PrepareLoadProfile(index);

  if (index == 0 && IsMasterProfile())
  {
    CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_HOME);
    if (pWindow)
      pWindow->ResetControlStates();

    UpdateCurrentProfileDate();
    Save();
    FinalizeLoadProfile();

    return true;
  }

  CSingleLock lock(m_critical);
  // check if the index is valid or not
  if (index >= m_profiles.size())
    return false;

  // check if the profile is already active
  if (m_currentProfile == index)
    return true;

  // save any settings of the currently used skin but only if the (master)
  // profile hasn't just been loaded as a temporary profile for login
  if (g_SkinInfo != nullptr && !m_profileLoadedForLogin)
    g_SkinInfo->SaveSettings();

  // @todo: why is m_settings not used here?
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  // unload any old settings
  settings->Unload();

  SetCurrentProfileId(index);
  m_profileLoadedForLogin = false;

  // load the new settings
  if (!settings->Load())
  {
    CLog::Log(LOGFATAL, "CProfileManager: unable to load settings for profile \"%s\"", m_profiles.at(index).getName().c_str());
    return false;
  }
  settings->SetLoaded();

  CreateProfileFolders();

  CServiceBroker::GetDatabaseManager().Initialize();
  CServiceBroker::GetInputManager().LoadKeymaps();

  CServiceBroker::GetInputManager().SetMouseEnabled(settings->GetBool(CSettings::SETTING_INPUT_ENABLEMOUSE));

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
  {
    CGUIInfoManager& infoMgr = gui->GetInfoManager();
    infoMgr.ResetCache();
    infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();
    infoMgr.GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();
  }

  if (m_currentProfile != 0)
  {
    CXBMCTinyXML doc;
    if (doc.LoadFile(URIUtils::AddFileToFolder(GetUserDataFolder(), "guisettings.xml")))
    {
      settings->LoadSetting(doc.RootElement(), CSettings::SETTING_MASTERLOCK_MAXRETRIES);
      settings->LoadSetting(doc.RootElement(), CSettings::SETTING_MASTERLOCK_STARTUPLOCK);
    }
  }

  CPasswordManager::GetInstance().Clear();

  // to set labels - shares are reloaded
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia::UpdateState();
#endif

  // init windows
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESET);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  CUtil::DeleteDirectoryCache();
  g_directoryCache.Clear();

  lock.Leave();

  UpdateCurrentProfileDate();
  Save();
  FinalizeLoadProfile();

  return true;
}

void CProfileManager::FinalizeLoadProfile()
{
  CContextMenuManager &contextMenuManager = CServiceBroker::GetContextMenuManager();
  ADDON::CServiceAddonManager &serviceAddons = CServiceBroker::GetServiceAddons();
  PVR::CPVRManager &pvrManager = CServiceBroker::GetPVRManager();
  CNetworkBase &networkManager = CServiceBroker::GetNetwork();
  ADDON::CAddonMgr &addonManager = CServiceBroker::GetAddonMgr();
  CWeatherManager &weatherManager = CServiceBroker::GetWeatherManager();
  CFavouritesService &favouritesManager = CServiceBroker::GetFavouritesService();
  PLAYLIST::CPlayListPlayer &playlistManager = CServiceBroker::GetPlaylistPlayer();
  CStereoscopicsManager &stereoscopicsManager = CServiceBroker::GetGUI()->GetStereoscopicsManager();

  if (m_lastUsedProfile != m_currentProfile)
  {
    playlistManager.ClearPlaylist(PLAYLIST_VIDEO);
    playlistManager.ClearPlaylist(PLAYLIST_MUSIC);
    playlistManager.SetCurrentPlaylist(PLAYLIST_NONE);
  }

  networkManager.NetworkMessage(CNetworkBase::SERVICES_UP, 1);

  // reload the add-ons, or we will first load all add-ons from the master account without checking disabled status
  addonManager.ReInit();

  // let CApplication know that we are logging into a new profile
  g_application.SetLoggingIn(true);

  if (!g_application.LoadLanguage(true))
  {
    CLog::Log(LOGFATAL, "Unable to load language for profile \"%s\"", GetCurrentProfile().getName().c_str());
    return;
  }

  weatherManager.Refresh();

  JSONRPC::CJSONRPC::Initialize();

  // Restart context menu manager
  contextMenuManager.Init();

  // restart PVR services
  pvrManager.Init();

  favouritesManager.ReInit(GetProfileUserDataFolder());

  serviceAddons.Start();

  g_application.UpdateLibraries();

  stereoscopicsManager.Initialize();

  // Load initial window
  int firstWindow = g_SkinInfo->GetFirstWindow();

  // the startup window is considered part of the initialization as it most likely switches to the final window
  bool uiInitializationFinished = firstWindow != WINDOW_STARTUP_ANIM;

  CServiceBroker::GetGUI()->GetWindowManager().ChangeActiveWindow(firstWindow);

  // if the user interfaces has been fully initialized let everyone know
  if (uiInitializationFinished)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}

void CProfileManager::LogOff()
{
  CNetworkBase &networkManager = CServiceBroker::GetNetwork();

  g_application.StopPlaying();

  if (g_application.IsMusicScanning())
    g_application.StopMusicScan();

  if (CVideoLibraryQueue::GetInstance().IsRunning())
    CVideoLibraryQueue::GetInstance().CancelAllJobs();

  networkManager.NetworkMessage(CNetwork::SERVICES_DOWN, 1);

  LoadMasterProfileForLogin();

  g_passwordManager.bMasterUser = false;

  g_application.WakeUpScreenSaverAndDPMS();
  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_LOGIN_SCREEN, {}, false);

  if (!CServiceBroker::GetNetwork().GetServices().StartEventServer()) // event server could be needed in some situations
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
}

bool CProfileManager::DeleteProfile(unsigned int index)
{
  CSingleLock lock(m_critical);
  const CProfile *profile = GetProfile(index);
  if (profile == NULL)
    return false;

  CGUIDialogYesNo* dlgYesNo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo == NULL)
    return false;

  std::string str = g_localizeStrings.Get(13201);
  dlgYesNo->SetHeading(CVariant{13200});
  dlgYesNo->SetLine(0, CVariant{StringUtils::Format(str.c_str(), profile->getName().c_str())});
  dlgYesNo->SetLine(1, CVariant{""});
  dlgYesNo->SetLine(2, CVariant{""});
  dlgYesNo->Open();

  if (!dlgYesNo->IsConfirmed())
    return false;

  // fall back to master profile if necessary
  if ((int)index == m_autoLoginProfile)
    m_autoLoginProfile = 0;

  // delete profile
  std::string strDirectory = profile->getDirectory();
  m_profiles.erase(m_profiles.begin() + index);

  // fall back to master profile if necessary
  if (index == m_currentProfile)
  {
    LoadProfile(0);
    m_settings->Save();
  }

  CFileItemPtr item = CFileItemPtr(new CFileItem(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory)));
  item->SetPath(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory + "/"));
  item->m_bIsFolder = true;
  item->Select(true);

  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui && gui->ConfirmDelete(item->GetPath()))
    CFileUtils::DeleteItem(item);

  return Save();
}

void CProfileManager::CreateProfileFolders()
{
  CDirectory::Create(GetDatabaseFolder());
  CDirectory::Create(GetCDDBFolder());
  CDirectory::Create(GetLibraryFolder());

  // create Thumbnails/*
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetVideoThumbFolder());
  CDirectory::Create(GetBookmarksThumbFolder());
  CDirectory::Create(GetSavestatesFolder());
  for (size_t hex = 0; hex < 16; hex++)
    CDirectory::Create(URIUtils::AddFileToFolder(GetThumbnailsFolder(), StringUtils::Format("%lx", hex)));

  CDirectory::Create("special://profile/addon_data");
  CDirectory::Create("special://profile/keymaps");
}

const CProfile& CProfileManager::GetMasterProfile() const
{
  CSingleLock lock(m_critical);
  if (!m_profiles.empty())
    return m_profiles[0];

  CLog::Log(LOGERROR, "%s: master profile doesn't exist", __FUNCTION__);
  return EmptyProfile;
}

const CProfile& CProfileManager::GetCurrentProfile() const
{
  CSingleLock lock(m_critical);
  if (m_currentProfile < m_profiles.size())
    return m_profiles[m_currentProfile];

  CLog::Log(LOGERROR, "CProfileManager: current profile index ({0}) is outside of the valid range ({1})", m_currentProfile, m_profiles.size());
  return EmptyProfile;
}

const CProfile* CProfileManager::GetProfile(unsigned int index) const
{
  CSingleLock lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

CProfile* CProfileManager::GetProfile(unsigned int index)
{
  CSingleLock lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

int CProfileManager::GetProfileIndex(const std::string &name) const
{
  CSingleLock lock(m_critical);
  for (int i = 0; i < static_cast<int>(m_profiles.size()); i++)
  {
    if (StringUtils::EqualsNoCase(m_profiles[i].getName(), name))
      return i;
  }

  return -1;
}

void CProfileManager::AddProfile(const CProfile &profile)
{
  CSingleLock lock(m_critical);
  // data integrity check - covers off migration from old profiles.xml,
  // incrementing of the m_nextIdProfile,and bad data coming in
  m_nextProfileId = std::max(m_nextProfileId, profile.getId() + 1);

  m_profiles.push_back(profile);
}

void CProfileManager::UpdateCurrentProfileDate()
{
  CSingleLock lock(m_critical);
  if (m_currentProfile < m_profiles.size())
    m_profiles[m_currentProfile].setDate();
}

void CProfileManager::LoadMasterProfileForLogin()
{
  CSingleLock lock(m_critical);
  // save the previous user
  m_lastUsedProfile = m_currentProfile;
  if (m_currentProfile != 0)
  {
    LoadProfile(0);

    // remember that the (master) profile has only been loaded for login
    m_profileLoadedForLogin = true;
  }
}

bool CProfileManager::GetProfileName(const unsigned int profileId, std::string& name) const
{
  CSingleLock lock(m_critical);
  const CProfile *profile = GetProfile(profileId);
  if (!profile)
    return false;

  name = profile->getName();
  return true;
}

std::string CProfileManager::GetUserDataFolder() const
{
  return GetMasterProfile().getDirectory();
}

std::string CProfileManager::GetProfileUserDataFolder() const
{
  if (m_currentProfile == 0)
    return GetUserDataFolder();

  return URIUtils::AddFileToFolder(GetUserDataFolder(), GetCurrentProfile().getDirectory());
}

std::string CProfileManager::GetDatabaseFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Database");
}

std::string CProfileManager::GetCDDBFolder() const
{
  return URIUtils::AddFileToFolder(GetDatabaseFolder(), "CDDB");
}

std::string CProfileManager::GetThumbnailsFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails");
}

std::string CProfileManager::GetVideoThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetThumbnailsFolder(), "Video");
}

std::string CProfileManager::GetBookmarksThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetVideoThumbFolder(), "Bookmarks");
}

std::string CProfileManager::GetLibraryFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "library");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "library");
}

std::string CProfileManager::GetSavestatesFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Savestates");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Savestates");
}

std::string CProfileManager::GetSettingsFile() const
{
  std::string settings;
  if (m_currentProfile == 0)
    return "special://masterprofile/guisettings.xml";

  return "special://profile/guisettings.xml";
}

std::string CProfileManager::GetUserDataItem(const std::string& strFile) const
{
  std::string path;
  path = "special://profile/" + strFile;

  // check if item exists in the profile (either for folder or
  // for a file (depending on slashAtEnd of strFile) otherwise
  // return path to masterprofile
  if ((URIUtils::HasSlashAtEnd(path) && !CDirectory::Exists(path)) || !CFile::Exists(path))
    path = "special://masterprofile/" + strFile;

  return path;
}

CEventLog& CProfileManager::GetEventLog()
{
  return m_eventLogs->GetEventLog(GetCurrentProfileId());
}

void CProfileManager::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_EVENTLOG_SHOW)
    GetEventLog().ShowFullEventLog();
}

void CProfileManager::SetCurrentProfileId(unsigned int profileId)
{
  CSingleLock lock(m_critical);
  m_currentProfile = profileId;
  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
}
