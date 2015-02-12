/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MultiPathFile.h"
#include "MultiPathDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"

using namespace std;
using namespace XFILE;

CMultiPathFile::CMultiPathFile(void)
  : COverrideFile(false)
{ }

CMultiPathFile::~CMultiPathFile(void)
{ }

bool CMultiPathFile::Open(const CURL& url)
{
  // grab the filename off the url
  std::string path, fileName;
  URIUtils::Split(url.Get(), path, fileName);
  vector<std::string> vecPaths;
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
  vector<std::string> vecPaths;
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
  vector<std::string> vecPaths;
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
