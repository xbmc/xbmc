#pragma once
#include "GUIDialog.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"
#include "PictureThumbLoader.h"

class CGUIDialogFileBrowser : public CGUIDialog, public IBackgroundLoaderObserver
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
  void SetHeading(const CStdString &heading);

  static bool ShowAndGetDirectory(const VECSHARES &shares, const CStdString &heading, CStdString &path, bool bWriteOnly=false);
  static bool ShowAndGetFile(const VECSHARES &shares, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs = false, bool useFileDirectories = false);
  static bool ShowAndGetFile(const CStdString &directory, const CStdString &mask, const CStdString &heading, CStdString &path, bool useThumbs = false, bool useFileDirectories = false);
  static bool ShowAndGetShare(CStdString &path, bool allowNetworkShares, VECSHARES* additionalShare = NULL, const CStdString& strType="");
  static bool ShowAndGetImage(const VECSHARES &shares, const CStdString &heading, CStdString &path);
  static bool ShowAndGetImage(const CFileItemList &items, VECSHARES &shares, const CStdString &heading, CStdString &path);

  void SetShares(const VECSHARES &shares);

  virtual void OnItemLoaded(CFileItem *item) {};

  virtual bool HasListItems() const { return true; };
  virtual CFileItem *GetCurrentListItem(int offset = 0);
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };

protected:
  void GoParentFolder();
  void OnClick(int iItem);
  void OnSort();
  void ClearFileItems();
  void Update(const CStdString &strDirectory);
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  bool OnPopupMenu(int iItem);
  void OnAddNetworkLocation();
  void OnAddMediaSource();
  void OnEditMediaSource(CFileItem* pItem);
  CGUIControl *GetFirstFocusableControl(int id);

  VECSHARES m_shares;
  DIRECTORY::CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CStdString m_strParentPath;
  CStdString m_selectedPath;
  CDirectoryHistory m_history;
  int m_browsingForFolders; // 0 - no, 1 - yes, 2 - yes, only writable
  bool m_bConfirmed;
  bool m_addNetworkShareEnabled;
  CStdString m_addSourceType;
  bool m_browsingForImages;
  bool m_useFileDirectories;
  bool m_singleList;              // if true, we have no shares or anything

  CPictureThumbLoader m_thumbLoader;
  CGUIViewControl m_viewControl;
};
