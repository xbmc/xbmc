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

namespace PVR
{
enum class ParentalCheckResult
{
  CANCELED,
  FAILED,
  SUCCESS
};

class CPVRChannel;

class CPVRGUIActionsParentalControl : public IPVRComponent
{
public:
  CPVRGUIActionsParentalControl();
  ~CPVRGUIActionsParentalControl() override = default;

  /*!
   * @brief Check if channel is parental locked. Ask for PIN if necessary.
   * @param channel The channel to do the check for.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalLock(const std::shared_ptr<const CPVRChannel>& channel) const;

  /*!
   * @brief Open Numeric dialog to check for parental PIN.
   * @return the result of the check (success, failed, or canceled by user).
   */
  ParentalCheckResult CheckParentalPIN() const;

private:
  CPVRGUIActionsParentalControl(const CPVRGUIActionsParentalControl&) = delete;
  CPVRGUIActionsParentalControl const& operator=(CPVRGUIActionsParentalControl const&) = delete;

  CPVRSettings m_settings;
};

namespace GUI
{
// pretty scope and name
using Parental = CPVRGUIActionsParentalControl;
} // namespace GUI

} // namespace PVR
