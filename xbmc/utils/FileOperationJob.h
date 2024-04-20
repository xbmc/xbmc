/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/IFileTypes.h"
#include "utils/ProgressJob.h"

#include <string>
#include <vector>

class CFileOperationJob : public CProgressJob
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
                    bool displayProgress = false,
                    int errorHeading = 0, int errorLine = 0);

  static std::string GetActionString(FileAction action);

  // implementations of CJob
  bool DoWork() override;
  const char* GetType() const override { return m_displayProgress ? "filemanager" : ""; }
  bool operator==(const CJob *job) const override;

  void SetFileOperation(FileAction action,
                        const CFileItemList& items,
                        const std::string& strDestFile);

  const std::string &GetAverageSpeed() const { return m_avgSpeed; }
  const std::string &GetCurrentOperation() const { return m_currentOperation; }
  const std::string &GetCurrentFile() const { return m_currentFile; }
  const CFileItemList &GetItems() const { return m_items; }
  FileAction GetAction() const { return m_action; }
  int GetHeading() const { return m_heading; }
  int GetLine() const { return m_line; }

private:
  class CFileOperation : public XFILE::IFileCallback
  {
  public:
    CFileOperation(FileAction action, const std::string &strFileA, const std::string &strFileB, int64_t time);

    bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) override;

    bool ExecuteOperation(CFileOperationJob *base, double &current, double opWeight);

  private:
    FileAction m_action;
    std::string m_strFileA, m_strFileB;
    int64_t m_time;
  };
  friend class CFileOperation;

  typedef std::vector<CFileOperation> FileOperationList;
  bool DoProcess(FileAction action,
                 const CFileItemList& items,
                 const std::string& strDestFile,
                 FileOperationList& fileOperations,
                 double& totalTime);
  bool DoProcessFolder(FileAction action, const std::string& strPath, const std::string& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFile(FileAction action, const std::string& strFileA, const std::string& strFileB, FileOperationList &fileOperations, double &totalTime);

  static inline bool CanBeRenamed(const std::string &strFileA, const std::string &strFileB);

  FileAction m_action = ActionCopy;
  CFileItemList m_items;
  std::string m_strDestFile;
  std::string m_avgSpeed, m_currentOperation, m_currentFile;
  bool m_displayProgress = false;
  int m_heading = 0;
  int m_line = 0;
};
