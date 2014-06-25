/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "ProfilesManager.h"
#include "Application.h"
#include "DatabaseManager.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "LangInfo.h"
#include "PasswordManager.h"
#include "Util.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/ButtonTranslator.h"
#include "input/MouseStat.h"
#include "settings/Settings.h"
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"
#include "utils/FileUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

// TODO
// eventually the profile should dictate where special://masterprofile/ is
// but for now it makes sense to leave all the profile settings in a user
// writeable location like special://masterprofile/
#define PROFILES_FILE     "special://masterprofile/profiles.xml"

#define XML_PROFILES      "profiles"
#define XML_AUTO_LOGIN    "autologin"
#define XML_LAST_LOADED   "lastloaded"
#define XML_LOGIN_SCREEN  "useloginscreen"
#define XML_NEXTID        "nextIdProfile"
#define XML_PROFILE       "profile"

using namespace std;
using namespace XFILE;

static CProfile EmptyProfile;

CProfilesManager::CProfilesManager()
  : m_usingLoginScreen(false), m_autoLoginProfile(-1), m_lastUsedProfile(0),
    m_currentProfile(0), m_nextProfileId(0)
{ }

CProfilesManager::~CProfilesManager()
{ }

CProfilesManager& CProfilesManager::Get()
{
  static CProfilesManager sProfilesManager;
  return sProfilesManager;
}

void CProfilesManager::OnSettingsLoaded()
{
  // check them all
  string strDir = CSettings::Get().GetString("system.playlistspath");
  if (strDir == "set default" || strDir.empty())
  {
    strDir = "special://profile/playlists/";
    CSettings::Get().SetString("system.playlistspath", strDir.c_str());
  }

  CDirectory::Create(strDir);
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"music"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"video"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"mixed"));
}

void CProfilesManager::OnSettingsSaved()
{
  // save mastercode
  Save();
}

void CProfilesManager::OnSettingsCleared()
{
  Clear();
}

bool CProfilesManager::Load()
{
  return Load(PROFILES_FILE);
}

bool CProfilesManager::Load(const std::string &file)
{
  CSingleLock lock(m_critical);
  bool ret = true;

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
        CLog::Log(LOGERROR, "CProfilesManager: error loading %s, no <profiles> node", file.c_str());
        ret = false;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CProfilesManager: error loading %s, Line %d\n%s", file.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
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

bool CProfilesManager::Save()
{
  return Save(PROFILES_FILE);
}

bool CProfilesManager::Save(const std::string &file) const
{
  CSingleLock lock(m_critical);

  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement(XML_PROFILES);
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  XMLUtils::SetInt(pRoot, XML_LAST_LOADED, m_currentProfile);
  XMLUtils::SetBoolean(pRoot, XML_LOGIN_SCREEN, m_usingLoginScreen);
  XMLUtils::SetInt(pRoot, XML_AUTO_LOGIN, m_autoLoginProfile);
  XMLUtils::SetInt(pRoot, XML_NEXTID, m_nextProfileId);      

  for (vector<CProfile>::const_iterator profile = m_profiles.begin(); profile != m_profiles.end(); profile++)
    profile->Save(pRoot);

  // save the file
  return xmlDoc.SaveFile(file);
}

void CProfilesManager::Clear()
{
  CSingleLock lock(m_critical);
  m_usingLoginScreen = false;
  m_lastUsedProfile = 0;
  m_nextProfileId = 0;
  SetCurrentProfileId(0);
  m_profiles.clear();
}

bool CProfilesManager::LoadProfile(size_t index)
{
  CSingleLock lock(m_critical);
  // check if the index is valid or not
  if (index >= m_profiles.size())
    return false;

  // check if the profile is already active
  if (m_currentProfile == index)
    return true;

  // unload any old settings
  CSettings::Get().Unload();

  SetCurrentProfileId(index);

  // load the new settings
  if (!CSettings::Get().Load())
  {
    CLog::Log(LOGFATAL, "CProfilesManager: unable to load settings for profile \"%s\"", m_profiles.at(index).getName().c_str());
    return false;
  }
  CSettings::Get().SetLoaded();

  CreateProfileFolders();

  // Load the langinfo to have user charset <-> utf-8 conversion
  string strLanguage = CSettings::Get().GetString("locale.language");
  strLanguage[0] = toupper(strLanguage[0]);

  string strLangInfoPath = StringUtils::Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());
  CLog::Log(LOGINFO, "CProfilesManager: load language info file: %s", strLangInfoPath.c_str());
  g_langInfo.Load(strLangInfoPath);

  CButtonTranslator::GetInstance().Load(true);
  g_localizeStrings.Load("special://xbmc/language/", strLanguage);

  CDatabaseManager::Get().Initialize();

  g_Mouse.SetEnabled(CSettings::Get().GetBool("input.enablemouse"));

  g_infoManager.ResetCache();
  g_infoManager.ResetLibraryBools();

  // always reload the skin - we need it for the new language strings
  g_application.ReloadSkin();

  if (m_currentProfile != 0)
  {
    CXBMCTinyXML doc;
    if (doc.LoadFile(URIUtils::AddFileToFolder(GetUserDataFolder(), "guisettings.xml")))
    {
      CSettings::Get().LoadSetting(doc.RootElement(), "masterlock.maxretries");
      CSettings::Get().LoadSetting(doc.RootElement(), "masterlock.startuplock");
    }
  }

  CPasswordManager::GetInstance().Clear();

  // to set labels - shares are reloaded
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia::UpdateState();
#endif

  // init windows
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESET);
  g_windowManager.SendMessage(msg);

  CUtil::DeleteDirectoryCache();
  g_directoryCache.Clear();

  return true;
}

