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

#include <limits.h>

#include "Settings.h"
#include "filesystem/File.h"
#include "guilib/WindowIDs.h"
#include "profiles/ProfilesManager.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "utils/XMLUtils.h"

using namespace std;
using namespace XFILE;

CSettings::CSettings(void)
{
}

void CSettings::RegisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.insert(settingsHandler);
}

void CSettings::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.erase(settingsHandler);
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
  m_videoStacking = false;

  m_bMyMusicSongInfoInVis = true;    // UNUSED - depreciated.
  m_bMyMusicSongThumbInVis = false;  // used for music info in vis screen

  m_bMyMusicPlaylistRepeat = false;
  m_bMyMusicPlaylistShuffle = false;

  m_bMyVideoPlaylistRepeat = false;
  m_bMyVideoPlaylistShuffle = false;
  m_bMyVideoNavFlatten = false;
  m_bStartVideoWindowed = false;
  m_bAddonAutoUpdate = true;
  m_bAddonNotifications = true;
  m_bAddonForeignFilter = false;

  m_fVolumeLevel = 1.0f;
  m_bMute = false;
  m_fZoomAmount = 1.0f;
  m_fPixelRatio = 1.0f;
  m_bNonLinStretch = false;

  m_pictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.m3u|.dng|.nef|.cr2|.crw|.orf|.arw|.erf|.3fr|.dcr|.x3f|.mef|.raf|.mrw|.pef|.sr2|.rss";
  m_musicExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|.imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.nsf|.spc|.gym|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.mid|.kar|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.cm3|.cms|.dlt|.brstm|.wtv|.mka";
  m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.tp|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.m3u8|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml|.vc1|.h264|.rcv|.rss|.mpls|.webm|.bdmv|.wtv";
  m_discStubExtensions = ".disc";
  // internal music extensions
  m_musicExtensions += "|.sidstream|.oggstream|.nsfstream|.asapstream|.cdda";
  // internal video extensions
  m_videoExtensions += "|.pvr";

  #if defined(TARGET_DARWIN)
    CStdString logDir = getenv("HOME");
    logDir += "/Library/Logs/";
    m_logFolder = logDir;
  #else
    m_logFolder = "special://home/";              // log file location
  #endif

  iAdditionalSubtitleDirectoryChecked = 0;
  m_iMyMusicStartWindow = WINDOW_MUSIC_FILES;
  m_iVideoStartWindow = WINDOW_VIDEO_FILES;

  m_iSystemTimeTotalUp = 0;

  m_userAgent = g_sysinfo.GetUserAgent();

  m_musicNeedsUpdate = 0;
  m_videoNeedsUpdate = 0;
}

CSettings::~CSettings(void)
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_subSettings.clear();

  Clear();
}


void CSettings::Save() const
{
  if (!SaveSettings(CProfilesManager::Get().GetSettingsFile()))
    CLog::Log(LOGERROR, "Unable to save settings to %s", CProfilesManager::Get().GetSettingsFile().c_str());
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(CProfilesManager::Get().GetSettingsFile());
  Save();
  return LoadSettings(CProfilesManager::Get().GetSettingsFile());
}

