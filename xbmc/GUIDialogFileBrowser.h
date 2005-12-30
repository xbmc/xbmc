#pragma once
#include "GUIDialog.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"

using namespace DIRECTORY;

class CGUIDialogFileBrowser : public CGUIDialog
{
public:
  CGUIDialogFileBrowser(void);
  virtual ~CGUIDialogFileBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  bool IsConfirmed() { return m_bConfirmed; };
  void SetHeading(const CStdStringW &heading);

  static bool ShowAndGetDirectory(VECSHARES &shares, const CStdStringW &heading, CStdString &path);
  static bool ShowAndGetFile(VECSHARES &shares, const CStdString &mask, const CStdStringW &heading, CStdString &path);

protected:
  void GoParentFolder();
  void OnClick(int iItem);
  void OnSort();
  void ClearFileItems();
  void Update(const CStdString &strDirectory);
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );

  CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CStdString m_strParentPath;
  CStdString m_selectedPath;
  CDirectoryHistory m_history;
  bool m_browsingForFolders;
  bool m_bConfirmed;

  CGUIViewControl m_viewControl;
};