bool CProfilesManager::DeleteProfile(size_t index)
{
  CSingleLock lock(m_critical);
  const CProfile *profile = GetProfile(index);
  if (profile == NULL)
    return false;

  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo == NULL)
    return false;

  string message;
  string str = g_localizeStrings.Get(13201);
  message = StringUtils::Format(str.c_str(), profile->getName().c_str());
  dlgYesNo->SetHeading(13200);
  dlgYesNo->SetLine(0, message);
  dlgYesNo->SetLine(1, "");
  dlgYesNo->SetLine(2, "");
  dlgYesNo->DoModal();

  if (!dlgYesNo->IsConfirmed())
    return false;

  // fall back to master profile if necessary
  if ((int)index == m_autoLoginProfile)
    m_autoLoginProfile = 0;

  // delete profile
  string strDirectory = profile->getDirectory();
  m_profiles.erase(m_profiles.begin() + index);

  // fall back to master profile if necessary
  if (index == m_currentProfile)
  {
    LoadProfile(0);
    CSettings::Get().Save();
  }

  CFileItemPtr item = CFileItemPtr(new CFileItem(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory)));
  item->SetPath(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory + "/"));
  item->m_bIsFolder = true;
  item->Select(true);
  CFileUtils::DeleteItem(item);

  return Save();
}

void CProfilesManager::CreateProfileFolders()
{
  CDirectory::Create(GetDatabaseFolder());
  CDirectory::Create(GetCDDBFolder());
  CDirectory::Create(GetLibraryFolder());

  // create Thumbnails/*
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetVideoThumbFolder());
  CDirectory::Create(GetBookmarksThumbFolder());
  for (size_t hex = 0; hex < 16; hex++)
    CDirectory::Create(URIUtils::AddFileToFolder(GetThumbnailsFolder(), StringUtils::Format("%lx", hex)));

  CDirectory::Create("special://profile/addon_data");
  CDirectory::Create("special://profile/keymaps");
}

