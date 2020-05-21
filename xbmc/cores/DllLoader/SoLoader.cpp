/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SoLoader.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

#include <dlfcn.h>

SoLoader::SoLoader(const std::string &so, bool bGlobal) : LibraryLoader(so)
{
  m_soHandle = NULL;
  m_bGlobal = bGlobal;
  m_bLoaded = false;
}

SoLoader::~SoLoader()
{
  if (m_bLoaded)
    Unload();
}

bool SoLoader::Load()
{
  if (m_soHandle != NULL)
    return true;

  std::string strFileName= CSpecialProtocol::TranslatePath(GetFileName());
  if (strFileName == "xbmc.so")
  {
    CLog::Log(LOGDEBUG, "Loading Internal Library");
    m_soHandle = RTLD_DEFAULT;
  }
  else
  {
    CLog::Log(LOGDEBUG, "Loading: %s", strFileName.c_str());
    int flags = RTLD_LAZY;
    m_soHandle = dlopen(strFileName.c_str(), flags);
    if (!m_soHandle)
    {
      CLog::Log(LOGERROR, "Unable to load %s, reason: %s", strFileName.c_str(), dlerror());
      return false;
    }
  }
  m_bLoaded = true;
  return true;
}

void SoLoader::Unload()
{

  if (m_soHandle)
  {
    if (dlclose(m_soHandle) != 0)
       CLog::Log(LOGERROR, "Unable to unload %s, reason: %s", GetName(), dlerror());
  }
  m_bLoaded = false;
  m_soHandle = NULL;
}

int SoLoader::ResolveExport(const char* symbol, void** f, bool logging)
{
  if (!m_bLoaded && !Load())
  {
    if (logging)
      CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: so not loaded", GetName(), symbol);
    return 0;
  }

  void* s = dlsym(m_soHandle, symbol);
  if (!s)
  {
    if (logging)
      CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: %s", GetName(), symbol, dlerror());
    return 0;
  }

  *f = s;
  return 1;
}

bool SoLoader::IsSystemDll()
{
  return false;
}

HMODULE SoLoader::GetHModule()
{
  return m_soHandle;
}

bool SoLoader::HasSymbols()
{
  return false;
}
