/*
 *      Copyright (C) 2012 Team XBMC
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

#include <sstream>
#include "AEDeviceInfo.h"
#include "AEUtil.h"

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
