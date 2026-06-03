/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2InputPublisher.h"

#include "games/controllers/Controller.h"
#include "peripherals/devices/Peripheral.h"
#include "smarthome/input/SmartHomeInputManager.h"
#include "smarthome/ros2/Ros2Translator.h"
#include "utils/log.h"

#include <functional>

#include <oasis_msgs/msg/accelerometer.hpp>
#include <oasis_msgs/msg/analog_button.hpp>
#include <oasis_msgs/msg/analog_stick.hpp>
#include <oasis_msgs/msg/digital_button.hpp>
#include <oasis_msgs/msg/peripheral_info.hpp>
#include <oasis_msgs/msg/peripheral_input.hpp>
#include <oasis_msgs/msg/throttle.hpp>
#include <oasis_msgs/msg/wheel.hpp>
#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace KODI
{
namespace SMART_HOME
{
constexpr const char* ROS_NAMESPACE = "oasis"; //! @todo

constexpr const char* PUBLISH_PERIPHERALS_TOPIC = "peripherals";
constexpr const char* PUBLISH_INPUT_TOPIC = "input";

constexpr const char* SERVICE_CAPTURE_INPUT = "capture_input";
} // namespace SMART_HOME
} // namespace KODI

CRos2InputPublisher::CRos2InputPublisher(std::shared_ptr<rclcpp::Node> node,
                                         CSmartHomeInputManager& inputManager,
                                         std::string hostname)
  : m_node(std::move(node)), m_inputManager(inputManager), m_hostname(std::move(hostname))
{
}

CRos2InputPublisher::~CRos2InputPublisher() = default;

void CRos2InputPublisher::Initialize()
{
  const std::string publishPeripheralsTopic =
      std::string("/") + ROS_NAMESPACE + "/" + m_hostname + "/" + PUBLISH_PERIPHERALS_TOPIC;
  const std::string publishInputTopic =
      std::string("/") + ROS_NAMESPACE + "/" + m_hostname + "/" + PUBLISH_INPUT_TOPIC;
  const std::string serviceCaptureInput =
      std::string("/") + ROS_NAMESPACE + "/" + m_hostname + "/" + SERVICE_CAPTURE_INPUT;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Publishing peripherals to {}", publishPeripheralsTopic);
  CLog::Log(LOGDEBUG, "ROS2: Publishing input to {}", publishInputTopic);
  CLog::Log(LOGDEBUG, "ROS2: Providing capture input service on {}", serviceCaptureInput);

  // Publishers
  m_peripheralPublisher =
      m_node->create_publisher<oasis_msgs::msg::PeripheralScan>(publishPeripheralsTopic, 10);
  m_inputPublisher =
      m_node->create_publisher<oasis_msgs::msg::PeripheralInput>(publishInputTopic, 10);

  // Services
  m_captureInputService = m_node->create_service<oasis_msgs::srv::CaptureInput>(
      serviceCaptureInput, std::bind(&CRos2InputPublisher::HandleCaptureInput, this, _1, _2, _3));

  // Timers
  m_publishPeripheralsTimer =
      m_node->create_wall_timer(10s, std::bind(&CRos2InputPublisher::PublishPeripherals, this));

  // Initialize input
  m_inputManager.RegisterPeripheralObserver(this);

  // Publish first message immediately
  PublishPeripherals();
}

void CRos2InputPublisher::Deinitialize()
{
  // Deinitialize input
  m_inputManager.UnregisterPeripheralObserver(this);

  // Deinitialize ROS
  m_publishPeripheralsTimer.reset();
  m_captureInputService.reset();
  m_inputPublisher.reset();
  m_peripheralPublisher.reset();
}

void CRos2InputPublisher::OnInputFrame(const PERIPHERALS::CPeripheral& peripheral,
                                       const GAME::CControllerState& controllerState)
{
  using Header = std_msgs::msg::Header;

  // Compare states and ignore if state hasn't changed
  if (controllerState == m_previousState)
    return;
  m_previousState = controllerState;

  // Build message
  auto inputMessage = PeripheralInput();

  // Add header and timestamp
  auto header = Header();
  header.stamp = m_node->get_clock()->now();
  header.frame_id = m_hostname; //! @todo
  inputMessage.header = std::move(header);

  inputMessage.name = peripheral.DeviceName();
  inputMessage.address = peripheral.Location();
  inputMessage.controller_profile = controllerState.ID();

  AddDigitalButtons(controllerState, inputMessage.digital_buttons);
  AddAnalogButtons(controllerState, inputMessage.analog_buttons);
  AddAnalogSticks(controllerState, inputMessage.analog_sticks);
  AddAccelerometers(controllerState, inputMessage.accelerometers);
  AddThrottles(controllerState, inputMessage.throttles);
  AddWheels(controllerState, inputMessage.wheels);

  // Publish message
  m_inputPublisher->publish(inputMessage);
}

void CRos2InputPublisher::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    {
      // Prune removed peripherals
      m_inputManager.PrunePeripherals();

      // Publish peripherals
      PublishPeripherals();

      break;
    }
    default:
      break;
  }
}

