/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/settings/PVRSettings.h"

#include <memory>

class CFileItem;

namespace PVR
{
class CPVRTimerInfoTag;

class CPVRGUIActionsPowerManagement : public IPVRComponent
{
public:
  CPVRGUIActionsPowerManagement();
  ~CPVRGUIActionsPowerManagement() override = default;

  /*!
   * @brief Check whether the system Kodi is running on can be powered down
   * (shutdown/reboot/suspend/hibernate) without stopping any active recordings and/or without
   * preventing the start of recordings scheduled for now + pvrpowermanagement.backendidletime.
   * @param bAskUser True to informs user in case of potential data loss. User can decide to allow
   * powerdown anyway. False to not to ask user and to not confirm power down.
   * @return True if system can be safely powered down, false otherwise.
   */
  bool CanSystemPowerdown(bool bAskUser = true) const;

private:
  CPVRGUIActionsPowerManagement(const CPVRGUIActionsPowerManagement&) = delete;
  CPVRGUIActionsPowerManagement const& operator=(CPVRGUIActionsPowerManagement const&) = delete;

  bool AllLocalBackendsIdle(std::shared_ptr<CPVRTimerInfoTag>& causingEvent) const;
  bool EventOccursOnLocalBackend(const std::shared_ptr<CPVRTimerInfoTag>& event) const;
  bool IsNextEventWithinBackendIdleTime() const;

  CPVRSettings m_settings;
};

namespace GUI
{
// pretty scope and name
using PowerManagement = CPVRGUIActionsPowerManagement;
} // namespace GUI

} // namespace PVR
