/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralHID.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "input/ButtonTranslator.h"

using namespace PERIPHERALS;
using namespace std;

CPeripheralHID::CPeripheralHID(const PeripheralScanResult& scanResult) :
  CPeripheral(scanResult)
{
  m_strDeviceName = scanResult.m_strDeviceName.empty() ? g_localizeStrings.Get(35001) : scanResult.m_strDeviceName;
  m_features.push_back(FEATURE_HID);
}

CPeripheralHID::~CPeripheralHID(void)
{
  if (!m_strKeymap.empty() && !GetSettingBool("do_not_use_custom_keymap"))
  {
    CLog::Log(LOGDEBUG, "%s - switching active keymapping to: default", __FUNCTION__);
    CButtonTranslator::GetInstance().RemoveDevice(m_strKeymap);
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
      m_strKeymap = StringUtils::Format("v%sp%s", VendorIdAsString(), ProductIdAsString());
      SetSetting("keymap", m_strKeymap);
    }

    if (!IsSettingVisible("keymap"))
      SetSettingVisible("do_not_use_custom_keymap", false);

    if (!m_strKeymap.empty())
    {
      bool bKeymapEnabled(!GetSettingBool("do_not_use_custom_keymap"));
      if (bKeymapEnabled)
      {
        CLog::Log(LOGDEBUG, "%s - adding keymapping for: %s", __FUNCTION__, m_strKeymap.c_str());
        CButtonTranslator::GetInstance().AddDevice(m_strKeymap);
      }
      else if (!bKeymapEnabled)
      {
        CLog::Log(LOGDEBUG, "%s - removing keymapping for: %s", __FUNCTION__, m_strKeymap.c_str());
        CButtonTranslator::GetInstance().RemoveDevice(m_strKeymap);
      }
    }

    CLog::Log(LOGDEBUG, "%s - initialised HID device (%s:%s)", __FUNCTION__, m_strVendorId.c_str(), m_strProductId.c_str());
  }

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralHID::OnSettingChanged(const std::string &strChangedSetting)
{
  if (m_bInitialised && ((StringUtils::EqualsNoCase(strChangedSetting, "keymap") && !GetSettingBool("do_not_use_custom_keymap")) || StringUtils::EqualsNoCase(strChangedSetting, "keymap_enabled")))
  {
    m_bInitialised = false;
    InitialiseFeature(FEATURE_HID);
  }
}

