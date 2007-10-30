
#include "stdafx.h"
#include "VirtualPathDirectory.h"
#include "Directory.h"
#include "../Settings.h"
#include "../Util.h"

using namespace DIRECTORY;

// virtualpath://type/sourcename

CVirtualPathDirectory::CVirtualPathDirectory()
{}

CVirtualPathDirectory::~CVirtualPathDirectory()
{}

bool CVirtualPathDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CVirtualPathDirectory::GetDirectory(%s)", strPath.c_str());

  CShare share;
  if (!GetMatchingShare(strPath, share))
    return false;

  DWORD progressTime = timeGetTime() + 3000L;   // 3 seconds before showing progress bar
  CGUIDialogProgress* dlgProgress = NULL;
  
  unsigned int iFailures = 0;
  for (int i = 0; i < (int)share.vecPaths.size(); ++i)
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
        dlgProgress->SetProgressMax((int)share.vecPaths.size()*2);
        dlgProgress->Progress();
      }
    }
    if (dlgProgress)
    {
      CURL url(share.vecPaths[i]);
      CStdString strStripped;
      url.GetURLWithoutUserDetails(strStripped);
      dlgProgress->SetLine(1, strStripped);
      dlgProgress->SetProgressAdvance();
      dlgProgress->Progress();
    }

    CFileItemList tempItems;
    CLog::Log(LOGDEBUG,"Getting Directory (%s)", share.vecPaths[i].c_str());
    if (CDirectory::GetDirectory(share.vecPaths[i], tempItems, m_strFileMask))
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
  CLog::Log(LOGDEBUG,"Testing Existance (%s)", strPath.c_str());
  CShare share;
  if (!GetMatchingShare(strPath, share))
    return false;

  unsigned int iFailures = 0;
  for (int i = 0; i < (int)share.vecPaths.size(); ++i)
  {
    CLog::Log(LOGDEBUG,"Testing Existance (%s)", share.vecPaths[i].c_str());
    if (!CDirectory::Exists(share.vecPaths[i]))
    {
      CLog::Log(LOGERROR,"Error Testing Existance (%s)", share.vecPaths[i].c_str());
      iFailures++;
    }
  }

  if (iFailures == share.vecPaths.size())
    return false;

  return true;
}

bool CVirtualPathDirectory::GetPathes(const CStdString& strPath, vector<CStdString>& vecPaths)
{
  CShare share;
  if (!GetMatchingShare(strPath, share))
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

bool CVirtualPathDirectory::GetMatchingShare(const CStdString &strPath, CShare& share)
{
  CStdString strType, strSource;
  if (!GetTypeAndSource(strPath, strType, strSource))
    return false;

  // no support for "files" operation
  if (strType == "files")
    return false;
  VECSHARES *vecShares = g_settings.GetSharesFromType(strType);
  if (!vecShares)
    return false;

  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingShare(strSource, *vecShares, bIsSourceName);
  if (!bIsSourceName)
    return false;
  if (iIndex < 0)
    return false;

  share = (*vecShares)[iIndex];
  return true;
}
