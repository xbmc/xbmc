/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "Application.h"
#include "input/KeyboardLayoutConfiguration.h"
#include "Util.h"
#include "URL.h"
#include "guilib/GUIFontManager.h"
#include "input/ButtonTranslator.h"
#include "utils/XMLUtils.h"
#include "PasswordManager.h"
#include "utils/RegExp.h"
#include "GUIPassword.h"
#include "GUIInfoManager.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "LangInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#ifdef _WIN32
#include "win32/WIN32Util.h"
#endif
#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
#include "filesystem/SMBDirectory.h"
#endif
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "input/MouseStat.h"
#include "filesystem/File.h"
#include "filesystem/DirectoryCache.h"
#include "DatabaseManager.h"
#include "network/upnp/UPnPSettings.h"
#include "threads/SingleLock.h"

using namespace std;
using namespace XFILE;

CSettings::CSettings(void)
{
}

void CSettings::RegisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.insert(subSettings);
}

void CSettings::UnregisterSubSettings(ISubSettings *subSettings)
{
  if (subSettings == NULL)
    return;

  CSingleLock lock(m_critical);
  m_subSettings.erase(subSettings);
}

void CSettings::Initialize()
{
  RESOLUTION_INFO res;
  vector<RESOLUTION_INFO>::iterator it = m_ResInfo.begin();

  m_ResInfo.insert(it, RES_CUSTOM, res);

  for (int i = RES_HDTV_1080i; i <= RES_PAL60_16x9; i++)
  {
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan((RESOLUTION)i, m_ResInfo[i].Overscan);
  }

  m_fZoomAmount = 1.0f;
  m_fPixelRatio = 1.0f;
  m_bNonLinStretch = false;

  m_usingLoginScreen = false;
  m_lastUsedProfile = 0;
  m_currentProfile = 0;
  m_nextIdProfile = 0;
}

CSettings::~CSettings(void)
{
  Clear();
}


void CSettings::Save() const
{
  if (!SaveSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to save settings to %s", GetSettingsFile().c_str());
  }
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(GetSettingsFile());
  Save();
  return LoadSettings(GetSettingsFile());
}

bool CSettings::Load()
{
  if (!OnSettingsLoading())
    return false;

  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
  CLog::Log(LOGNOTICE, "loading %s", GetSettingsFile().c_str());
  if (!LoadSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", GetSettingsFile().c_str(), GetSettingsFile().c_str());
    if (!Reset())
      return false;
  }

  LoadUserFolderLayout();

  OnSettingsLoaded();

  return true;
}

