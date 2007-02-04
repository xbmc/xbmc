#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoPlaylist(void);
  virtual ~CGUIWindowVideoPlaylist(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:
  virtual bool OnPlayMedia(int iItem);
  virtual void UpdateButtons();
  void MarkPlaying();

  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  void OnMove(int iItem, int iAction);

  void ClearPlayList();
  void RemovePlayListItem(int iItem);
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);
  void MoveItem(int iStart, int iDest);

  void SavePlayList();

  int iPos;
  VECSHARES m_shares;
};
