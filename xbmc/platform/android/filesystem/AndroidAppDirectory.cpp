/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_ANDROID)
#include "AndroidAppDirectory.h"
#include "platform/android/activity/XBMCApp.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include <vector>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "CompileInfo.h"

using namespace XFILE;

CAndroidAppDirectory::CAndroidAppDirectory(void)
{
}

CAndroidAppDirectory::~CAndroidAppDirectory(void)
{
}

bool CAndroidAppDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string dirname = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(dirname);
  CLog::Log(LOGDEBUG, "CAndroidAppDirectory::GetDirectory: %s",dirname.c_str());
  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string className = CCompileInfo::GetPackage();

  if (dirname == "apps")
  {
    std::vector<androidPackage> applications = CXBMCApp::GetApplications();
    if (applications.empty())
    {
      CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Application lookup listing failed");
      return false;
    }
    for(std::vector<androidPackage>::iterator i = applications.begin(); i != applications.end(); ++i)
    {
      if ((*i).packageName == className.c_str())
        continue;
      CFileItemPtr pItem(new CFileItem((*i).packageName));
      pItem->m_bIsFolder = false;
      std::string path = StringUtils::Format("androidapp://%s/%s/%s", url.GetHostName().c_str(), dirname.c_str(), (*i).packageName.c_str());
      pItem->SetPath(path);
      pItem->SetLabel((*i).packageLabel);
      pItem->SetArt("thumb", path+".png");
      pItem->m_dwSize = -1;  // No size
      items.Add(pItem);
    }
    return true;
  }

  CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Failed to open %s", url.Get().c_str());
  return false;
}

#endif
