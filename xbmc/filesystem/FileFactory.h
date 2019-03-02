/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// FileFactory1.h: interface for the CFileFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "IFile.h"
#include <string>

namespace XFILE
{
class CFileFactory
{
public:
  CFileFactory();
  virtual ~CFileFactory();
  static IFile* CreateLoader(const std::string& strFileName);
  static IFile* CreateLoader(const CURL& url);
};
}
