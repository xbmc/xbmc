/*
 *  Copyright (C) 2021-2026 Team Kodi
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
#include <exception>

#include <image_transport/image_transport.hpp>
#include <image_transport/transport_hints.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rmw/qos_profiles.h>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

using namespace KODI;
using namespace SMART_HOME;

CRos2VideoSubscription::CRos2VideoSubscription(std::shared_ptr<rclcpp::Node> node,
                                               CSmartHomeGuiBridge& guiBridge,
                                               const std::string& topic,
                                               const std::string& imageTransport)
  : m_node(std::move(node)),
    m_guiBridge(guiBridge),
    m_topic(topic),
    m_imageTransport(imageTransport),
    m_streamManager(std::make_unique<CSmartHomeStreamManager>()),
    m_renderer(std::make_unique<CSmartHomeRenderer>(m_guiBridge, *m_streamManager))
{
}

CRos2VideoSubscription::~CRos2VideoSubscription()
{
  Deinitialize();
}

void CRos2VideoSubscription::Initialize()
{
  // Initialize rendering
  m_renderer->Initialize();
  m_rendererInitialized = true;

  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {} using transport {}", m_topic, m_imageTransport);

  // Create subscriber
  try
  {
    m_imgSubscriber = image_transport::create_subscription(
        m_node.get(), m_topic, [this](const sensor_msgs::msg::Image::ConstSharedPtr& msg)
        { ReceiveImage(msg); }, m_imageTransport, rclcpp::QoS{1}.get_rmw_qos_profile());
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "ROS2: Failed to subscribe to {} using transport {}: {}", m_topic,
              m_imageTransport, e.what());
    throw;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "ROS2: Failed to subscribe to {} using transport {}: unknown error",
              m_topic, m_imageTransport);
    throw;
  }
}

void CRos2VideoSubscription::Deinitialize()
{
  // Deinitialize ROS
  m_imgSubscriber.shutdown();

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
  if (m_rendererInitialized)
  {
    m_renderer->Deinitialize();
    m_rendererInitialized = false;
  }

  m_lastTimestamp.reset();
}

void CRos2VideoSubscription::FrameMove()
{
  //! @todo Remove GUI dependency
  if (m_renderer)
    m_renderer->FrameMove();
}

void CRos2VideoSubscription::ReceiveImage(const std::shared_ptr<const sensor_msgs::msg::Image>& msg)
{
  const rclcpp::Time msgTimestamp{msg->header.stamp};
  if (m_lastTimestamp && msgTimestamp <= *m_lastTimestamp)
  {
    const rclcpp::Duration latency = *m_lastTimestamp - msgTimestamp;
    const double latencyMs = static_cast<double>(latency.nanoseconds()) / 1'000'000.0;
    CLog::Log(LOGDEBUG,
              "ROS2 Video: Dropping out-of-order frame {}.{:09} (last {}.{:09}, late by {:.3f} ms)",
              msg->header.stamp.sec, msg->header.stamp.nanosec, m_lastTimestamp->seconds(),
              m_lastTimestamp->nanoseconds(), latencyMs);
    return;
  }
  m_lastTimestamp = msgTimestamp;

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
