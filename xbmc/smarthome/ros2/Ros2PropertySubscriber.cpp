/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2PropertySubscriber.h"

#include "utils/log.h"

#include <condition_variable>
#include <exception>
#include <mutex>

#include <rclcpp/expand_topic_or_service_name.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/int16.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/int64.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/u_int16.hpp>
#include <std_msgs/msg/u_int32.hpp>
#include <std_msgs/msg/u_int64.hpp>
#include <std_msgs/msg/u_int8.hpp>

using namespace KODI::SMART_HOME;

namespace
{
template<typename Message>
std::shared_ptr<rclcpp::SubscriptionBase> CreateSubscription(
    const std::shared_ptr<rclcpp::Node>& node,
    const std::string& topic,
    std::function<void(SmartHomePropertyValue)> callback)
{
  return node->create_subscription<Message>(
      topic, rclcpp::SensorDataQoS(),
      [callback = std::move(callback)](const typename Message::SharedPtr message)
      { callback(message->data); });
}
} // namespace

struct CRos2PropertySubscriber::State
{
  explicit State(ActivityCallback callback) : activityCallback(std::move(callback)) {}

  ActivityCallback activityCallback;
  std::shared_ptr<rclcpp::SubscriptionBase> subscription;
  std::optional<SmartHomePropertyValue> value;
  bool active{false};
  unsigned int activeCallbacks{0};
  std::mutex mutex;
  std::condition_variable callbackFinished;
};

CRos2PropertySubscriber::CRos2PropertySubscriber(std::string topic,
                                                 SmartHomePropertyType type,
                                                 ActivityCallback activityCallback)
  : m_topic(std::move(topic)),
    m_type(type),
    m_state(std::make_shared<State>(std::move(activityCallback)))
{
}

CRos2PropertySubscriber::~CRos2PropertySubscriber()
{
  Deinitialize();
}

void CRos2PropertySubscriber::Initialize(const std::shared_ptr<rclcpp::Node>& node)
{
  rclcpp::expand_topic_or_service_name(m_topic, node->get_name(), node->get_namespace());

  const std::weak_ptr<State> weakState = m_state;
  auto callback = [weakState](SmartHomePropertyValue value)
  {
    const auto state = weakState.lock();
    if (!state)
      return;

    ActivityCallback activityCallback;
    {
      std::lock_guard lock(state->mutex);
      if (!state->active)
        return;
      state->value = std::move(value);
      ++state->activeCallbacks;
      activityCallback = state->activityCallback;
    }

    // The callback can enter another manager, so invoke it without the subscriber mutex.
    try
    {
      if (activityCallback)
        activityCallback();
    }
    catch (const std::exception& error)
    {
      CLog::Log(LOGERROR, "ROS2: Property subscriber activity callback failed: {}", error.what());
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "ROS2: Property subscriber activity callback failed with an unknown "
                          "exception");
    }

    {
      std::lock_guard lock(state->mutex);
      --state->activeCallbacks;
    }
    state->callbackFinished.notify_all();
  };

  std::shared_ptr<rclcpp::SubscriptionBase> subscription;
  switch (m_type)
  {
    case SmartHomePropertyType::BOOL:
      subscription = CreateSubscription<std_msgs::msg::Bool>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::INT8:
      subscription = CreateSubscription<std_msgs::msg::Int8>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::INT16:
      subscription = CreateSubscription<std_msgs::msg::Int16>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::INT32:
      subscription = CreateSubscription<std_msgs::msg::Int32>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::INT64:
      subscription = CreateSubscription<std_msgs::msg::Int64>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::UINT8:
      subscription = CreateSubscription<std_msgs::msg::UInt8>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::UINT16:
      subscription = CreateSubscription<std_msgs::msg::UInt16>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::UINT32:
      subscription = CreateSubscription<std_msgs::msg::UInt32>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::UINT64:
      subscription = CreateSubscription<std_msgs::msg::UInt64>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::FLOAT32:
      subscription = CreateSubscription<std_msgs::msg::Float32>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::FLOAT64:
      subscription = CreateSubscription<std_msgs::msg::Float64>(node, m_topic, callback);
      break;
    case SmartHomePropertyType::STRING:
      subscription = CreateSubscription<std_msgs::msg::String>(node, m_topic, callback);
      break;
  }

  std::lock_guard lock(m_state->mutex);
  m_state->active = true;
  m_state->subscription = std::move(subscription);
}

void CRos2PropertySubscriber::Deinitialize()
{
  std::unique_lock lock(m_state->mutex);
  m_state->active = false;
  m_state->subscription.reset();
  m_state->value.reset();
  m_state->callbackFinished.wait(lock, [this] { return m_state->activeCallbacks == 0; });
}

std::optional<SmartHomePropertyValue> CRos2PropertySubscriber::Value() const
{
  std::lock_guard lock(m_state->mutex);
  return m_state->value;
}

std::string_view CRos2PropertySubscriber::MessageTypeName(SmartHomePropertyType type)
{
  switch (type)
  {
    case SmartHomePropertyType::BOOL:
      return "std_msgs/msg/Bool";
    case SmartHomePropertyType::INT8:
      return "std_msgs/msg/Int8";
    case SmartHomePropertyType::INT16:
      return "std_msgs/msg/Int16";
    case SmartHomePropertyType::INT32:
      return "std_msgs/msg/Int32";
    case SmartHomePropertyType::INT64:
      return "std_msgs/msg/Int64";
    case SmartHomePropertyType::UINT8:
      return "std_msgs/msg/UInt8";
    case SmartHomePropertyType::UINT16:
      return "std_msgs/msg/UInt16";
    case SmartHomePropertyType::UINT32:
      return "std_msgs/msg/UInt32";
    case SmartHomePropertyType::UINT64:
      return "std_msgs/msg/UInt64";
    case SmartHomePropertyType::FLOAT32:
      return "std_msgs/msg/Float32";
    case SmartHomePropertyType::FLOAT64:
      return "std_msgs/msg/Float64";
    case SmartHomePropertyType::STRING:
      return "std_msgs/msg/String";
  }
  return {};
}
