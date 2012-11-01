/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"

#if defined(TARGET_ANDROID)
#include "AndroidAppDirectory.h"
#include "xbmc/android/activity/XBMCApp.h"
#include "FileItem.h"
#include "File.h"
#include "utils/URIUtils.h"
#include <vector>
#include "utils/log.h"
#include "URL.h"

using namespace XFILE;
using namespace std;

CAndroidAppDirectory::CAndroidAppDirectory(void)
{
}

CAndroidAppDirectory::~CAndroidAppDirectory(void)
{
}

bool CAndroidAppDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString dirname = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(dirname);
  CLog::Log(LOGDEBUG, "CAndroidAppDirectory::GetDirectory: %s",dirname.c_str()); 
  if (dirname == "apps")
  {
    vector<androidPackage> applications;
    CXBMCApp::ListApplications(&applications);
    if (!applications.size())
    {
      CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Application lookup listing failed");
      return false;
    }
    for(unsigned int i = 0; i < applications.size(); i++)
    {
      if (applications[i].packageName == "org.xbmc.xbmc")
        continue;
      CFileItemPtr pItem(new CFileItem(applications[i].packageName));
      pItem->m_bIsFolder = false;
      CStdString path;
      path.Format("androidapp://%s/%s/%s", url.GetHostName(), dirname,  applications[i].packageName);
      pItem->SetPath(path);
      pItem->SetLabel(applications[i].packageLabel);
      pItem->SetArt("thumb", path+".png");
      items.Add(pItem);
    }
    return true;
  }

  CLog::Log(LOGERROR, "CAndroidAppDirectory::GetDirectory Failed to open %s",strPath.c_str());
  return false;
}

bool CAndroidAppDirectory::IsAllowed(const CStdString& strFile) const
{
  // Entries are virtual, so we want them all.
  return true;
}

#endif
