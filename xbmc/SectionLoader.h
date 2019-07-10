/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <string>
#include <vector>

//  forward
class LibraryLoader;

class CSectionLoader
{
public:
  class CDll
  {
  public:
    std::string m_strDllName;
    long m_lReferenceCount;
    LibraryLoader *m_pDll;
    unsigned int m_unloadDelayStartTick;
    bool m_bDelayUnload;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static LibraryLoader* LoadDLL(const std::string& strSection, bool bDelayUnload=true, bool bLoadSymbols=false);
  static void UnloadDLL(const std::string& strSection);
  static void UnloadDelayed();
  void UnloadAll();

protected:
  std::vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;

};

extern  CSectionLoader g_sectionLoader;

