#pragma once
#ifndef ADDON_DLL_H
#define ADDON_DLL_H

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
#include "log.h"

namespace ADDON
{
  template<class T>
  class CAddonDll : public CAddon
  {
  public:
    CAddonDll();
    virtual ~CAddonDll() {}

    virtual void Remove();
    virtual bool HasSettings();
    virtual bool GetSettings();
    virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);
    virtual ADDON_STATUS GetStatus();

  private:
    T* m_pDll;
  };

template<class T>
inline void CAddonDll<T>::Remove()
{
  /* Unload library file */
  try
  {
    m_pDll->Unload();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "ADDON: %s - exception '%s' during Remove occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
  }
}

template<typename T>
ADDON_STATUS CAddonDll<T>::GetStatus()
{
  try
  {
    return m_pDll->GetStatus();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "ADDON: %s - exception '%s' during GetStatus occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
  }
  return STATUS_UNKNOWN;
}

template<typename T>
bool CAddonDll<T>::HasSettings()
{
  try
  {
    return m_pDll->HasSettings();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "ADDON:: %s - exception '%s' during HasSettings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
    return false;
  }
}

template<typename T>
bool CAddonDll<T>::GetSettings()
{
  try
  {
    return m_pDll->GetSettings();
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "ADDON:: %s - exception '%s' during GetSettings occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
    return NULL;
  }
}

template<typename T>
ADDON_STATUS CAddonDll<T>::SetSetting(const char *settingName, const void *settingValue)
{
  try
  {
    return m_pDll->SetSetting(settingName, settingValue);
  }
  catch (std::exception &e)
  {
    CLog::Log(LOGERROR, "ADDON:: %s - exception '%s' during SetSetting occurred, contact Developer '%s' of this AddOn", m_strName.c_str(), e.what(), m_strCreator.c_str());
    return STATUS_UNKNOWN;
  }
}

}; /* namespace ADDON */
#endif /* ADDON_DLL_H */