const CProfile& CProfilesManager::GetMasterProfile() const
{
  CSingleLock lock(m_critical);
  if (!m_profiles.empty())
    return m_profiles[0];

  CLog::Log(LOGERROR, "%s: master profile doesn't exist", __FUNCTION__);
  return EmptyProfile;
}

const CProfile& CProfilesManager::GetCurrentProfile() const
{
  CSingleLock lock(m_critical);
  if (m_currentProfile < m_profiles.size())
    return m_profiles[m_currentProfile];

  CLog::Log(LOGERROR, "CProfilesManager: current profile index (%u) is outside of the valid range (%" PRIdS ")", m_currentProfile, m_profiles.size());
  return EmptyProfile;
}

const CProfile* CProfilesManager::GetProfile(size_t index) const
{
  CSingleLock lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

CProfile* CProfilesManager::GetProfile(size_t index)
{
  CSingleLock lock(m_critical);
  if (index < m_profiles.size())
    return &m_profiles[index];

  return NULL;
}

int CProfilesManager::GetProfileIndex(const std::string &name) const
{
  CSingleLock lock(m_critical);
  for (size_t i = 0; i < m_profiles.size(); i++)
  {
    if (StringUtils::EqualsNoCase(m_profiles[i].getName(), name))
      return i;
  }

  return -1;
}

void CProfilesManager::AddProfile(const CProfile &profile)
{
  CSingleLock lock(m_critical);
  // data integrity check - covers off migration from old profiles.xml,
  // incrementing of the m_nextIdProfile,and bad data coming in
  m_nextProfileId = max(m_nextProfileId, profile.getId() + 1);

  m_profiles.push_back(profile);
}

void CProfilesManager::UpdateCurrentProfileDate()
{
  CSingleLock lock(m_critical);
  if (m_currentProfile < m_profiles.size())
    m_profiles[m_currentProfile].setDate();
}

void CProfilesManager::LoadMasterProfileForLogin()
{
  CSingleLock lock(m_critical);
  // save the previous user
  m_lastUsedProfile = m_currentProfile;
  if (m_currentProfile != 0)
    LoadProfile(0);
}

bool CProfilesManager::GetProfileName(const size_t profileId, std::string& name) const
{
  CSingleLock lock(m_critical);
  const CProfile *profile = GetProfile(profileId);
  if (!profile)
    return false;

  name = profile->getName();
  return true;
}

std::string CProfilesManager::GetUserDataFolder() const
{
  return GetMasterProfile().getDirectory();
}

std::string CProfilesManager::GetProfileUserDataFolder() const
{
  if (m_currentProfile == 0)
    return GetUserDataFolder();

  return URIUtils::AddFileToFolder(GetUserDataFolder(), GetCurrentProfile().getDirectory());
}

std::string CProfilesManager::GetDatabaseFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Database");
}

std::string CProfilesManager::GetCDDBFolder() const
{
  return URIUtils::AddFileToFolder(GetDatabaseFolder(), "CDDB");
}

std::string CProfilesManager::GetThumbnailsFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails");
}

std::string CProfilesManager::GetVideoThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetThumbnailsFolder(), "Video");
}

std::string CProfilesManager::GetBookmarksThumbFolder() const
{
  return URIUtils::AddFileToFolder(GetVideoThumbFolder(), "Bookmarks");
}

std::string CProfilesManager::GetLibraryFolder() const
{
  if (GetCurrentProfile().hasDatabases())
    return URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "library");

  return URIUtils::AddFileToFolder(GetUserDataFolder(), "library");
}

std::string CProfilesManager::GetSettingsFile() const
{
  std::string settings;
  if (m_currentProfile == 0)
    return "special://masterprofile/guisettings.xml";

  return "special://profile/guisettings.xml";
}

std::string CProfilesManager::GetUserDataItem(const std::string& strFile) const
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

void CProfilesManager::SetCurrentProfileId(size_t profileId)
{
  CSingleLock lock(m_critical);
  m_currentProfile = profileId;
  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
}
