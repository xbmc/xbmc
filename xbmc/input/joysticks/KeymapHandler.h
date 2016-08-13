/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include <vector>

namespace JOYSTICK
{
  class CKeymapHandler : public IKeymapHandler
  {
  public:
    CKeymapHandler(void);

    virtual ~CKeymapHandler(void);

    // implementation of IKeymapHandler
    virtual INPUT_TYPE GetInputType(unsigned int keyId) const override;
    virtual void OnDigitalKey(unsigned int keyId, bool bPressed, unsigned int holdTimeMs = 0) override;
    virtual void OnAnalogKey(unsigned int keyId, float magnitude) override;

  private:
    enum BUTTON_STATE
    {
      STATE_UNPRESSED,
      STATE_BUTTON_PRESSED,
      STATE_BUTTON_HELD,
    };

    void ProcessButtonPress(unsigned int keyId, unsigned int holdTimeMs);
    void ProcessButtonRelease(unsigned int keyId);
    bool IsPressed(unsigned int keyId) const;

    static bool SendDigitalAction(unsigned int keyId, unsigned int holdTimeMs = 0);
    static bool SendAnalogAction(unsigned int keyId, float magnitude);

    BUTTON_STATE              m_state;
    unsigned int              m_lastButtonPress;
    unsigned int              m_lastDigitalActionMs;
    std::vector<unsigned int> m_pressedButtons;
  };
}
