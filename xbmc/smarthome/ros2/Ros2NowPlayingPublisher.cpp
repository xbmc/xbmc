/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2NowPlayingPublisher.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfoProviders.h"
#include "guilib/guiinfo/PlayerGUIInfo.h"
#include "smarthome/utils/RosNameUtils.h"
#include "video/VideoInfoTag.h"

#include <functional>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/timer.hpp>
#include <std_msgs/msg/header.hpp>

using namespace KODI;
using namespace SMART_HOME;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace KODI
{
namespace SMART_HOME
{
constexpr const char* PUBLISH_NOW_PLAYING_TOPIC = "now_playing";
} // namespace SMART_HOME
} // namespace KODI

CRos2NowPlayingPublisher::CRos2NowPlayingPublisher(std::shared_ptr<rclcpp::Node> node,
                                                   std::string hostname,
                                                   std::string rosNamespace)
  : m_node(std::move(node)),
    m_hostname(std::move(hostname)),
    m_rosNamespace(std::move(rosNamespace))
{
}

CRos2NowPlayingPublisher::~CRos2NowPlayingPublisher() = default;

void CRos2NowPlayingPublisher::Initialize()
{
  const std::string publishNowPlayingTopic =
      BuildRosName(m_rosNamespace, {m_hostname, PUBLISH_NOW_PLAYING_TOPIC});

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Publishing now playing to {}", publishNowPlayingTopic);

  // Publishers
  m_nowPlayingPublisher =
      m_node->create_publisher<oasis_msgs::msg::MediaItem>(publishNowPlayingTopic, 1);

  // Timers
  m_publishNowPlayingTimer =
      m_node->create_wall_timer(1s, std::bind(&CRos2NowPlayingPublisher::PublishNowPlaying, this));

  // Publish first message immediately
  PublishNowPlaying();
}

void CRos2NowPlayingPublisher::Deinitialize()
{
  // Deinitialize ROS
  m_publishNowPlayingTimer.reset();
  m_nowPlayingPublisher.reset();
}

void CRos2NowPlayingPublisher::PublishNowPlaying()
{
  using Header = std_msgs::msg::Header;

  const std::shared_ptr<const CFileItem> currentItem =
      std::const_pointer_cast<const CFileItem>(g_application.CurrentFileItemPtr());

  const CVideoInfoTag* videoInfoTag = currentItem->GetVideoInfoTag();
  if (videoInfoTag != nullptr)
  {
    // Build message
    auto mediaItemMessage = MediaItem();

    auto headerMessage = Header();
    headerMessage.stamp = m_node->get_clock()->now();
    headerMessage.frame_id = m_hostname; //! @todo
    mediaItemMessage.header = std::move(headerMessage);

    mediaItemMessage.media_type = videoInfoTag->m_type;
    mediaItemMessage.title = videoInfoTag->GetTitle();
    mediaItemMessage.year = videoInfoTag->GetYear();
    mediaItemMessage.poster_url = currentItem->GetArt("poster");
    mediaItemMessage.playtime_secs = std::lrint(g_application.GetTime());

    // Publish message
    m_nowPlayingPublisher->publish(mediaItemMessage);
  }
}
