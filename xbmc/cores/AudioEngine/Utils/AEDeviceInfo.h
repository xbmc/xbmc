#pragma once
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

#include <string>
#include <vector>
#include "AEAudioFormat.h"
#include "Utils/AEChannelInfo.h"

typedef std::vector<unsigned int     > AESampleRateList;
typedef std::vector<enum AEDataFormat> AEDataFormatList;

enum AEDeviceType {
  AE_DEVTYPE_PCM,
  AE_DEVTYPE_IEC958,
  AE_DEVTYPE_HDMI,
  AE_DEVTYPE_DP
};

/**
 * This classt provides the details of what the audio output hardware is capable of
 */
class CAEDeviceInfo
{
public:
  std::string       m_deviceName;	/* the driver device name */
  std::string       m_displayName;	/* the friendly display name */
  std::string       m_displayNameExtra;	/* additional display name info, ie, monitor name from ELD */
  enum AEDeviceType m_deviceType;	/* the device type, PCM, IEC958 or HDMI */
  CAEChannelInfo    m_channels;		/* the channels the device is capable of rendering */
  AESampleRateList  m_sampleRates;	/* the samplerates the device is capable of rendering */
  AEDataFormatList  m_dataFormats;	/* the dataformats the device is capable of rendering */

  operator std::string();
  static std::string DeviceTypeToString(enum AEDeviceType deviceType);
};

typedef std::vector<CAEDeviceInfo> AEDeviceInfoList;
