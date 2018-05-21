/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "LibraryLoader.h"
#include <string.h>
#include <stdlib.h>
#include "utils/log.h"

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
  CLog::Log(LOGWARNING, "%s - Unable to resolve %lu in dll %s", __FUNCTION__, ordinal, GetName());
  return 0;
}
