/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "NFSFile.h"

#include <memory>
#include <vector>

class CFileItem;
struct nfsdirent;

namespace XFILE
{
  class CNFSDirectory : public IDirectory
  {
    public:
      CNFSDirectory(void);
      ~CNFSDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      CacheType GetCacheType(const CURL& url) const override { return CacheType::ONCE; }
      bool Create(const CURL& url) override;
      bool Exists(const CURL& url) override;
      bool Remove(const CURL& url) override;
    private:
      std::vector<std::shared_ptr<CFileItem>> GetServerList() const;
      std::vector<std::shared_ptr<CFileItem>> GetDirectoryFromExportList(
          const CURL& inputURL) const;
  };
}

