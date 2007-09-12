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
  virtual bool OnAction(const CAction &action);

  void PlayMovie(const CFileItem *item);
  int  GetResumeItemOffset(const CFileItem *item);

  void AddToDatabase(int iItem);
  static void OnScan(const CStdString& strPath, const SScraperInfo& info, const VIDEO::SScanSettings& settings);

private:
  bool IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual void OnPrepareFileItems(CFileItemList &items);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  void GetNonContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnInfo(int iItem, const SScraperInfo& info);
  virtual void OnAssignContent(int iItem, int iFound, SScraperInfo& info, VIDEO::SScanSettings& settings) {};
  virtual void OnUnAssignContent(int iItem) {};
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items) {};
  virtual CStdString GetQuickpathName(const CStdString& strPath) const {return strPath;};

  bool OnClick(int iItem);
  void OnRestartItem(int iItem);
  void OnResumeItem(int iItem);
  void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);
  void DisplayEmptyDatabaseMessage(bool bDisplay);

  void ShowIMDB(CFileItem *item, const SScraperInfo& info);

  void OnManualIMDB();
  bool CheckMovie(const CStdString& strFileName);

  void AddItemToPlayList(const CFileItem* pItem, CFileItemList &queuedItems);
  void GetStackedFiles(const CStdString &strFileName, std::vector<CStdString> &movies);

  void MarkUnWatched(int iItem);
  void MarkWatched(int iItem);
  void UpdateVideoTitle(int iItem);
  void OnSearch();
  void OnSearchItemFound(const CFileItem* pSelItem);
  int GetScraperForItem(CFileItem *item, SScraperInfo &info, VIDEO::SScanSettings& settings);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
  bool m_bDisplayEmptyDatabaseMessage;

  CVideoThumbLoader m_thumbLoader;
};
