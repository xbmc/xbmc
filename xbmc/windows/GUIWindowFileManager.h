/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "guilib/GUIWindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "utils/JobManager.h"

class CFileItem;
class CFileItemList;
class CGUIDialogProgress;

class CGUIWindowFileManager :
      public CGUIWindow,
      public CJobQueue
{
public:

  CGUIWindowFileManager(void);
  ~CGUIWindowFileManager(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  const CFileItem &CurrentDirectory(int indx) const;

  static int64_t CalculateFolderSize(const std::string &strDirectory, CGUIDialogProgress *pProgress = NULL);

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
protected:
  void OnInitWindow() override;
  void SetInitialPath(const std::string &path);
  void GoParentFolder(int iList);
  void UpdateControl(int iList, int item);
  bool Update(int iList, const std::string &strDirectory); //???
  void OnStart(CFileItem *pItem, const std::string &player);
  bool SelectItem(int iList, int &item);
  void ClearFileItems(int iList);
  void OnClick(int iList, int iItem);
  void OnMark(int iList, int iItem);
  void OnSort(int iList);
  void UpdateButtons();
  void OnCopy(int iList);
  void OnMove(int iList);
  void OnDelete(int iList);
  void OnRename(int iList);
  void OnSelectAll(int iList);
  void OnNewFolder(int iList);
  void Refresh();
  void Refresh(int iList);
  int GetSelectedItem(int iList);
  bool HaveDiscOrConnection( std::string& strPath, int iDriveType );
  void GetDirectoryHistoryString(const CFileItem* pItem, std::string& strHistoryString);
  bool GetDirectory(int iList, const std::string &strDirectory, CFileItemList &items);
  int NumSelected(int iList);
  int GetFocusedList() const;
  // functions to check for actions that we can perform
  bool CanRename(int iList);
  bool CanCopy(int iList);
  bool CanMove(int iList);
  bool CanDelete(int iList);
  bool CanNewFolder(int iList);
  void OnPopupMenu(int iList, int iItem, bool bContextDriven = true);
  void ShowShareErrorMessage(CFileItem* pItem);
  void UpdateItemCounts();

  //
  bool bCheckShareConnectivity;
  std::string strCheckSharePath;

  XFILE::CVirtualDirectory m_rootDir;
  CFileItemList* m_vecItems[2];
  typedef std::vector <CFileItem*> ::iterator ivecItems;
  CFileItem* m_Directory[2];
  std::string m_strParentPath[2];
  CDirectoryHistory m_history[2];

  int m_errorHeading, m_errorLine;
private:
  std::atomic_bool m_updating = {false};
  class CUpdateGuard
  {
  public:
    CUpdateGuard(std::atomic_bool &update) : m_update(update)
    {
      m_update = true;
    }
    ~CUpdateGuard()
    {
      m_update = false;
    }
  private:
    std::atomic_bool &m_update;
  };
};
