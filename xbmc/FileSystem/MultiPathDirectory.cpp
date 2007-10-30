
#include "stdafx.h"
#include "MultiPathDirectory.h"
#include "Directory.h"
#include "../Settings.h"
#include "../Util.h"

using namespace DIRECTORY;

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

bool CMultiPathDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CMultiPathDirectory::GetDirectory(%s)", strPath.c_str());

  vector<CStdString> vecPaths;
  if (!GetPaths(strPath, vecPaths))
    return false;

  DWORD progressTime = timeGetTime() + 3000L;   // 3 seconds before showing progress bar
  CGUIDialogProgress* dlgProgress = NULL;
  
  unsigned int iFailures = 0;
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    // show the progress dialog if we have passed our time limit
    if (timeGetTime() > progressTime && !dlgProgress)
    {
      dlgProgress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
      CStdString strStripped;
      url.GetURLWithoutUserDetails(strStripped);
      dlgProgress->SetLine(1, strStripped);
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }

    CFileItemList tempItems;
    CLog::Log(LOGDEBUG,"Getting Directory (%s)", vecPaths[i].c_str());
    if (CDirectory::GetDirectory(vecPaths[i], tempItems, m_strFileMask))
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

bool CMultiPathDirectory::Exists(const CStdString& strPath)
{
  CLog::Log(LOGDEBUG,"Testing Existance (%s)", strPath.c_str());

  vector<CStdString> vecPaths;
  if (!GetPaths(strPath, vecPaths))
    return false;

  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    CLog::Log(LOGDEBUG,"Testing Existance (%s)", vecPaths[i].c_str());
    if (CDirectory::Exists(vecPaths[i]))
      return true;
  }
  return false;
}

bool CMultiPathDirectory::Remove(const char* strPath)
{
  vector<CStdString> vecPaths;
  if (!GetPaths(strPath, vecPaths))
    return false;

  bool success = false;
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
  {
    if (CDirectory::Remove(vecPaths[i]))
      success = true;
  }
  return success;
}

CStdString CMultiPathDirectory::GetFirstPath(const CStdString &strPath)
{
  int pos = strPath.Find("/", 12);
  if (pos >= 0)
  {
    CStdString firstPath = strPath.Mid(12, pos - 12);
    CUtil::UrlDecode(firstPath);
    return firstPath;
  }
  return "";
}

bool CMultiPathDirectory::GetPaths(const CStdString& strPath, vector<CStdString>& vecPaths)
{
  vecPaths.empty();
  CStdString strPath1 = strPath;

  // remove multipath:// from path and any trailing / (so that the last path doesn't get any more than it originally had)
  strPath1 = strPath1.Mid(12);
  CUtil::RemoveSlashAtEnd(strPath1);

  // split on "/"
  vector<CStdString> vecTemp;
  StringUtils::SplitString(strPath1, "/", vecTemp);
  if (vecTemp.size() == 0)
    return false;

  // check each item
  for (unsigned int i = 0; i < vecTemp.size(); i++)
  {
    CStdString tempPath = vecTemp[i];
    CUtil::UrlDecode(tempPath);
    vecPaths.push_back(tempPath);
  }
  return true;
}

CStdString CMultiPathDirectory::ConstructMultiPath(const CFileItemList& items, const vector<int> &stack)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  CStdString newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  for (unsigned int i = 0; i < stack.size(); ++i)
    AddToMultiPath(newPath, items[stack[i]]->m_strPath);

  //CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
  return newPath;
}

void CMultiPathDirectory::AddToMultiPath(CStdString& strMultiPath, const CStdString& strPath)
{
  CStdString strPath1 = strPath;
  CUtil::AddSlashAtEnd(strMultiPath);
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  CUtil::URLEncode(strPath1);  
  strMultiPath += strPath1;
  strMultiPath += "/";
}

CStdString CMultiPathDirectory::ConstructMultiPath(const vector<CStdString> &vecPaths)
{
  // we replace all instances of comma's with double comma's, then separate
  // the paths using " , "
  //CLog::Log(LOGDEBUG, "Building multipath");
  CStdString newPath = "multipath://";
  //CLog::Log(LOGDEBUG, "-- adding path: %s", strPath.c_str());
  for (unsigned int i = 0; i < vecPaths.size(); ++i)
    AddToMultiPath(newPath, vecPaths[i]);
  //CLog::Log(LOGDEBUG, "Final path: %s", newPath.c_str());
  return newPath;
}

void CMultiPathDirectory::MergeItems(CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i", (int)items.Size());
  DWORD dwTime=GetTickCount();
  if (items.Size() == 0)
    return;
  // sort items by label
  // folders are before files in this sort method
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  int i = 0;

  // if first item in the sorted list is a file, just abort
  if (!items.Get(i)->m_bIsFolder)
    return;

  while (i + 1 < items.Size())
  {
    // there are no more folders left, so exit the loop
    CFileItem* pItem1 = items.Get(i);
    if (!pItem1->m_bIsFolder)
      break;

    vector<int> stack;
    stack.push_back(i);
    CLog::Log(LOGDEBUG,"Testing path: [%03i] %s", i, pItem1->m_strPath.c_str());

    int j = i + 1;
    do
    {
      CFileItem* pItem2 = items.Get(j);
      if (!pItem2->GetLabel().Equals(pItem1->GetLabel()))
        break;

      // ignore any filefolders which may coincidently have
      // the same label as a true folder
      if (!pItem2->IsFileFolder())
      {
        stack.push_back(j);
        CLog::Log(LOGDEBUG,"  Adding path: [%03i] %s", j, pItem2->m_strPath.c_str());
      }
      j++;
    }
    while (j < items.Size());

    // do we have anything to combine?
    if (stack.size() > 1)
    {
      // we have a multipath so remove the items and add the new item
      CStdString newPath = ConstructMultiPath(items, stack);
      for (unsigned int k = stack.size() - 1; k > 0; --k)
        items.Remove(stack[k]);
      pItem1->m_strPath = newPath;
      CLog::Log(LOGDEBUG,"  New path: %s", pItem1->m_strPath.c_str());
    }

    i++;
  }

  CLog::Log(LOGDEBUG, "CMultiPathDirectory::MergeItems, items = %i,  took %ld ms", items.Size(), GetTickCount()-dwTime);
}

bool CMultiPathDirectory::SupportsFileOperations(const CStdString &strPath)
{
  vector<CStdString> paths;
  GetPaths(strPath, paths);
  for (unsigned int i = 0; i < paths.size(); ++i)
    if (CUtil::SupportsFileOperations(paths[i]))
      return true;
  return false;
}
