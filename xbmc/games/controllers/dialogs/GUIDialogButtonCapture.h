/*
 *      Copyright (C) 2016 Team Kodi
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

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMapper.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <vector>

namespace GAME
{
  class CGUIDialogButtonCapture : public JOYSTICK::IButtonMapper,
                                  public Observer,
                                  protected CThread
  {
  public:
    CGUIDialogButtonCapture();

    // implementation of IButtonMapper
    virtual std::string ControllerID(void) const override;
    virtual bool NeedsCooldown(void) const override { return false; }
    virtual bool MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                              JOYSTICK::IActionMap* actionMap,
                              const JOYSTICK::CDriverPrimitive& primitive) override;
    virtual void OnEventFrame(const JOYSTICK::IButtonMap* buttonMap, bool bMotion) override { }
    virtual void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override { }

    // implementation of Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;

    void Show();

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    bool AddPrimitive(const JOYSTICK::CDriverPrimitive& primitive);

    std::string GetDialogText();

    void InstallHooks();
    void RemoveHooks();

    static std::string GetPrimitiveName(const JOYSTICK::CDriverPrimitive& primitive);

    // Button capture parameters
    std::string m_deviceName;
    std::vector<JOYSTICK::CDriverPrimitive> m_capturedPrimitives;
    CEvent m_captureEvent;
  };
}
