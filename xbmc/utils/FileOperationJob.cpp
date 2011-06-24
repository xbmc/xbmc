/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "FileOperationJob.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/ZipManager.h"
#include "filesystem/FactoryFileDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "log.h"
#include "Util.h"
#include "URIUtils.h"
#include "guilib/LocalizeStrings.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif

using namespace std;
using namespace XFILE;

CFileOperationJob::CFileOperationJob()
{
}

CFileOperationJob::CFileOperationJob(FileAction action, CFileItemList & items, const CStdString& strDestFile)
{
  SetFileOperation(action, items, strDestFile);
}

void CFileOperationJob::SetFileOperation(FileAction action, CFileItemList &items, const CStdString &strDestFile)
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
  bool success = DoProcess(m_action, m_items, m_strDestFile, ops, totalTime);

  unsigned int size = ops.size();

  double opWeight = 100.0 / totalTime;
  double current = 0.0;

  for (unsigned int i = 0; i < size && success; i++)
    success &= ops[i].ExecuteOperation(this, current, opWeight);

  return success;
}

bool CFileOperationJob::DoProcessFile(FileAction action, const CStdString& strFileA, const CStdString& strFileB, FileOperationList &fileOperations, double &totalTime)
{
  int64_t time = 1;

  if (action == ActionCopy || action == ActionReplace || (action == ActionMove && !CanBeRenamed(strFileA, strFileB)))
  {
    struct __stat64 data;
    if(CFile::Stat(strFileA, &data) == 0)
      time += data.st_size;
  }

  fileOperations.push_back(CFileOperation(action, strFileA, strFileB, time));

  totalTime += time;

  return true;
}

bool CFileOperationJob::DoProcessFolder(FileAction action, const CStdString& strPath, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime)
{
  // check whether this folder is a filedirectory - if so, we don't process it's contents
  CFileItem item(strPath, false);
  IFileDirectory *file = CFactoryFileDirectory::Create(strPath, &item);
  if (file)
  {
    delete file;
    return true;
  }
  CLog::Log(LOGDEBUG,"FileManager, processing folder: %s",strPath.c_str());
  CFileItemList items;
  //m_rootDir.GetDirectory(strPath, items);
  CDirectory::GetDirectory(strPath, items, "", false, false, DIR_CACHE_ONCE, true, false, true);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    pItem->Select(true);
    CLog::Log(LOGDEBUG,"  -- %s",pItem->m_strPath.c_str());
  }

  if (!DoProcess(action, items, strDestFile, fileOperations, totalTime)) return false;

  if (action == ActionMove)
  {
    fileOperations.push_back(CFileOperation(ActionDeleteFolder, strPath, "", 1));
    totalTime += 1.0;
  }

  return true;
}

bool CFileOperationJob::DoProcess(FileAction action, CFileItemList & items, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime)
{
  for (int iItem = 0; iItem < items.Size(); ++iItem)
  {
    CFileItemPtr pItem = items[iItem];
    if (pItem->IsSelected())
    {
      CStdString strNoSlash = pItem->m_strPath;
      URIUtils::RemoveSlashAtEnd(strNoSlash);
      CStdString strFileName = URIUtils::GetFileName(strNoSlash);

      // special case for upnp
      if (URIUtils::IsUPnP(items.m_strPath) || URIUtils::IsUPnP(pItem->m_strPath))
      {
        // get filename from label instead of path
        strFileName = pItem->GetLabel();

        if(!pItem->m_bIsFolder && URIUtils::GetExtension(strFileName).length() == 0)
        {
          // FIXME: for now we only work well if the url has the extension
          // we should map the content type to the extension otherwise
          strFileName += URIUtils::GetExtension(pItem->m_strPath);
        }

        strFileName = CUtil::MakeLegalFileName(strFileName);
      }

      CStdString strnewDestFile;
      if(!strDestFile.IsEmpty()) // only do this if we have a destination
        URIUtils::AddFileToFolder(strDestFile, strFileName, strnewDestFile);

      if (pItem->m_bIsFolder)
      {
        // in ActionReplace mode all subdirectories will be removed by the below
        // DoProcessFolder(ActionDelete) call as well, so ActionCopy is enough when
        // processing those
        FileAction subdirAction = (action == ActionReplace) ? ActionCopy : action;
        // create folder on dest. drive
        if (action != ActionDelete)
          DoProcessFile(ActionCreateFolder, strnewDestFile, "", fileOperations, totalTime);
        if (action == ActionReplace && CDirectory::Exists(strnewDestFile))
          DoProcessFolder(ActionDelete, strnewDestFile, "", fileOperations, totalTime);
        if (!DoProcessFolder(subdirAction, pItem->m_strPath, strnewDestFile, fileOperations, totalTime))
          return false;
        if (action == ActionDelete)
          DoProcessFile(ActionDeleteFolder, pItem->m_strPath, "", fileOperations, totalTime);
      }
      else
        DoProcessFile(action, pItem->m_strPath, strnewDestFile, fileOperations, totalTime);
    }
  }
  return true;
}

CFileOperationJob::CFileOperation::CFileOperation(FileAction action, const CStdString &strFileA, const CStdString &strFileB, int64_t time) : m_action(action), m_strFileA(strFileA), m_strFileB(strFileB), m_time(time)
{
}

struct DataHolder
{
  CFileOperationJob *base;
  double current;
  double opWeight;
};

bool CFileOperationJob::CFileOperation::ExecuteOperation(CFileOperationJob *base, double &current, double opWeight)
{
  bool bResult = true;

  base->m_currentFile = CURL(m_strFileA).GetFileNameWithoutPath();

  switch (m_action)
  {
    case ActionCopy:
    case ActionReplace:
      base->m_currentOperation = g_localizeStrings.Get(115);
      break;
    case ActionMove:
      base->m_currentOperation = g_localizeStrings.Get(116);
      break;
    case ActionDelete:
    case ActionDeleteFolder:
      base->m_currentOperation = g_localizeStrings.Get(117);
      break;
    case ActionCreateFolder:
      base->m_currentOperation = g_localizeStrings.Get(119);
      break;
    default:
      base->m_currentOperation = "";
      break;
  }

  if (base->ShouldCancel((unsigned)current, 100))
    return false;

  DataHolder data = {base, current, opWeight};

  switch (m_action)
  {
    case ActionCopy:
    case ActionReplace:
    {
      CLog::Log(LOGDEBUG,"FileManager: copy %s -> %s\n", m_strFileA.c_str(), m_strFileB.c_str());

      bResult = CFile::Cache(m_strFileA, m_strFileB, this, &data);
    }
    break;
    case ActionMove:
    {
      CLog::Log(LOGDEBUG,"FileManager: move %s -> %s\n", m_strFileA.c_str(), m_strFileB.c_str());

      if (CanBeRenamed(m_strFileA, m_strFileB))
        bResult = CFile::Rename(m_strFileA, m_strFileB);
      else if (CFile::Cache(m_strFileA, m_strFileB, this, &data))
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

inline bool CFileOperationJob::CanBeRenamed(const CStdString &strFileA, const CStdString &strFileB)
{
#ifndef _LINUX
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
    data->base->m_avgSpeed.Format("%.1f Mb/s", avgSpeed / 1000000.0f);
  else
    data->base->m_avgSpeed.Format("%.1f Kb/s", avgSpeed / 1000.0f);

  return !data->base->ShouldCancel((unsigned)current, 100);
}
