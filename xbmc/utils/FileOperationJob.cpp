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

#include "system.h"

#include "FileOperationJob.h"
#include "URL.h"
#include "Util.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;

CFileOperationJob::CFileOperationJob()
  : m_action(ActionCopy),
    m_items(),
    m_strDestFile(),
    m_avgSpeed(),
    m_currentOperation(),
    m_currentFile(),
    m_displayProgress(false),
    m_heading(0),
    m_line(0)
{ }

CFileOperationJob::CFileOperationJob(FileAction action, CFileItemList & items,
                                    const std::string& strDestFile,
                                    bool displayProgress /* = false */,
                                    int heading /* = 0 */, int line /* = 0 */)
  : m_action(action),
    m_items(),
    m_strDestFile(strDestFile),
    m_avgSpeed(),
    m_currentOperation(),
    m_currentFile(),
    m_displayProgress(displayProgress),
    m_heading(heading),
    m_line(line)
{
  SetFileOperation(action, items, strDestFile);
}

void CFileOperationJob::SetFileOperation(FileAction action, CFileItemList &items, const std::string &strDestFile)
{
  m_action = action;
  m_strDestFile = strDestFile;

  m_items.Clear();
  for (int i = 0; i < items.Size(); i++)
    m_items.Add(CFileItemPtr(new CFileItem(*items[i])));
}

bool CFileOperationJob::DoWork()
{
  FileOperationList ops;
  double totalTime = 0.0;

  if (m_displayProgress && GetProgressDialog() == NULL)
  {
    CGUIDialogExtendedProgressBar* dialog =
      (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    SetProgressBar(dialog->GetHandle(GetActionString(m_action)));
  }

  bool success = DoProcess(m_action, m_items, m_strDestFile, ops, totalTime);

  unsigned int size = ops.size();

  double opWeight = 100.0 / totalTime;
  double current = 0.0;

  for (unsigned int i = 0; i < size && success; i++)
    success &= ops[i].ExecuteOperation(this, current, opWeight);

  MarkFinished();

  return success;
}

bool CFileOperationJob::DoProcessFile(FileAction action, const std::string& strFileA, const std::string& strFileB, FileOperationList &fileOperations, double &totalTime)
{
  int64_t time = 1;

  if (action == ActionCopy || action == ActionReplace || (action == ActionMove && !CanBeRenamed(strFileA, strFileB)))
  {
    struct __stat64 data;
    if (CFile::Stat(strFileA, &data) == 0)
      time += data.st_size;
  }

  fileOperations.push_back(CFileOperation(action, strFileA, strFileB, time));

  totalTime += time;

  return true;
}

bool CFileOperationJob::DoProcessFolder(FileAction action, const std::string& strPath, const std::string& strDestFile, FileOperationList &fileOperations, double &totalTime)
{
  // check whether this folder is a filedirectory - if so, we don't process it's contents
  CFileItem item(strPath, false);
  IFileDirectory *file = CFileDirectoryFactory::Create(item.GetURL(), &item);
  if (file)
  {
    delete file;
    return true;
  }

  CLog::Log(LOGDEBUG,"FileManager, processing folder: %s",strPath.c_str());
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_GET_HIDDEN);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    pItem->Select(true);
    CLog::Log(LOGDEBUG,"  -- %s",pItem->GetPath().c_str());
  }

  if (!DoProcess(action, items, strDestFile, fileOperations, totalTime))
    return false;

  if (action == ActionMove)
  {
    fileOperations.push_back(CFileOperation(ActionDeleteFolder, strPath, "", 1));
    totalTime += 1.0;
  }

  return true;
}

bool CFileOperationJob::DoProcess(FileAction action, CFileItemList & items, const std::string& strDestFile, FileOperationList &fileOperations, double &totalTime)
{
  for (int iItem = 0; iItem < items.Size(); ++iItem)
  {
    CFileItemPtr pItem = items[iItem];
    if (pItem->IsSelected())
    {
      std::string strNoSlash = pItem->GetPath();
      URIUtils::RemoveSlashAtEnd(strNoSlash);
      std::string strFileName = URIUtils::GetFileName(strNoSlash);

      // special case for upnp
      if (URIUtils::IsUPnP(items.GetPath()) || URIUtils::IsUPnP(pItem->GetPath()))
      {
        // get filename from label instead of path
        strFileName = pItem->GetLabel();

        if (!pItem->m_bIsFolder && !URIUtils::HasExtension(strFileName))
        {
          // FIXME: for now we only work well if the url has the extension
          // we should map the content type to the extension otherwise
          strFileName += URIUtils::GetExtension(pItem->GetPath());
        }

        strFileName = CUtil::MakeLegalFileName(strFileName);
      }

      std::string strnewDestFile;
      if (!strDestFile.empty()) // only do this if we have a destination
        strnewDestFile = URIUtils::ChangeBasePath(pItem->GetPath(), strFileName, strDestFile); // Convert (URL) encoding + slashes (if source / target differ)

      if (pItem->m_bIsFolder)
      {
        // in ActionReplace mode all subdirectories will be removed by the below
        // DoProcessFolder(ActionDelete) call as well, so ActionCopy is enough when
        // processing those
        FileAction subdirAction = (action == ActionReplace) ? ActionCopy : action;
        // create folder on dest. drive
        if (action != ActionDelete && action != ActionDeleteFolder)
          DoProcessFile(ActionCreateFolder, strnewDestFile, "", fileOperations, totalTime);

        if (action == ActionReplace && CDirectory::Exists(strnewDestFile))
          DoProcessFolder(ActionDelete, strnewDestFile, "", fileOperations, totalTime);

        if (!DoProcessFolder(subdirAction, pItem->GetPath(), strnewDestFile, fileOperations, totalTime))
          return false;

        if (action == ActionDelete || action == ActionDeleteFolder)
          DoProcessFile(ActionDeleteFolder, pItem->GetPath(), "", fileOperations, totalTime);
      }
      else
        DoProcessFile(action, pItem->GetPath(), strnewDestFile, fileOperations, totalTime);
    }
  }

  return true;
}

