/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPnPFile.h"

#include "FileFactory.h"
#include "FileItem.h"
#include "UPnPDirectory.h"
#include "URL.h"

using namespace XFILE;

CUPnPFile::CUPnPFile() = default;

CUPnPFile::~CUPnPFile() = default;

bool CUPnPFile::Open(const CURL& url)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());
    CURL *pNewUrl = new CURL(item_new.GetPath());
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;
  }
  return false;
}

int CUPnPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());
    CURL *pNewUrl = new CURL(item_new.GetPath());
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;
  }
  return -1;
}

bool CUPnPFile::Exists(const CURL& url)
{
  CFileItem item_new;
  if (CUPnPDirectory::GetResource(url, item_new))
  {
    //CLog::Log(LOGDEBUG,"FileUPnP - file redirect to %s.", item_new.GetPath().c_str());
    IFile *pNewImp = CFileFactory::CreateLoader(item_new.GetPath());
    CURL *pNewUrl = new CURL(item_new.GetPath());
    if (pNewImp)
    {
      throw new CRedirectException(pNewImp, pNewUrl);
    }
    delete pNewUrl;
  }
  return false;
}
