/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "Peripheral.h"
#include "input/joysticks/DefaultJoystick.h"
#include "input/joysticks/IDriverHandler.h"
#include "input/joysticks/IDriverReceiver.h"
#include "input/joysticks/JoystickMonitor.h"
#include "input/joysticks/JoystickTypes.h"
#include "threads/CriticalSection.h"

#include <string>
#include <vector>

#define JOYSTICK_PORT_UNKNOWN  (-1)

namespace PERIPHERALS
{
  class CPeripheralJoystick : public CPeripheral, //! @todo extend CPeripheralHID
                              public JOYSTICK::IDriverHandler,
                              public JOYSTICK::IDriverReceiver
  {
  public:
    CPeripheralJoystick(const PeripheralScanResult& scanResult, CPeripheralBus* bus);

    virtual ~CPeripheralJoystick(void);

    // implementation of CPeripheral
    virtual bool InitialiseFeature(const PeripheralFeature feature) override;
    virtual void OnUserNotification() override;
    virtual bool TestFeature(PeripheralFeature feature) override;
    virtual void RegisterJoystickDriverHandler(IDriverHandler* handler, bool bPromiscuous) override;
    virtual void UnregisterJoystickDriverHandler(IDriverHandler* handler) override;
    virtual JOYSTICK::IDriverReceiver* GetDriverReceiver() override { return this; }

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, JOYSTICK::HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position) override;
    virtual void ProcessAxisMotions(void) override;

    // implementation of IDriverReceiver
    virtual bool SetMotorState(unsigned int motorIndex, float magnitude) override;

    /*!
     * \brief Get the name of the driver or API providing this joystick
     */
    const std::string& Provider(void) const { return m_strProvider; }

    /*!
     * \brief Get the specific port number requested by this joystick
     *
     * This could indicate that the joystick is connected to a hardware port
     * with a number label; some controllers, such as the Xbox 360 controller,
     * also have LEDs that indicate the controller is on a specific port.
     *
     * \return The 0-indexed port number, or JOYSTICK_PORT_UNKNOWN if no port is requested
     */
    int RequestedPort(void) const { return m_requestedPort; }

    /*!
     * \brief Get the number of elements reported by the driver
     */
    unsigned int ButtonCount(void) const { return m_buttonCount; }
    unsigned int HatCount(void) const    { return m_hatCount; }
    unsigned int AxisCount(void) const   { return m_axisCount; }
    unsigned int MotorCount(void) const  { return m_motorCount; }
    bool SupportsPowerOff(void) const    { return m_supportsPowerOff; }

    /*!
     * \brief Set joystick properties
     */
    void SetProvider(const std::string& provider) { m_strProvider   = provider; }
    void SetRequestedPort(int port)               { m_requestedPort = port; }
    void SetButtonCount(unsigned int buttonCount) { m_buttonCount   = buttonCount; }
    void SetHatCount(unsigned int hatCount)       { m_hatCount      = hatCount; }
    void SetAxisCount(unsigned int axisCount)     { m_axisCount     = axisCount; }
    void SetMotorCount(unsigned int motorCount); // specialized to update m_features
    void SetSupportsPowerOff(bool supportsPowerOff) { m_supportsPowerOff = supportsPowerOff; }

  protected:
    struct DriverHandler
    {
      JOYSTICK::IDriverHandler* handler;
      bool                      bPromiscuous;
    };

    std::string                         m_strProvider;
    int                                 m_requestedPort;
    unsigned int                        m_buttonCount;
    unsigned int                        m_hatCount;
    unsigned int                        m_axisCount;
    unsigned int                        m_motorCount;
    bool                                m_supportsPowerOff;
    JOYSTICK::CDefaultJoystick          m_defaultInputHandler;
    JOYSTICK::CJoystickMonitor          m_joystickMonitor;
    std::vector<DriverHandler>          m_driverHandlers;
    CCriticalSection                    m_handlerMutex;
  };
}
