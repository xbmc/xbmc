/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Application.h"
#include "AddonHelpers.h"
#include "Settings.h"
#include "settings/AddonSettings.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogSelect.h"
#include "GUIDialogProgress.h"
#include "GUIDialogKeyboard.h"
#include "GUIAudioManager.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/PluginDirectory.h"
#include "Util.h"
#include "URL.h"
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#endif
#include "Crc32.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"
#include "SectionLoader.h"
#include "LangInfo.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

namespace ADDON
{

int CAddonUtils::m_iGUILockRef = 0;

extern "C"
{

void CAddonUtils::CreateAddOnCallbacks(AddonCB *cbTable)
{
  /* AddOn Helper functions */
  cbTable->AddOn.Log                = CAddonUtils::AddOnLog;
  cbTable->AddOn.ReportStatus       = CAddonUtils::AddonStatusHandler;
  cbTable->AddOn.GetSetting         = CAddonUtils::GetAddonSetting;
  cbTable->AddOn.OpenSettings       = CAddonUtils::OpenAddonSettings;
  cbTable->AddOn.GetAddonDirectory  = CAddonUtils::GetAddonDirectory;
  cbTable->AddOn.GetUserDirectory   = CAddonUtils::GetUserDirectory;

  /* Utilities Helper functions */
  cbTable->Utils.Shutdown           = CAddonUtils::Shutdown;
  cbTable->Utils.Restart            = CAddonUtils::Restart;
  cbTable->Utils.Dashboard          = CAddonUtils::Dashboard;
  cbTable->Utils.ExecuteScript      = CAddonUtils::ExecuteScript;
  cbTable->Utils.ExecuteBuiltIn     = CAddonUtils::ExecuteBuiltIn;
  cbTable->Utils.ExecuteHttpApi     = CAddonUtils::ExecuteHttpApi;
  cbTable->Utils.UnknownToUTF8      = CAddonUtils::UnknownToUTF8;
  cbTable->Utils.GetSkinDir         = CAddonUtils::GetSkinDir;
  cbTable->Utils.GetLanguage        = CAddonUtils::GetLanguage;
  cbTable->Utils.GetIPAddress       = CAddonUtils::GetIPAddress;
  cbTable->Utils.GetInfoLabel       = CAddonUtils::GetInfoLabel;
  cbTable->Utils.GetInfoImage       = CAddonUtils::GetInfoImage;
  cbTable->Utils.GetCondVisibility  = CAddonUtils::GetCondVisibility;
  cbTable->Utils.PlaySFX            = CAddonUtils::PlaySFX;
  cbTable->Utils.EnableNavSounds    = CAddonUtils::EnableNavSounds;
  cbTable->Utils.GetCacheThumbName  = CAddonUtils::GetCacheThumbName;
  cbTable->Utils.GetDVDState        = CAddonUtils::GetDVDState;
  cbTable->Utils.GetFreeMem         = CAddonUtils::GetFreeMem;
  cbTable->Utils.GetGlobalIdleTime  = CAddonUtils::GetGlobalIdleTime;
  cbTable->Utils.LocalizedString    = CAddonUtils::GetLocalizedString;
  cbTable->Utils.GetRegion          = CAddonUtils::GetRegion;
  cbTable->Utils.GetSupportedMedia  = CAddonUtils::GetSupportedMedia;
  cbTable->Utils.MakeLegalFilename  = CAddonUtils::MakeLegalFilename;
  cbTable->Utils.SkinHasImage       = CAddonUtils::SkinHasImage;
  cbTable->Utils.TranslatePath      = CAddonUtils::TranslatePath;
  cbTable->Utils.UnknownToUTF8      = CAddonUtils::UnknownToUTF8;
  cbTable->Utils.CreateDirectory    = CAddonUtils::CreateDirectory;

  /* GUI Dialog Helper functions */
  cbTable->Dialog.OpenOK            = CAddonUtils::OpenDialogOK;
  cbTable->Dialog.OpenYesNo         = CAddonUtils::OpenDialogYesNo;
  cbTable->Dialog.OpenBrowse        = CAddonUtils::OpenDialogBrowse;
  cbTable->Dialog.OpenNumeric       = CAddonUtils::OpenDialogNumeric;
  cbTable->Dialog.OpenKeyboard      = CAddonUtils::OpenDialogKeyboard;
  cbTable->Dialog.OpenSelect        = CAddonUtils::OpenDialogSelect;
  cbTable->Dialog.ProgressCreate    = CAddonUtils::ProgressDialogCreate;
  cbTable->Dialog.ProgressUpdate    = CAddonUtils::ProgressDialogUpdate;
  cbTable->Dialog.ProgressIsCanceled= CAddonUtils::ProgressDialogIsCanceled;
  cbTable->Dialog.ProgressClose     = CAddonUtils::ProgressDialogClose;
}

void CAddonUtils::AddOnLog(void *addonData, const addon_log_t loglevel, const char *msg)
{
  const CAddon* addon = (CAddon*) addonData;
  if (!addon)
    return;

  try
  {
    CStdString xbmcMsg;
    xbmcMsg.Format("AddOnLog: %s/%s: %s", GetAddonTypeName(addon->m_addonType).c_str(), addon->m_strName.c_str(), msg);

    int xbmclog;
    switch (loglevel)
    {
      case LOG_ERROR:
        xbmclog = LOGERROR;
        break;
      case LOG_INFO:
        xbmclog = LOGINFO;
        break;
      case LOG_NOTICE:
        xbmclog = LOGNOTICE;
        break;
      case LOG_DEBUG:
      default:
        xbmclog = LOGDEBUG;
        break;
    }

    /* finally write the logmessage */
    CLog::Log(xbmclog, xbmcMsg);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "AddOnLog: %s/%s - exception '%s' during AddOnLogCallback occurred, contact Developer '%s' of this AddOn", GetAddonTypeName(addon->m_addonType).c_str(), addon->m_strName.c_str());
    return;
  }
}

void CAddonUtils::AddonStatusHandler(void *addonData, const ADDON_STATUS status, const char* msg)
{
  const CAddon* addon = (CAddon*) addonData;
  if (!addon)
    return;

  new CAddonStatusHandler(addon, status, msg, true);
  return;
}

void CAddonUtils::OpenAddonSettings(void *addonData)
{
  const CAddon* addon = (CAddon*) addonData;
  if (!addon)
    return;

  try
  {
    CLog::Log(LOGDEBUG, "Calling OpenAddonSettings for: %s", addon->m_strName.c_str());

    if (!CAddonSettings::SettingsExist(addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", addon->m_strName.c_str());
      return;
    }

    CURL cUrl(addon->m_strPath);
    CGUIDialogAddonSettings::ShowAndGetInput(cUrl);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "CAddonUtils: %s - exception '%s' during OpenAddonSettings occurred, contact Developer '%s' of this AddOn", addon->m_strName.c_str(), e.what(), addon->m_strCreator.c_str());
  }
}

void CAddonUtils::TransferAddonSettings(const CAddon &addon)
{
  bool restart = false;
  ADDON_STATUS reportStatus = STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferAddonSettings for: %s", addon.m_strName.c_str());

  /* Transmit current unified user settings to the PVR Addon */
  ADDON::IAddonCallback* addonCB = g_addonmanager.GetCallbackForType(addon.m_addonType);

  CAddonSettings settings;
  if (!settings.Load(addon))
  {
    CLog::Log(LOGERROR, "Could't get Settings for AddOn: %s during transfer", addon.m_strName.c_str());
    return;
  }

  TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
  while (setting)
  {
    ADDON_STATUS status;
    const char *id = setting->Attribute("id");
    const char *type = setting->Attribute("type");

    if (type)
    {
      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
          strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
          strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
      {
        status = addonCB->SetSetting(&addon, id, (const char*) settings.Get(id).c_str());
      }
      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
               strcmpi(type, "labelenum") == 0)
      {
        int tmp = atoi(settings.Get(id));
        status = addonCB->SetSetting(&addon, id, (int*) &tmp);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        bool tmp = settings.Get(id) == "true" ? true : false;
        status = addonCB->SetSetting(&addon, id, (bool*) &tmp);
      }
      else
      {
        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, addon.m_strName.c_str());
      }

      if (status == STATUS_NEED_RESTART)
        restart = true;
      else if (status != STATUS_OK)
        reportStatus = status;
    }
    setting = setting->NextSiblingElement("setting");
  }

  if (restart || reportStatus != STATUS_OK)
    new CAddonStatusHandler(&addon, restart ? STATUS_NEED_RESTART : reportStatus, "", true);
}

