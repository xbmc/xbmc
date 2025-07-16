/*
 *  Copyright (C) 2022-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

namespace KODI::GAME
{
class CController;

/*!
 * \ingroup games
 *
 * \brief Stores the input state of a controller
 */
class CControllerState
{
public:
  /*
   * Public types
   */
  using DigitalButton = bool;
  using AnalogButton = float;
  struct AnalogStick
  {
    float x;
    float y;
    bool operator==(const AnalogStick& rhs) const = default;
  };
  struct Accelerometer
  {
    float x;
    float y;
    float z;
    bool operator==(const Accelerometer& rhs) const = default;
  };
  using Throttle = float;
  using Wheel = float;

  template<typename CONTROL>
  using ControlMap = std::map<std::string, CONTROL, std::less<>>;

  // Constructors
  explicit CControllerState() = default;
  explicit CControllerState(const CControllerState& controllerState) = default;
  explicit CControllerState(const CController& controller);

  // Destructor
  ~CControllerState() = default;

  // Operators
  bool operator==(const CControllerState& rhs) const = default;

  // Controller state
  const std::string& ID() const { return m_controllerId; }

  // Input state (const accessors)
  const ControlMap<DigitalButton>& DigitalButtons() const { return m_digitalButtons; }
  const ControlMap<AnalogButton>& AnalogButtons() const { return m_analogButtons; }
  const ControlMap<AnalogStick>& AnalogSticks() const { return m_analogSticks; }
  const ControlMap<Accelerometer>& Accelerometers() const { return m_accelerometers; }
  const ControlMap<Throttle>& Throttles() const { return m_throttles; }
  const ControlMap<Wheel>& Wheels() const { return m_wheels; }

  // Input state (mutable accessors)
  DigitalButton GetDigitalButton(const std::string& featureName) const;
  AnalogButton GetAnalogButton(const std::string& featureName) const;
  AnalogStick GetAnalogStick(const std::string& featureName) const;
  Accelerometer GetAccelerometer(const std::string& featureName) const;
  Throttle GetThrottle(const std::string& featureName) const;
  Wheel GetWheel(const std::string& featureName) const;

  // Input state (mutators)
  void SetDigitalButton(const std::string& featureName, DigitalButton state);
  void SetAnalogButton(const std::string& featureName, AnalogButton state);
  void SetAnalogStick(const std::string& featureName, AnalogStick state);
  void SetAccelerometer(const std::string& featureName, Accelerometer state);
  void SetThrottle(const std::string& featureName, Throttle state);
  void SetWheel(const std::string& featureName, Wheel state);

private:
  // Controller state
  std::string m_controllerId;

  // Input state
  ControlMap<DigitalButton> m_digitalButtons;
  ControlMap<AnalogButton> m_analogButtons;
  ControlMap<AnalogStick> m_analogSticks;
  ControlMap<Accelerometer> m_accelerometers;
  ControlMap<Throttle> m_throttles;
  ControlMap<Wheel> m_wheels;
};

} // namespace KODI::GAME