CFileOperationJob::CFileOperation::CFileOperation(FileAction action, const std::string &strFileA, const std::string &strFileB, int64_t time)
  : m_action(action),
    m_strFileA(strFileA),
    m_strFileB(strFileB),
    m_time(time)
{ }

struct DataHolder
{
  CFileOperationJob *base;
  double current;
  double opWeight;
};

std::string CFileOperationJob::GetActionString(FileAction action)
{
  std::string result;
  switch (action)
  {
    case ActionCopy:
    case ActionReplace:
      result = g_localizeStrings.Get(115);
      break;

    case ActionMove:
      result = g_localizeStrings.Get(116);
      break;

    case ActionDelete:
    case ActionDeleteFolder:
      result = g_localizeStrings.Get(117);
      break;

    case ActionCreateFolder:
      result = g_localizeStrings.Get(119);
      break;

    default:
      break;
  }

  return result;
}

bool CFileOperationJob::CFileOperation::ExecuteOperation(CFileOperationJob *base, double &current, double opWeight)
{
  bool bResult = true;

  base->m_currentFile = CURL(m_strFileA).GetFileNameWithoutPath();
  base->m_currentOperation = GetActionString(m_action);

  if (base->ShouldCancel((unsigned int)current, 100))
    return false;

  base->SetText(base->GetCurrentFile());

  DataHolder data = {base, current, opWeight};

  switch (m_action)
  {
    case ActionCopy:
    case ActionReplace:
    {
      CLog::Log(LOGDEBUG,"FileManager: copy %s -> %s\n", m_strFileA.c_str(), m_strFileB.c_str());

      bResult = CFile::Copy(m_strFileA, m_strFileB, this, &data);
    }
    break;

    case ActionMove:
    {
      CLog::Log(LOGDEBUG,"FileManager: move %s -> %s\n", m_strFileA.c_str(), m_strFileB.c_str());

      if (CanBeRenamed(m_strFileA, m_strFileB))
        bResult = CFile::Rename(m_strFileA, m_strFileB);
      else if (CFile::Copy(m_strFileA, m_strFileB, this, &data))
        bResult = CFile::Delete(m_strFileA);
      else
        bResult = false;
    }
    break;

    case ActionDelete:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete %s\n", m_strFileA.c_str());

      bResult = CFile::Delete(m_strFileA);
    }
    break;

    case ActionDeleteFolder:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete folder %s\n", m_strFileA.c_str());

      bResult = CDirectory::Remove(m_strFileA);
    }
    break;

    case ActionCreateFolder:
    {
      CLog::Log(LOGDEBUG,"FileManager: create folder %s\n", m_strFileA.c_str());

      bResult = CDirectory::Create(m_strFileA);
    }
    break;
  }

  current += (double)m_time * opWeight;

  return bResult;
}

inline bool CFileOperationJob::CanBeRenamed(const std::string &strFileA, const std::string &strFileB)
{
#ifndef TARGET_POSIX
  if (strFileA[1] == ':' && strFileA[0] == strFileB[0])
    return true;
#else
  if (URIUtils::IsHD(strFileA) && URIUtils::IsHD(strFileB))
    return true;
#endif
  return false;
}

void CFileOperationJob::CFileOperation::Debug()
{
  printf("%i | %s > %s\n", m_action, m_strFileA.c_str(), m_strFileB.c_str());
}

bool CFileOperationJob::CFileOperation::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  DataHolder *data = (DataHolder *)pContext;
  double current = data->current + ((double)ipercent * data->opWeight * (double)m_time)/ 100.0;

  if (avgSpeed > 1000000.0f)
    data->base->m_avgSpeed = StringUtils::Format("%.1f MB/s", avgSpeed / 1000000.0f);
  else
    data->base->m_avgSpeed = StringUtils::Format("%.1f KB/s", avgSpeed / 1000.0f);

  std::string line;
  line = StringUtils::Format("%s (%s)",
                              data->base->GetCurrentFile().c_str(),
                              data->base->GetAverageSpeed().c_str());
  data->base->SetText(line);
  return !data->base->ShouldCancel((unsigned)current, 100);
}

bool CFileOperationJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CFileOperationJob* rjob = dynamic_cast<const CFileOperationJob*>(job);
  if (rjob == NULL)
    return false;

  if (GetAction() != rjob->GetAction() ||
      m_strDestFile != rjob->m_strDestFile ||
      m_items.Size() != rjob->m_items.Size())
    return false;

  for (int i = 0; i < m_items.Size(); i++)
  {
    if (m_items[i]->GetPath() != rjob->m_items[i]->GetPath() ||
        m_items[i]->IsSelected() != rjob->m_items[i]->IsSelected())
      return false;
  }

  return true;
}
