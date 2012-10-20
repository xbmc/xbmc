#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "Addon.h"
#include "DllAddon.h"
#include "AddonManager.h"
#include "AddonStatusHandler.h"
#include "AddonCallbacks.h"
#include "settings/GUIDialogSettings.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/Directory.h"
#include "utils/log.h"

using namespace XFILE;

namespace ADDON
{
  template<class TheDll, typename TheStruct, typename TheProps>
  class CAddonDll : public CAddon
  {
  public:
    CAddonDll(const AddonProps &props);
    CAddonDll(const cp_extension_t *ext);
    virtual ~CAddonDll();
    AddonPtr Clone() const;
    virtual ADDON_STATUS GetStatus();

    // addon settings
    virtual void SaveSettings();
    virtual CStdString GetSetting(const CStdString& key);

    ADDON_STATUS Create();
    virtual void Stop();
    void Destroy();

    bool DllLoaded(void) const;

  protected:
    void HandleException(std::exception &e, const char* context);
    bool Initialized() { return m_initialized; }
    virtual void BuildLibName(const cp_extension_t *ext = NULL) {}
    virtual bool LoadSettings();
    TheStruct* m_pStruct;
    TheProps*     m_pInfo;
    CAddonCallbacks* m_pHelpers;

  private:
    TheDll* m_pDll;
    bool m_initialized;
    bool LoadDll();
    bool m_needsavedsettings;

    virtual ADDON_STATUS TransferSettings();
    TiXmlElement MakeSetting(DllSetting& setting) const;

    static void AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg);
    static bool AddOnGetSetting(void *userData, const char *settingName, void *settingValue);
    static void AddOnOpenSettings(const char *url, bool bReload);
    static void AddOnOpenOwnSettings(void *userData, bool bReload);
    static const char* AddOnGetAddonDirectory(void *userData);
    static const char* AddOnGetUserDirectory(void *userData);
  };

template<class TheDll, typename TheStruct, typename TheProps>
CAddonDll<TheDll, TheStruct, TheProps>::CAddonDll(const cp_extension_t *ext)
  : CAddon(ext)
{
  // if library attribute isn't present, look for a system-dependent one
  if (ext && m_strLibName.IsEmpty())
  {
#if defined(TARGET_ANDROID)
  m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_android");
#elif defined(_LINUX) && !defined(TARGET_DARWIN)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_linux");
#elif defined(_WIN32) && defined(HAS_SDL_OPENGL)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_wingl");
#elif defined(_WIN32) && defined(HAS_DX)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_windx");
#elif defined(TARGET_DARWIN)
    m_strLibName = CAddonMgr::Get().GetExtValue(ext->configuration, "@library_osx");
#endif
  }

  m_pStruct     = NULL;
  m_initialized = false;
  m_pDll        = NULL;
  m_pInfo       = NULL;
  m_needsavedsettings = false;
}

template<class TheDll, typename TheStruct, typename TheProps>
CAddonDll<TheDll, TheStruct, TheProps>::CAddonDll(const AddonProps &props)
  : CAddon(props)
{
  m_pStruct     = NULL;
  m_initialized = false;
  m_pDll        = NULL;
  m_pInfo       = NULL;
  m_pHelpers    = NULL;
  m_needsavedsettings = false;
}

template<class TheDll, typename TheStruct, typename TheProps>
CAddonDll<TheDll, TheStruct, TheProps>::~CAddonDll()
{
  if (m_initialized)
    Destroy();
}

