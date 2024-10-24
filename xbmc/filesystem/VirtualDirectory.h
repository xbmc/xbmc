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
    void SetSources(const std::vector<CMediaSource>& sources);
    inline unsigned int GetNumberOfSources() { return static_cast<uint32_t>(m_sources.size()); }

    bool IsSource(const std::string& strPath,
                  std::vector<CMediaSource>* sources = NULL,
                  std::string* name = NULL) const;
    bool IsInSource(const std::string& strPath) const;

    inline const CMediaSource& operator[](const int index) const { return m_sources[index]; }

    inline CMediaSource& operator[](const int index) { return m_sources[index]; }

    void GetSources(std::vector<CMediaSource>& sources) const;

    void AllowNonLocalSources(bool allow) { m_allowNonLocalSources = allow; }

    std::shared_ptr<IDirectory> GetDirImpl() { return m_pDir; }
    void ReleaseDirImpl() { m_pDir.reset(); }

  protected:
    void CacheThumbs(CFileItemList &items);

    std::vector<CMediaSource> m_sources;
    bool m_allowNonLocalSources;
    std::shared_ptr<IDirectory> m_pDir;
  };
}