void CRos2InputPublisher::HandleCaptureInput(
    const std::shared_ptr<rmw_request_id_t> request_header,
    const std::shared_ptr<oasis_msgs::srv::CaptureInput::Request> request,
    std::shared_ptr<oasis_msgs::srv::CaptureInput::Response> response)
{
  // Translate parameters
  const bool capture = (request->capture != 0);
  const std::string peripheralLocation = request->peripheral_address;
  const std::string controllerProfile = request->controller_profile;

  if (capture)
  {
    CLog::Log(LOGDEBUG, "Received request to capture input for {} as type {}", peripheralLocation,
              controllerProfile);
    if (!m_inputManager.OpenJoystick(peripheralLocation, controllerProfile, *this))
      CLog::Log(LOGERROR, "Failed to open joystick");
  }
  else
  {
    CLog::Log(LOGDEBUG, "Received request to uncapture input for {}", peripheralLocation);
    m_inputManager.CloseJoystick(peripheralLocation);
  }
}

void CRos2InputPublisher::PublishPeripherals()
{
  using Header = std_msgs::msg::Header;

  std::lock_guard<std::mutex> lock(m_publishMutex);

  // Do scan
  PERIPHERALS::PeripheralVector peripherals = m_inputManager.GetPeripherals();

  // Build message
  auto scanMessage = PeripheralScan();

  auto header = Header();
  header.stamp = m_node->get_clock()->now();
  header.frame_id = m_hostname; //! @todo
  scanMessage.header = std::move(header);

  AddPeripherals(peripherals, scanMessage.peripherals);

  // Publish message
  m_peripheralPublisher->publish(scanMessage);
}

void CRos2InputPublisher::AddPeripherals(const PERIPHERALS::PeripheralVector& peripherals,
                                         std::vector<PeripheralInfo>& peripheralScan)
{
  for (const PERIPHERALS::PeripheralPtr& peripheral : peripherals)
  {
    auto infoMessage = PeripheralInfo();

    infoMessage.type = CRos2Translator::TranslatePeripheralType(peripheral->Type());
    infoMessage.name = peripheral->DeviceName();
    infoMessage.address = peripheral->Location();
    infoMessage.vendor_id = peripheral->VendorId();
    infoMessage.product_id = peripheral->ProductId();
    if (peripheral->ControllerProfile())
      infoMessage.controller_profile = peripheral->ControllerProfile()->ID();

    peripheralScan.emplace_back(std::move(infoMessage));
  }
}

void CRos2InputPublisher::AddDigitalButtons(
    const GAME::CControllerState& controllerState,
    std::vector<oasis_msgs::msg::DigitalButton>& digitalButtons)
{
  for (auto it : controllerState.DigitalButtons())
  {
    const std::string& featureName = it.first;
    const bool pressed = it.second;

    auto digitalButton = DigitalButton();
    digitalButton.name = featureName;
    digitalButton.pressed = pressed;

    digitalButtons.emplace_back(std::move(digitalButton));
  }
}

void CRos2InputPublisher::AddAnalogButtons(
    const GAME::CControllerState& controllerState,
    std::vector<oasis_msgs::msg::AnalogButton>& analogButtons)
{
  for (auto it : controllerState.AnalogButtons())
  {
    const std::string& featureName = it.first;
    const float magnitude = it.second;

    auto analogButton = AnalogButton();
    analogButton.name = featureName;
    analogButton.magnitude = magnitude;

    analogButtons.emplace_back(std::move(analogButton));
  }
}

void CRos2InputPublisher::AddAnalogSticks(const GAME::CControllerState& controllerState,
                                          std::vector<oasis_msgs::msg::AnalogStick>& analogSticks)
{
  for (auto it : controllerState.AnalogSticks())
  {
    const std::string& featureName = it.first;
    const float x = it.second.x;
    const float y = it.second.y;

    auto analogStick = AnalogStick();
    analogStick.name = featureName;
    analogStick.x = x;
    analogStick.y = y;

    analogSticks.emplace_back(std::move(analogStick));
  }
}

void CRos2InputPublisher::AddAccelerometers(
    const GAME::CControllerState& controllerState,
    std::vector<oasis_msgs::msg::Accelerometer>& accelerometers)
{
  for (auto it : controllerState.Accelerometers())
  {
    const std::string& featureName = it.first;
    const float x = it.second.x;
    const float y = it.second.y;
    const float z = it.second.z;

    auto acceleromter = Accelerometer();
    acceleromter.name = featureName;
    acceleromter.x = x;
    acceleromter.y = y;
    acceleromter.z = z;

    accelerometers.emplace_back(std::move(acceleromter));
  }
}

void CRos2InputPublisher::AddThrottles(const GAME::CControllerState& controllerState,
                                       std::vector<oasis_msgs::msg::Throttle>& throttles)
{
  for (auto it : controllerState.Throttles())
  {
    const std::string& featureName = it.first;
    const float position = it.second;

    auto throttle = Throttle();
    throttle.name = featureName;
    throttle.position = position;

    throttles.emplace_back(std::move(throttle));
  }
}

void CRos2InputPublisher::AddWheels(const GAME::CControllerState& controllerState,
                                    std::vector<oasis_msgs::msg::Wheel>& wheels)
{
  for (auto it : controllerState.Wheels())
  {
    const std::string& featureName = it.first;
    const float position = it.second;

    auto wheel = Wheel();
    wheel.name = featureName;
    wheel.position = position;

    wheels.emplace_back(std::move(wheel));
  }
}
