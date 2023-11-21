/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileOperationJob.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>

using namespace XFILE;

CFileOperationJob::CFileOperationJob()
  : m_items(),
    m_strDestFile(),
    m_avgSpeed(),
    m_currentOperation(),
    m_currentFile()
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

void CFileOperationJob::SetFileOperation(FileAction action,
                                         const CFileItemList& items,
                                         const std::string& strDestFile)
{
  m_action = action;
  m_strDestFile = strDestFile;

  m_items.Clear();
  for (int i = 0; i < items.Size(); i++)
    m_items.Add(std::make_shared<CFileItem>(*items[i]));
}

bool CFileOperationJob::DoWork()
{
  FileOperationList ops;
  double totalTime = 0.0;

  if (m_displayProgress && GetProgressDialog() == NULL)
  {
    CGUIDialogExtendedProgressBar* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
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

  fileOperations.emplace_back(action, strFileA, strFileB, time);

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

  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_GET_HIDDEN);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    pItem->Select(true);
  }

  if (!DoProcess(action, items, strDestFile, fileOperations, totalTime))
  {
    CLog::Log(LOGERROR, "FileManager: error while processing folder: {}", strPath);
    return false;
  }

  if (action == ActionMove)
  {
    fileOperations.emplace_back(ActionDeleteFolder, strPath, "", 1);
    totalTime += 1.0;
  }

  return true;
}

bool CFileOperationJob::DoProcess(FileAction action,
                                  const CFileItemList& items,
                                  const std::string& strDestFile,
                                  FileOperationList& fileOperations,
                                  double& totalTime)
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

        strFileName = CUtil::MakeLegalFileName(std::move(strFileName));
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
      bResult = CFile::Copy(m_strFileA, m_strFileB, this, &data);
      break;

    case ActionMove:
      if (CanBeRenamed(m_strFileA, m_strFileB))
        bResult = CFile::Rename(m_strFileA, m_strFileB);
      else if (CFile::Copy(m_strFileA, m_strFileB, this, &data))
        bResult = CFile::Delete(m_strFileA);
      else
        bResult = false;
      break;

    case ActionDelete:
      bResult = CFile::Delete(m_strFileA);
      break;

    case ActionDeleteFolder:
      bResult = CDirectory::Remove(m_strFileA);
      break;

    case ActionCreateFolder:
      bResult = CDirectory::Create(m_strFileA);
      break;

    default:
      CLog::Log(LOGERROR, "FileManager: unknown operation");
      bResult = false;
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
  else if (URIUtils::IsSmb(strFileA) && URIUtils::IsSmb(strFileB)) {
    CURL smbFileA(strFileA), smbFileB(strFileB);
    return smbFileA.GetHostName() == smbFileB.GetHostName() &&
           smbFileA.GetShareName() == smbFileB.GetShareName();
  }
#endif
  return false;
}

bool CFileOperationJob::CFileOperation::OnFileCallback(void* pContext, int ipercent, float avgSpeed)
{
  DataHolder *data = static_cast<DataHolder*>(pContext);
  double current = data->current + ((double)ipercent * data->opWeight * (double)m_time)/ 100.0;

  if (avgSpeed > 1000000.0f)
    data->base->m_avgSpeed = StringUtils::Format("{:.1f} MB/s", avgSpeed / 1000000.0f);
  else
    data->base->m_avgSpeed = StringUtils::Format("{:.1f} KB/s", avgSpeed / 1000.0f);

  std::string line;
  line =
      StringUtils::Format("{} ({})", data->base->GetCurrentFile(), data->base->GetAverageSpeed());
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
