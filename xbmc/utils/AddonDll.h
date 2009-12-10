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
#include "../DllAddon.h"
//#include "../addons/include/libaddon.h"
#include "GUIDialogSettings.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "log.h"

using namespace XFILE;
using namespace DIRECTORY;


namespace ADDON
{
  template<class TheDll, typename TheStruct, typename Props>
  class CAddonDll : public CAddon
  {
  public:
    CAddonDll(const AddonProps &props);
    virtual ~CAddonDll();
    AddonPtr Clone() const;
//    virtual ADDON_STATUS GetStatus();
//    virtual void Remove();
//
//    // addon settings
//    virtual bool HasSettings();
//    virtual bool LoadSettings();
//    virtual void SaveSettings();
//    virtual void SaveFromDefault();
//    virtual CStdString GetSetting(const CStdString& key);
//
    bool Create();
//    void Destroy();

  protected:
    bool LoadDll();
    TheStruct* m_pStruct;
    Props*      m_pInfo;
    bool m_initialized;

  private:
    TheDll* m_pDll;
//    virtual ADDON_STATUS TransferSettings();
//    TiXmlElement MakeSetting(addon_setting_t setting) const;
//
//    static void AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg);
//    static bool AddOnGetSetting(void *userData, const char *settingName, void *settingValue);
//    static void AddOnOpenSettings(const char *url, bool bReload);
//    static void AddOnOpenOwnSettings(void *userData, bool bReload);
//    static const char* AddOnGetAddonDirectory(void *userData);
//    static const char* AddOnGetUserDirectory(void *userData);
  };

template<class TheDll, typename TheStruct, typename Props>
CAddonDll<TheDll, TheStruct, Props>::CAddonDll(const AddonProps &props)
  : CAddon(props)
{
  m_pStruct     = NULL;
  m_initialized = false;
  m_pDll        = NULL;
  m_pInfo       = NULL;
}

template<class TheDll, typename TheStruct, typename Props>
CAddonDll<TheDll, TheStruct, Props>::~CAddonDll()
{
//  if (m_initialized)
//    Destroy();
}

template<class TheDll, typename TheStruct, typename Props>
AddonPtr CAddonDll<TheDll, TheStruct, Props>::Clone() const
{
  return AddonPtr(new CAddonDll<TheDll, TheStruct, Props>(*this));
}

template<class TheDll, typename TheStruct, typename Props>
bool CAddonDll<TheDll, TheStruct, Props>::LoadDll()
{
  /* Determine the path to the Dll */
  CStdString strFileName;
  if (Parent().IsEmpty())
  {
    strFileName = Path() + LibName();
  }
  else
  {
    CStdString extension = CUtil::GetExtension(LibName());
    strFileName = "special://temp/" + LibName();
    CUtil::RemoveExtension(strFileName);
    strFileName += "-" + UUID() + extension;

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
    //TODO report the problem and disable this addon
    //m_callbacks->AddOn.ReportStatus(this, STATUS_UNKNOWN, "Can't load Dll");
    // can't disable internally, must be done by addonmanager
    /*m_disabled = true;*/
    return false;
  }
  m_pStruct = (TheStruct*)malloc(sizeof(TheStruct));
  ZeroMemory(m_pStruct, sizeof(TheStruct));
  m_pDll->GetAddon(m_pStruct);
  return (m_pStruct != NULL);
}

template<class TheDll, typename TheStruct, typename Props>
bool CAddonDll<TheDll, TheStruct, Props>::Create()
{
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());

  if (!LoadDll())
    return false;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
//  try
//  {
//    ADDON_STATUS status = m_pDll->Create(NULL, m_pInfo);
//    if (status != STATUS_OK)
//      throw status;
    m_initialized = true;
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "ADDON: Dll %s - exception '%s' during Create occurred, contact Developer '%s' of this AddOn", Name().c_str(), e.what(), Author().c_str());
//    m_initialized = false;
//  }
//  catch (ADDON_STATUS status)
//  {
//    if (status == STATUS_NEED_SETTINGS)
//    { // catch request for settings in initalization
//      if (TransferSettings() == STATUS_OK)
//        m_initialized = true;
//    }
//    else
//    { // Addon failed initialization
//      m_initialized = false;
//      CLog::Log(LOGERROR, "ADDON: Dll %s - Client returned bad status (%i) from Create and is not usable", Name().c_str(), status);
//    }
//
//    ///* Delete is performed by the calling class */
//    //new CAddonStatusHandler(this, status, "", false);
//  }

  return m_initialized;
}

