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

#include "threads/SystemClock.h"
#include "MultiPathDirectory.h"
#include "Directory.h"
#include "Util.h"
#include "URL.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;

//
// multipath://{path1}/{path2}/{path3}/.../{path-N}
//
// unlike the older virtualpath:// protocol, sub-folders are combined together into a new
// multipath:// style url.
//

CMultiPathDirectory::CMultiPathDirectory()
{}

CMultiPathDirectory::~CMultiPathDirectory()
{}

bool CMultiPathDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CMultiPathDirectory::GetDirectory(%s)", url.GetRedacted().c_str());

  vector<std::string> vecPaths;
  if (!GetPaths(url, vecPaths))
    return false;

  XbmcThreads::EndTime progressTime(3000); // 3 seconds before showing progress bar
  CGUIDialogProgress* dlgProgress = NULL;

  unsigned int iFailures = 0;
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    // show the progress dialog if we have passed our time limit
    if (progressTime.IsTimePast() && !dlgProgress)
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
        dlgProgress->SetProgressMax((int)vecPaths.size()*2);
        dlgProgress->Progress();
      }
    }
    if (dlgProgress)
    {
      CURL url(vecPaths[i]);
      dlgProgress->SetLine(1, url.GetWithoutUserDetails());
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }

    CFileItemList tempItems;
    CLog::Log(LOGDEBUG,"Getting Directory (%s)", vecPaths[i].c_str());
    if (CDirectory::GetDirectory(vecPaths[i], tempItems, m_strFileMask, m_flags))
      items.Append(tempItems);
    else
    {
      CLog::Log(LOGERROR,"Error Getting Directory (%s)", vecPaths[i].c_str());
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

  if (iFailures == vecPaths.size())
    return false;

  // merge like-named folders into a sub multipath:// style url
  MergeItems(items);

  return true;
}

bool CMultiPathDirectory::Exists(const CURL& url)
{
  CLog::Log(LOGDEBUG,"Testing Existence (%s)", url.GetRedacted().c_str());

  vector<std::string> vecPaths;
  if (!GetPaths(url, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    CLog::Log(LOGDEBUG,"Testing Existence (%s)", vecPaths[i].c_str());
    if (CDirectory::Exists(vecPaths[i]))
      return true;
  }
  return false;
}

bool CMultiPathDirectory::Remove(const CURL& url)
{
  vector<std::string> vecPaths;
  if (!GetPaths(url, vecPaths))
    return false;

  bool success = false;
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    if (CDirectory::Remove(vecPaths[i]))
      success = true;
  }
  return success;
}

std::string CMultiPathDirectory::GetFirstPath(const std::string &strPath)
{
  size_t pos = strPath.find("/", 12);
  if (pos != std::string::npos)
    return CURL::Decode(strPath.substr(12, pos - 12));
  return "";
}

bool CMultiPathDirectory::GetPaths(const CURL& url, vector<std::string>& vecPaths)
{
  const std::string pathToUrl(url.Get());
  return GetPaths(pathToUrl, vecPaths);
}

bool CMultiPathDirectory::GetPaths(const std::string& path, std::vector<std::string>& paths)
{
  paths.clear();

  // remove multipath:// from path and any trailing / (so that the last path doesn't get any more than it originally had)
  std::string path1 = path.substr(12);
  path1.erase(path1.find_last_not_of('/')+1);

  // split on "/"
  vector<string> temp = StringUtils::Split(path1, '/');
  if (temp.size() == 0)
    return false;

  // URL decode each item
  paths.resize(temp.size());
  std::transform(temp.begin(), temp.end(), paths.begin(), CURL::Decode);
  return true;
}