bool CAddonUtils::GetAddonSetting(void *addonData, const char* settingName, void *settingValue)
{
  const CAddon* addon = (CAddon*) addonData;
  if (addon == NULL || settingName == NULL || settingValue == NULL)
    return false;

  try
  {
    CLog::Log(LOGDEBUG, "CAddonUtils: AddOn %s request Setting %s", addon->m_strName.c_str(), settingName);

    /* TODO: Add a caching mechanism to prevent a reloading of settings file on every call */
    CAddonSettings settings;
    if (!settings.Load(*addon))
    {
      CLog::Log(LOGERROR, "Could't get Settings for AddOn: %s", addon->m_strName.c_str());
      return false;
    }

    TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");

      if (strcmpi(id, settingName) == 0 && type)
      {
        if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
            strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
            strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
            strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
            strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
        {
          strcpy((char*) settingValue, settings.Get(id).c_str());
          return true;
        }
        else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
                 strcmpi(type, "labelenum") == 0)
        {
          *(int*) settingValue = (int) atoi(settings.Get(id));
          return true;
        }
        else if (strcmpi(type, "bool") == 0)
        {
          *(bool*) settingValue = (bool) (settings.Get(id) == "true" ? true : false);
          return true;
        }
        else
        {
          CLog::Log(LOGERROR, "Unknown setting type '%s' for id %s in %s", type, id, addon->m_strName.c_str());
        }
      }
      setting = setting->NextSiblingElement("setting");
    }
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "PVR: %s - exception '%s' during GetAddonSetting occurred, contact Developer '%s' of this AddOn", addon->m_strName.c_str(), e.what(), addon->m_strCreator.c_str());
  }
  return false;
}

