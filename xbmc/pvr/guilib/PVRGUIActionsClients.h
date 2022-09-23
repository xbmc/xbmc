/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PVR
{
class CPVRGUIActionsClients
{
public:
  CPVRGUIActionsClients() = default;
  virtual ~CPVRGUIActionsClients() = default;

  /*!
   * @brief Select and invoke client-specific settings actions
   * @return true on success, false otherwise.
   */
  bool ProcessSettingsMenuHooks();

private:
  CPVRGUIActionsClients(const CPVRGUIActionsClients&) = delete;
  CPVRGUIActionsClients const& operator=(CPVRGUIActionsClients const&) = delete;
};

} // namespace PVR
