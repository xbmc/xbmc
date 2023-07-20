/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

namespace XFILE
{
class IFileDirectory : public IDirectory
{
public:
  ~IFileDirectory() override = default;
  virtual bool ContainsFiles(const CURL& url)=0;
};

}
