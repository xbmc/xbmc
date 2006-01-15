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

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void OnClick(int iItem);
  virtual void SortItems(CFileItemList& items) {};
  virtual void OnRetrieveMusicInfo(CFileItemList& items) {};
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual void ClearFileItems();
  virtual bool Update(const CStdString& strDirectory);
  virtual void OnPopupMenu(int iItem);
  void OnMove(int iItem, int iAction);

  void SavePlayList();
  void ClearPlayList();
  void ShufflePlayList();
  void RemovePlayListItem(int iItem);
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);
  void MoveItem(int iStart, int iDest);

  MAPSONGS m_songsMap;
  CStdString m_strPrevPath;
  CMusicInfoLoader m_tagloader;
  int iPos;
};
