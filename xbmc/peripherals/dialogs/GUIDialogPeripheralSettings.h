/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CFileItem;

namespace PERIPHERALS
{
class CPeripherals;

/*!
 * \ingroup peripherals
 */
class CGUIDialogPeripheralSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogPeripheralSettings();
  ~CGUIDialogPeripheralSettings() override;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage& message) override;

  // Implementation of CGUIWindow
  void OnDeinitWindow(int nextWindowID) override;

  void RegisterPeripheralManager(CPeripherals& manager);
  void UnregisterPeripheralManager();

  virtual void SetFileItem(const CFileItem* item);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override;
  void OnCancel() override;
  void OnResetSettings() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  void UpdateIcon(const KODI::GAME::ControllerPtr& controller);

  // Dialog state
  CPeripherals* m_manager{nullptr};
  CFileItem* m_item;
  bool m_initialising = false;

  //! The values that were changed in this dialog, keyed by setting ID. They are applied to the
  //! peripheral when the dialog is confirmed, and discarded when it isn't
  std::map<std::string, std::string> m_changedValues;

  // Defaults waiting to be shown by the next InitializeSettings(), set by OnResetSettings().
  // Only populated while the controls are being re-created.
  std::map<std::string, std::string> m_pendingDefaults;

  //! Whether a reset to defaults was confirmed in this dialog. The peripheral is reset when the
  //! dialog itself is confirmed, and not at all when it isn't
  bool m_resetRequested{false};
};
} // namespace PERIPHERALS