template<class TheDll, typename TheStruct, typename TheProps>
AddonPtr CAddonDll<TheDll, TheStruct, TheProps>::Clone() const
{
  return AddonPtr(new CAddonDll<TheDll, TheStruct, TheProps>(*this));
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::LoadDll()
{
  CStdString strFileName;
  if (!Parent())
  {
    strFileName = LibPath();
  }
  else
  { //FIXME hack to load same Dll twice
    CStdString extension = URIUtils::GetExtension(m_strLibName);
    strFileName = "special://temp/" + m_strLibName;
    URIUtils::RemoveExtension(strFileName);
    strFileName += "-" + ID() + extension;

    if (!CFile::Exists(strFileName))
      CFile::Cache(LibPath(), strFileName);

    CLog::Log(LOGNOTICE, "ADDON: Loaded virtual child addon %s", strFileName.c_str());
  }

  /* Check if lib being loaded exists, else check in XBMC binary location */
#if defined(TARGET_ANDROID)
  // Android libs MUST live in this path, else multi-arch will break.
  // The usual soname requirements apply. no subdirs, and filename is ^lib.*\.so$
  if (!CFile::Exists(strFileName))
  {
    CStdString tempbin = getenv("XBMC_ANDROID_LIBS");
    strFileName = tempbin + "/" + m_strLibName;
  }
#endif
  if (!CFile::Exists(strFileName))
  {
    CStdString temp = CSpecialProtocol::TranslatePath("special://xbmc/");
    CStdString tempbin = CSpecialProtocol::TranslatePath("special://xbmcbin/");
    strFileName.erase(0, temp.size());
    strFileName = tempbin + strFileName;
    if (!CFile::Exists(strFileName))
    {
      CLog::Log(LOGERROR, "ADDON: Could not locate %s", m_strLibName.c_str());
      return false;
    }
  }

  /* Load the Dll */
  m_pDll = new TheDll;
  m_pDll->SetFile(strFileName);
  m_pDll->EnableDelayedUnload(false);
  if (!m_pDll->Load())
  {
    delete m_pDll;
    m_pDll = NULL;
    new CAddonStatusHandler(ID(), ADDON_STATUS_UNKNOWN, "Can't load Dll", false);
    return false;
  }

  m_pStruct = (TheStruct*)malloc(sizeof(TheStruct));
  if (m_pStruct)
  {
    ZeroMemory(m_pStruct, sizeof(TheStruct));
    m_pDll->GetAddon(m_pStruct);
    return true;
  }

  return false;
}

template<class TheDll, typename TheStruct, typename TheProps>
ADDON_STATUS CAddonDll<TheDll, TheStruct, TheProps>::Create()
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());
  m_initialized = false;

  if (!LoadDll())
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Allocate the helper function class to allow crosstalk over
     helper libraries */
  m_pHelpers = new CAddonCallbacks(this);

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  try
  {
    status = m_pDll->Create(m_pHelpers->GetCallbacks(), m_pInfo);
    if (status == ADDON_STATUS_OK)
      m_initialized = true;
    else if ((status == ADDON_STATUS_NEED_SETTINGS) || (status == ADDON_STATUS_NEED_SAVEDSETTINGS))
    {
      m_needsavedsettings = (status == ADDON_STATUS_NEED_SAVEDSETTINGS);
      if ((status = TransferSettings()) == ADDON_STATUS_OK)
        m_initialized = true;
      else
        new CAddonStatusHandler(ID(), status, "", false);
    }
    else
    { // Addon failed initialization
      CLog::Log(LOGERROR, "ADDON: Dll %s - Client returned bad status (%i) from Create and is not usable", Name().c_str(), status);
      new CAddonStatusHandler(ID(), status, "", false);
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Create");
  }

  if (!m_initialized)
    SAFE_DELETE(m_pHelpers);

  return status;
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::Stop()
{
  /* Inform dll to stop all activities */
  try
  {
    if (m_needsavedsettings)  // If the addon supports it we save some settings to settings.xml before stop
    {
      char   str_id[64] = "";
      char   str_value[1024];
      CAddon::LoadUserSettings();
      for (unsigned int i=0; (strcmp(str_id,"###End") != 0); i++)
      {
        strcpy(str_id, "###GetSavedSettings");
        sprintf (str_value, "%i", i);
        ADDON_STATUS status = m_pDll->SetSetting((const char*)&str_id, (void*)&str_value);

        if (status == ADDON_STATUS_UNKNOWN)
          break;

        if (strcmp(str_id,"###End") != 0) UpdateSetting(str_id, str_value);
      }
      CAddon::SaveSettings();
    }
    if (m_pDll)
    {
      m_pDll->Stop();
      CLog::Log(LOGINFO, "ADDON: Dll Stopped - %s", Name().c_str());
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Stop");
  }
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::Destroy()
{
  /* Unload library file */
  try
  {
    if (m_pDll)
    {
      m_pDll->Destroy();
      m_pDll->Unload();
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Unload");
  }
  delete m_pHelpers;
  m_pHelpers = NULL;
  free(m_pStruct);
  m_pStruct = NULL;
  if (m_pDll)
  {
    delete m_pDll;
    m_pDll = NULL;
    CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
  }
  m_initialized = false;
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::DllLoaded(void) const
{
  return m_pDll != NULL;
}

template<class TheDll, typename TheStruct, typename TheProps>
ADDON_STATUS CAddonDll<TheDll, TheStruct, TheProps>::GetStatus()
{
  try
  {
    return m_pDll->GetStatus();
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->GetStatus()");
  }
  return ADDON_STATUS_UNKNOWN;
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::LoadSettings()
{
  if (m_settingsLoaded)
    return true;

  if (!LoadDll())
    return false;

  ADDON_StructSetting** sSet;
  std::vector<DllSetting> vSet;
  unsigned entries = 0;
  try
  {
    entries = m_pDll->GetSettings(&sSet);
    DllUtils::StructToVec(entries, &sSet, &vSet);
    m_pDll->FreeSettings();
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->GetSettings()");
    return false;
  }

  if (vSet.size())
  {
    // regenerate XML doc
    m_addonXmlDoc.Clear();
    TiXmlElement node("settings");
    m_addonXmlDoc.InsertEndChild(node);

    for (unsigned i=0; i < entries; i++)
    {
       DllSetting& setting = vSet[i];
       m_addonXmlDoc.RootElement()->InsertEndChild(MakeSetting(setting));
    }
    CAddon::SettingsFromXML(m_addonXmlDoc, true);
  }
  else
    return CAddon::LoadSettings();

  m_settingsLoaded = true;
  CAddon::LoadUserSettings();
  return true;
}

template<class TheDll, typename TheStruct, typename TheProps>
TiXmlElement CAddonDll<TheDll, TheStruct, TheProps>::MakeSetting(DllSetting& setting) const
{
  TiXmlElement node("setting");

  switch (setting.type)
  {
    case DllSetting::CHECK:
    {
      node.SetAttribute("id", setting.id);
      node.SetAttribute("type", "bool");
      node.SetAttribute("label", setting.label);
      break;
    }
    case DllSetting::SPIN:
    {
      node.SetAttribute("id", setting.id);
      node.SetAttribute("type", "enum");
      node.SetAttribute("label", setting.label);
      CStdString values;
      for (unsigned int i = 0; i < setting.entry.size(); i++)
      {
        values.append(setting.entry[i]);
        values.append("|");
      }
      node.SetAttribute("values", values.c_str());
      break;
    }
  default:
    break;
  }

  return node;
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::SaveSettings()
{
  // must save first, as TransferSettings() reloads saved settings!
  CAddon::SaveSettings();
  if (m_initialized)
    TransferSettings();
}

template<class TheDll, typename TheStruct, typename TheProps>
CStdString CAddonDll<TheDll, TheStruct, TheProps>::GetSetting(const CStdString& key)
{
  return CAddon::GetSetting(key);
}

template<class TheDll, typename TheStruct, typename TheProps>
ADDON_STATUS CAddonDll<TheDll, TheStruct, TheProps>::TransferSettings()
{
  bool restart = false;
  ADDON_STATUS reportStatus = ADDON_STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());

  LoadSettings();

  const TiXmlElement *category = m_addonXmlDoc.RootElement() ? m_addonXmlDoc.RootElement()->FirstChildElement("category") : NULL;
  if (!category)
    category = m_addonXmlDoc.RootElement(); // no categories

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      ADDON_STATUS status = ADDON_STATUS_OK;
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");
      const char *option = setting->Attribute("option");

      if (type)
      {
        if (strcmpi(type,"sep") == 0 || strcmpi(type,"lsep") == 0)
        {
          /* Don't propagate separators */
        }
        else if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
          strcmpi(type, "video") == 0 || strcmpi(type, "audio") == 0 ||
          strcmpi(type, "image") == 0 || strcmpi(type, "folder") == 0 ||
          strcmpi(type, "executable") == 0 || strcmpi(type, "file") == 0 ||
          strcmpi(type, "action") == 0 || strcmpi(type, "date") == 0 ||
          strcmpi(type, "time") == 0 || strcmpi(type, "select") == 0 ||
          strcmpi(type, "addon") == 0 || strcmpi(type, "labelenum") == 0 ||
          strcmpi(type, "fileenum") == 0 )
        {
          status = m_pDll->SetSetting(id, (const char*) GetSetting(id).c_str());
        }
        else if ((strcmpi(type, "enum") == 0 || strcmpi(type,"integer") == 0) ||
          strcmpi(type, "labelenum") == 0 || strcmpi(type, "rangeofnum") == 0)
        {
          int tmp = atoi(GetSetting(id));
          status = m_pDll->SetSetting(id, (int*) &tmp);
        }
        else if (strcmpi(type, "bool") == 0)
        {
          bool tmp = (GetSetting(id) == "true") ? true : false;
          status = m_pDll->SetSetting(id, (bool*) &tmp);
        }
        else if (strcmpi(type, "rangeofnum") == 0 || strcmpi(type, "slider") == 0 ||
                 strcmpi(type, "number") == 0)
        {
          float tmpf = (float)atof(GetSetting(id));
          int   tmpi;

          if (option && strcmpi(option,"int") == 0)
          {
            tmpi = (int)floor(tmpf);
            status = m_pDll->SetSetting(id, (int*) &tmpi);
          }
          else
          {
            status = m_pDll->SetSetting(id, (float*) &tmpf);
          }
        }
        else
        {
          /* Log unknowns as an error, but go ahead and transfer the string */
          CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, Name().c_str());
          status = m_pDll->SetSetting(id, (const char*) GetSetting(id).c_str());
        }

        if (status == ADDON_STATUS_NEED_RESTART)
          restart = true;
        else if (status != ADDON_STATUS_OK)
          reportStatus = status;
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }

  if (restart || reportStatus != ADDON_STATUS_OK)
  {
    new CAddonStatusHandler(ID(), restart ? ADDON_STATUS_NEED_RESTART : reportStatus, "", true);
  }

  return ADDON_STATUS_OK;
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::HandleException(std::exception &e, const char* context)
{
  m_initialized = false;
  m_pDll->Unload();
  CLog::Log(LOGERROR, "ADDON: Dll %s, throws an exception '%s' during %s. Contact developer '%s' with bug reports", Name().c_str(), e.what(), context, Author().c_str());
}

}; /* namespace ADDON */

