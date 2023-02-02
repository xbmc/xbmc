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

namespace KODI
{
namespace GAME
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
    bool operator!=(const AnalogStick& rhs) const = default;
  };
  struct Accelerometer
  {
    float x;
    float y;
    float z;
    bool operator==(const Accelerometer& rhs) const = default;
    bool operator!=(const Accelerometer& rhs) const = default;
  };
  using Throttle = float;
  using Wheel = float;

  // Constructors
  explicit CControllerState() = default;
  explicit CControllerState(const CControllerState& controllerState) = default;
  explicit CControllerState(const CController& controller);

  // Destructor
  ~CControllerState() = default;

  // Operators
  bool operator==(const CControllerState& rhs) const = default;
  bool operator!=(const CControllerState& rhs) const = default;

  // Controller state
  const std::string& ID() const { return m_controllerId; }

  // Input state (const accessors)
  const std::map<std::string, DigitalButton>& DigitalButtons() const { return m_digitalButtons; }
  const std::map<std::string, AnalogButton>& AnalogButtons() const { return m_analogButtons; }
  const std::map<std::string, AnalogStick>& AnalogSticks() const { return m_analogSticks; }
  const std::map<std::string, Accelerometer>& Accelerometers() const { return m_accelerometers; }
  const std::map<std::string, Throttle>& Throttles() const { return m_throttles; }
  const std::map<std::string, Wheel>& Wheels() const { return m_wheels; }

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
  std::map<std::string, DigitalButton> m_digitalButtons;
  std::map<std::string, AnalogButton> m_analogButtons;
  std::map<std::string, AnalogStick> m_analogSticks;
  std::map<std::string, Accelerometer> m_accelerometers;
  std::map<std::string, Throttle> m_throttles;
  std::map<std::string, Wheel> m_wheels;
};

} // namespace GAME
} // namespace KODI
