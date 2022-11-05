/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace ADDON
{

class IAddon;

namespace GUI
{

class CHelpers
{
public:
  /*!
   * @brief This shows an Yes/No dialog with information about the add-on if it is
   * not in the normal status.
   *
   * This asks the user whether he really wants to use the add-on and informs with
   * text why the other status is.
   *
   * @note The dialog is currently displayed for @ref AddonLifecycleState::BROKEN
   * and @ref AddonLifecycleState::DEPRECATED.
   *
   * @param[in] addon Class of the add-on to be checked
   * @return True if user activation is desired, false if not
   */
  static bool DialogAddonLifecycleUseAsk(const std::shared_ptr<const IAddon>& addon);
};

} /* namespace GUI */
} /* namespace ADDON */
