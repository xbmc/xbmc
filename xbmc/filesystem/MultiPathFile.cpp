/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultiPathFile.h"
#include "MultiPathDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"

using namespace XFILE;

CMultiPathFile::CMultiPathFile(void)
  : COverrideFile(false)
{ }

CMultiPathFile::~CMultiPathFile(void) = default;

bool CMultiPathFile::Open(const CURL& url)
{
  // grab the filename off the url
  std::string path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  std::vector<std::string> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    std::string filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    if (m_file.Open(filePath))
      return true;
  }
  return false;
}

bool CMultiPathFile::Exists(const CURL& url)
{
  // grab the filename off the url
  std::string path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  std::vector<std::string> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    std::string filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    if (CFile::Exists(filePath))
      return true;
  }
  return false;
}

int CMultiPathFile::Stat(const CURL& url, struct __stat64* buffer)
{
  // grab the filename off the url
  std::string path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  std::vector<std::string> vecPaths;
  if (!CMultiPathDirectory::GetPaths(path, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); i++)
  {
    std::string filePath = vecPaths[i];
    filePath = URIUtils::AddFileToFolder(filePath, fileName);
    int ret = CFile::Stat(filePath, buffer);
    if (ret == 0)
      return ret;
  }
  return -1;
}

std::string CMultiPathFile::TranslatePath(const CURL& url)
{
  return url.Get();
}
