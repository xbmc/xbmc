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

#include "LibraryLoader.h"
#include <string.h>
#include <stdlib.h>
#include "utils/log.h"

LibraryLoader::LibraryLoader(const char* libraryFile)
{
  m_sFileName = strdup(libraryFile);

  char* sPath = strrchr(m_sFileName, '\\');
  if (!sPath) sPath = strrchr(m_sFileName, '/');
  if (sPath)
  {
    sPath++;
    m_sPath=(char*)malloc(sPath - m_sFileName+1);
    strncpy(m_sPath, m_sFileName, sPath - m_sFileName);
    m_sPath[sPath - m_sFileName] = 0;
  }
  else
    m_sPath=NULL;

  m_iRefCount = 1;
}

LibraryLoader::~LibraryLoader()
{
  free(m_sFileName);
  free(m_sPath);
}

char* LibraryLoader::GetName()
{
  if (m_sFileName)
  {
    char* sName = strrchr(m_sFileName, '/');
    if (sName) return sName + 1;
    else return m_sFileName;
  }
  return (char*)"";
}

char* LibraryLoader::GetFileName()
{
  if (m_sFileName) return m_sFileName;
  return (char*)"";
}

char* LibraryLoader::GetPath()
{
  if (m_sPath) return m_sPath;
  return (char*)"";
}

int LibraryLoader::IncrRef()
{
  m_iRefCount++;
  return m_iRefCount;
}

int LibraryLoader::DecrRef()
{
  m_iRefCount--;
  return m_iRefCount;
}

int LibraryLoader::ResolveOrdinal(unsigned long ordinal, void** ptr)
{
  CLog::Log(LOGWARNING, "%s - Unable to resolve %lu in dll %s", __FUNCTION__, ordinal, GetName());
  return 0;
}
