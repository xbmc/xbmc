/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/IRunnable.h"

#include <map>
#include <memory>
#include <string>

class CThread;

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2InputPublisher;
class CRos2StationSubscriber;
class CRos2TrainSubscriber;
class CRos2VideoSubscription;
class CSmartHomeGuiBridge;
class CSmartHomeInputManager;
class IStationHUD;
class ITrainHUD;

class CRos2Node : public IRunnable
{
public:
  CRos2Node(CSmartHomeInputManager& inputManager);
  virtual ~CRos2Node();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // GUI interface
  void RegisterImageTopic(CSmartHomeGuiBridge& guiBridge, const std::string& topic);
  void UnregisterImageTopic(const std::string& topic);
  IStationHUD* GetStationHUD() const;
  ITrainHUD* GetTrainHUD() const;

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  // Implementation of IRunnable
  void Run() override;

  // Construction parameters
  CSmartHomeInputManager& m_inputManager;

  // ROS parameters
  std::shared_ptr<rclcpp::Node> m_node;
  std::map<std::string, std::unique_ptr<CRos2VideoSubscription>> m_videoSubs; // Topic -> subscriber
  std::unique_ptr<CRos2InputPublisher> m_peripheralPublisher;
  std::unique_ptr<CRos2StationSubscriber> m_stationSubscriber;
  std::unique_ptr<CRos2TrainSubscriber> m_trainSubscriber;

  // Threading parameters
  std::unique_ptr<CThread> m_thread;
};
} // namespace SMART_HOME
} // namespace KODI
