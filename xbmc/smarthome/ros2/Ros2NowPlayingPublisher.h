/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include <oasis_msgs/msg/media_item.hpp>
#include <rclcpp/publisher.hpp>

namespace rclcpp
{
class Node;
class TimerBase;
} // namespace rclcpp

namespace KODI
{
namespace SMART_HOME
{
class CRos2NowPlayingPublisher
{
public:
  CRos2NowPlayingPublisher(std::shared_ptr<rclcpp::Node> node,
                           std::string hostname,
                           std::string rosNamespace);
  ~CRos2NowPlayingPublisher();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

private:
  // ROS messages
  using MediaItem = oasis_msgs::msg::MediaItem;

  // Timer callbacks
  void PublishNowPlaying();

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;
  const std::string m_hostname;
  const std::string m_rosNamespace;

  // ROS parameters
  std::shared_ptr<rclcpp::Publisher<MediaItem>> m_nowPlayingPublisher;
  std::shared_ptr<rclcpp::TimerBase> m_publishNowPlayingTimer;
};
} // namespace SMART_HOME
} // namespace KODI
