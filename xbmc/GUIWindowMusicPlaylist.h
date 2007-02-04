#pragma once
#include "GUIWindowMusicBase.h"
#include "BackgroundInfoLoader.h"

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
  virtual void GoParentFolder() {};
  virtual void UpdateButtons();
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual bool Update(const CStdString& strDirectory);
  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  void OnMove(int iItem, int iAction);
  virtual bool OnPlayMedia(int iItem);

  void SavePlayList();
  void ClearPlayList();
  void MarkPlaying();
  
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);

  int iPos;
  VECSHARES m_shares;
};
