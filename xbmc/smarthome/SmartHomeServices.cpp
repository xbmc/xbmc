/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeServices.h"

#include "ServiceBroker.h"
#include "application/AppParams.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/guibridge/SmartHomeGuiManager.h"
#include "smarthome/guiinfo/SmartHomeGuiInfo.h"
#include "smarthome/input/SmartHomeInputManager.h"
#include "smarthome/ros2/Ros2.h"
#include "utils/log.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeServices::CSmartHomeServices(PERIPHERALS::CPeripherals& peripheralManager)
  : m_guiManager(std::make_unique<CSmartHomeGuiManager>()),
    m_inputManager(std::make_unique<CSmartHomeInputManager>(peripheralManager)),
    m_ros2(
#if defined(HAS_ROS2)
        std::make_unique<CRos2>(*m_guiManager, *m_inputManager, GetCmdLineArgs())
#endif
    )
{
}

CSmartHomeServices::~CSmartHomeServices() = default;

void CSmartHomeServices::Initialize(GAME::CGameServices& gameServices,
                                    CGUIInfoManager* guiInfoManager)
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Initializing services");

  m_inputManager->Initialize(gameServices);

  if (m_ros2)
  {
    m_ros2->Initialize();

    if (guiInfoManager != nullptr)
    {
      m_guiInfo = std::make_unique<CSmartHomeGuiInfo>(
          *guiInfoManager, *m_ros2->GetSystemHealthHUD(), *m_ros2->GetPowerMeterHUD(),
          *m_ros2->GetVehicleHUD());
      m_guiInfo->Initialize();
    }
  }
}

void CSmartHomeServices::Deinitialize()
{
  CLog::Log(LOGDEBUG, "SMARTHOME: Deinitializing services");

  if (m_guiInfo)
  {
    m_guiInfo->Deinitialize();
    m_guiInfo.reset();
  }

  if (m_ros2)
    m_ros2->Deinitialize();

  m_inputManager->Deinitialize();

  CLog::Log(LOGDEBUG, "SMARTHOME: Services deinitialized");
}

CSmartHomeGuiBridge& CSmartHomeServices::GuiBridge(const ImageSubscriptionKey& subscription)
{
  return m_guiManager->GetGuiBridge(subscription);
}

void CSmartHomeServices::FrameMove()
{
  //! @todo Remove GUI dependency
  if (m_ros2)
    m_ros2->FrameMove();
}

std::vector<std::string> CSmartHomeServices::GetCmdLineArgs()
{
  std::vector<std::string> args;

  auto appParams = CServiceBroker::GetAppParams();
  if (appParams)
    args = appParams->GetRawArgs();

  return args;
}
