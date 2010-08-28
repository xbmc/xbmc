#include "FileUtils.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogKeyboard.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "JobManager.h"
#include "FileOperationJob.h"
#include "Util.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include <vector>

using namespace XFILE;
using namespace std;

bool CFileUtils::DeleteItem(const CFileItemPtr &item)
{
  if (!item)
    return false;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CUtil::GetFileName(item->m_strPath));
    pDialog->SetLine(2, "");
    pDialog->DoModal();
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItemPtr pItemTemp(new CFileItem(*item));
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CFileOperationJob op(CFileOperationJob::ActionDelete, items, "");

  return op.DoWork();
}

bool CFileUtils::RenameFile(const CStdString &strFile)
{
  CStdString strFileAndPath(strFile);
  CUtil::RemoveSlashAtEnd(strFileAndPath);
  CStdString strFileName = CUtil::GetFileName(strFileAndPath);
  CStdString strPath = strFile.Left(strFileAndPath.size() - strFileName.size());
  if (CGUIDialogKeyboard::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath += strFileName;
    CLog::Log(LOGINFO,"FileUtils: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (CUtil::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      vector<CStdString> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        CStdString filePath(paths[i]);
        CUtil::RemoveSlashAtEnd(filePath);
        CUtil::GetDirectory(filePath, filePath);
        CUtil::AddFileToFolder(filePath, strFileName, filePath);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}

/* Function here simply calls CFile::Delete(). The purpose for this is to
 * avoid an include of "File.h" for various source files.
 */
bool CFileUtils::Delete(const CStdString& strFileName)
{
  return CFile::Delete(strFileName);
}

/* Function to copy files to the local filesystem.
 *
 * Like CFile::Cache, except the destination file is a temporary file that is
 * created, whose path is returned, and in which the file must be explicitely
 * deleted.
 */
bool CFileUtils::TmpCache(const CStdString& strFileName, CStdString& strTempName, XFILE::IFileCallback* pCallback, void* pContext)
{
  char str[L_tmpnam];
  strTempName.assign(tmpnam_r(str));

  if (open(strTempName.c_str(), O_CREAT | O_EXCL) < 0)
  {
    CLog::Log(LOGERROR, "%s - Failed to create temporary file", __FUNCTION__);
    strTempName = "";
    return false;
  }

  if (!CFile::Cache(strFileName, strTempName, pCallback, pContext))
  {
    CLog::Log(LOGERROR, "%s - Failed to create temporary copy of %s", __FUNCTION__, strFileName.c_str());
    strTempName = "";
    return false;
  }

  return true;
}

/* Function that returns translated path of files on local filesystem. Will
 * return an empty string for files not in the local filesystem.
 */
bool CFileUtils::GetLocalPath(const CStdString& strFileName, CStdString& strLocalPath)
{
  CURL url(strFileName);
  CStdString strProtocol = url.GetProtocol();

  if (strProtocol == "special")
    strLocalPath = _P(strFileName);
  else if (strProtocol == "file" || strProtocol.IsEmpty())
    strLocalPath = strFileName;
  else
  {
    strLocalPath = "";
    return false;
  }

  return true;
}
