/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidAppDirectory.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

#include <vector>

using namespace XFILE;

bool CAndroidAppDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string dirname = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(dirname);
  CLog::Log(LOGDEBUG, "CAndroidAppDirectory::GetDirectory: {}", dirname);
  std::string appName = CCompileInfo::GetAppName();
  StringUtils::ToLower(appName);
  std::string className = CCompileInfo::GetPackage();

  if (dirname == "apps")
  {
    const std::vector<androidPackage> applications = CXBMCApp::Get().GetApplications();
    if (applications.empty())
    {
      CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Application lookup listing failed");
      return false;
    }
    for (auto& i : applications)
    {
      if (i.packageName == className.c_str())
        continue;
      CFileItemPtr pItem(new CFileItem(i.packageName));
      pItem->m_bIsFolder = false;
      std::string path =
          StringUtils::Format("androidapp://{}/{}/{}", url.GetHostName(), dirname, i.packageName);
      pItem->SetPath(path);
      pItem->SetLabel(i.packageLabel);
      pItem->SetArt("thumb", path+".png");
      pItem->m_dwSize = -1;  // No size
      items.Add(pItem);
    }
    return true;
  }

  CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Failed to open {}", url.Get());
  return false;
}
