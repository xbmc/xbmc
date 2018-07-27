/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideDirectory.h"

namespace XFILE
{
  class CResourceDirectory : public COverrideDirectory
  {
  public:
    CResourceDirectory();
    ~CResourceDirectory() override;

    bool GetDirectory(const CURL& url, CFileItemList &items) override;

  protected:
    std::string TranslatePath(const CURL &url) override;
  };
}