bool CSettings::GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue)
{
  CStdString strDefault = strValue;
  if (XMLUtils::GetPath(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefault;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue, const CStdString& strDefaultValue)
{
  if (XMLUtils::GetString(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefaultValue;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, char *szValue, const CStdString& strDefaultValue)
{
  CStdString strValue;
  bool ret = GetString(pRootElement, tagName, strValue, strDefaultValue);
  if (szValue)
    strcpy(szValue, strValue.c_str());
  return ret;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
  if (XMLUtils::GetInt(pRootElement, tagName, iValue, iMin, iMax))
    return true;
  // default
  iValue = iDefault;
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
  if (XMLUtils::GetFloat(pRootElement, tagName, fValue, fMin, fMax))
    return true;
  // default
  fValue = fDefault;
  return false;
}

bool CSettings::LoadCalibration(const TiXmlElement* pRoot, const CStdString& strSettingsFile)
{
  m_Calibrations.clear();

  const TiXmlElement *pElement = pRoot->FirstChildElement("resolutions");
  if (!pElement)
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <resolutions>", strSettingsFile.c_str());
    return false;
  }
  const TiXmlElement *pResolution = pElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this calibration
    RESOLUTION_INFO cal;

    XMLUtils::GetString(pResolution, "description", cal.strMode);
    XMLUtils::GetInt(pResolution, "subtitles", cal.iSubtitles);
    XMLUtils::GetFloat(pResolution, "pixelratio", cal.fPixelRatio);
#ifdef HAS_XRANDR
    XMLUtils::GetFloat(pResolution, "refreshrate", cal.fRefreshRate);
    XMLUtils::GetString(pResolution, "output", cal.strOutput);
    XMLUtils::GetString(pResolution, "xrandrid", cal.strId);
#endif

    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      XMLUtils::GetInt(pOverscan, "left", cal.Overscan.left);
      XMLUtils::GetInt(pOverscan, "top", cal.Overscan.top);
      XMLUtils::GetInt(pOverscan, "right", cal.Overscan.right);
      XMLUtils::GetInt(pOverscan, "bottom", cal.Overscan.bottom);
    }

    // mark calibration as not updated
    // we must not delete those, resolution just might not be available
    cal.iWidth = cal.iHeight = 0;

    // store calibration, avoid adding duplicates
    bool found = false;
    for (std::vector<RESOLUTION_INFO>::iterator  it = m_Calibrations.begin(); it != m_Calibrations.end(); ++it)
    {
      if (it->strMode.Equals(cal.strMode))
      {
        found = true;
        break;
      }
    }
    if (!found)
      m_Calibrations.push_back(cal);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }
  ApplyCalibrations();
  return true;
}

void CSettings::ApplyCalibrations()
{
  // apply all calibrations to the resolutions
  for (size_t i = 0; i < m_Calibrations.size(); ++i)
  {
    // find resolutions
    for (size_t res = 0; res < m_ResInfo.size(); ++res)
    {
      if (res == RES_WINDOW)
        continue;
      if (m_Calibrations[i].strMode.Equals(m_ResInfo[res].strMode))
      {
        // overscan
        m_ResInfo[res].Overscan.left = m_Calibrations[i].Overscan.left;
        if (m_ResInfo[res].Overscan.left < -m_ResInfo[res].iWidth/4)
          m_ResInfo[res].Overscan.left = -m_ResInfo[res].iWidth/4;
        if (m_ResInfo[res].Overscan.left > m_ResInfo[res].iWidth/4)
          m_ResInfo[res].Overscan.left = m_ResInfo[res].iWidth/4;

        m_ResInfo[res].Overscan.top = m_Calibrations[i].Overscan.top;
        if (m_ResInfo[res].Overscan.top < -m_ResInfo[res].iHeight/4)
          m_ResInfo[res].Overscan.top = -m_ResInfo[res].iHeight/4;
        if (m_ResInfo[res].Overscan.top > m_ResInfo[res].iHeight/4)
          m_ResInfo[res].Overscan.top = m_ResInfo[res].iHeight/4;

        m_ResInfo[res].Overscan.right = m_Calibrations[i].Overscan.right;
        if (m_ResInfo[res].Overscan.right < m_ResInfo[res].iWidth / 2)
          m_ResInfo[res].Overscan.right = m_ResInfo[res].iWidth / 2;
        if (m_ResInfo[res].Overscan.right > m_ResInfo[res].iWidth * 3/2)
          m_ResInfo[res].Overscan.right = m_ResInfo[res].iWidth *3/2;

        m_ResInfo[res].Overscan.bottom = m_Calibrations[i].Overscan.bottom;
        if (m_ResInfo[res].Overscan.bottom < m_ResInfo[res].iHeight / 2)
          m_ResInfo[res].Overscan.bottom = m_ResInfo[res].iHeight / 2;
        if (m_ResInfo[res].Overscan.bottom > m_ResInfo[res].iHeight * 3/2)
          m_ResInfo[res].Overscan.bottom = m_ResInfo[res].iHeight * 3/2;

        m_ResInfo[res].iSubtitles = m_Calibrations[i].iSubtitles;
        if (m_ResInfo[res].iSubtitles < m_ResInfo[res].iHeight / 2)
          m_ResInfo[res].iSubtitles = m_ResInfo[res].iHeight / 2;
        if (m_ResInfo[res].iSubtitles > m_ResInfo[res].iHeight* 5/4)
          m_ResInfo[res].iSubtitles = m_ResInfo[res].iHeight* 5/4;

        m_ResInfo[res].fPixelRatio = m_Calibrations[i].fPixelRatio;
        if (m_ResInfo[res].fPixelRatio < 0.5f)
          m_ResInfo[res].fPixelRatio = 0.5f;
        if (m_ResInfo[res].fPixelRatio > 2.0f)
          m_ResInfo[res].fPixelRatio = 2.0f;
        break;
      }
    }
  }
}

void CSettings::UpdateCalibrations()
{
  for (size_t res = RES_DESKTOP; res < m_ResInfo.size(); ++res)
  {
    // find calibration
    bool found = false;
    for (std::vector<RESOLUTION_INFO>::iterator  it = m_Calibrations.begin(); it != m_Calibrations.end(); ++it)
    {
      if (it->strMode.Equals(m_ResInfo[res].strMode))
      {
        // TODO: erase calibrations with default values
        (*it) = m_ResInfo[res];
        found = true;
        break;
      }
    }
    if (!found)
      m_Calibrations.push_back(m_ResInfo[res]);
  }
}

bool CSettings::SaveCalibration(TiXmlNode* pRootNode) const
{
  TiXmlElement xmlRootElement("resolutions");
  TiXmlNode *pRoot = pRootNode->InsertEndChild(xmlRootElement);

  // save calibrations
  for (size_t i = 0 ; i < m_Calibrations.size() ; i++)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    // Now write each of the pieces of information we need...
    XMLUtils::SetString(pNode, "description", m_Calibrations[i].strMode);
    XMLUtils::SetInt(pNode, "subtitles", m_Calibrations[i].iSubtitles);
    XMLUtils::SetFloat(pNode, "pixelratio", m_Calibrations[i].fPixelRatio);
#ifdef HAS_XRANDR
    XMLUtils::SetFloat(pNode, "refreshrate", m_Calibrations[i].fRefreshRate);
    XMLUtils::SetString(pNode, "output", m_Calibrations[i].strOutput);
    XMLUtils::SetString(pNode, "xrandrid", m_Calibrations[i].strId);
#endif
    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    XMLUtils::SetInt(pOverscanNode, "left", m_Calibrations[i].Overscan.left);
    XMLUtils::SetInt(pOverscanNode, "top", m_Calibrations[i].Overscan.top);
    XMLUtils::SetInt(pOverscanNode, "right", m_Calibrations[i].Overscan.right);
    XMLUtils::SetInt(pOverscanNode, "bottom", m_Calibrations[i].Overscan.bottom);
  }
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{
  // load the xml file
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    CLog::Log(LOGERROR, "%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }

  LoadCalibration(pRootElement, strSettingsFile);
  g_guiSettings.LoadXML(pRootElement);

  // load any ISubSettings implementations
  return Load(pRootElement);
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings /* = NULL */) const
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  if (!OnSettingsSaving())
    return false;

  // write our tags one by one - just a big list for now (can be flashed up later)

  SaveCalibration(pRoot);

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  // For mastercode
  SaveProfiles( PROFILES_FILE );

  OnSettingsSaved();

  if (!Save(pRoot))
    return false;

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::LoadProfile(unsigned int index)
{
  unsigned int oldProfile = m_currentProfile;
  m_currentProfile = index;
  CStdString strOldSkin = g_guiSettings.GetString("lookandfeel.skin");
  CStdString strOldFont = g_guiSettings.GetString("lookandfeel.font");
  CStdString strOldTheme = g_guiSettings.GetString("lookandfeel.skintheme");
  CStdString strOldColors = g_guiSettings.GetString("lookandfeel.skincolors");
  if (Load())
  {
    CreateProfileFolders();

    // initialize our charset converter
    g_charsetConverter.reset();

    // Load the langinfo to have user charset <-> utf-8 conversion
    CStdString strLanguage = g_guiSettings.GetString("locale.language");
    strLanguage[0] = toupper(strLanguage[0]);

    CStdString strLangInfoPath;
    strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load language info file:%s", strLangInfoPath.c_str());
    g_langInfo.Load(strLangInfoPath);

    CButtonTranslator::GetInstance().Load(true);
    g_localizeStrings.Load("special://xbmc/language/", strLanguage);

    CDatabaseManager::Get().Initialize();

    g_Mouse.SetEnabled(g_guiSettings.GetBool("input.enablemouse"));

    g_infoManager.ResetCache();
    g_infoManager.ResetLibraryBools();

    // always reload the skin - we need it for the new language strings
    g_application.ReloadSkin();

    if (m_currentProfile != 0)
    {
      CXBMCTinyXML doc;
      if (doc.LoadFile(URIUtils::AddFileToFolder(GetUserDataFolder(),"guisettings.xml")))
        g_guiSettings.LoadMasterLock(doc.RootElement());
    }

    CPasswordManager::GetInstance().Clear();

    // to set labels - shares are reloaded
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
    MEDIA_DETECT::CDetectDVDMedia::UpdateState();
#endif
    // init windows
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WINDOW_RESET);
    g_windowManager.SendMessage(msg);

    CUtil::DeleteDirectoryCache();
    g_directoryCache.Clear();

    return true;
  }

  m_currentProfile = oldProfile;

  return false;
}

bool CSettings::DeleteProfile(unsigned int index)
{
  const CProfile *profile = GetProfile(index);
  if (!profile)
    return false;

  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(13201);
    message.Format(str.c_str(), profile->getName());
    dlgYesNo->SetHeading(13200);
    dlgYesNo->SetLine(0, message);
    dlgYesNo->SetLine(1, "");
    dlgYesNo->SetLine(2, "");
    dlgYesNo->DoModal();

    if (dlgYesNo->IsConfirmed())
    {
      //delete profile
      CStdString strDirectory = profile->getDirectory();
      m_vecProfiles.erase(m_vecProfiles.begin()+index);
      if (index == m_currentProfile)
      {
        LoadProfile(0);
        Save();
      }

      CFileItemPtr item = CFileItemPtr(new CFileItem(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory)));
      item->SetPath(URIUtils::AddFileToFolder(GetUserDataFolder(), strDirectory + "/"));
      item->m_bIsFolder = true;
      item->Select(true);
      CFileUtils::DeleteItem(item);
    }
    else
      return false;
  }

  SaveProfiles( PROFILES_FILE );

  return true;
}