char* CAddonUtils::GetAddonDirectory(void *addonData)
{
  CStdString addonDir;

  const CAddon* addon = (CAddon*) addonData;
  if (addon != NULL)
  {
    addonDir = addon->m_strPath;
    CUtil::RemoveSlashAtEnd(addonDir);
  }
  else
  {
    addonDir = "";
  }
  char *buffer = (char*) malloc (addonDir.length()+1);
  strcpy(buffer, addonDir.c_str());
  return buffer;
}

char* CAddonUtils::GetUserDirectory(void *addonData)
{
  CStdString addonUserDir;

  const CAddon* addon = (CAddon*) addonData;
  if (addon != NULL)
  {
    addonUserDir = addon->m_strPath;
    // Remove the special path
    addonUserDir.Replace("special://home/addons/", "");
    addonUserDir.Replace("special://xbmc/addons/", "");

    // and create the users filepath
    addonUserDir.Format("special://profile/addon_data/%s", addonUserDir.c_str());
    CUtil::RemoveSlashAtEnd(addonUserDir);

    // create the users filepath if not exists
    if (!CDirectory::Exists(addonUserDir))
      CUtil::CreateDirectoryEx(addonUserDir);
  }
  else
    addonUserDir = "";

  char *buffer = (char*) malloc (addonUserDir.length()+1);
  strcpy(buffer, addonUserDir.c_str());
  return buffer;
}


/**
* XBMC AddOn Utils callbacks
* Helper to access XBMC Utilities
*/

