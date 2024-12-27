/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

class CFileItem;
class CFileItemList;

namespace tinyxml2
{
class XMLElement;
}

namespace XFILE
{
  class CDAVDirectory : public IDirectory
  {
    public:
      CDAVDirectory(void);
      ~CDAVDirectory(void) override;
      bool GetDirectory(const CURL& url, CFileItemList &items) override;
      bool Create(const CURL& url) override;
      bool Exists(const CURL& url) override;
      bool Remove(const CURL& url) override;
      CacheType GetCacheType(const CURL& url) const override { return CacheType::ONCE; }

    private:
      void ParseResponse(const tinyxml2::XMLElement* element, CFileItem& item);
  };
}
