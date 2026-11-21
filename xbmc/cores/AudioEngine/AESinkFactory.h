/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Utils/AEAudioFormat.h"
#include "Utils/AEDeviceInfo.h"

#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class IAESink;

namespace AE
{

struct AESinkInfo
{
  std::string m_sinkName;
  AEDeviceInfoList m_deviceInfoList;
};

using CreateSink =
    std::function<std::unique_ptr<IAESink>(std::string& device, AEAudioFormat& desiredFormat)>;
using Enumerate = std::function<void(AEDeviceInfoList& list, bool force)>;
using Cleanup = std::function<void()>;
using HasDeviceVolume = std::function<bool(const std::string& device)>;
using SetDeviceVolume = std::function<bool(const std::string& device, float volume, bool mute)>;

struct AESinkRegEntry
{
  std::string sinkName;
  CreateSink createFunc;
  Enumerate enumerateFunc;
  Cleanup cleanupFunc;
  HasDeviceVolume hasDeviceVolumeFunc;
  SetDeviceVolume setDeviceVolumeFunc;
};

struct AESinkDevice
{
  std::string driver;
  std::string name;
  std::string friendlyName;

  bool IsSameDeviceAs(const AESinkDevice& d) const
  {
    return driver == d.driver && (name == d.name || friendlyName == d.friendlyName);
  }
};

class CAESinkFactory
{
public:
  static void RegisterSink(const AESinkRegEntry& regEntry);
  static void ClearSinks();
  static bool HasSinks();

  static AESinkDevice ParseDevice(const std::string& device);
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);
  static void EnumerateEx(std::vector<AESinkInfo>& list, bool force, const std::string& driver);
  static void Cleanup();
  static bool HasDeviceVolume(const std::string& device);
  static bool SetDeviceVolume(const std::string& device, float volume, bool mute);

protected:
  static std::map<std::string, AESinkRegEntry> m_AESinkRegEntry;
};

}