void CAddonUtils::Shutdown()
{
  ThreadMessage tMsg = {TMSG_SHUTDOWN};
  g_application.getApplicationMessenger().SendMessage(tMsg);
  return;
}

void CAddonUtils::Restart()
{
  ThreadMessage tMsg = {TMSG_RESTART};
  g_application.getApplicationMessenger().SendMessage(tMsg);
  return;
}

void CAddonUtils::Dashboard()
{
  ThreadMessage tMsg = {TMSG_DASHBOARD};
  g_application.getApplicationMessenger().SendMessage(tMsg);
}

void CAddonUtils::ExecuteScript(const char *script)
{
  if (script == NULL)
    return;

  ThreadMessage tMsg = {TMSG_EXECUTE_SCRIPT};
  tMsg.strParam = script;
  g_application.getApplicationMessenger().SendMessage(tMsg);
}

void CAddonUtils::ExecuteBuiltIn(const char *function)
{
  if (function == NULL)
    return;

  g_application.getApplicationMessenger().ExecBuiltIn(function);
}

char* CAddonUtils::ExecuteHttpApi(const char *httpcommand)
{
  CStdString ret = "";

#ifdef HAS_WEB_SERVER
  if (httpcommand != NULL)
  {
    if (!m_pXbmcHttp)
    {
      CSectionLoader::Load("LIBHTTP");
      m_pXbmcHttp = new CXbmcHttp();
    }
    if (!pXbmcHttpShim)
    {
      pXbmcHttpShim = new CXbmcHttpShim();
      if (pXbmcHttpShim)
        ret = pXbmcHttpShim->xbmcExternalCall(httpcommand);
    }
 }
#endif
  char *buffer = (char*) malloc (ret.length()+1);
  strcpy(buffer, ret.c_str());
  return buffer;
}

