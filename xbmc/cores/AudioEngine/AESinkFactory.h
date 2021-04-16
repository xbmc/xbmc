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

#include <map>
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

typedef IAESink* (*CreateSink)(std::string &device, AEAudioFormat &desiredFormat);
typedef void (*Enumerate)(AEDeviceInfoList &list, bool force);
typedef void (*Cleanup)();

struct AESinkRegEntry
{
  std::string sinkName;
  CreateSink createFunc = nullptr;
  Enumerate enumerateFunc = nullptr;
  Cleanup cleanupFunc = nullptr;
};

class CAESinkFactory
{
public:
  static void RegisterSink(const AESinkRegEntry& regEntry);
  static void ClearSinks();
  static bool HasSinks();

  static void ParseDevice(std::string &device, std::string &driver);
  static IAESink *Create(std::string &device, AEAudioFormat &desiredFormat);
  static void EnumerateEx(std::vector<AESinkInfo>& list, bool force, const std::string& driver);
  static void Cleanup();

protected:
  static std::map<std::string, AESinkRegEntry> m_AESinkRegEntry;
};

}
