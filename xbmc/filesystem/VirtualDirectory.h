/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "IDirectory.h"
#include "MediaSource.h"

#include <memory>
#include <string>

namespace XFILE
{

  /*!
  \ingroup windows
  \brief Get access to shares and it's directories.
  */
  class CVirtualDirectory : public IDirectory
  {
  public:
    CVirtualDirectory(void);
    ~CVirtualDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    void CancelDirectory() override;
    bool GetDirectory(const CURL& url, CFileItemList &items, bool bUseFileDirectories, bool keepImpl);
    void SetSources(const VECSOURCES& vecSources);
    inline unsigned int GetNumberOfSources()
    {
      return static_cast<uint32_t>(m_vecSources.size());
    }

    bool IsSource(const std::string& strPath, VECSOURCES *sources = NULL, std::string *name = NULL) const;
    bool IsInSource(const std::string& strPath) const;

    inline const CMediaSource& operator [](const int index) const
    {
      return m_vecSources[index];
    }

    inline CMediaSource& operator[](const int index)
    {
      return m_vecSources[index];
    }

    void GetSources(VECSOURCES &sources) const;

    void AllowNonLocalSources(bool allow) { m_allowNonLocalSources = allow; };

    std::shared_ptr<IDirectory> GetDirImpl() { return m_pDir; }
    void ReleaseDirImpl() { m_pDir.reset(); }

  protected:
    void CacheThumbs(CFileItemList &items);

    VECSOURCES m_vecSources;
    bool m_allowNonLocalSources;
    std::shared_ptr<IDirectory> m_pDir;
  };
}
