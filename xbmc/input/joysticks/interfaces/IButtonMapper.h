/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <string>

namespace KODI
{
namespace KEYMAP
{
class IKeymap;
} // namespace KEYMAP

namespace JOYSTICK
{
class CDriverPrimitive;
class IButtonMap;
class IButtonMapCallback;

/*!
 * \ingroup joystick
 *
 * \brief Button mapper interface to assign the driver's raw button/hat/axis
 *        elements to physical joystick features using a provided button map.
 *
 * \sa IButtonMap
 */
class IButtonMapper
{
public:
  IButtonMapper() = default;

  virtual ~IButtonMapper() = default;

  /*!
   * \brief The add-on ID of the game controller associated with this button mapper
   *
   * \return The ID of the add-on extending kodi.game.controller
   */
  virtual std::string ControllerID(void) const = 0;

  /*!
   * \brief Return true if the button mapper wants a cooldown between button
   *        mapping commands
   *
   * \return True to only send button mapping commands that occur after a small
   *         timeout from the previous command.
   */
  virtual bool NeedsCooldown(void) const = 0;

  /*!
   * \brief Return true if the button mapper accepts primitives of the given type
   *
   * \param type The primitive type
   *
   * \return True if the button mapper can map the primitive type, false otherwise
   */
  virtual bool AcceptsPrimitive(PRIMITIVE_TYPE type) const = 0;

  /*!
   * \brief Handle button/hat press or axis threshold
   *
   * \param buttonMap  The button map being manipulated
   * \param keymap     An interface capable of translating features to Kodi actions
   * \param primitive  The driver primitive
   *
   * Called in the same thread as \ref IButtonMapper::OnFrame.
   *
   * \return True if driver primitive was mapped to a feature
   */
  virtual bool MapPrimitive(IButtonMap* buttonMap,
                            KEYMAP::IKeymap* keyMap,
                            const CDriverPrimitive& primitive) = 0;

  /*!
   * \brief Called once per event frame to notify the implementation of motion status
   *
   * \param buttonMap The button map passed to MapPrimitive() (shall not be modified)
   * \param bMotion True if a previously-mapped axis is still in motion
   *
   * This allows the implementer to wait for an axis to be centered before
   * allowing it to be used as Kodi input.
   *
   * If mapping finishes on an axis, then the axis will still be pressed and
   * sending input every frame when the mapping ends. For example, when the
   * right analog stick is the last feature to be mapped, it is still pressed
   * when mapping ends and immediately sends Volume Down actions.
   *
   * The fix is to allow implementers to wait until all axes are motionless
   * before detaching themselves.
   *
   * Called in the same thread as \ref IButtonMapper::MapPrimitive.
   */
  virtual void OnEventFrame(const IButtonMap* buttonMap, bool bMotion) = 0;

  /*!
   * \brief Called when an axis has been detected after mapping began
   *
   * \param axisIndex The index of the axis being discovered
   *
   * Some joystick drivers don't report an initial value for analog axes.
   *
   * Called in the same thread as \ref IButtonMapper::MapPrimitive.
   */
  virtual void OnLateAxis(const IButtonMap* buttonMap, unsigned int axisIndex) = 0;

  // Button map callback interface
  void SetButtonMapCallback(const std::string& deviceLocation, IButtonMapCallback* callback)
  {
    m_callbacks[deviceLocation] = callback;
  }
  void ResetButtonMapCallbacks(void) { m_callbacks.clear(); }
  std::map<std::string, IButtonMapCallback*>& ButtonMapCallbacks(void) { return m_callbacks; }

private:
  std::map<std::string, IButtonMapCallback*> m_callbacks; // Device location -> callback
};
} // namespace JOYSTICK
} // namespace KODI
