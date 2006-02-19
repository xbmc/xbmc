#pragma once
#include "GUIWindowMusicBase.h"
#include "MusicInfoLoader.h"

class CGUIWindowMusicPlayList : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicPlayList(void);
  virtual ~CGUIWindowMusicPlayList(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void RemovePlayListItem(int iItem);
  void MoveItem(int iStart, int iDest);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void SortItems(CFileItemList& items) {};
  virtual void OnRetrieveMusicInfo(CFileItemList& items) {};
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual void ClearFileItems();
  virtual bool Update(const CStdString& strDirectory);
  virtual void OnPopupMenu(int iItem);
  void OnMove(int iItem, int iAction);
  virtual void OnPlayMedia(int iItem);

  void SavePlayList();
  void ClearPlayList();
  void ShufflePlayList();
  
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);

  MAPSONGS m_songsMap;
  CStdString m_strPrevPath;
  CMusicInfoLoader m_tagloader;
  int iPos;
};
