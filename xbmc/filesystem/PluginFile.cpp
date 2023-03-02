/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PluginFile.h"

#include "URL.h"

using namespace XFILE;

CPluginFile::CPluginFile(void) : COverrideFile(false)
{
}

CPluginFile::~CPluginFile(void) = default;

bool CPluginFile::Open(const CURL& url)
{
  return false;
}

bool CPluginFile::Exists(const CURL& url)
{
  return true;
}

int CPluginFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

int CPluginFile::Stat(struct __stat64* buffer)
{
  return -1;
}

bool CPluginFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return false;
}

bool CPluginFile::Delete(const CURL& url)
{
  return false;
}

bool CPluginFile::Rename(const CURL& url, const CURL& urlnew)
{
  return false;
}

std::string CPluginFile::TranslatePath(const CURL& url)
{
  return url.Get();
}
