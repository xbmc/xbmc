/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIDialogButtonCapture.h"
#include "input/joysticks/DriverPrimitive.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
  class CGUIDialogIgnoreInput : public CGUIDialogButtonCapture
  {
  public:
    CGUIDialogIgnoreInput() = default;

    virtual ~CGUIDialogIgnoreInput() = default;

  protected:
    // implementation of CGUIDialogButtonCapture
    virtual std::string GetDialogText() override;
    virtual std::string GetDialogHeader() override;
    virtual bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                      JOYSTICK::IActionMap* actionMap,
                                      const JOYSTICK::CDriverPrimitive& primitive) override;
    void OnClose(bool bAccepted) override;

  private:
    bool AddPrimitive(const JOYSTICK::CDriverPrimitive& primitive);

    std::string m_deviceName;
    std::vector<JOYSTICK::CDriverPrimitive> m_capturedPrimitives;
  };
}
}
