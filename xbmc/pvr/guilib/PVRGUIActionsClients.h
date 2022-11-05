/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"

namespace PVR
{
class CPVRGUIActionsClients : public IPVRComponent
{
public:
  CPVRGUIActionsClients() = default;
  ~CPVRGUIActionsClients() override = default;

  /*!
   * @brief Select and invoke client-specific settings actions
   * @return true on success, false otherwise.
   */
  bool ProcessSettingsMenuHooks();

private:
  CPVRGUIActionsClients(const CPVRGUIActionsClients&) = delete;
  CPVRGUIActionsClients const& operator=(CPVRGUIActionsClients const&) = delete;
};

namespace GUI
{
// pretty scope and name
using Clients = CPVRGUIActionsClients;
} // namespace GUI

} // namespace PVR
