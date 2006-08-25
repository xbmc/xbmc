#pragma once
#include "GUIMediaWindow.h"
#include "VideoDatabase.h"
#include "playlistplayer.h"
#include "ThumbLoader.h"

class CGUIWindowVideoBase : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowVideoBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void PlayMovie(const CFileItem *item);
  int  GetResumeItemOffset(const CFileItem *item);

private:
  bool IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual void OnPrepareFileItems(CFileItemList &items);

  virtual void OnPopupMenu(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnScan() {};
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(int iItem);

  bool OnClick(int iItem);
  void OnRestartItem(int iItem);
  void OnResumeItem(int iItem);
  void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);
  void DisplayEmptyDatabaseMessage(bool bDisplay);
  void SetDatabaseDirectory(const VECMOVIES &movies, CFileItemList &items);

  void ShowIMDB(CFileItem *item);
  void ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);
  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);

  void AddItemToPlayList(const CFileItem* pItem, int iPlaylist = PLAYLIST_VIDEO);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);
  CStdString GetnfoFile(CFileItem *item);

  void MarkUnWatched(int iItem);
  void MarkWatched(int iItem);
  void UpdateVideoTitle(int iItem);
  bool UpdateVideoTitleXML(const CStdString strIMDBNumber, CStdString& strTitle);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
  bool m_bDisplayEmptyDatabaseMessage;

  CVideoThumbLoader m_thumbLoader;
};