bool CSettings::Load()
{
  if (!OnSettingsLoading())
    return false;

  CLog::Log(LOGNOTICE, "loading %s", CProfilesManager::Get().GetSettingsFile().c_str());
  if (!LoadSettings(CProfilesManager::Get().GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", CProfilesManager::Get().GetSettingsFile().c_str(), CProfilesManager::Get().GetSettingsFile().c_str());
    if (!Reset())
      return false;
  }

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

  // mymusic settings
  TiXmlElement *pElement = pRootElement->FirstChildElement("mymusic");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyMusicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyMusicPlaylistShuffle);
    }
    GetInteger(pElement, "startwindow", m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    XMLUtils::GetBoolean(pElement, "songinfoinvis", m_bMyMusicSongInfoInVis);
    XMLUtils::GetBoolean(pElement, "songthumbinvis", m_bMyMusicSongThumbInVis);
    GetInteger(pElement, "needsupdate", m_musicNeedsUpdate, 0, 0, INT_MAX);
    GetPath(pElement, "defaultlibview", m_defaultMusicLibSource);
  }

  // myvideos settings
  pElement = pRootElement->FirstChildElement("myvideos");
  if (pElement)
  {
    GetInteger(pElement, "startwindow", m_iVideoStartWindow, WINDOW_VIDEO_FILES, WINDOW_VIDEO_FILES, WINDOW_VIDEO_NAV);
    XMLUtils::GetBoolean(pElement, "stackvideos", m_videoStacking);
    XMLUtils::GetBoolean(pElement, "flatten", m_bMyVideoNavFlatten);
    GetInteger(pElement, "needsupdate", m_videoNeedsUpdate, 0, 0, INT_MAX);

    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyVideoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyVideoPlaylistShuffle);
    }
  }

  // general settings
  pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "systemtotaluptime", m_iSystemTimeTotalUp, 0, 0, INT_MAX);
    XMLUtils::GetBoolean(pElement, "addonautoupdate", m_bAddonAutoUpdate);
    XMLUtils::GetBoolean(pElement, "addonnotifications", m_bAddonNotifications);
    XMLUtils::GetBoolean(pElement, "addonforeignfilter", m_bAddonForeignFilter);
  }

  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mute", m_bMute);
    GetFloat(pElement, "fvolumelevel", m_fVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
  }

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
  // write our tags one by one - just a big list for now (can be flashed up later)

  if (!OnSettingsSaving())
    return false;

  // mymusic settings
  TiXmlElement musicNode("mymusic");
  TiXmlNode *pNode = pRoot->InsertEndChild(musicNode);
  if (!pNode) return false;
  {
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", m_bMyMusicPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", m_bMyMusicPlaylistShuffle);
  }

  XMLUtils::SetInt(pNode, "needsupdate", m_musicNeedsUpdate);
  XMLUtils::SetInt(pNode, "startwindow", m_iMyMusicStartWindow);
  XMLUtils::SetBoolean(pNode, "songinfoinvis", m_bMyMusicSongInfoInVis);
  XMLUtils::SetBoolean(pNode, "songthumbinvis", m_bMyMusicSongThumbInVis);
  XMLUtils::SetPath(pNode, "defaultlibview", m_defaultMusicLibSource);

  // myvideos settings
  TiXmlElement videosNode("myvideos");
  pNode = pRoot->InsertEndChild(videosNode);
  if (!pNode) return false;

  XMLUtils::SetInt(pNode, "startwindow", m_iVideoStartWindow);

  XMLUtils::SetBoolean(pNode, "stackvideos", m_videoStacking);
  XMLUtils::SetInt(pNode, "needsupdate", m_videoNeedsUpdate);
  XMLUtils::SetBoolean(pNode, "flatten", m_bMyVideoNavFlatten);

  { // playlist window
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", m_bMyVideoPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", m_bMyVideoPlaylistShuffle);
  }

  // general settings
  TiXmlElement generalNode("general");
  pNode = pRoot->InsertEndChild(generalNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "systemtotaluptime", m_iSystemTimeTotalUp);
  XMLUtils::SetBoolean(pNode, "addonautoupdate", m_bAddonAutoUpdate);
  XMLUtils::SetBoolean(pNode, "addonnotifications", m_bAddonNotifications);
  XMLUtils::SetBoolean(pNode, "addonforeignfilter", m_bAddonForeignFilter);

  // audio settings
  TiXmlElement volumeNode("audio");
  pNode = pRoot->InsertEndChild(volumeNode);
  if (!pNode) return false;
  XMLUtils::SetBoolean(pNode, "mute", m_bMute);
  XMLUtils::SetFloat(pNode, "fvolumelevel", m_fVolumeLevel);

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  OnSettingsSaved();
  
  if (!Save(pRoot))
    return false;

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

void CSettings::Clear()
{
  m_pictureExtensions.clear();
  m_musicExtensions.clear();
  m_videoExtensions.clear();
  m_discStubExtensions.clear();

  m_logFolder.clear();
  m_userAgent.clear();

  m_defaultMusicLibSource.clear();

  OnSettingsCleared();

  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    (*it)->Clear();
}

bool CSettings::OnSettingsLoading()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettings::OnSettingsLoaded()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsLoaded();
}

bool CSettings::OnSettingsSaving() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
  {
    if (!(*it)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettings::OnSettingsSaved() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsSaved();
}

void CSettings::OnSettingsCleared()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); it++)
    (*it)->OnSettingsCleared();
}

bool CSettings::Load(const TiXmlNode *settings)
{
  bool ok = true;
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
    ok &= (*it)->Load(settings);

  return ok;
}

bool CSettings::Save(TiXmlNode *settings) const
{
  CSingleLock lock(m_critical);
  for (SubSettings::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); it++)
  {
    if (!(*it)->Save(settings))
      return false;
  }

  return true;
}