void CSettings::LoadProfiles(const CStdString& profilesFile)
{
  // clear out our profiles
  m_vecProfiles.clear();

  CXBMCTinyXML profilesDoc;
  if (CFile::Exists(profilesFile))
  {
    if (profilesDoc.LoadFile(profilesFile))
    {
      TiXmlElement *rootElement = profilesDoc.RootElement();
      if (rootElement && strcmpi(rootElement->Value(),"profiles") == 0)
      {
        XMLUtils::GetUInt(rootElement, "lastloaded", m_lastUsedProfile);
        XMLUtils::GetBoolean(rootElement, "useloginscreen", m_usingLoginScreen);
        XMLUtils::GetInt(rootElement, "nextIdProfile", m_nextIdProfile);

        TiXmlElement* pProfile = rootElement->FirstChildElement("profile");
        
        CStdString defaultDir("special://home/userdata");
        if (!CDirectory::Exists(defaultDir))
          defaultDir = "special://xbmc/userdata";
        while (pProfile)
        {
          CProfile profile(defaultDir);
          profile.Load(pProfile,GetNextProfileId());
          AddProfile(profile);
          pProfile = pProfile->NextSiblingElement("profile");
        }
      }
      else
        CLog::Log(LOGERROR, "Error loading %s, no <profiles> node", profilesFile.c_str());
    }
    else
      CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", profilesFile.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
  }

  if (m_vecProfiles.empty())
  { // add the master user
    CProfile profile("special://masterprofile/", "Master user",0);
    AddProfile(profile);
  }

  // check the validity of the previous profile index
  if (m_lastUsedProfile >= m_vecProfiles.size())
    m_lastUsedProfile = 0;

  m_currentProfile = m_lastUsedProfile;

  // the login screen runs as the master profile, so if we're using this, we need to ensure
  // we switch to the master profile
  if (m_usingLoginScreen)
    m_currentProfile = 0;
}