//template<class TheDll, typename TheStruct, typename Props>
//void CAddonDll<TheDll, TheStruct, Props>::Destroy()
//{
//  delete m_pStruct;
//  m_pStruct = NULL;
//  delete m_pDll;
//  m_pDll = NULL;
//  m_initialized = false;
//  CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//void CAddonDll<TheDll, TheStruct, Props>::Remove()
//{
//  /* Unload library file */
//  try
//  {
//    m_pDll->Unload();
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "ADDON: %s - exception '%s' during Remove occurred, contact Developer '%s' of this AddOn", Name().c_str(), e.what(), Author().c_str());
//  }
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//ADDON_STATUS CAddonDll<TheDll, TheStruct, Props>::GetStatus()
//{
//  try
//  {
//    return m_pDll->GetStatus();
//  }
//  catch (std::exception &e)
//  {
//    m_initialized = false;
//    CLog::Log(LOGERROR, "ADDON: %s - exception '%s' during GetStatus occurred, contact Developer '%s' of this AddOn", Name().c_str(), e.what(), Author().c_str());
//  }
//  return STATUS_UNKNOWN;
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//bool CAddonDll<TheDll, TheStruct, Props>::HasSettings()
//{
//  if (!LoadDll())
//    return false;
//
//  try
//  {
//    return m_pDll->HasSettings();
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "ADDON:: %s - exception '%s' during HasSettings occurred, contact Developer '%s' of this AddOn", Name().c_str(), e.what(), Author().c_str());
//    return false;
//  }
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//bool CAddonDll<TheDll, TheStruct, Props>::LoadSettings()
//{
//  if (!LoadDll())
//    return false;
//
//  addon_settings_t settings = NULL;
//  try
//  {
//    settings = m_pDll->GetSettings();
//  }
//  catch (std::exception &e)
//  {
//    CLog::Log(LOGERROR, "ADDON:: %s - exception '%s' during GetSettings occurred, contact Developer '%s' of this AddOn", Name().c_str(), e.what(), Author().c_str());
//    return false;
//  }
//
//  if (settings)
//  {
//    // regenerate XML doc
//    m_addonXmlDoc.Clear();
//    TiXmlElement node("settings");
//    m_addonXmlDoc.InsertEndChild(node);
//
//    int count = addon_settings_get_count(settings);
//    addon_setting_t setting;
//    for (int i=0; i < count; i++)
//    {
//       setting = addon_settings_get_item(settings, i);
//       if (!setting)
//         return false;
//
//       m_addonXmlDoc.RootElement()->InsertEndChild(MakeSetting(setting));
//       addon_release(setting);
//    }
//  }
//  else
//    return false;
//
//  return CAddon::LoadUserSettings();
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//TiXmlElement CAddonDll<TheDll, TheStruct, Props>::MakeSetting(addon_setting_t setting) const
//{
//  TiXmlElement node("setting");
//  addon_setting_type_t type = addon_setting_type(setting);
//  CStdString id = addon_setting_id(setting);
//  CStdString label = addon_setting_label(setting);
//  CStdString lvalues = addon_setting_lvalues(setting);
//
//  switch (type)
//  {
//  case SETTING_BOOL:
//    {
//      node.SetAttribute("id", id.c_str());
//      node.SetAttribute("type", "bool");
//      node.SetAttribute("label", label.c_str());
//      break;
//    }
//  case SETTING_ENUM:
//    {
//      node.SetAttribute("id", id.c_str());
//      node.SetAttribute("type", "enum");
//      node.SetAttribute("label", label.c_str());
//      node.SetAttribute("lvalues", lvalues.c_str());
//      break;
//    }
//  default:
//    break;
//  }
//
//  return node;
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//void CAddonDll<TheDll, TheStruct, Props>::SaveSettings()
//{
//  // must save first, as TransferSettings() reloads saved settings!
//  CAddon::SaveSettings();
//  TransferSettings();
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//void CAddonDll<TheDll, TheStruct, Props>::SaveFromDefault()
//{
//  CAddon::SaveFromDefault();
//  TransferSettings();
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//CStdString CAddonDll<TheDll, TheStruct, Props>::GetSetting(const CStdString& key)
//{
//  return CAddon::GetSetting(key);
//}
//
//template<class TheDll, typename TheStruct, typename Props>
//ADDON_STATUS CAddonDll<TheDll, TheStruct, Props>::TransferSettings()
//{
//  bool restart = false;
//  ADDON_STATUS reportStatus = STATUS_OK;
//
//  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());
//
//  if (!LoadUserSettings())
//    return STATUS_MISSING_FILE;
//
//  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
//  while (setting)
//  {
//    ADDON_STATUS status = STATUS_OK;
//    const char *id = setting->Attribute("id");
//    const char *type = setting->Attribute("type");
//
//    if (type)
//    {
//      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
//        strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
//        strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
//        strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
//        strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
//      {
//        status = m_pDll->SetSetting(id, (const char*) GetSetting(id).c_str());
//      }
//      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
//        strcmpi(type, "labelenum") == 0)
//      {
//        int tmp = atoi(GetSetting(id));
//        status = m_pDll->SetSetting(id, (int*) &tmp);
//      }
//      else if (strcmpi(type, "bool") == 0)
//      {
//        bool tmp = (GetSetting(id) == "true") ? true : false;
//        status = m_pDll->SetSetting(id, (bool*) &tmp);
//      }
//      else
//      {
//        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, Name().c_str());
//      }
//
//      if (status == STATUS_NEED_RESTART)
//        restart = true;
//      else if (status != STATUS_OK)
//        reportStatus = status;
//    }
//    setting = setting->NextSiblingElement("setting");
//  }
//
//  if (restart || reportStatus != STATUS_OK)
//  {
//    //new CAddonStatusHandler(this, restart ? STATUS_NEED_RESTART : reportStatus, "", true);
//  }
//
//  return STATUS_OK;
//}

}; /* namespace ADDON */

