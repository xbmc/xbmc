/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/settings/PVRSettings.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>

class CFileItem;

namespace PVR
{
enum class ParentalCheckResult
{
  CANCELED,
  FAILED,
  SUCCESS
};

class CPVRChannel;

class CPVRGUIActions : public CPVRGUIActionsChannels,
                       public CPVRGUIActionsEPG,
                       public CPVRGUIActionsPlayback,
                       public CPVRGUIActionsPowerManagement,
                       public CPVRGUIActionsRecordings,
                       public CPVRGUIActionsTimers
{
public:
  CPVRGUIActions();
  virtual ~CPVRGUIActions() = default;

  /*!
   * @brief Select and invoke client-specific settings actions
   * @return true on success, false otherwise.
   */
  bool ProcessSettingsMenuHooks();

  /*!
   * @brief Reset the TV database to it's initial state and delete all the data.
   * @param bResetEPGOnly True to only reset the EPG database, false to reset both PVR and EPG database.
   * @return true on success, false otherwise.
   */
  bool ResetPVRDatabase(bool bResetEPGOnly);

  /*!
   * @brief Check if channel is parental locked. Ask for PIN if necessary.
   * @param channel The channel to do the check for.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalLock(const std::shared_ptr<CPVRChannel>& channel) const;

  /*!
   * @brief Open Numeric dialog to check for parental PIN.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalPIN() const;

  /*!
   * @brief Get the currently selected item path; used across several windows/dialogs to share item selection.
   * @param bRadio True to query the selected path for PVR radio, false for Live TV.
   * @return the path.
   */
  std::string GetSelectedItemPath(bool bRadio) const;

  /*!
   * @brief Set the currently selected item path; used across several windows/dialogs to share item selection.
   * @param bRadio True to set the selected path for PVR radio, false for Live TV.
   * @param path The new path to set.
   */
  void SetSelectedItemPath(bool bRadio, const std::string& path);

  /*!
   * @brief Process info action for the given item.
   * @param item The item.
   */
  bool OnInfo(const std::shared_ptr<CFileItem>& item);

private:
  CPVRGUIActions(const CPVRGUIActions&) = delete;
  CPVRGUIActions const& operator=(CPVRGUIActions const&) = delete;

  mutable CCriticalSection m_critSection;
  CPVRSettings m_settings;
  std::string m_selectedItemPathTV;
  std::string m_selectedItemPathRadio;
};

} // namespace PVR
