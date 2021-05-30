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
#include <vector>

class CAppParams;

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace GAME
{
class CGameServices;
}

namespace SMART_HOME
{
class CSmartHomeGuiBridge;
class CSmartHomeGuiManager;
class CSmartHomeInputManager;
class IRos2;

class CSmartHomeServices
{
public:
  CSmartHomeServices(PERIPHERALS::CPeripherals& peripheralManager);
  ~CSmartHomeServices();

  void Initialize(GAME::CGameServices& gameServices);
  void Deinitialize();

  // Smart home subsystems
  IRos2* Ros2() { return m_ros2.get(); }
  CSmartHomeGuiBridge& GuiBridge(const std::string& pubSubTopic);

  //! @todo Remove GUI dependency
  void FrameMove();

private:
  static std::vector<std::string> GetCmdLineArgs();

  // Subsystems
  std::unique_ptr<CSmartHomeGuiManager> m_guiManager;
  std::unique_ptr<CSmartHomeInputManager> m_inputManager;
  std::unique_ptr<IRos2> m_ros2;
};

} // namespace SMART_HOME
} // namespace KODI