bool CMultiPathDirectory::HasPath(const std::string& strPath, const std::string& strPathToFind)
{
  // remove multipath:// from path and any trailing / (so that the last path doesn't get any more than it originally had)
  std::string strPath1 = strPath.substr(12);
  URIUtils::RemoveSlashAtEnd(strPath1);

  // split on "/"
  vector<string> vecTemp = StringUtils::Split(strPath1, '/');
  if (vecTemp.empty())
    return false;

  // check each item
  for (unsigned int i = 0; i < vecTemp.size(); i++)
  {
    if (CURL::Decode(vecTemp[i]) == strPathToFind)
      return true;
  }
  return false;
}

std::string CMultiPathDirectory::ConstructMultiPath(const CFileItemList& items, const vector<int> &stack)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  std::string newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  for (unsigned int i = 0; i < stack.size(); ++i)
    AddToMultiPath(newPath, items[stack[i]]->GetPath());

  //CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
  return newPath;
}

void CMultiPathDirectory::AddToMultiPath(std::string& strMultiPath, const std::string& strPath)
{
  URIUtils::AddSlashAtEnd(strMultiPath);
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  strMultiPath += CURL::Encode(strPath);
  strMultiPath += "/";
}

std::string CMultiPathDirectory::ConstructMultiPath(const vector<string> &vecPaths)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  std::string newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  for (vector<string>::const_iterator path = vecPaths.begin(); path != vecPaths.end(); ++path)
    AddToMultiPath(newPath, *path);
  //CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
  return newPath;
}

std::string CMultiPathDirectory::ConstructMultiPath(const set<string> &setPaths)
{
  std::string newPath = "multipath://";
  for (set<string>::const_iterator path = setPaths.begin(); path != setPaths.end(); ++path)
    AddToMultiPath(newPath, *path);

  return newPath;
}

void CMultiPathDirectory::MergeItems(CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i", (int)items.Size());
  unsigned int time = XbmcThreads::SystemClockMillis();
  if (items.Size() == 0)
    return;
  // sort items by label
  // folders are before files in this sort method
  items.Sort(SortByLabel, SortOrderAscending);
  int i = 0;

  // if first item in the sorted list is a file, just abort
  if (!items.Get(i)->m_bIsFolder)
    return;

  while (i + 1 < items.Size())
  {
    // there are no more folders left, so exit the loop
    CFileItemPtr pItem1 = items.Get(i);
    if (!pItem1->m_bIsFolder)
      break;

    vector<int> stack;
    stack.push_back(i);
    CLog::Log(LOGDEBUG,"Testing path: [%03i] %s", i, pItem1->GetPath().c_str());

    int j = i + 1;
    do
    {
      CFileItemPtr pItem2 = items.Get(j);
      if (pItem2->GetLabel() != pItem1->GetLabel())
        break;

      // ignore any filefolders which may coincidently have
      // the same label as a true folder
      if (!pItem2->IsFileFolder())
      {
        stack.push_back(j);
        CLog::Log(LOGDEBUG,"  Adding path: [%03i] %s", j, pItem2->GetPath().c_str());
      }
      j++;
    }
    while (j < items.Size());

    // do we have anything to combine?
    if (stack.size() > 1)
    {
      // we have a multipath so remove the items and add the new item
      std::string newPath = ConstructMultiPath(items, stack);
      for (unsigned int k = stack.size() - 1; k > 0; --k)
        items.Remove(stack[k]);
      pItem1->SetPath(newPath);
      CLog::Log(LOGDEBUG,"  New path: %s", pItem1->GetPath().c_str());
    }

    i++;
  }

  CLog::Log(LOGDEBUG,
            "CMultiPathDirectory::MergeItems, items = %i,  took %d ms",
            items.Size(), XbmcThreads::SystemClockMillis() - time);
}

bool CMultiPathDirectory::SupportsWriteFileOperations(const std::string &strPath)
{
  vector<std::string> paths;
  GetPaths(strPath, paths);
  for (unsigned int i = 0; i < paths.size(); ++i)
    if (CUtil::SupportsWriteFileOperations(paths[i]))
      return true;
  return false;
}
