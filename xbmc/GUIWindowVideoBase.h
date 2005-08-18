#pragma once
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "VideoDatabase.h"
#include "GUIViewControl.h"

class CGUIWindowVideoBase : public CGUIWindow
{
public:
  CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowVideoBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();

private:
  bool IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
  virtual void SetIMDBThumbs(CFileItemList& items) {};
//  void UpdateThumbPanel();
  // overrideable stuff for the different window classes
  virtual void LoadViewMode() = 0;
  virtual void SaveViewMode() = 0;
  virtual int SortMethod() = 0;
  virtual bool SortAscending() = 0;
  virtual void SortItems(CFileItemList& items) = 0;
  virtual void FormatItemLabels() = 0;
  virtual void UpdateButtons();
  virtual void OnPopupMenu(int iItem);

  void ClearFileItems();
  void OnSort();
  virtual bool Update(const CStdString &strDirectory) { return false; }; // CONSOLIDATE??
  virtual void OnClick(int iItem) {};  // CONSOLIDATE??
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items) { return false; }; //FIXME - this should be in all classes

  void GoParentFolder();
  virtual void OnInfo(int iItem);
  virtual void OnScan() {};
  void DisplayEmptyDatabaseMessage(bool bDisplay);

  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder);
  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(int iItem);

  void OnResumeItem(int iItem);

  int  ResumeItemOffset(int iItem);
  void AddItemToPlayList(const CFileItem* pItem);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);
  void PlayMovies(VECMOVIESFILES &movies, long lStartOffset);
  void ShowShareErrorMessage(CFileItem* pItem);

  CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CDirectoryHistory m_history;
  int m_iItemSelected;
  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
  int m_iLastControl;
  bool m_bDisplayEmptyDatabaseMessage;
  CStdString m_strParentPath; ///< Parent path to handle going up a dir
  int m_iViewAsIcons;
  int m_iViewAsIconsRoot;
  CGUIViewControl m_viewControl;  ///< Handles our various views
};
