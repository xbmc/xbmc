/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \ingroup joystick
 *
 * \brief Interface for handling button maps
 */
class IButtonMapCallback
{
public:
  virtual ~IButtonMapCallback() = default;

  /*!
   * \brief Save the button map
   */
  virtual void SaveButtonMap() = 0;

  /*!
   * \brief Clear the list of ignored driver primitives
   *
   * Called if the user begins capturing primitives to be ignored, and
   * no primitives are captured before the dialog is accepted by the user.
   *
   * In this case, the button mapper won't have been given access to the
   * button map, so a callback is needed to indicate that no primitives were
   * captured and the user accepted this.
   */
  virtual void ResetIgnoredPrimitives() = 0;

  /*!
   * \brief Revert changes to the button map since the last time it was loaded
   *        or committed to disk
   */
  virtual void RevertButtonMap() = 0;
};
} // namespace JOYSTICK
} // namespace KODI
