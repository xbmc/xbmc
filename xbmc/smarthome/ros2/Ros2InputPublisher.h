/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/input/ControllerState.h"
#include "peripherals/PeripheralTypes.h"
#include "smarthome/input/ISmartHomeJoystickHandler.h"
#include "utils/Observer.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <oasis_msgs/msg/peripheral_input.hpp>
#include <oasis_msgs/msg/peripheral_scan.hpp>
#include <oasis_msgs/srv/capture_input.hpp>
#include <rclcpp/publisher.hpp>
#include <rclcpp/timer.hpp>
#include <rmw/types.h>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CSmartHomeInputManager;

class CRos2InputPublisher : public ISmartHomeJoystickHandler, public Observer
{
public:
  CRos2InputPublisher(std::shared_ptr<rclcpp::Node> node,
                      CSmartHomeInputManager& inputManager,
                      std::string hostname);
  ~CRos2InputPublisher();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Implementation of ISmartHomeJoystickHandler
  void OnInputFrame(const PERIPHERALS::CPeripheral& peripheral,
                    const GAME::CControllerState& controllerState) override;

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  // ROS messages
  using PeripheralInfo = oasis_msgs::msg::PeripheralInfo;
  using PeripheralInput = oasis_msgs::msg::PeripheralInput;
  using PeripheralScan = oasis_msgs::msg::PeripheralScan;
  using Accelerometer = oasis_msgs::msg::Accelerometer;
  using AnalogButton = oasis_msgs::msg::AnalogButton;
  using AnalogStick = oasis_msgs::msg::AnalogStick;
  using DigitalButton = oasis_msgs::msg::DigitalButton;
  using Throttle = oasis_msgs::msg::Throttle;
  using Wheel = oasis_msgs::msg::Wheel;

  // ROS services
  using CaptureInput = oasis_msgs::srv::CaptureInput;

  void HandleCaptureInput(const std::shared_ptr<rmw_request_id_t> request_header,
                          const std::shared_ptr<CaptureInput::Request> request,
                          std::shared_ptr<CaptureInput::Response> response);

  void PublishPeripherals();

  void AddPeripherals(const PERIPHERALS::PeripheralVector& peripherals,
                      std::vector<PeripheralInfo>& peripheralScan);
  void AddDigitalButtons(const GAME::CControllerState& controllerState,
                         std::vector<DigitalButton>& digitalButtons);
  void AddAnalogButtons(const GAME::CControllerState& controllerState,
                        std::vector<AnalogButton>& analogButtons);
  void AddAnalogSticks(const GAME::CControllerState& controllerState,
                       std::vector<AnalogStick>& analogSticks);
  void AddAccelerometers(const GAME::CControllerState& controllerState,
                         std::vector<Accelerometer>& accelerometers);
  void AddThrottles(const GAME::CControllerState& controllerState,
                    std::vector<Throttle>& throttles);
  void AddWheels(const GAME::CControllerState& controllerState, std::vector<Wheel>& wheels);

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;
  CSmartHomeInputManager& m_inputManager;
  const std::string m_hostname;

  // ROS parameters
  rclcpp::Publisher<oasis_msgs::msg::PeripheralScan>::SharedPtr m_peripheralPublisher;
  rclcpp::Publisher<oasis_msgs::msg::PeripheralInput>::SharedPtr m_inputPublisher;
  rclcpp::Service<oasis_msgs::srv::CaptureInput>::SharedPtr m_captureInputService;
  rclcpp::TimerBase::SharedPtr m_publishPeripheralsTimer;

  // Input parameters
  GAME::CControllerState m_previousState;

  // Threading parameters
  std::mutex m_publishMutex;
};
} // namespace SMART_HOME
} // namespace KODI
