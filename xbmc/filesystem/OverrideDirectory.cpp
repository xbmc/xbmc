/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OverrideDirectory.h"
#include "URL.h"
#include "filesystem/Directory.h"

using namespace XFILE;


COverrideDirectory::COverrideDirectory() = default;


COverrideDirectory::~COverrideDirectory() = default;

bool COverrideDirectory::Create(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Create(translatedPath.c_str());
}

bool COverrideDirectory::Remove(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Remove(translatedPath.c_str());
}

bool COverrideDirectory::Exists(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Exists(translatedPath.c_str());
}
