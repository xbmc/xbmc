/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DynamicDll.h"

#include "SectionLoader.h"
#include "utils/FileUtils.h"
#include "utils/log.h"

DllDynamic::DllDynamic()
{
  m_dll=NULL;
  m_DelayUnload=true;
}

DllDynamic::DllDynamic(const std::string& strDllName):
  m_strDllName(strDllName)
{
  m_dll=NULL;
  m_DelayUnload=true;
}

DllDynamic::~DllDynamic()
{
  Unload();
}

bool DllDynamic::Load()
{
  if (m_dll)
    return true;

  if (!(m_dll=CSectionLoader::LoadDLL(m_strDllName, m_DelayUnload, LoadSymbols())))
    return false;

  if (!ResolveExports())
  {
    CLog::Log(LOGERROR, "Unable to resolve exports from dll {}", m_strDllName);
    Unload();
    return false;
  }

  return true;
}

void DllDynamic::Unload()
{
  if(m_dll)
    CSectionLoader::UnloadDLL(m_strDllName);
  m_dll=NULL;
}

bool DllDynamic::CanLoad()
{
  return CFileUtils::Exists(m_strDllName);
}

bool DllDynamic::EnableDelayedUnload(bool bOnOff)
{
  if (m_dll)
    return false;

  m_DelayUnload=bOnOff;

  return true;
}

bool DllDynamic::SetFile(const std::string& strDllName)
{
  if (m_dll)
    return false;

  m_strDllName=strDllName;
  return true;
}

