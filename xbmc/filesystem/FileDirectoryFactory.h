/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include <string>

class CFileItem;

namespace XFILE
{
class CFileDirectoryFactory
{
public:
  CFileDirectoryFactory(void);
  virtual ~CFileDirectoryFactory(void);
  static IFileDirectory* Create(const CURL& url, CFileItem* pItem, const std::string& strMask="");
};
}
