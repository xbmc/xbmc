/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"

namespace XFILE
{

  class CBlurayFile : public COverrideFile
  {
  public:
    CBlurayFile();
    ~CBlurayFile() override;

    bool Exists(const CURL& url) override;

  protected:
    std::string TranslatePath(const CURL& url) override;
  };
}
