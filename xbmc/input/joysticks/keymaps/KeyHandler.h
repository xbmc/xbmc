/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IKeyHandler.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <string>
#include <vector>

class CAction;
class IActionListener;
class IKeymap;

namespace KODI
{
namespace JOYSTICK
{
  class IKeymapHandler;

  /*!
   * \ingroup joystick
   * \brief
   */
  class CKeyHandler : public IKeyHandler
  {
  public:
    CKeyHandler(const std::string &keyName, IActionListener *actionHandler, const IKeymap *keymap, IKeymapHandler *keymapHandler);

    virtual ~CKeyHandler() = default;

    // implementation of IKeyHandler
    virtual bool IsPressed() const override { return m_bHeld; }
    virtual bool OnDigitalMotion(bool bPressed, unsigned int holdTimeMs) override;
    virtual bool OnAnalogMotion(float magnitude, unsigned int motionTimeMs) override;

  private:
    void Reset();

    bool HandleActions(std::vector<const KeymapAction*> actions, int windowId, float magnitude, unsigned int holdTimeMs);
    bool HandleRelease(std::vector<const KeymapAction*> actions, int windowId);

    bool HandleAction(const KeymapAction& action, int windowId, float magnitude, unsigned int holdTimeMs);

    // Check criteria for sending a repeat action
    bool SendRepeatAction(unsigned int holdTimeMs);

    // Helper function
    static bool IsPressed(float magnitude);

    // Construction parameters
    const std::string m_keyName;
    IActionListener *const m_actionHandler;
    const IKeymap *const m_keymap;
    IKeymapHandler *const m_keymapHandler;

    // State variables
    bool m_bHeld;
    float m_magnitude;
    unsigned int m_holdStartTimeMs;
    unsigned int m_lastHoldTimeMs;
    bool m_bActionSent;
    unsigned int m_lastActionMs;
    int m_activeWindowId = -1; // Window that activated the key
  };
}
}
