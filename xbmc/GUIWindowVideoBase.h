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
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory) { return false; }; // CONSOLIDATE??
  virtual void OnSort();

  virtual void SetIMDBThumbs(CFileItemList& items) {};

  virtual void OnPopupMenu(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnScan() {};
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);

  void OnResumeItem(int iItem);
  void PlayItem(int iItem);
  virtual void OnPlayMedia(int iItem);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);
  void DisplayEmptyDatabaseMessage(bool bDisplay);

  void ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder);
  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);

  int  ResumeItemOffset(int iItem);
  void AddItemToPlayList(const CFileItem* pItem, int iPlaylist = PLAYLIST_VIDEO);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);

  void MarkUnWatched(int iItem);
  void MarkWatched(int iItem);
  void UpdateVideoTitle(int iItem);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
  bool m_bDisplayEmptyDatabaseMessage;

  int m_iShowMode;
};