bool CSettings::SaveProfiles(const CStdString& profilesFile) const
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("profiles");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  XMLUtils::SetInt(pRoot,"lastloaded", m_currentProfile);
  XMLUtils::SetBoolean(pRoot,"useloginscreen",m_usingLoginScreen);
  XMLUtils::SetInt(pRoot,"nextIdProfile",m_nextIdProfile);      
  for (unsigned int i = 0; i < m_vecProfiles.size(); ++i)
    m_vecProfiles[i].Save(pRoot);

  // save the file
  return xmlDoc.SaveFile(profilesFile);
}

void CSettings::Clear()
{
  m_vecProfiles.clear();

  m_ResInfo.clear();
  m_Calibrations.clear();

  CUPnPSettings::Get().Clear();

  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->Clear();
}

void CSettings::LoadUserFolderLayout()
{
  // check them all
  CStdString strDir = g_guiSettings.GetString("system.playlistspath");
  if (strDir == "set default")
  {
    strDir = "special://profile/playlists/";
    g_guiSettings.SetString("system.playlistspath",strDir.c_str());
  }
  CDirectory::Create(strDir);
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"music"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"video"));
  CDirectory::Create(URIUtils::AddFileToFolder(strDir,"mixed"));
}

CStdString CSettings::GetProfileUserDataFolder() const
{
  CStdString folder;
  if (m_currentProfile == 0)
    return GetUserDataFolder();

  URIUtils::AddFileToFolder(GetUserDataFolder(),GetCurrentProfile().getDirectory(),folder);

  return folder;
}

CStdString CSettings::GetUserDataItem(const CStdString& strFile) const
{
  CStdString folder;
  folder = "special://profile/"+strFile;
  //check if item exists in the profile
  //(either for folder or for a file (depending on slashAtEnd of strFile)
  //otherwise return path to masterprofile
  if ( (URIUtils::HasSlashAtEnd(folder) && !CDirectory::Exists(folder)) || !CFile::Exists(folder))
    folder = "special://masterprofile/"+strFile;
  return folder;
}

