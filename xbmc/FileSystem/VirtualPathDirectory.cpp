
#include "../stdafx.h"
#include "virtualpathdirectory.h"
#include "directory.h"
#include "../settings.h"
#include "../util.h"

using namespace DIRECTORY;

// virtualpath://type/bookmarkname

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

  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dlgProgress)
  {
    dlgProgress->SetHeading(15310);
    dlgProgress->SetLine(0, 15311);
    dlgProgress->SetLine(1, L"");
    dlgProgress->SetLine(2, L"");
    dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
    dlgProgress->ShowProgressBar(true);
    dlgProgress->SetProgressBarMax((int)share.vecPaths.size()*2);
    dlgProgress->Progress();
  }

  int iFailures = 0;
  for (int i = 0; i < (int)share.vecPaths.size(); ++i)
  {
    if (dlgProgress)
    {
      CURL url(share.vecPaths[i]);
      CStdString strStripped;
      url.GetURLWithoutUserDetails(strStripped);
      dlgProgress->SetLine(1, strStripped);
      dlgProgress->StepProgressBar();
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
      dlgProgress->StepProgressBar();
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
  CShare share;
  if (!GetMatchingShare(strPath, share))
    return false;

  int iFailures = 0;
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


bool CVirtualPathDirectory::GetTypeAndBookmark(const CStdString& strPath, CStdString& strType, CStdString& strBookmark)
{
  // format: virtualpath://type/bookmarkname
  CStdString strTemp = strPath;
  CStdString strTest = "virtualpath://";
  if (strTemp.Left(strTest.length()).Equals(strTest))
  {
    strTemp = strTemp.Mid(strTest.length());
    int iPos = strTemp.Find('/');
    if (iPos < 1)
      return false;
    strType = strTemp.Mid(0, iPos);
    strBookmark = strTemp.Mid(iPos + 1);
    CLog::Log(LOGDEBUG,"CVirtualPathDirectory::GetTypeAndBookmark(%s) = [%s],[%s]", strPath.c_str(), strType.c_str(), strBookmark.c_str());
    return true;
  }
  return false;
}

bool CVirtualPathDirectory::GetShares(const CStdString& strType, VECSHARES& vecShares)
{
  VECSHARES *pShares = NULL;

  // not valid for myprograms or files
  if (strType == "myprograms")
    return false;
  else if (strType == "files")
    return false;
  else if (strType == "music")
    pShares = &g_settings.m_vecMyMusicShares;
  else if (strType == "video")
    pShares = &g_settings.m_vecMyVideoShares;
  else if (strType == "pictures")
    pShares = &g_settings.m_vecMyPictureShares;

  if (!pShares) return false;
  
  vecShares = *pShares;
  return true;
}

bool CVirtualPathDirectory::GetMatchingShare(const CStdString &strPath, CShare& share)
{
  CStdString strType, strBookmark;
  if (!GetTypeAndBookmark(strPath, strType, strBookmark))
    return false;

  VECSHARES vecShares;
  if (!GetShares(strType, vecShares))
    return false;

  bool bIsBookmarkName = false;
  int iIndex = CUtil::GetMatchingShare(strBookmark, vecShares, bIsBookmarkName);
  if (!bIsBookmarkName)
    return false;
  if (iIndex < 0)
    return false;

  share = vecShares[iIndex];
  return true;
}