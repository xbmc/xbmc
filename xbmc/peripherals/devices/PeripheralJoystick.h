/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Peripheral.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IDriverReceiver.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>
#include <vector>

#define JOYSTICK_PORT_UNKNOWN (-1)

namespace KODI
{
namespace JOYSTICK
{
class CDeadzoneFilter;
class CKeymapHandling;
class CRumbleGenerator;
class IButtonMap;
class IDriverHandler;
class IInputHandler;
} // namespace JOYSTICK
} // namespace KODI

namespace PERIPHERALS
{
class CPeripherals;

class CPeripheralJoystick : public CPeripheral, //! @todo extend CPeripheralHID
                            public KODI::JOYSTICK::IDriverReceiver
{
public:
  CPeripheralJoystick(CPeripherals& manager,
                      const PeripheralScanResult& scanResult,
                      CPeripheralBus* bus);

  ~CPeripheralJoystick(void) override;

  // implementation of CPeripheral
  bool InitialiseFeature(const PeripheralFeature feature) override;
  void OnUserNotification() override;
  bool TestFeature(PeripheralFeature feature) override;
  void RegisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler,
                                     bool bPromiscuous) override;
  void UnregisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler) override;
  KODI::JOYSTICK::IDriverReceiver* GetDriverReceiver() override { return this; }
  IKeymap* GetKeymap(const std::string& controllerId) override;
  CDateTime LastActive() override { return m_lastActive; }

  bool OnButtonMotion(unsigned int buttonIndex, bool bPressed);
  bool OnHatMotion(unsigned int hatIndex, KODI::JOYSTICK::HAT_STATE state);
  bool OnAxisMotion(unsigned int axisIndex, float position);
  void ProcessAxisMotions(void);

  // implementation of IDriverReceiver
  bool SetMotorState(unsigned int motorIndex, float magnitude) override;

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
  unsigned int HatCount(void) const { return m_hatCount; }
  unsigned int AxisCount(void) const { return m_axisCount; }
  unsigned int MotorCount(void) const { return m_motorCount; }
  bool SupportsPowerOff(void) const { return m_supportsPowerOff; }

  /*!
   * \brief Set joystick properties
   */
  void SetProvider(const std::string& provider) { m_strProvider = provider; }
  void SetRequestedPort(int port) { m_requestedPort = port; }
  void SetButtonCount(unsigned int buttonCount) { m_buttonCount = buttonCount; }
  void SetHatCount(unsigned int hatCount) { m_hatCount = hatCount; }
  void SetAxisCount(unsigned int axisCount) { m_axisCount = axisCount; }
  void SetMotorCount(unsigned int motorCount); // specialized to update m_features
  void SetSupportsPowerOff(bool bSupportsPowerOff); // specialized to update m_features

protected:
  void InitializeDeadzoneFiltering();

  void PowerOff();

  struct DriverHandler
  {
    KODI::JOYSTICK::IDriverHandler* handler;
    bool bPromiscuous;
  };

  // State parameters
  std::string m_strProvider;
  int m_requestedPort;
  unsigned int m_buttonCount;
  unsigned int m_hatCount;
  unsigned int m_axisCount;
  unsigned int m_motorCount;
  bool m_supportsPowerOff;
  CDateTime m_lastActive;

  // Input clients
  std::unique_ptr<KODI::JOYSTICK::CKeymapHandling> m_appInput;
  std::unique_ptr<KODI::JOYSTICK::CRumbleGenerator> m_rumbleGenerator;
  std::unique_ptr<KODI::JOYSTICK::IInputHandler> m_joystickMonitor;
  std::unique_ptr<KODI::JOYSTICK::IButtonMap> m_buttonMap;
  std::unique_ptr<KODI::JOYSTICK::CDeadzoneFilter> m_deadzoneFilter;
  std::vector<DriverHandler> m_driverHandlers;

  // Synchronization parameters
  CCriticalSection m_handlerMutex;
};
} // namespace PERIPHERALS
