/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AEDeviceInfo.h"

#include "AEUtil.h"

#include <sstream>

CAEDeviceInfo::operator std::string()
{
  std::stringstream ss;
  ss << "m_deviceName      : " << m_deviceName << '\n';
  ss << "m_displayName     : " << m_displayName << '\n';
  ss << "m_displayNameExtra: " << m_displayNameExtra << '\n';
  ss << "m_deviceType      : " << DeviceTypeToString(m_deviceType) + '\n';
  ss << "m_channels        : " << (std::string)m_channels << '\n';

  ss << "m_sampleRates     : ";
  for (AESampleRateList::iterator itt = m_sampleRates.begin(); itt != m_sampleRates.end(); ++itt)
  {
    if (itt != m_sampleRates.begin())
      ss << ',';
    ss << *itt;
  }
  ss << '\n';

  ss << "m_dataFormats     : ";
  for (AEDataFormatList::iterator itt = m_dataFormats.begin(); itt != m_dataFormats.end(); ++itt)
  {
    if (itt != m_dataFormats.begin())
      ss << ',';
    ss << CAEUtil::DataFormatToStr(*itt);
  }
  ss << '\n';

  ss << "m_streamTypes     : ";
  for (AEDataTypeList::iterator itt = m_streamTypes.begin(); itt != m_streamTypes.end(); ++itt)
  {
    if (itt != m_streamTypes.begin())
      ss << ',';
    ss << CAEUtil::StreamTypeToStr(*itt);
  }
  if (m_streamTypes.empty())
    ss << "No passthrough capabilities";
  ss << '\n';

  return ss.str();
}

std::string CAEDeviceInfo::DeviceTypeToString(enum AEDeviceType deviceType)
{
  switch (deviceType)
  {
    case AE_DEVTYPE_PCM   : return "AE_DEVTYPE_PCM"   ; break;
    case AE_DEVTYPE_IEC958: return "AE_DEVTYPE_IEC958"; break;
    case AE_DEVTYPE_HDMI  : return "AE_DEVTYPE_HDMI"  ; break;
    case AE_DEVTYPE_DP    : return "AE_DEVTYPE_DP"    ; break;
  }
  return "INVALID";
}

std::string CAEDeviceInfo::GetFriendlyName() const
{
  return (m_deviceName != m_displayName) ? m_displayName : m_displayNameExtra;
}

std::string CAEDeviceInfo::ToDeviceString(const std::string& driver) const
{
  std::string device = driver.empty() ? m_deviceName : driver + ":" + m_deviceName;

  const std::string fn = GetFriendlyName();
  if (!fn.empty())
    device += "|" + fn;

  return device;
}
