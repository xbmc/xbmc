/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultiPathDirectory.h"

#include "Directory.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace XFILE;

using namespace std::chrono_literals;

//
// multipath://{path1}/{path2}/{path3}/.../{path-N}
//
// unlike the older virtualpath:// protocol, sub-folders are combined together into a new
// multipath:// style url.
//

CMultiPathDirectory::CMultiPathDirectory() = default;

CMultiPathDirectory::~CMultiPathDirectory() = default;

bool CMultiPathDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CMultiPathDirectory::GetDirectory({})", url.GetRedacted());

  std::vector<std::string> vecPaths;
  if (!GetPaths(url, vecPaths))
    return false;

  XbmcThreads::EndTime<> progressTime(3000ms); // 3 seconds before showing progress bar
  CGUIDialogProgress* dlgProgress = NULL;

  unsigned int iFailures = 0;
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    // show the progress dialog if we have passed our time limit
    if (progressTime.IsTimePast() && !dlgProgress)
    {
      dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (dlgProgress)
      {
        dlgProgress->SetHeading(CVariant{15310});
        dlgProgress->SetLine(0, CVariant{15311});
        dlgProgress->SetLine(1, CVariant{""});
        dlgProgress->SetLine(2, CVariant{""});
        dlgProgress->Open();
        dlgProgress->ShowProgressBar(true);
        dlgProgress->SetProgressMax((int)vecPaths.size()*2);
        dlgProgress->Progress();
      }
    }
    if (dlgProgress)
    {
      CURL url(vecPaths[i]);
      dlgProgress->SetLine(1, CVariant{url.GetWithoutUserDetails()});
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }

    CFileItemList tempItems;
    CLog::Log(LOGDEBUG, "Getting Directory ({})", CURL::GetRedacted(vecPaths[i]));
    if (CDirectory::GetDirectory(vecPaths[i], tempItems, m_strFileMask, m_flags))
      items.Append(tempItems);
    else
    {
      CLog::Log(LOGERROR, "Error Getting Directory ({})", CURL::GetRedacted(vecPaths[i]));
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
  CLog::Log(LOGDEBUG, "Testing Existence ({})", url.GetRedacted());

  std::vector<std::string> vecPaths;
  if (!GetPaths(url, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    CLog::Log(LOGDEBUG, "Testing Existence ({})", CURL::GetRedacted(vecPaths[i]));
    if (CDirectory::Exists(vecPaths[i]))
      return true;
  }
  return false;
}

bool CMultiPathDirectory::Remove(const CURL& url)
{
  std::vector<std::string> vecPaths;
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
  size_t pos = strPath.find('/', 12);
  if (pos != std::string::npos)
    return CURL::Decode(strPath.substr(12, pos - 12));
  return "";
}

bool CMultiPathDirectory::GetPaths(const CURL& url, std::vector<std::string>& vecPaths)
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
  std::vector<std::string> temp = StringUtils::Split(path1, '/');
  if (temp.empty())
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
  std::vector<std::string> vecTemp = StringUtils::Split(strPath1, '/');
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

std::string CMultiPathDirectory::ConstructMultiPath(const CFileItemList& items, const std::vector<int> &stack)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  std::string newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: {}", strPath);
  for (unsigned int i = 0; i < stack.size(); ++i)
    AddToMultiPath(newPath, items[stack[i]]->GetPath());

  //CLog::Log(LOGDEBUG, "Final path: {}", newPath);
  return newPath;
}

void CMultiPathDirectory::AddToMultiPath(std::string& strMultiPath, const std::string& strPath)
{
  URIUtils::AddSlashAtEnd(strMultiPath);
  //CLog::Log(LOGDEBUG, "-- adding path: {}", strPath);
  strMultiPath += CURL::Encode(strPath);
  strMultiPath += "/";
}

std::string CMultiPathDirectory::ConstructMultiPath(const std::vector<std::string> &vecPaths)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  std::string newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: {}", strPath);
  for (std::vector<std::string>::const_iterator path = vecPaths.begin(); path != vecPaths.end(); ++path)
    AddToMultiPath(newPath, *path);
  //CLog::Log(LOGDEBUG, "Final path: {}", newPath);
  return newPath;
}

std::string CMultiPathDirectory::ConstructMultiPath(const std::set<std::string> &setPaths)
{
  std::string newPath = "multipath://";
  for (const std::string& path : setPaths)
    AddToMultiPath(newPath, path);

  return newPath;
}

void CMultiPathDirectory::MergeItems(CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = {}", items.Size());
  auto start = std::chrono::steady_clock::now();
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

    std::vector<int> stack;
    stack.push_back(i);
    CLog::Log(LOGDEBUG, "Testing path: [{:03}] {}", i, CURL::GetRedacted(pItem1->GetPath()));

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
        CLog::Log(LOGDEBUG, "  Adding path: [{:03}] {}", j, CURL::GetRedacted(pItem2->GetPath()));
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
      CLog::Log(LOGDEBUG, "  New path: {}", CURL::GetRedacted(pItem1->GetPath()));
    }

    i++;
  }

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = {},  took {} ms", items.Size(),
            duration.count());
}

bool CMultiPathDirectory::SupportsWriteFileOperations(const std::string &strPath)
{
  std::vector<std::string> paths;
  GetPaths(strPath, paths);
  for (unsigned int i = 0; i < paths.size(); ++i)
    if (CUtil::SupportsWriteFileOperations(paths[i]))
      return true;
  return false;
}
