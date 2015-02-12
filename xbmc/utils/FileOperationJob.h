#pragma once
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
#include "FileItem.h"
#include "Job.h"
#include "filesystem/File.h"
#include <string>
#include <vector>

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
                    const std::string& strDestFile,
                    bool displayProgress=false,
                    int errorHeading=0, int errorLine=0);

  void SetFileOperation(FileAction action, CFileItemList &items, const std::string &strDestFile);

  virtual bool operator==(const CJob *job) const;

  static std::string GetActionString(FileAction action);

  const char* GetType() const { return m_displayProgress?"filemanager":""; }

  virtual bool DoWork();
  const std::string &GetAverageSpeed()     const { return m_avgSpeed; }
  const std::string &GetCurrentOperation() const { return m_currentOperation; }
  const std::string &GetCurrentFile()      const { return m_currentFile; }
  const CFileItemList &GetItems()         const { return m_items; }
  FileAction GetAction() const            { return m_action; }
  int GetHeading() const                  { return m_heading; }
  int GetLine() const                     { return m_line; }
private:
  class CFileOperation : public XFILE::IFileCallback
  {
  public:
    CFileOperation(FileAction action, const std::string &strFileA, const std::string &strFileB, int64_t time);
    bool ExecuteOperation(CFileOperationJob *base, double &current, double opWeight);
    void Debug();
    virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed);
  private:
    FileAction m_action;
    std::string m_strFileA, m_strFileB;
    int64_t m_time;
  };
  friend class CFileOperation;
  typedef std::vector<CFileOperation> FileOperationList;
  bool DoProcess(FileAction action, CFileItemList & items, const std::string& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFolder(FileAction action, const std::string& strPath, const std::string& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFile(FileAction action, const std::string& strFileA, const std::string& strFileB, FileOperationList &fileOperations, double &totalTime);

  static inline bool CanBeRenamed(const std::string &strFileA, const std::string &strFileB);

  FileAction m_action;
  CFileItemList m_items;
  std::string m_strDestFile;
  std::string m_avgSpeed, m_currentOperation, m_currentFile;
  CGUIDialogProgressBarHandle* m_handle;
  bool m_displayProgress;
  int m_heading;
  int m_line;
};
