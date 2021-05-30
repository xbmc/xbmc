/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Node.h"

#include "Ros2InputPublisher.h"
#include "Ros2VideoSubscription.h"
#include "ServiceBroker.h"
#include "network/Network.h"
#include "threads/Thread.h"

#include <sstream>

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

CRos2Node::CRos2Node(CSmartHomeInputManager& inputManager) : m_inputManager(inputManager)
{
}

CRos2Node::~CRos2Node() = default;

void CRos2Node::Initialize()
{
  std::string hostname;
  if (!CServiceBroker::GetNetwork().GetHostName(hostname) || hostname.empty())
    hostname = HOSTNAME_UNKNOWN;

  // Can't make virtual calls from constructor
  m_node = std::make_shared<rclcpp::Node>(NODE_NAME_PREFIX + hostname);
  m_thread = std::make_unique<CThread>(this, THREAD_NAME);

  // Publishers
  m_peripheralPublisher =
      std::make_unique<CRos2InputPublisher>(m_node, m_inputManager, std::move(hostname));
  m_peripheralPublisher->Initialize();

  // Create thread
  m_thread->Create(false);
}

void CRos2Node::Deinitialize()
{
  // Deinitialize ROS
  rclcpp::shutdown();

  // Stop thread
  if (m_thread)
    m_thread->StopThread(true);

  for (const auto& videoSub : m_videoSubs)
    videoSub.second->Deinitialize();
  m_videoSubs.clear();

  if (m_peripheralPublisher)
  {
    m_peripheralPublisher->Deinitialize();
    m_peripheralPublisher.reset();
  }

  m_thread.reset();
  m_node.reset();
}

void CRos2Node::RegisterImageTopic(CSmartHomeGuiBridge& guiBridge, const std::string& topic)
{
  if (m_videoSubs.find(topic) == m_videoSubs.end())
  {
    auto subscription = std::make_unique<CRos2VideoSubscription>(m_node, guiBridge, topic);
    subscription->Initialize();
    m_videoSubs.insert(std::make_pair(std::move(topic), std::move(subscription)));
  }
}

void CRos2Node::UnregisterImageTopic(const std::string& topic)
{
  auto it = m_videoSubs.find(topic);
  if (it != m_videoSubs.end())
    m_videoSubs.erase(it);
}

void CRos2Node::FrameMove()
{
  for (const auto& videoSub : m_videoSubs)
    videoSub.second->FrameMove();
}

void CRos2Node::Run()
{
  // Enter ROS main loop
  rclcpp::spin(m_node);
}
