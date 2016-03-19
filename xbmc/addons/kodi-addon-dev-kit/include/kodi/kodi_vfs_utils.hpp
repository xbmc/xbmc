/*
 *      Copyright (C) 2015-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/*!
 * @file kodi_vfs_utils.hpp - C++ wrappers for Kodi's VFS operations
 */

#include "libXBMC_addon.h"
#include "kodi_vfs_types.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace ADDON
{
  class CVFSDirEntry
  {
  public:
    CVFSDirEntry(const std::string& label = "",
                 const std::string& path = "",
                 bool bFolder = false,
                 int64_t size = -1) :
      m_label(label),
      m_path(path),
      m_bFolder(bFolder),
      m_size(size)
    {
    }

    CVFSDirEntry(const VFSDirEntry& dirEntry) :
      m_label(dirEntry.label ? dirEntry.label : ""),
      m_path(dirEntry.path ? dirEntry.path : ""),
      m_bFolder(dirEntry.folder),
      m_size(dirEntry.size)
    {
    }

    const std::string& Label(void) const { return m_label; }
    const std::string& Path(void) const { return m_path; }
    bool IsFolder(void) const { return m_bFolder; }
    int64_t Size(void) const { return m_size; }

    void SetLabel(const std::string& label) { m_label = label; }
    void SetPath(const std::string& path) { m_path = path; }
    void SetFolder(bool bFolder) { m_bFolder = bFolder; }
    void SetSize(int64_t size) { m_size = size; }

  private:
    std::string m_label;
    std::string m_path;
    bool m_bFolder;
    int64_t m_size;
  };

  class VFSUtils
  {
  public:
    static bool GetDirectory(ADDON::CHelper_libXBMC_addon* frontend,
                             const std::string& path,
                             const std::string& mask,
                             std::vector<CVFSDirEntry>& items)
    {
      VFSDirEntry* dir_list = nullptr;
      unsigned int num_items = 0;
      if (frontend->GetDirectory(path.c_str(), mask.c_str(), &dir_list, &num_items))
      {
        for (unsigned int i = 0; i < num_items; i++)
          items.push_back(CVFSDirEntry(dir_list[i]));

        frontend->FreeDirectory(dir_list, num_items);

        return true;
      }
      return false;
    }
  };
}
