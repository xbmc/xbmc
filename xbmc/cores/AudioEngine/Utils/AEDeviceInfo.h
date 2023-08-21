/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <string>
#include <vector>

typedef std::vector<unsigned int> AESampleRateList;
typedef std::vector<enum AEDataFormat> AEDataFormatList;
typedef std::vector<CAEStreamInfo::DataType> AEDataTypeList;

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
  std::string m_deviceName;	/* the driver device name */
  std::string m_displayName;	/* the friendly display name */
  std::string m_displayNameExtra;	/* additional display name info, ie, monitor name from ELD */
  enum AEDeviceType m_deviceType;	/* the device type, PCM, IEC958 or HDMI */
  CAEChannelInfo m_channels;		/* the channels the device is capable of rendering */
  AESampleRateList m_sampleRates;	/* the samplerates the device is capable of rendering */
  AEDataFormatList m_dataFormats;	/* the dataformats the device is capable of rendering */
  AEDataTypeList m_streamTypes;

  bool m_wantsIECPassthrough;           /* if sink supports passthrough encapsulation is done when set to true */

  bool m_onlyPassthrough{false}; // sink only only should be used for passthrough (audio PT device)
  bool m_onlyPCM{false}; // sink only should be used for PCM (audio device)

  operator std::string();
  static std::string DeviceTypeToString(enum AEDeviceType deviceType);
  std::string GetFriendlyName() const;
  std::string ToDeviceString(const std::string& driver) const;
};

typedef std::vector<CAEDeviceInfo> AEDeviceInfoList;