CStdString CSettings::GetUserDataFolder() const
{
  return GetMasterProfile().getDirectory();
}

CStdString CSettings::GetDatabaseFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Database", folder);

  return folder;
}

CStdString CSettings::GetCDDBFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Database/CDDB", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Database/CDDB", folder);

  return folder;
}

CStdString CSettings::GetThumbnailsFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails", folder);

  return folder;
}

CStdString CSettings::GetVideoThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Video", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Video", folder);

  return folder;
}

CStdString CSettings::GetBookmarksThumbFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);

  return folder;
}

CStdString CSettings::GetLibraryFolder() const
{
  CStdString folder;
  if (GetCurrentProfile().hasDatabases())
    URIUtils::AddFileToFolder(GetProfileUserDataFolder(), "library", folder);
  else
    URIUtils::AddFileToFolder(GetUserDataFolder(), "library", folder);

  return folder;
}

CStdString CSettings::GetSettingsFile() const
{
  CStdString settings;
  if (m_currentProfile == 0)
    settings = "special://masterprofile/guisettings.xml";
  else
    settings = "special://profile/guisettings.xml";
  return settings;
}

void CSettings::CreateProfileFolders()
{
  CDirectory::Create(GetDatabaseFolder());
  CDirectory::Create(GetCDDBFolder());

  // Thumbnails/
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetVideoThumbFolder());
  CDirectory::Create(GetBookmarksThumbFolder());
  CLog::Log(LOGINFO, "thumbnails folder: %s", GetThumbnailsFolder().c_str());
  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strHex;
    strHex.Format("%x",hex);
    CDirectory::Create(URIUtils::AddFileToFolder(GetThumbnailsFolder(), strHex));
  }
  CDirectory::Create("special://profile/addon_data");
  CDirectory::Create("special://profile/keymaps");
  CDirectory::Create(GetLibraryFolder());
}

static CProfile emptyProfile;

const CProfile &CSettings::GetMasterProfile() const
{
  if (GetNumProfiles())
    return m_vecProfiles[0];
  CLog::Log(LOGERROR, "%s - master profile requested while none exists", __FUNCTION__);
  return emptyProfile;
}

const CProfile &CSettings::GetCurrentProfile() const
{
  if (m_currentProfile < m_vecProfiles.size())
    return m_vecProfiles[m_currentProfile];
  CLog::Log(LOGERROR, "%s - last profile index (%u) is outside the valid range (%" PRIdS ")", __FUNCTION__, m_currentProfile, m_vecProfiles.size());
  return emptyProfile;
}

 int CSettings::GetCurrentProfileId() const
 {
   return GetCurrentProfile().getId();
 }

void CSettings::UpdateCurrentProfileDate()
{
  if (m_currentProfile < m_vecProfiles.size())
    m_vecProfiles[m_currentProfile].setDate();
}

const CProfile *CSettings::GetProfile(unsigned int index) const
{
  if (index < GetNumProfiles())
    return &m_vecProfiles[index];
  return NULL;
}

CProfile *CSettings::GetProfile(unsigned int index)
{
  if (index < GetNumProfiles())
    return &m_vecProfiles[index];
  return NULL;
}

unsigned int CSettings::GetNumProfiles() const
{
  return m_vecProfiles.size();
}

int CSettings::GetProfileIndex(const CStdString &name) const
{
  for (unsigned int i = 0; i < m_vecProfiles.size(); i++)
    if (m_vecProfiles[i].getName().Equals(name))
      return i;
  return -1;
}

void CSettings::AddProfile(const CProfile &profile)
{
  //data integrity check - covers off migration from old profiles.xml, incrementing of the m_nextIdProfile,and bad data coming in
  m_nextIdProfile = max(m_nextIdProfile, profile.getId() + 1); 

  m_vecProfiles.push_back(profile);
}

void CSettings::LoadMasterForLogin()
{
  // save the previous user
  m_lastUsedProfile = m_currentProfile;
  if (m_currentProfile != 0)
    LoadProfile(0);
}

bool CSettings::Load(const TiXmlNode *settings)
{
  bool ok = true;
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    ok &= (*it)->Load(settings);

  return ok;
}

bool CSettings::Save(TiXmlNode *settings) const
{
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->Save(settings))
      return false;
  }

  return true;
}

bool CSettings::OnSettingsLoading()
{
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettings::OnSettingsLoaded()
{
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->OnSettingsLoaded();
}

bool CSettings::OnSettingsSaving() const
{
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettings::OnSettingsSaved() const
{
  CSingleLock lock(m_critical);
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->OnSettingsSaved();
}
