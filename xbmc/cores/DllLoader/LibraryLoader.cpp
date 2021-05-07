/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryLoader.h"

#include "utils/log.h"

#include <stdlib.h>
#include <string.h>

LibraryLoader::LibraryLoader(const std::string& libraryFile):
  m_fileName(libraryFile)
{
  size_t pos = m_fileName.find_last_of("\\/");
  if (pos != std::string::npos)
    m_path = m_fileName.substr(0, pos);

  m_iRefCount = 1;
}

LibraryLoader::~LibraryLoader() = default;

const char *LibraryLoader::GetName() const
{
  size_t pos = m_fileName.find_last_of('/');
  if (pos != std::string::npos)
    return &m_fileName.at(pos + 1); // don't include /
  return m_fileName.c_str();
}

const char *LibraryLoader::GetFileName() const
{
  return m_fileName.c_str();
}

const char *LibraryLoader::GetPath() const
{
  return m_path.c_str();
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
  CLog::Log(LOGWARNING, "{} - Unable to resolve {} in dll {}", __FUNCTION__, ordinal, GetName());
  return 0;
}
