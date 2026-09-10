/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "utils/Variant.h"

#include <map>
#include <string>

namespace KODI::GAME
{
/*!
 * \ingroup games
 *
 * \brief The cheats found for the game being played
 *
 * One switch per cheat out of the game's cheat file. Built on the settings
 * dialog so the switches are the ones the rest of Kodi uses, and so no skin
 * has to know this dialog exists.
 *
 * Reachable while the game has cheats, and while the add-on that carries them
 * can still be fetched, which is the only place that offer is made.
 */
class CDialogGameCheats : public CGUIDialogSettingsManualBase
{
public:
  CDialogGameCheats();
  ~CDialogGameCheats() override;

protected:
  // Implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  // Implementation of CGUIDialogSettingsBase
  //! Each switch is built with the state the dialog opened in as its default,
  //! so the generic reset would re-apply that rather than restore anything, and
  //! switch cheats back on that were just switched off
  bool AllowResettingSettings() const override { return false; }
  void SetupView() override;
  bool Save() override { return true; }
  std::string GetSettingsLabel(const std::shared_ptr<ISetting>& setting) override;
  void SetDescription(const CVariant& label) override;

  // Implementation of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  //! \brief What is switched on, for the panel beside the list
  std::string EnabledSummary() const;

  //! The name each switch should carry, by setting id
  std::map<std::string, std::string> m_labels;

  //! What to say about a cheat while it is the one focused, by setting id
  std::map<std::string, std::string> m_descriptions;
};
} // namespace KODI::GAME
