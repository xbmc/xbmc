#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "FileItem.h"
#include "Job.h"
#include "filesystem/File.h"

class CGUIDialogProgressBarHandle;

class CFileOperationJob : public CJob
{
public:
  enum FileAction
  {
    ActionCopy = 1,
    ActionMove,
    ActionDelete,
    ActionReplace, ///< Copy, emptying any existing destination directories first
    ActionCreateFolder,
    ActionDeleteFolder,
  };

  CFileOperationJob();
  CFileOperationJob(FileAction action, CFileItemList & items,
                    const CStdString& strDestFile,
                    bool displayProgress=false,
                    int errorHeading=0, int errorLine=0);

  void SetFileOperation(FileAction action, CFileItemList &items, const CStdString &strDestFile);

  virtual bool operator==(const CJob *job) const;

  static CStdString GetActionString(FileAction action);

  const char* GetType() const { return m_displayProgress?"filemanager":""; }

  virtual bool DoWork();
  const CStdString &GetAverageSpeed()     { return m_avgSpeed; }
  const CStdString &GetCurrentOperation() { return m_currentOperation; }
  const CStdString &GetCurrentFile()      { return m_currentFile; }
  const CFileItemList &GetItems()         { return m_items; }
  FileAction GetAction() const            { return m_action; }
  int GetHeading() const                  { return m_heading; }
  int GetLine() const                     { return m_line; }
private:
  class CFileOperation : public XFILE::IFileCallback
  {
  public:
    CFileOperation(FileAction action, const CStdString &strFileA, const CStdString &strFileB, int64_t time);
    bool ExecuteOperation(CFileOperationJob *base, double &current, double opWeight);
    void Debug();
    virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed);
  private:
    FileAction m_action;
    CStdString m_strFileA, m_strFileB;
    int64_t m_time;
  };
  friend class CFileOperation;
  typedef std::vector<CFileOperation> FileOperationList;
  bool DoProcess(FileAction action, CFileItemList & items, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFolder(FileAction action, const CStdString& strPath, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFile(FileAction action, const CStdString& strFileA, const CStdString& strFileB, FileOperationList &fileOperations, double &totalTime);

  static inline bool CanBeRenamed(const CStdString &strFileA, const CStdString &strFileB);

  FileAction m_action;
  CFileItemList m_items;
  CStdString m_strDestFile;
  CStdString m_avgSpeed, m_currentOperation, m_currentFile;
  CGUIDialogProgressBarHandle* m_handle;
  bool m_displayProgress;
  int m_heading;
  int m_line;
};
