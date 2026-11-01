/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/SystemClock.h"

#include <memory>
#include <utility>
#include <vector>

class CFileItemList;

namespace KODI
{
namespace GAME
{
class CDialogGameOSDHelp;

/*!
 * \ingroup games
 */
class CDialogGameOSD : public CGUIDialog
{
public:
  CDialogGameOSD();

  ~CDialogGameOSD() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;

  // Implementation of CGUIWindow via CGUIDialog
  void OnDeinitWindow(int nextWindowID) override;

  /*!
   * \brief Decide if the game should play behind the given dialog
   *
   * If true, the game should be played at regular speed.
   *
   * \param dialog The current dialog
   *
   * \return True if the game should be played at regular speed behind the
   *         dialog, false otherwise
   */
  static bool PlayInBackground(int dialogId);

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;

private:
  void UpdatePlayers();

  std::unique_ptr<CDialogGameOSDHelp> m_helpDialog;
  std::unique_ptr<CFileItemList> m_playerItems;

  // Player number and battery of the players in m_playerItems
  std::vector<std::pair<unsigned int, PERIPHERALS::PeripheralBattery>> m_players;
  XbmcThreads::EndTime<> m_playerTimeout;
};
} // namespace GAME
} // namespace KODI
