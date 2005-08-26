#pragma once
#include "GUIWindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"

using namespace DIRECTORY;

class CGUIWindowFileManager :
      public CGUIWindow,
      public XFILE::IFileCallback
{
public:

  CGUIWindowFileManager(void);
  virtual ~CGUIWindowFileManager(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual bool OnFileCallback(void* pContext, int ipercent);
  bool Delete(const CFileItem *pItem);
  CFileItem CurrentDirectory(int indx) const { return m_Directory[indx];};

protected:
  void GoParentFolder(int iList);
  void UpdateControl(int iList);
  __int64 CalculateFolderSize(const CStdString &strDirectory, CGUIDialogProgress *pProgress = NULL);
  bool Update(int iList, const CStdString &strDirectory); //???
  void OnStart(CFileItem *pItem);
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
  void RenameFile(const CStdString &strFile);
  void OnNewFolder(int iList);
  bool DoProcess(int iAction, CFileItemList & items, const CStdString& strDestFile);
  bool DoProcessFile(int iAction, const CStdString& strFile, const CStdString& strDestFile);
  bool DoProcessFolder(int iAction, const CStdString& strPath, const CStdString& strDestFile);
  void Refresh();
  void Refresh(int iList);
  int GetSelectedItem(int iList);
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  bool GetDirectory(int iList, const CStdString &strDirectory, CFileItemList &items);
  int NumSelected(int iList);
  int GetFocusedList() const;
  // functions to check for actions that we can perform
  bool CanRename(int iList);
  bool CanCopy(int iList);
  bool CanMove(int iList);
  bool CanDelete(int iList);
  bool CanNewFolder(int iList);
  void OnPopupMenu(int iList, int iItem);
  void ShowShareErrorMessage(CFileItem* pItem);

  CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems[2];
  typedef vector <CFileItem*> ::iterator ivecItems;
  CFileItem m_Directory[2];
  CStdString m_strParentPath[2];
  CGUIDialogProgress* m_dlgProgress;
  CDirectoryHistory m_history[2];
  int m_iItemSelected;
  int m_iLastControl;
};
