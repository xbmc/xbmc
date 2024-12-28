/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AxisConfiguration.h"
#include "AxisDetector.h"
#include "ButtonDetector.h"
#include "HatDetector.h"
#include "KeyDetector.h"
#include "MouseButtonDetector.h"
#include "PointerDetector.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/MouseTypes.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"

#include <chrono>
#include <map>
#include <memory>
#include <stdint.h>

namespace KODI
{
namespace KEYMAP
{
class IKeymap;
} // namespace KEYMAP

namespace JOYSTICK
{
class IButtonMap;
class IButtonMapper;

struct AxisConfiguration;

/*!
 * \ingroup joystick
 *
 * \brief Generic implementation of a class that provides button mapping by
 *        translating driver events to button mapping commands
 *
 * Button mapping commands are invoked instantly for buttons and hats.
 *
 * Button mapping commands are deferred for a short while after an axis is
 * activated, and only one button mapping command will be invoked per
 * activation.
 */
class CButtonMapping : public IDriverHandler,
                       public KEYBOARD::IKeyboardDriverHandler,
                       public MOUSE::IMouseDriverHandler,
                       public IButtonMapCallback
{
public:
  /*!
   * \brief Constructor for CButtonMapping
   *
   * \param buttonMapper Carries out button-mapping commands using <buttonMap>
   * \param buttonMap The button map given to <buttonMapper> on each command
   */
  CButtonMapping(IButtonMapper* buttonMapper, IButtonMap* buttonMap, KEYMAP::IKeymap* keymap);

  ~CButtonMapping() override = default;

  // implementation of IDriverHandler
  bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
  bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) override;
  bool OnAxisMotion(unsigned int axisIndex,
                    float position,
                    int center,
                    unsigned int range) override;
  void OnInputFrame() override;

  // implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override {}

  // implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(MOUSE::BUTTON_ID button) override;

  // implementation of IButtonMapCallback
  void SaveButtonMap() override;
  void ResetIgnoredPrimitives() override;
  void RevertButtonMap() override;

  /*!
   * \brief Process the primitive mapping command
   *
   * First, this function checks if the input should be dropped. This can
   * happen if the input is ignored or the cooldown period is active. If the
   * input is dropped, this returns true with no effect, effectively absorbing
   * the input. Otherwise, the mapping command is sent to m_buttonMapper.
   *
   * \param primitive The primitive being mapped
   * \return True if the mapping command was handled, false otherwise
   */
  bool MapPrimitive(const CDriverPrimitive& primitive);

private:
  bool IsMapping() const;

  void OnLateDiscovery(unsigned int axisIndex);

  CButtonDetector& GetButton(unsigned int buttonIndex);
  CHatDetector& GetHat(unsigned int hatIndex);
  CAxisDetector& GetAxis(unsigned int axisIndex,
                         float position,
                         const AxisConfiguration& initialConfig = AxisConfiguration());
  CKeyDetector& GetKey(XBMCKey keycode);
  CMouseButtonDetector& GetMouseButton(MOUSE::BUTTON_ID buttonIndex);
  CPointerDetector& GetPointer();

  // Construction parameters
  IButtonMapper* const m_buttonMapper;
  IButtonMap* const m_buttonMap;
  KEYMAP::IKeymap* const m_keymap;

  std::map<unsigned int, CButtonDetector> m_buttons;
  std::map<unsigned int, CHatDetector> m_hats;
  std::map<unsigned int, CAxisDetector> m_axes;
  std::map<XBMCKey, CKeyDetector> m_keys;
  std::map<MOUSE::BUTTON_ID, CMouseButtonDetector> m_mouseButtons;
  std::unique_ptr<CPointerDetector> m_pointer;
  std::chrono::time_point<std::chrono::steady_clock> m_lastAction;
  uint64_t m_frameCount = 0;
};
} // namespace JOYSTICK
} // namespace KODI
