/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VideoSubscription.h"

#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/rendering/SmartHomeRenderer.h"
#include "smarthome/ros2/Ros2Translator.h"
#include "smarthome/streams/ISmartHomeStream.h"
#include "smarthome/streams/SmartHomeStreamManager.h"
#include "smarthome/streams/SmartHomeStreamSwFramebuffer.h"
#include "utils/log.h"

#include <cstring>

#include <image_transport/image_transport.hpp>
#include <image_transport/transport_hints.hpp>
#include <rclcpp/rclcpp.hpp>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

using namespace KODI;
using namespace SMART_HOME;

CRos2VideoSubscription::CRos2VideoSubscription(std::shared_ptr<rclcpp::Node> node,
                                               CSmartHomeGuiBridge& guiBridge,
                                               const std::string& topic)
  : m_node(std::move(node)),
    m_guiBridge(guiBridge),
    m_topic(topic),
    m_streamManager(std::make_unique<CSmartHomeStreamManager>()),
    m_renderer(std::make_unique<CSmartHomeRenderer>(m_guiBridge, *m_streamManager))
{
}

CRos2VideoSubscription::~CRos2VideoSubscription() = default;

void CRos2VideoSubscription::Initialize()
{
  // Initialize rendering
  m_renderer->Initialize();

  // Initialize ROS
  m_imgTransport = std::make_unique<image_transport::ImageTransport>(m_node);
  m_imgSubscriber = std::make_unique<image_transport::Subscriber>();

  const auto transportHints = image_transport::TransportHints(m_node.get(), "compressed");

  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", m_topic);

  *m_imgSubscriber = m_imgTransport->subscribe(m_topic, 1, &CRos2VideoSubscription::ReceiveImage,
                                               this, &transportHints);
}

void CRos2VideoSubscription::Deinitialize()
{
  // Deinitialize ROS
  m_imgSubscriber.reset();
  m_imgTransport.reset();

  // Deinitialize stream
  if (m_stream)
    m_streamManager->CloseStream(std::move(m_stream));

  // Deinitialize video
  if (m_pixelScaler != nullptr)
  {
    sws_freeContext(m_pixelScaler);
    m_pixelScaler = nullptr;
  }

  // Deinitialize rendering
  m_renderer->Deinitialize();
}

void CRos2VideoSubscription::FrameMove()
{
  //! @todo Remove GUI dependency
  if (m_renderer)
    m_renderer->FrameMove();
}

void CRos2VideoSubscription::ReceiveImage(const std::shared_ptr<const sensor_msgs::msg::Image>& msg)
{
  const uint32_t width = msg->width;
  const uint32_t height = msg->height;
  const bool isBigEndian = static_cast<bool>(msg->is_bigendian);
  const AVPixelFormat format = CRos2Translator::TranslateEncoding(msg->encoding, isBigEndian);
  const uint32_t stride = msg->step;
  const size_t size = msg->data.size();
  const uint8_t* const data = msg->data.data();

  //! @todo Remove verbose logging
  //CLog::Log(LOGDEBUG, "SMARTHOME: Got frame");

  if (format == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "ROS2: Unknown encoding: {}", msg->encoding);
    return;
  }

  if (stride * height != size)
  {
    CLog::Log(LOGERROR, "ROS2 Video: Invalid stride (width={}, height={}, stride={}, size={}",
              width, height, stride, size);
    return;
  }

  // Create stream if it doesn't exist
  if (!m_stream)
  {
    m_stream = m_streamManager->OpenStream(format, width, height);
    if (!m_stream)
    {
      CLog::Log(LOGERROR, "ROS2: Failed to create stream of type {} ({}x{})", msg->encoding, width,
                height);
      return;
    }
  }

  AVPixelFormat targetFormat = AV_PIX_FMT_NONE;
  uint8_t* targetData = nullptr;
  size_t targetSize = 0;
  if (!m_stream->GetBuffer(width, height, targetFormat, targetData, targetSize) ||
      targetData == nullptr)
  {
    // Getting a buffer could fail if the stream has no visible targets
    return;
  }

  // Pixel conversion
  const unsigned int sourceStride = static_cast<unsigned int>(size / height);
  const unsigned int targetStride = static_cast<unsigned int>(targetSize / height);

  SwsContext*& scalerContext = m_pixelScaler;
  scalerContext = sws_getCachedContext(scalerContext, width, height, format, width, height,
                                       targetFormat, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (scalerContext == nullptr)
  {
    CLog::Log(LOGERROR, "ROS2: Failed to create pixel scaler");
    m_stream->ReleaseBuffer(targetData);
    return;
  }

  uint8_t* src[] = {const_cast<uint8_t*>(data), nullptr, nullptr, nullptr};
  int srcStride[] = {static_cast<int>(sourceStride), 0, 0, 0};
  uint8_t* dst[] = {targetData, nullptr, nullptr, nullptr};
  int dstStride[] = {static_cast<int>(targetStride), 0, 0, 0};

  sws_scale(scalerContext, src, srcStride, 0, height, dst, dstStride);

  m_stream->AddData(width, height, targetData, targetSize);

  m_stream->ReleaseBuffer(targetData);
}
