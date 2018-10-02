/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    bool AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const override;
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
