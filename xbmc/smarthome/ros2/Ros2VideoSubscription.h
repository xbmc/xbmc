/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include <image_transport/subscriber.hpp>
#include <sensor_msgs/msg/image.hpp>

struct SwsContext;

namespace image_transport
{
class ImageTransport;
}

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
                         const std::string& topic);
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

  // ROS parameters
  std::unique_ptr<image_transport::ImageTransport> m_imgTransport;
  std::unique_ptr<image_transport::Subscriber> m_imgSubscriber;

  // Smart home parameters
  std::unique_ptr<CSmartHomeStreamManager> m_streamManager;
  std::unique_ptr<ISmartHomeStream> m_stream;
  std::unique_ptr<CSmartHomeRenderer> m_renderer;

  // Video parameters
  SwsContext* m_pixelScaler = nullptr;
};
} // namespace SMART_HOME
} // namespace KODI
