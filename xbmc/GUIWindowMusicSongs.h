#pragma once
#include "GUIWindowMusicBase.h"

class CGUIWindowMusicSongs : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicSongs(void);
  virtual ~CGUIWindowMusicSongs(void);

  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void UpdateButtons();
  virtual void OnPopupMenu(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  virtual void OnScan();

  // new method
  virtual void PlayItem(int iItem);

  void DeleteDirectoryCache();
  void DeleteRemoveableMediaDirectoryCache();
};
