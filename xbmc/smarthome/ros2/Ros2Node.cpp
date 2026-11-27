/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Node.h"

#include "Ros2InputPublisher.h"
#include "Ros2NowPlayingPublisher.h"
#include "Ros2PowerMeterManager.h"
#include "Ros2SystemHealthManager.h"
#include "Ros2VehicleManager.h"
#include "Ros2VideoSubscription.h"
#include "ServiceBroker.h"
#include "network/Network.h"
#include "platform/Environment.h"
#include "smarthome/utils/RosNameUtils.h"
#include "threads/Thread.h"

#include <sstream>

#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

namespace
{
// Name of the ROS node (hostname is concatenated at the end)
constexpr const char* NODE_NAME_PREFIX = "kodi_"; //! @todo Get from version.txt

// Name of the OS thread
constexpr const char* THREAD_NAME = "ROS2"; // TODO

// Hostname to use when the system hostname can't be retrieved
constexpr const char* HOSTNAME_UNKNOWN = "unknown";
} // namespace

CRos2Node::CRos2Node(CSmartHomeInputManager& inputManager)
  : m_inputManager(inputManager),
    m_rosNamespace(NormalizeRosNamespace(CEnvironment::getenv("ROS_NAMESPACE"))),
    m_systemHealthManager(std::make_unique<CRos2SystemHealthManager>(m_rosNamespace)),
    m_powerMeterManager(std::make_unique<CRos2PowerMeterManager>(
        m_rosNamespace,
        [this](const std::string& systemName) { m_systemHealthManager->MarkActive(systemName); })),
    m_vehicleManager(std::make_unique<CRos2VehicleManager>(m_rosNamespace))
{
}

CRos2Node::~CRos2Node() = default;

void CRos2Node::Initialize()
{
  std::string hostname;
  if (!CServiceBroker::GetNetwork().GetHostName(hostname) || hostname.empty())
    hostname = HOSTNAME_UNKNOWN;

  // Replace "-" with "_" in hostname
  StringUtils::Replace(hostname, '-', '_');

  // Can't make virtual calls from constructor
  m_node = std::make_shared<rclcpp::Node>(NODE_NAME_PREFIX + hostname, m_rosNamespace);
  m_executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  m_executor->add_node(m_node);
  m_thread = std::make_unique<CThread>(this, THREAD_NAME);

  // Managers
  m_systemHealthManager->Initialize(m_node);
  m_powerMeterManager->Initialize(m_node);
  m_vehicleManager->Initialize(m_node);

  // Publishers
  m_peripheralPublisher =
      std::make_unique<CRos2InputPublisher>(m_node, m_inputManager, hostname, m_rosNamespace);
  m_peripheralPublisher->Initialize();
  m_nowPlayingPublisher =
      std::make_unique<CRos2NowPlayingPublisher>(m_node, std::move(hostname), m_rosNamespace);
  m_nowPlayingPublisher->Initialize();

  // Create thread
  m_thread->Create(false);
}

void CRos2Node::Deinitialize()
{
  // Deinitialize ROS
  if (m_executor)
    m_executor->cancel();

  rclcpp::shutdown();

  // Stop thread
  if (m_thread)
    m_thread->StopThread(true);

  if (m_executor && m_node)
    m_executor->remove_node(m_node);

  m_powerMeterManager->Deinitialize();
  m_systemHealthManager->Deinitialize();

  for (const auto& videoSub : m_videoSubs)
    videoSub.second.subscription->Deinitialize();
  m_videoSubs.clear();

  if (m_nowPlayingPublisher)
  {
    m_nowPlayingPublisher->Deinitialize();
    m_nowPlayingPublisher.reset();
  }

  if (m_peripheralPublisher)
  {
    m_peripheralPublisher->Deinitialize();
    m_peripheralPublisher.reset();
  }

  m_thread.reset();
  m_executor.reset();
  m_node.reset();
}

void CRos2Node::RegisterImageTopic(CSmartHomeGuiBridge& guiBridge,
                                   const ImageSubscriptionKey& subscription)
{
  auto it = m_videoSubs.find(subscription);
  if (it != m_videoSubs.end())
  {
    ++it->second.referenceCount;
    return;
  }

  auto videoSubscription = std::make_unique<CRos2VideoSubscription>(
      m_node, guiBridge, subscription.first, subscription.second);
  videoSubscription->Initialize();
  m_videoSubs.emplace(subscription, VideoSubscription{std::move(videoSubscription), 1});
}

void CRos2Node::UnregisterImageTopic(const ImageSubscriptionKey& subscription)
{
  auto it = m_videoSubs.find(subscription);
  if (it == m_videoSubs.end())
    return;

  if (--it->second.referenceCount == 0)
  {
    it->second.subscription->Deinitialize();
    m_videoSubs.erase(it);
  }
}

ISystemHealthHUD* CRos2Node::GetSystemHealthHUD() const
{
  return m_systemHealthManager.get();
}

IPowerMeterHUD* CRos2Node::GetPowerMeterHUD() const
{
  return m_powerMeterManager.get();
}

IVehicleHUD* CRos2Node::GetVehicleHUD() const
{
  return m_vehicleManager.get();
}

void CRos2Node::FrameMove()
{
  for (const auto& videoSub : m_videoSubs)
    videoSub.second.subscription->FrameMove();
}

void CRos2Node::Run()
{
  // Enter ROS main loop
  if (m_executor)
    m_executor->spin();
}
