/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsDatabase.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
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
class CPVRGUIActions : public CPVRGUIActionsChannels,
                       public CPVRGUIActionsDatabase,
                       public CPVRGUIActionsEPG,
                       public CPVRGUIActionsParentalControl,
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
