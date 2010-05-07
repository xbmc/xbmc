#pragma once
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
#include "Addon.h"
#include "DllAddon.h"
#include "AddonManager.h"
#include "AddonStatusHandler.h"
#include "GUIDialogSettings.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "log.h"

using namespace XFILE;

namespace ADDON
{
  template<class TheDll, typename TheStruct, typename TheProps>
  class CAddonDll : public CAddon
  {
  public:
    CAddonDll(const AddonProps &props);
    CAddonDll(cp_plugin_info_t *props);
    virtual ~CAddonDll();
    AddonPtr Clone() const;
    virtual ADDON_STATUS GetStatus();

    // addon settings
    virtual bool HasSettings();
    virtual bool LoadSettings();
    virtual void SaveSettings();
    virtual void SaveFromDefault();
    virtual CStdString GetSetting(const CStdString& key);

    bool Create();
    virtual void Stop();
    void Destroy();

  protected:
    void HandleException(std::exception &e, const char* context);
    bool Initialized() { return m_initialized; }
    virtual void BuildLibName(cp_plugin_info_t *props = NULL) {}
    TheStruct* m_pStruct;
    TheProps*     m_pInfo;

  private:
    TheDll* m_pDll;
    bool m_initialized;
    bool LoadDll();

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
CAddonDll<TheDll, TheStruct, TheProps>::CAddonDll(cp_plugin_info_t *props)
  : CAddon(props)
{
#if defined(_LINUX) && !defined(__APPLE__)
  m_strLibName = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library_linux");
#elif defined(_WIN32) && defined(HAS_SDL_OPENGL)
  m_strLibName = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library_wingl");
#elif defined(_WIN32) && defined(HAS_DX)
  m_strLibName = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library_windx");
#elif defined(__APPLE__)
  m_strLibName = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library_osx");
#elif defined(_XBOX)
  m_strLibName = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library_xbox");
#endif

  m_pStruct     = NULL;
  m_initialized = false;
  m_pDll        = NULL;
  m_pInfo       = NULL;
}

template<class TheDll, typename TheStruct, typename TheProps>
CAddonDll<TheDll, TheStruct, TheProps>::CAddonDll(const AddonProps &props)
  : CAddon(props)
{
  m_pStruct     = NULL;
  m_initialized = false;
  m_pDll        = NULL;
  m_pInfo       = NULL;
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
    strFileName = CUtil::AddFileToFolder(Path(), LibName());
  }
  else
  { //FIXME hack to load same Dll twice
    CStdString extension = CUtil::GetExtension(LibName());
    strFileName = "special://temp/" + LibName();
    CUtil::RemoveExtension(strFileName);
    strFileName += "-" + ID() + extension;

    if (!CFile::Exists(strFileName))
      CFile::Cache(Path() + LibName(), strFileName);

    CLog::Log(LOGNOTICE, "ADDON: Loaded virtual child addon %s", strFileName.c_str());
  }

  /* Load the Dll */
  m_pDll = new TheDll;
  m_pDll->SetFile(strFileName);
  m_pDll->EnableDelayedUnload(false);
  if (!m_pDll->Load())
  {
    delete m_pDll;
    m_pDll = NULL;
    new CAddonStatusHandler(ID(), STATUS_UNKNOWN, "Can't load Dll", false);
    return false;
  }
  m_pStruct = (TheStruct*)malloc(sizeof(TheStruct));
  ZeroMemory(m_pStruct, sizeof(TheStruct));
  m_pDll->GetAddon(m_pStruct);
  return (m_pStruct != NULL);
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::Create()
{
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());
  m_initialized = false;

  if (!LoadDll())
    return false;

  try
  {
    ADDON_STATUS status = m_pDll->Create(NULL, m_pInfo);
    if (status != STATUS_OK)
      throw status;
    m_initialized = true;
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Create");
  }
  catch (ADDON_STATUS status)
  { 
    if (status == STATUS_NEED_SETTINGS)
    { // catch request for settings in initalization
      if (TransferSettings() == STATUS_OK)
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

  return m_initialized;
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::Stop()
{
  /* Inform dll to stop all activities */
  try
  {
    if (m_pDll) m_pDll->Stop();
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Stop");
  }
  CLog::Log(LOGINFO, "ADDON: Dll Stopped - %s", Name().c_str());
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
  delete m_pStruct;
  m_pStruct = NULL;
  delete m_pDll;
  m_pDll = NULL;
  m_initialized = false;
  CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
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
  return STATUS_UNKNOWN;
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::HasSettings()
{
  if (!LoadDll())
    return false;

  try
  {
    return m_pDll->HasSettings();
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->HasSettings()");
    return false;
  }
}

template<class TheDll, typename TheStruct, typename TheProps>
bool CAddonDll<TheDll, TheStruct, TheProps>::LoadSettings()
{
  if (!LoadDll())
    return false;

  StructSetting** sSet;
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
  }
  else
    return CAddon::LoadSettings();

  return CAddon::LoadUserSettings();
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
void CAddonDll<TheDll, TheStruct, TheProps>::SaveFromDefault()
{
  CAddon::SaveFromDefault();
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
  ADDON_STATUS reportStatus = STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());

  LoadUserSettings();

  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    ADDON_STATUS status = STATUS_OK;
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
        status = m_pDll->SetSetting(id, (const char*) GetSetting(id).c_str());
      }
      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
        strcmpi(type, "labelenum") == 0)
      {
        int tmp = atoi(GetSetting(id));
        status = m_pDll->SetSetting(id, (int*) &tmp);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        bool tmp = (GetSetting(id) == "true") ? true : false;
        status = m_pDll->SetSetting(id, (bool*) &tmp);
      }
      else
      {
        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, Name().c_str());
      }

      if (status == STATUS_NEED_RESTART)
        restart = true;
      else if (status != STATUS_OK)
        reportStatus = status;
    }
    setting = setting->NextSiblingElement("setting");
  }

  if (restart || reportStatus != STATUS_OK)
  {
    new CAddonStatusHandler(ID(), restart ? STATUS_NEED_RESTART : reportStatus, "", true);
  }

  return STATUS_OK;
}

template<class TheDll, typename TheStruct, typename TheProps>
void CAddonDll<TheDll, TheStruct, TheProps>::HandleException(std::exception &e, const char* context)
{
  m_initialized = false;
  m_pDll->Unload();
  CLog::Log(LOGERROR, "ADDON: Dll %s, throws an exception '%s' during %s. Contact developer '%s' with bug reports", Name().c_str(), e.what(), context, Author().c_str());
}

}; /* namespace ADDON */

