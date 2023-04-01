/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralHID.h"

#include "guilib/LocalizeStrings.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

using namespace PERIPHERALS;

CPeripheralHID::CPeripheralHID(CPeripherals& manager,
                               const PeripheralScanResult& scanResult,
                               CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35001)
                                                       : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_HID);
}

CPeripheralHID::~CPeripheralHID(void)
{
  if (!m_strKeymap.empty() && !GetSettingBool("do_not_use_custom_keymap"))
  {
    CLog::LogF(LOGDEBUG, "switching active keymapping to: default");
    m_manager.GetInputManager().RemoveKeymap(m_strKeymap);
  }
}

bool CPeripheralHID::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_HID && !m_bInitialised)
  {
    m_bInitialised = true;

    if (HasSetting("keymap"))
      m_strKeymap = GetSettingString("keymap");

    if (m_strKeymap.empty())
    {
      m_strKeymap = StringUtils::Format("v{}p{}", VendorIdAsString(), ProductIdAsString());
      SetSetting("keymap", m_strKeymap);
    }

    if (!IsSettingVisible("keymap"))
      SetSettingVisible("do_not_use_custom_keymap", false);

    if (!m_strKeymap.empty())
    {
      bool bKeymapEnabled(!GetSettingBool("do_not_use_custom_keymap"));
      if (bKeymapEnabled)
      {
        CLog::LogF(LOGDEBUG, "adding keymapping for: {}", m_strKeymap);
        m_manager.GetInputManager().AddKeymap(m_strKeymap);
      }
      else
      {
        CLog::LogF(LOGDEBUG, "removing keymapping for: {}", m_strKeymap);
        m_manager.GetInputManager().RemoveKeymap(m_strKeymap);
      }
    }

    CLog::LogF(LOGDEBUG, "initialised HID device ({}:{})", m_strVendorId, m_strProductId);
  }

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralHID::OnSettingChanged(const std::string& strChangedSetting)
{
  if (m_bInitialised && ((StringUtils::EqualsNoCase(strChangedSetting, "keymap") &&
                          !GetSettingBool("do_not_use_custom_keymap")) ||
                         StringUtils::EqualsNoCase(strChangedSetting, "keymap_enabled")))
  {
    m_bInitialised = false;
    InitialiseFeature(FEATURE_HID);
  }
}
