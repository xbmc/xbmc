#pragma once
/*
 *      Copyright (C) 2010-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "Utils/AEDeviceInfo.h"

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
  static void RegisterSink(AESinkRegEntry regEntry);
  static void ClearSinks();
  static bool HasSinks();

  static void ParseDevice(std::string &device, std::string &driver);
  static IAESink *Create(std::string &device, AEAudioFormat &desiredFormat);
  static void EnumerateEx(std::vector<AESinkInfo> &list, bool force);
  static void Cleanup();

protected:
  static std::map<std::string, AESinkRegEntry> m_AESinkRegEntry;
};

}
