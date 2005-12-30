#pragma once
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"

using namespace DIRECTORY;

// base class for all media windows
class CGUIMediaWindow : public CGUIWindow
{
public:
  CGUIMediaWindow(DWORD id, const char *xmlFile);
  virtual ~CGUIMediaWindow(void);
  virtual bool IsMediaWindow() const { return true; };
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  const CFileItem *GetCurrentListItem() const;

protected:
  virtual void GoParentFolder();
  virtual void OnClick(int iItem);
  virtual void FormatItemLabels() {};
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnSort();

  virtual void ClearFileItems();
  virtual void SortItems(CFileItemList &items);

  // check for a disc or connection
  virtual bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );

  CVirtualDirectory m_rootDir;
  CGUIViewControl m_viewControl;

  // current path and history
  CFileItemList m_vecItems;
  CStdString m_strParentPath;
  CDirectoryHistory m_history;

  // save control state on window exit
  int m_iLastControl;
  int m_iSelectedItem;
};
