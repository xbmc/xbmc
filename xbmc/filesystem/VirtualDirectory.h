/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
