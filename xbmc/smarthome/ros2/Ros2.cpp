/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2.h"

#include "smarthome/guibridge/SmartHomeGuiManager.h"
#include "smarthome/ros2/Ros2Node.h"
#include "smarthome/ros2/Ros2VideoSubscription.h" //! @todo Header needed?
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;

CRos2::CRos2(CSmartHomeGuiManager& guiManager,
             CSmartHomeInputManager& inputManager,
             std::vector<std::string> cmdLineArgs)
  : m_guiManager(guiManager),
    m_inputManager(inputManager),
    m_cmdLineArgs(std::move(cmdLineArgs)),
    m_node(std::make_unique<CRos2Node>(m_inputManager))
{
}

CRos2::~CRos2() = default;

void CRos2::Initialize()
{
  CLog::Log(LOGDEBUG, "ROS2: Initializing ROS with args \"{}\"",
            StringUtils::Join(m_cmdLineArgs, "\", \""));

  // Convert arguments to flat array for rclcpp::init()
  int argc = 0;
  char const* const* argv = nullptr;
  TranslateArguments(m_cmdLineArgs, argc, argv);

  // Initialize ROS
  rclcpp::init(argc, argv);

  // Initialize node
  m_node->Initialize();
}

void CRos2::Deinitialize()
{
  if (m_node)
  {
    m_node->Deinitialize();
    m_node.reset();
  }

  // Deinitialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Deinitializing ROS");
  rclcpp::shutdown();
}

void CRos2::RegisterImageTopic(const std::string& topic)
{
  CSmartHomeGuiBridge& guiBridge = m_guiManager.GetGuiBridge(topic);
  m_node->RegisterImageTopic(guiBridge, topic);
}

void CRos2::UnregisterImageTopic(const std::string& topic)
{
  m_node->UnregisterImageTopic(topic);
}

IStationHUD* CRos2::GetStationHUD() const
{
  return m_node->GetStationHUD();
}

ITrainHUD* CRos2::GetTrainHUD() const
{
  return m_node->GetTrainHUD();
}

void CRos2::FrameMove()
{
  //! @todo Remove GUI dependency
  m_node->FrameMove();
}

void CRos2::TranslateArguments(const std::vector<std::string>& cmdLineArgs,
                               int& argc,
                               char const* const*& argv)
{
  std::vector<const char*> tempCmdLineArgs;
  tempCmdLineArgs.reserve(cmdLineArgs.size());

  unsigned int i = 0;
  for (const auto& arg : cmdLineArgs)
    tempCmdLineArgs[i++] = arg.c_str();

  argc = static_cast<int>(cmdLineArgs.size());
  argv = argc > 0 ? reinterpret_cast<char const* const*>(cmdLineArgs.data()) : nullptr;
}
