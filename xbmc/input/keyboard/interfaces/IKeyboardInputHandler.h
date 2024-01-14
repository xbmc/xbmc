/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/KeyboardTypes.h"

#include <stdint.h>
#include <string>

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \ingroup keyboard
 *
 * \brief Interface for handling input events for keyboards
 *
 * Input events are an abstraction over driver events. Keys are identified by
 * the name defined in the keyboard's controller profile.
 */
class IKeyboardInputHandler
{
public:
  virtual ~IKeyboardInputHandler() = default;

  /*!
   * \brief The add-on ID of the keyboard's controller profile
   *
   * \return The ID of the controller profile add-on
   */
  virtual std::string ControllerID() const = 0;

  /*!
   * \brief Return true if the input handler accepts the given key
   *
   * \param key A key belonging to the controller specified by ControllerID()
   *
   * \return True if the key is used for input, false otherwise
   */
  virtual bool HasKey(const KeyName& key) const = 0;

  /*!
   * \brief A key has been pressed
   *
   * \param key A key belonging to the controller specified by ControllerID()
   * \param mod A combination of modifiers
   * \param unicode The unicode value associated with the key, or 0 if unknown
   *
   * \return True if the event was handled, false otherwise
   */
  virtual bool OnKeyPress(const KeyName& key, Modifier mod, uint32_t unicode) = 0;

  /*!
   * \brief A key has been released
   *
   * \param key A key belonging to the controller specified by ControllerID()
   * \param mod A combination of modifiers
   * \param unicode The unicode value associated with the key, or 0 if unknown
   */
  virtual void OnKeyRelease(const KeyName& key, Modifier mod, uint32_t unicode) = 0;
};
} // namespace KEYBOARD
} // namespace KODI
