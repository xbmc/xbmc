/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeGuiInfo.h"

#include "GUIInfoManager.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "smarthome/guiinfo/ILabHUD.h"
#include "smarthome/guiinfo/IStationHUD.h"
#include "smarthome/guiinfo/ITrainHUD.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeGuiInfo::CSmartHomeGuiInfo(CGUIInfoManager& infoManager,
                                     ILabHUD& labHud,
                                     IStationHUD& stationHud,
                                     ITrainHUD& trainHud)
  : m_infoManager(infoManager), m_labHud(labHud), m_stationHud(stationHud), m_trainHud(trainHud)
{
}

CSmartHomeGuiInfo::~CSmartHomeGuiInfo() = default;

void CSmartHomeGuiInfo::Initialize()
{
  m_infoManager.RegisterInfoProvider(this);
}

void CSmartHomeGuiInfo::Deinitialize()
{
  m_infoManager.UnregisterInfoProvider(this);
}

bool CSmartHomeGuiInfo::GetLabel(std::string& value,
                                 const CFileItem* item,
                                 int contextWindow,
                                 const GUILIB::GUIINFO::CGUIInfo& info,
                                 std::string* fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SMARTHOME_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SMARTHOME_LAB_CPU:
    {
      value = StringUtils::Format("{} %", m_labHud.CPUPercent());
      return true;
    }
    case SMARTHOME_LAB_MEMORY:
    {
      value = StringUtils::Format("{} %", m_labHud.MemoryPercent());
      return true;
    }
    case SMARTHOME_LAB_CURRENT:
    {
      value = StringUtils::Format("{} mA",
                                  static_cast<unsigned int>(m_labHud.ShuntCurrent() * 1000.0f));
      return true;
    }
    case SMARTHOME_LAB_IR:
    {
      value = StringUtils::Format("{:0.2f} V", m_labHud.IRVoltage());
      return true;
    }
    case SMARTHOME_STATION_SUPPLY:
    {
      value = StringUtils::Format("{:.1f} V", m_stationHud.SupplyVoltage());
      return true;
    }
    case SMARTHOME_STATION_MOTOR:
    {
      value = StringUtils::Format("{:.1f} V", m_stationHud.MotorVoltage());
      return true;
    }
    case SMARTHOME_STATION_CURRENT:
    {
      value = StringUtils::Format("{:.1f} A", m_stationHud.MotorCurrent());
      return true;
    }
    case SMARTHOME_STATION_CPU:
    {
      value = StringUtils::Format("{} %", m_stationHud.CPUPercent());
      return true;
    }
    case SMARTHOME_STATION_MESSAGE:
    {
      value = m_stationHud.Message();
      return true;
    }
    case SMARTHOME_TRAIN_CPU:
    {
      value = StringUtils::Format("{} %", m_trainHud.CPUPercent());
      return true;
    }
    case SMARTHOME_TRAIN_MEMORY:
    {
      value = StringUtils::Format("{} %", m_trainHud.MemoryPercent());
      return true;
    }
    case SMARTHOME_TRAIN_SUPPLY:
    {
      value = StringUtils::Format("{:0.2f} V", m_trainHud.SupplyVoltage());
      return true;
    }
    case SMARTHOME_TRAIN_MESSAGE:
    {
      value = m_trainHud.Message();
      return true;
    }
    default:
      break;
  }

  return false;
}

bool CSmartHomeGuiInfo::GetBool(bool& value,
                                const CGUIListItem* item,
                                int contextWindow,
                                const GUILIB::GUIINFO::CGUIInfo& info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SMARTHOME_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SMARTHOME_HAS_LAB:
    {
      value = m_labHud.IsActive();
      return true;
    }
    case SMARTHOME_HAS_STATION:
    {
      value = m_stationHud.IsActive();
      return true;
    }
    case SMARTHOME_HAS_TRAIN:
    {
      value = m_trainHud.IsActive();
      return true;
    }
    default:
      break;
  }

  return false;
}
