/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/ApplicationPlayLists.h"
#include "application/IApplicationComponent.h"

#include <memory>

class CFileItem;
class CFileItemList;

namespace KODI::APPLICATION
{
enum class Direction
{
  Forward,
  Back
};

/*!
 * \brief Read a playlist file or smart playlist into its entries, as listed, behind the busy
 * dialog.
 * \return false if the user cancelled.
 */
bool ExpandAsListed(const std::shared_ptr<CFileItem>& item, CFileItemList& entries);

//! Tell the user when a request to move on or back found nothing there.
void NotifyIfNothingThere(CApplicationPlayLists::Step step, Direction direction);
} // namespace KODI::APPLICATION

/*!
 * \brief Shows the GUI what the playlists do by themselves: window refreshes, the end of a run, and
 * playback given up after failures.
 */
class CPlayListsGUIListener : public IApplicationComponent,
                              public CApplicationPlayLists::IGUIListener
{
public:
  explicit CPlayListsGUIListener(std::shared_ptr<CApplicationPlayLists> playLists);
  ~CPlayListsGUIListener() override;

  void OnPlayListsChanged() override;
  void OnStopped() override;
  void OnEntriesFailed() override;

private:
  const std::shared_ptr<CApplicationPlayLists> m_playLists;
};
