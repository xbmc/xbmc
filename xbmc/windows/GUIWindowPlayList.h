/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogContextMenu.h"
#include "playlists/PlayListTypes.h"

#include <memory>
#include <string>

class CAction;
class CApplicationPlayLists;
class CGUIMessage;

int GetPlayListWindowId(KODI::PLAYLIST::Type type);
void ShowPlayListWindow(KODI::PLAYLIST::Type type);

/*!
 * \brief The window showing what one playlist holds, and moving, removing, saving and playing its
 * entries. Base is the media window of the kind of media the playlist plays.
 */
template<typename Base>
class CGUIWindowPlayList : public Base
{
public:
  explicit CGUIWindowPlayList(KODI::PLAYLIST::Type type);
  ~CGUIWindowPlayList() override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  bool OnBack(int actionID) override;

protected:
  using Base::GetID;

  bool GoParentFolder() override { return false; }
  // what this window lists is already queued
  void OnQueueItem(int iItem, bool first = false) override {}
  bool OnPlayMedia(int iItem, const std::string& player = "") override;
  void UpdateButtons() override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;

  /*!
   * \brief Start the playlist at the entry on this row.
   */
  virtual void PlayEntry(int iItem, const std::string& player);

  //! The list position of the entry on a row; rows are in play order.
  int ListPosition(int iItem) const;

  /*!
   * \brief Stop filling in details of the listed items before the list is changed.
   * \return Whether it was running, and so should be started again afterwards.
   */
  virtual bool StopLoadingItems() { return false; }
  virtual void StartLoadingItems() {}

  const KODI::PLAYLIST::Type m_type;
  const std::shared_ptr<CApplicationPlayLists> m_playLists;

private:
  //! The row listing the entry at a list position, or -1.
  int RowOf(int position) const;

  void OnMove(int iItem, int iAction);
  void MoveItem(int iStart, int iDest);
  void MoveCurrentPlayListItem(int iItem, int iAction);
  void RemovePlayListItem(int iItem);
  void ClearPlayList();
  void SavePlayList();
  void ToggleShuffle();
  void CycleRepeat();

  int m_movingFrom{-1};
};
