/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "GUIDialogButtonCapture.h"

#include <string>
#include <vector>
#include <utility>

namespace KODI
{
namespace GAME
{
  class CGUIDialogAxisDetection : public CGUIDialogButtonCapture
  {
  public:
    CGUIDialogAxisDetection() = default;

    virtual ~CGUIDialogAxisDetection() = default;

    // specialization of IButtonMapper via CGUIDialogButtonCapture
    void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override;

  protected:
    // implementation of CGUIDialogButtonCapture
    virtual std::string GetDialogText() override;
    virtual std::string GetDialogHeader() override;
    virtual bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                      IKeymap* keymap,
                                      const JOYSTICK::CDriverPrimitive& primitive) override;
    virtual void OnClose(bool bAccepted) override { }

  private:
    void AddAxis(const std::string& deviceName, unsigned int axisIndex);

    // Axis types
    using DeviceName = std::string;
    using AxisIndex = unsigned int;
    using AxisEntry = std::pair<DeviceName, AxisIndex>;
    using AxisVector = std::vector<AxisEntry>;

    // Axis detection
    AxisVector m_detectedAxes;
  };
}
}
