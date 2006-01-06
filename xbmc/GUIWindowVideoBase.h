#pragma once
#include "GUIMediaWindow.h"
#include "VideoDatabase.h"
#include "playlistplayer.h"

class CGUIWindowVideoBase : public CGUIMediaWindow
{
public:
  CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowVideoBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void PlayMovie(const CFileItem *item);

private:
  bool IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
  virtual void GoParentFolder();
  virtual void OnClick(int iItem) {};  // CONSOLIDATE??
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory) { return false; }; // CONSOLIDATE??
  virtual void OnSort();

  virtual void SetIMDBThumbs(CFileItemList& items) {};
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items) { return false; }; //FIXME - this should be in all classes

  virtual void OnPopupMenu(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnScan() {};
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);

  void OnResumeItem(int iItem);
  void PlayItem(int iItem);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);
  void DisplayEmptyDatabaseMessage(bool bDisplay);

  void ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder);
  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);

  int  ResumeItemOffset(int iItem);
  void AddItemToPlayList(const CFileItem* pItem, int iPlaylist = PLAYLIST_VIDEO);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);
  void ShowShareErrorMessage(CFileItem* pItem);

  void MarkUnWatched(int iItem);
  void MarkWatched(int iItem);
  void UpdateVideoTitle(int iItem);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
  bool m_bDisplayEmptyDatabaseMessage;
  vector<CStdString> m_vecPathHistory; ///< History of traversed directories

  int m_iShowMode;
};