char* CAddonUtils::GetLocalizedString(const void* addonData, long dwCode)
{
  CStdString string;
  if (addonData != NULL)
  {
    const CAddon* addon = (CAddon*) addonData;

    // Load language strings temporarily
    CURL cUrl(addon->m_strPath);
    CAddon::LoadAddonStrings(cUrl);

    if (dwCode >= 30000 && dwCode <= 30999)
      string = g_localizeStringsTemp.Get(dwCode).c_str();
    else if (dwCode >= 32000 && dwCode <= 32999)
      string = g_localizeStringsTemp.Get(dwCode).c_str();
    else
      string = g_localizeStrings.Get(dwCode).c_str();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();
  }
  else
    string = "";

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::GetSkinDir()
{
  CStdString string = g_guiSettings.GetString("lookandfeel.skin");
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::UnknownToUTF8(const char *sourceDest)
{
  CStdString string;
  if (sourceDest != NULL)
    g_charsetConverter.unknownToUTF8(sourceDest, string);
  else
    string = "";
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::GetLanguage()
{
  CStdString string = g_guiSettings.GetString("locale.language");
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::GetIPAddress()
{
  CStdString string = g_infoManager.GetLabel(NETWORK_IP_ADDRESS);
  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

int CAddonUtils::GetDVDState()
{
  return CIoSupport::GetTrayState();
}

int CAddonUtils::GetFreeMem()
{
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  return stat.dwAvailPhys  / ( 1024 * 1024 );
}

char* CAddonUtils::GetInfoLabel(const char *infotag)
{
  CStdString string;

  if (infotag != NULL)
  {
    int ret = g_infoManager.TranslateString(infotag);
    string = g_infoManager.GetLabel(ret);
  }
  else
    string = "";

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::GetInfoImage(const char *infotag)
{
  CStdString string;

  if (infotag != NULL)
  {
    int ret = g_infoManager.TranslateString(infotag);
    string = g_infoManager.GetImage(ret, WINDOW_INVALID);
  }
  else
    string = "";

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

bool CAddonUtils::GetCondVisibility(const char *condition)
{
  if (condition == NULL)
    return false;

  DWORD dwId = g_windowManager.GetTopMostModalDialogID();
  if (dwId == WINDOW_INVALID) dwId = g_windowManager.GetActiveWindow();

  int ret = g_infoManager.TranslateString(condition);
  return g_infoManager.GetBool(ret,dwId);
}

bool CAddonUtils::CreateDirectory(const char *dir)
{
  if (!CDirectory::Exists(dir))
  {
    if (!CUtil::CreateDirectoryEx(dir))
    {
      return false;
    }
  }

  return true;
}

void CAddonUtils::EnableNavSounds(bool yesNo)
{
  g_audioManager.Enable(yesNo);
}

void CAddonUtils::PlaySFX(const char *filename)
{
  if (filename == NULL)
    return;

  if (CFile::Exists(filename))
  {
    g_audioManager.PlayPythonSound(filename);
  }
}

int CAddonUtils::GetGlobalIdleTime()
{
  return g_application.GlobalIdleTime();
}

char* CAddonUtils::GetCacheThumbName(const char *path)
{
  CStdString strPath;

  if (path != NULL)
  {
    string strText = path;

    Crc32 crc;
    strPath;
    crc.ComputeFromLowerCase(strText);
    strPath.Format("%08x.tbn", (unsigned __int32)crc);
  }
  else
    strPath = "";

  char *buffer = (char*) malloc (strPath.length()+1);
  strcpy(buffer, strPath.c_str());
  return buffer;
}

char* CAddonUtils::MakeLegalFilename(const char *filename)
{
  CStdString string;

  if (filename != NULL)
    string = CUtil::MakeLegalPath(filename);
  else
    string = "";

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::TranslatePath(const char *path)
{
  CStdString string;

  if (path != NULL)
    string = CSpecialProtocol::TranslatePath(path);
  else
    string = "";

  char *buffer = (char*) malloc (string.length()+1);
  strcpy(buffer, string.c_str());
  return buffer;
}

char* CAddonUtils::GetRegion(int id)
{
  CStdString result;

  if (id == 0)      // datelong
    result = g_langInfo.GetDateFormat(true);
  else if (id == 1) // dateshort
    result = g_langInfo.GetDateFormat(false);
  else if (id == 2) // tempunit
    result = g_langInfo.GetTempUnitString();
  else if (id == 3) // speedunit
    result = g_langInfo.GetSpeedUnitString();
  else if (id == 4) // time
    result = g_langInfo.GetTimeFormat();
  else if (id == 5) // meridiem
    result.Format("%s/%s", g_langInfo.GetMeridiemSymbol(CLangInfo::MERIDIEM_SYMBOL_AM), g_langInfo.GetMeridiemSymbol(CLangInfo::MERIDIEM_SYMBOL_PM));

  char *buffer = (char*) malloc (result.length()+1);
  strcpy(buffer, result.c_str());
  return buffer;
}

char* CAddonUtils::GetSupportedMedia(int media)
{
  CStdString result;
  if (media == 0)
    result = g_stSettings.m_videoExtensions;
  else if (media == 1)
    result = g_stSettings.m_musicExtensions;
  else if (media == 2)
    result = g_stSettings.m_pictureExtensions;

  char *buffer = (char*) malloc (result.length()+1);
  strcpy(buffer, result.c_str());
  return buffer;
}

bool CAddonUtils::SkinHasImage(const char *filename)
{
  return g_TextureManager.HasTexture(filename);
}

void CAddonUtils::FreeDemuxPacket(demux_packet* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket((DemuxPacket*) pPacket);
}

demux_packet* CAddonUtils::AllocateDemuxPacket(int iDataSize)
{
  return (demux_packet*) CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

/**
* XBMC AddOn Dialog callbacks
* Helper functions to access GUI Dialog functions
*/

bool CAddonUtils::OpenDialogOK(const char* heading, const char* line1, const char* line2, const char* line3)
{
  const DWORD dWindow = WINDOW_DIALOG_OK;
  CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(dWindow);
  if (!pDialog) return false;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->IsConfirmed();
}

bool CAddonUtils::OpenDialogYesNo(const char* heading, const char* line1, const char* line2,
                             const char* line3, const char* nolabel, const char* yeslabel)
{
  const DWORD dWindow = WINDOW_DIALOG_YES_NO;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(dWindow);
  if (!pDialog) return false;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  if (nolabel != NULL )
    pDialog->SetChoice(0,nolabel);

  if (yeslabel != NULL )
    pDialog->SetChoice(1,yeslabel);

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->IsConfirmed();
}

char* CAddonUtils::OpenDialogBrowse(int type, const char* heading, const char* shares, const char* mask, bool useThumbs, bool treatAsFolder, const char* default_folder)
{
  CStdString value;
  CStdString type_mask = mask;

  if (treatAsFolder && !type_mask.size() == 0)
    type_mask += "|.rar|.zip";

  VECSOURCES *shares_type = g_settings.GetSourcesFromType(shares);
  if (shares_type)
  {
    value = default_folder;
    if (type == 1)
      CGUIDialogFileBrowser::ShowAndGetFile(*shares_type, type_mask, heading, value, useThumbs, treatAsFolder);
    else if (type == 2)
      CGUIDialogFileBrowser::ShowAndGetImage(*shares_type, heading, value);
    else
      CGUIDialogFileBrowser::ShowAndGetDirectory(*shares_type, heading, value, type != 0);
  }
  else
    value = "";

  char *buffer = (char*) malloc (value.length()+1);
  strcpy(buffer, value.c_str());
  return buffer;
}

char* CAddonUtils::OpenDialogNumeric(int type, const char* heading, const char* default_value)
{
  CStdString value;
  SYSTEMTIME timedate;
  GetLocalTime(&timedate);

  if (heading)
  {
    if (type == 1)
    {
      if (default_value && strlen(default_value) == 10)
      {
        CStdString sDefault = default_value;
        timedate.wDay = atoi(sDefault.Left(2));
        timedate.wMonth = atoi(sDefault.Mid(3,4));
        timedate.wYear = atoi(sDefault.Right(4));
      }
      if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
        value.Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
      else
        value = default_value;
    }
    else if (type == 2)
    {
      if (default_value && strlen(default_value) == 5)
      {
        CStdString sDefault = default_value;
        timedate.wHour = atoi(sDefault.Left(2));
        timedate.wMinute = atoi(sDefault.Right(2));
      }
      if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
        value.Format("%2d:%02d", timedate.wHour, timedate.wMinute);
      else
        value = default_value;
    }
    else if (type == 3)
    {
      value = default_value;
      CGUIDialogNumeric::ShowAndGetIPAddress(value, heading);
    }
    else
    {
      value = default_value;
      CGUIDialogNumeric::ShowAndGetNumber(value, heading);
    }
  }
  char *buffer = (char*) malloc (value.length()+1);
  strcpy(buffer, value.c_str());
  return buffer;
}

char* CAddonUtils::OpenDialogKeyboard(const char* heading, const char* default_value, bool hidden)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return NULL;

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  if (heading != NULL)
    pKeyboard->SetHeading(heading);
  else
    pKeyboard->SetHeading("");
  pKeyboard->SetHiddenInput(hidden);
  if (default_value != NULL)
    pKeyboard->SetText(default_value);
  else
    pKeyboard->SetText("");

  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    CStdString TextString = pKeyboard->GetText();
    if (TextString.IsEmpty())
      return NULL;

    char *buffer = (char*) malloc (TextString.length()+1);
    strcpy(buffer, TextString.c_str());
    return buffer;
  }

  return NULL;
}

int CAddonUtils::OpenDialogSelect(const char* heading, addon_string_list_s* list)
{
  const DWORD dWindow = WINDOW_DIALOG_SELECT;
  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(dWindow);
  if (!pDialog) return NULL;

  pDialog->Reset();

  if (heading != NULL)
    pDialog->SetHeading(heading);

  const char *listLine = NULL;
  for(int i = 0; i < list->Items; i++)
  {
    listLine = list->Strings[i];
    if (listLine)
      pDialog->Add(listLine);
  }

  //send message and wait for user input
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);

  return pDialog->GetSelectedLabel();
}

bool CAddonUtils::ProgressDialogCreate(const char* heading, const char* line1, const char* line2, const char* line3)
{
  CGUIDialogProgress* pDialog= (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return true;

  if (heading != NULL ) pDialog->SetHeading(heading);
  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  pDialog->StartModal();

  return false;
}

void CAddonUtils::ProgressDialogUpdate(int percent, const char* line1, const char* line2, const char* line3)
{
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return;

  if (percent >= 0 && percent <= 100)
  {
    pDialog->SetPercentage(percent);
    pDialog->ShowProgressBar(true);
  }
  else
  {
    pDialog->ShowProgressBar(false);
  }

  if (line1 != NULL )   pDialog->SetLine(0, line1);
  if (line2 != NULL )   pDialog->SetLine(1, line2);
  if (line3 != NULL )   pDialog->SetLine(2, line3);

  return;
}

bool CAddonUtils::ProgressDialogIsCanceled()
{
  bool canceled = false;
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return canceled;

  canceled = pDialog->IsCanceled();

  return canceled;
}

void CAddonUtils::ProgressDialogClose()
{
  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!pDialog) return;

  pDialog->Close();

  return;
}


/**
 * XBMC AddOn GUI callbacks
 * Helper to access different types of GUI functions
 */

void CAddonUtils::GUILock()
{
  if (m_iGUILockRef == 0) g_graphicsContext.Lock();
    m_iGUILockRef++;
}

void CAddonUtils::GUIUnlock()
{
  if (m_iGUILockRef > 0)
  {
    m_iGUILockRef--;
    if (m_iGUILockRef == 0) g_graphicsContext.Unlock();
  }
}

int CAddonUtils::GUIGetCurrentWindowId()
{
  GUILock();
  DWORD dwId = g_windowManager.GetActiveWindow();
  GUIUnlock();
  return dwId;
}

int CAddonUtils::GUIGetCurrentWindowDialogId()
{
  GUILock();
  DWORD dwId = g_windowManager.GetTopMostModalDialogID();
  GUIUnlock();
  return dwId;
}

}

CStdString CAddonUtils::GetAddonTypeName(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      return "MULTITYPE";
    case ADDON_VIZ:
      return "VIZ";
    case ADDON_SKIN:
      return "SKIN";
    case ADDON_PVRDLL:
      return "PVRDLL";
    case ADDON_SCRIPT:
      return "SCRIPT";
    case ADDON_SCRAPER_PVR:
      return "SCRAPER_PVR";
    case ADDON_SCRAPER_VIDEO:
      return "SCRAPER_VIDEO";
    case ADDON_SCRAPER_MUSIC:
      return "SCRAPER_MUSIC";
    case ADDON_SCRAPER_PROGRAM:
      return "SCRAPER_PROGRAM";
    case ADDON_SCREENSAVER:
      return "SCREENSAVER";
    case ADDON_PLUGIN_PVR:
      return "PLUGIN_PVR";
    case ADDON_PLUGIN_MUSIC:
      return "PLUGIN_MUSIC";
    case ADDON_PLUGIN_VIDEO:
      return "PLUGIN_VIDEO";
    case ADDON_PLUGIN_PROGRAM:
      return "PLUGIN_PROGRAM";
    case ADDON_PLUGIN_PICTURES:
      return "PLUGIN_PICTURES";
    case ADDON_PLUGIN_WEATHER:
      return "PLUGIN_WEATHER";
    case ADDON_DSP_AUDIO:
      return "DSP_AUDIO";
    case ADDON_UNKNOWN:
    default:
      return "unknown";
  }
}

} /* namespace ADDON */
