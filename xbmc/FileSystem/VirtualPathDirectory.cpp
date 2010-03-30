/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "VirtualPathDirectory.h"
#include "Directory.h"
#include "Settings.h"
#include "Util.h"
#include "URL.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace std;
using namespace XFILE;

// virtualpath://type/sourcename

CVirtualPathDirectory::CVirtualPathDirectory()
{}

CVirtualPathDirectory::~CVirtualPathDirectory()
{}

bool CVirtualPathDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CVirtualPathDirectory::GetDirectory(%s)", strPath.c_str());

  CMediaSource share;
  if (!GetMatchingSource(strPath, share))
    return false;

  unsigned int progressTime = CTimeUtils::GetTimeMS() + 3000L;   // 3 seconds before showing progress bar
  CGUIDialogProgress* dlgProgress = NULL;

  unsigned int iFailures = 0;
  for (int i = 0; i < (int)share.vecPaths.size(); ++i)
  {
    // show the progress dialog if we have passed our time limit
    if (CTimeUtils::GetTimeMS() > progressTime && !dlgProgress)
    {
      dlgProgress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dlgProgress)
      {
        dlgProgress->SetHeading(15310);
        dlgProgress->SetLine(0, 15311);
        dlgProgress->SetLine(1, "");
        dlgProgress->SetLine(2, "");
        dlgProgress->StartModal();
        dlgProgress->ShowProgressBar(true);
        dlgProgress->SetProgressMax((int)share.vecPaths.size()*2);
        dlgProgress->Progress();
      }
    }
    if (dlgProgress)
    {
      CURL url(share.vecPaths[i]);
      dlgProgress->SetLine(1, url.GetWithoutUserDetails());
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }

    CFileItemList tempItems;
    CLog::Log(LOGDEBUG,"Getting Directory (%s)", share.vecPaths[i].c_str());
    if (CDirectory::GetDirectory(share.vecPaths[i], tempItems, m_strFileMask, m_useFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo, true))
      items.Append(tempItems);
    else
    {
      CLog::Log(LOGERROR,"Error Getting Directory (%s)", share.vecPaths[i].c_str());
      iFailures++;
    }

    if (dlgProgress)
    {
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }
  }

  if (dlgProgress)
    dlgProgress->Close();

  if (iFailures == share.vecPaths.size())
    return false;

  return true;
}

bool CVirtualPathDirectory::Exists(const CStdString& strPath)
{
  CLog::Log(LOGDEBUG,"Testing Existence (%s)", strPath.c_str());
  CMediaSource share;
  if (!GetMatchingSource(strPath, share))
    return false;

  unsigned int iFailures = 0;
  for (int i = 0; i < (int)share.vecPaths.size(); ++i)
  {
    CLog::Log(LOGDEBUG,"Testing Existence (%s)", share.vecPaths[i].c_str());
    if (!CDirectory::Exists(share.vecPaths[i]))
    {
      CLog::Log(LOGERROR,"Error Testing Existence (%s)", share.vecPaths[i].c_str());
      iFailures++;
    }
  }

  if (iFailures == share.vecPaths.size())
    return false;

  return true;
}

bool CVirtualPathDirectory::GetPathes(const CStdString& strPath, vector<CStdString>& vecPaths)
{
  CMediaSource share;
  if (!GetMatchingSource(strPath, share))
    return false;
  vecPaths = share.vecPaths;
  return true;
}

bool CVirtualPathDirectory::GetTypeAndSource(const CStdString& strPath, CStdString& strType, CStdString& strSource)
{
  // format: virtualpath://type/sourcename
  CStdString strTemp = strPath;
  CUtil::RemoveSlashAtEnd(strTemp);
  CStdString strTest = "virtualpath://";
  if (strTemp.Left(strTest.length()).Equals(strTest))
  {
    strTemp = strTemp.Mid(strTest.length());
    int iPos = strTemp.Find('/');
    if (iPos < 1)
      return false;
    strType = strTemp.Mid(0, iPos);
    strSource = strTemp.Mid(iPos + 1);
    //CLog::Log(LOGDEBUG,"CVirtualPathDirectory::GetTypeAndSource(%s) = [%s],[%s]", strPath.c_str(), strType.c_str(), strSource.c_str());
    return true;
  }
  return false;
}

bool CVirtualPathDirectory::GetMatchingSource(const CStdString &strPath, CMediaSource& share)
{
  CStdString strType, strSource;
  if (!GetTypeAndSource(strPath, strType, strSource))
    return false;

  // no support for "files" operation
  if (strType == "files")
    return false;
  VECSOURCES *VECSOURCES = g_settings.GetSourcesFromType(strType);
  if (!VECSOURCES)
    return false;

  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(strSource, *VECSOURCES, bIsSourceName);
  if (!bIsSourceName)
    return false;
  if (iIndex < 0 || iIndex >= (int)VECSOURCES->size())
    return false;

  share = (*VECSOURCES)[iIndex];
  return true;
}

