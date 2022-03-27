/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRos2.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SMART_HOME
{

class CRos2Node;
class CSmartHomeGuiManager;
class CSmartHomeInputManager;

class CRos2 : public IRos2
{
public:
  CRos2(CSmartHomeGuiManager& guiManager,
        CSmartHomeInputManager& inputManager,
        std::vector<std::string> cmdLineArgs);
  ~CRos2() override;

  // Implementation of IRos2
  void Initialize() override;
  void Deinitialize() override;
  void RegisterImageTopic(const std::string& topic) override;
  void UnregisterImageTopic(const std::string& topic) override;
  IStationHUD* GetStationHUD() const override;
  void FrameMove() override;

private:
  // Utility functions
  // TODO: Test cases
  void TranslateArguments(const std::vector<std::string>& cmdLineArgs,
                          int& argc,
                          char const* const*& argv);

  // Construction parameters
  CSmartHomeGuiManager& m_guiManager;
  CSmartHomeInputManager& m_inputManager;
  std::vector<std::string> m_cmdLineArgs;

  // ROS parameters
  std::unique_ptr<CRos2Node> m_node;
};
} // namespace SMART_HOME
} // namespace KODI
