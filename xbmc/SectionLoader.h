#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "utils/StdString.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"

//  forward
class LibraryLoader;

class CSectionLoader
{
public:
  class CSection
  {
  public:
    CStdString m_strSectionName;
    long m_lReferenceCount;
    unsigned int m_unloadDelayStartTick;
  };
  class CDll
  {
  public:
    CStdString m_strDllName;
    long m_lReferenceCount;
    LibraryLoader *m_pDll;
    unsigned int m_unloadDelayStartTick;
    bool m_bDelayUnload;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
  static LibraryLoader* LoadDLL(const CStdString& strSection, bool bDelayUnload=true, bool bLoadSymbols=false);
  static void UnloadDLL(const CStdString& strSection);
  static void UnloadDelayed();
  static void UnloadAll();
protected:
  std::vector<CSection> m_vecLoadedSections;
  typedef std::vector<CSection>::iterator ivecLoadedSections;
  std::vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;
};

XBMC_GLOBAL_REF(CSectionLoader,g_sectionLoader);

