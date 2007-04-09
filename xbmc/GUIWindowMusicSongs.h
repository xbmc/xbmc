#pragma once
#include "GUIWindowMusicBase.h"
#include "ThumbLoader.h"

class CGUIWindowMusicSongs : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicSongs(void);
  virtual ~CGUIWindowMusicSongs(void);

  virtual bool OnMessage(CGUIMessage& message);

  void DoScan(const CStdString &strPath);
protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnScan(int iItem);


  // new method
  virtual void PlayItem(int iItem);

  void DeleteDirectoryCache();
  void DeleteRemoveableMediaDirectoryCache();

  CMusicThumbLoader m_thumbLoader;
};
