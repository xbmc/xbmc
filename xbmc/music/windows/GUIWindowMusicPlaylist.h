#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowMusicBase.h"
#include "BackgroundInfoLoader.h"

class CGUIWindowMusicPlayList : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicPlayList(void);
  virtual ~CGUIWindowMusicPlayList(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);

  void RemovePlayListItem(int iItem);
  void MoveItem(int iStart, int iDest);

protected:
  virtual void GoParentFolder() {};
  virtual void UpdateButtons();
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual bool Update(const CStdString& strDirectory);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  void OnMove(int iItem, int iAction);
  virtual bool OnPlayMedia(int iItem);

  void SavePlayList();
  void ClearPlayList();
  void MarkPlaying();

  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);

  int m_movingFrom;
  VECSOURCES m_shares;
};
