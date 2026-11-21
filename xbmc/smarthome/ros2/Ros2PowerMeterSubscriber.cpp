/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Ros2PowerMeterSubscriber.h"

#include "utils/log.h"

#include <condition_variable>
#include <exception>
#include <mutex>

#include <oasis_msgs/msg/power_meter.hpp>
#include <rclcpp/expand_topic_or_service_name.hpp>
#include <rclcpp/rclcpp.hpp>

using namespace KODI::SMART_HOME;

struct CRos2PowerMeterSubscriber::State
{
  explicit State(ActivityCallback callback) : activityCallback(std::move(callback)) {}

  ActivityCallback activityCallback;
  std::shared_ptr<rclcpp::Subscription<oasis_msgs::msg::PowerMeter>> subscription;
  std::optional<PowerMeterReading> reading;
  bool active{false};
  unsigned int activeCallbacks{0};
  std::mutex mutex;
  std::condition_variable callbackFinished;
};

CRos2PowerMeterSubscriber::CRos2PowerMeterSubscriber(std::string topic,
                                                     ActivityCallback activityCallback)
  : m_topic(std::move(topic)),
    m_state(std::make_shared<State>(std::move(activityCallback)))
{
}

CRos2PowerMeterSubscriber::~CRos2PowerMeterSubscriber()
{
  Deinitialize();
}

void CRos2PowerMeterSubscriber::Initialize(const std::shared_ptr<rclcpp::Node>& node)
{
  rclcpp::expand_topic_or_service_name(m_topic, node->get_name(), node->get_namespace());

  const std::weak_ptr<State> weakState = m_state;
  auto subscription = node->create_subscription<oasis_msgs::msg::PowerMeter>(
      m_topic, rclcpp::SensorDataQoS(),
      [weakState](const oasis_msgs::msg::PowerMeter::SharedPtr message)
      {
        const auto state = weakState.lock();
        if (!state)
          return;

        ActivityCallback activityCallback;
        {
          std::lock_guard lock(state->mutex);
          if (!state->active)
            return;
          state->reading = PowerMeterReading{message->voltage, message->current, message->power};
          ++state->activeCallbacks;
          activityCallback = state->activityCallback;
        }

        // The callback can enter another manager, so invoke it without the
        // subscriber mutex
        try
        {
          if (activityCallback)
            activityCallback();
        }
        catch (const std::exception& error)
        {
          CLog::Log(LOGERROR, "ROS2: Power-meter activity callback failed: {}", error.what());
        }
        catch (...)
        {
          CLog::Log(LOGERROR,
                    "ROS2: Power-meter activity callback failed with an unknown exception");
        }

        {
          std::lock_guard lock(state->mutex);
          --state->activeCallbacks;
        }
        state->callbackFinished.notify_all();
      });

  std::lock_guard lock(m_state->mutex);
  m_state->active = true;
  m_state->subscription = std::move(subscription);
}

void CRos2PowerMeterSubscriber::Deinitialize()
{
  std::unique_lock lock(m_state->mutex);
  m_state->active = false;
  m_state->subscription.reset();
  m_state->reading.reset();
  m_state->callbackFinished.wait(lock, [this] { return m_state->activeCallbacks == 0; });
}

std::optional<PowerMeterReading> CRos2PowerMeterSubscriber::Reading() const
{
  std::lock_guard lock(m_state->mutex);
  return m_state->reading;
}
