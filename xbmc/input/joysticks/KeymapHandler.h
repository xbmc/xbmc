/*
 *      Copyright (C) 2015-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "input/joysticks/IKeymapHandler.h"

#include <map>
#include <vector>

class CAction;
class IActionListener;

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * Handles keymaps
   */
  class CKeymapHandler : public IKeymapHandler
  {
  public:
    CKeymapHandler(IActionListener *actionHandler);

    virtual ~CKeymapHandler(void);

    // implementation of IKeymapHandler
    virtual unsigned int GetActionID(unsigned int keyId, int windowId, bool bFallthrough) const override;
    virtual void OnDigitalKey(unsigned int keyId, int windowId, bool bFallthrough, bool bPressed, unsigned int holdTimeMs = 0) override;
    virtual void OnAnalogKey(unsigned int keyId, int windowId, bool bFallthrough, float magnitude, unsigned int motionTimeMs) override;

  private:
    static INPUT_TYPE GetInputType(unsigned int keyId, int windowId, bool bFallthrough);
    void SendDigitalAction(const CAction& action);
    void ProcessButtonRelease(unsigned int keyId);
    bool IsPressed(unsigned int keyId) const;

    // Construction parameter
    IActionListener* const m_actionHandler;

    unsigned int              m_lastButtonPress;
    unsigned int              m_lastDigitalActionMs;
    std::vector<unsigned int> m_pressedButtons;
    std::map<unsigned int, unsigned int> m_holdStartTimes; // Key ID -> hold start time (ms)
  };
}
}
