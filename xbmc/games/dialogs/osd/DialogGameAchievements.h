/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

class CFileItemList;
class CGUIMessage;
class CGUIViewControl;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Lists the achievements of the currently-playing game
 *
 * The list is built from the achievement runtime, which is populated by the
 * game add-on. The dialog performs no RetroAchievements network I/O; badge
 * images are remote URLs resolved by Kodi's texture cache.
 */
class CDialogGameAchievements : public CGUIDialog
{
public:
  CDialogGameAchievements();
  ~CDialogGameAchievements() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  /*!
   * \brief Close the dialog without it ever being drawn
   *
   * Used when OnInitWindow() finds there is nothing to show.
   */
  void Abort();

  /*!
   * \brief Rebuild the list from the achievement runtime
   */
  void RefreshList();

  // Dialog parameters
  std::unique_ptr<CFileItemList> m_items;
  std::unique_ptr<CGUIViewControl> m_viewControl;
};
} // namespace GAME
} // namespace KODI
