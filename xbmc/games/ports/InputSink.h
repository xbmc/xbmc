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

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/IInputHandler.h"

namespace KODI
{
namespace GAME
{
  class CGameClient;

  class CInputSink : public JOYSTICK::IInputHandler
  {
  public:
    CInputSink(CGameClient &m_gameClient);

    virtual ~CInputSink() = default;

    // Implementation of IInputHandler
    virtual std::string ControllerID(void) const override;
    virtual bool HasFeature(const std::string& feature) const override { return true; }
    virtual bool AcceptsInput(void) override;
    virtual JOYSTICK::INPUT_TYPE GetInputType(const std::string& feature) const override;
    virtual unsigned int GetDelayMs(const std::string& feature) const override { return 0; }
    virtual bool OnButtonPress(const std::string& feature, bool bPressed) override { return true; }
    virtual void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override { }
    virtual bool OnButtonMotion(const std::string& feature, float magnitude) override { return true; }
    virtual bool OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs = 0) override { return true; }
    virtual bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override { return true; }

  private:
    const CGameClient &m_gameClient;
    const ControllerPtr m_controller;
  };
}
}
