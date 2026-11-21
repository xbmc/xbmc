/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>

#include <image_transport/subscriber.hpp>
#include <rclcpp/time.hpp>
#include <sensor_msgs/msg/image.hpp>

struct SwsContext;

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CSmartHomeGuiBridge;
class CSmartHomeRenderer;
class CSmartHomeStreamManager;
class ISmartHomeStream;

class CRos2VideoSubscription
{
public:
  CRos2VideoSubscription(std::shared_ptr<rclcpp::Node> node,
                         CSmartHomeGuiBridge& guiBridge,
                         const std::string& topic,
                         const std::string& imageTransport);
  ~CRos2VideoSubscription();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // ROS callback
  void ReceiveImage(const std::shared_ptr<const sensor_msgs::msg::Image>& msg);

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;
  CSmartHomeGuiBridge& m_guiBridge;
  const std::string m_topic;
  const std::string m_imageTransport;

  // ROS parameters
  image_transport::Subscriber m_imgSubscriber;

  // Smart home parameters
  std::unique_ptr<CSmartHomeStreamManager> m_streamManager;
  std::unique_ptr<ISmartHomeStream> m_stream;
  std::unique_ptr<CSmartHomeRenderer> m_renderer;

  // Video parameters
  SwsContext* m_pixelScaler = nullptr;
  std::optional<rclcpp::Time> m_lastTimestamp;
  bool m_rendererInitialized{false};
};
} // namespace SMART_HOME
} // namespace KODI
