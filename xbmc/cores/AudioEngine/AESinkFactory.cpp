/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkFactory.h"

#include "Interfaces/AESink.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace AE;

std::map<std::string, AESinkRegEntry> CAESinkFactory::m_AESinkRegEntry;

void CAESinkFactory::RegisterSink(const AESinkRegEntry& regEntry)
{
  m_AESinkRegEntry[regEntry.sinkName] = regEntry;

  IAE *ae = CServiceBroker::GetActiveAE();
  if (ae)
    ae->DeviceChange();
}

void CAESinkFactory::ClearSinks()
{
  m_AESinkRegEntry.clear();
}

bool CAESinkFactory::HasSinks()
{
  return !m_AESinkRegEntry.empty();
}

AESinkDevice CAESinkFactory::ParseDevice(const std::string& device)
{
  AESinkDevice dev{};
  dev.name = device;

  size_t pos = dev.name.find_first_of(':');
  bool found = false;

  if (pos != std::string::npos)
  {
    dev.driver = device.substr(0, pos);

    for (const auto& reg : m_AESinkRegEntry)
    {
      if (!StringUtils::EqualsNoCase(dev.driver, reg.second.sinkName))
        continue;

      dev.name = dev.name.substr(pos + 1, dev.name.length() - pos - 1);
      found = true;
    }
  }

  if (!found)
    dev.driver.clear();

  pos = dev.name.find_last_of('|');

  if (pos != std::string::npos)
  {
    // if no known driver found considers the string starts
    // with the device name and discarts the rest
    if (found)
      dev.friendlyName = dev.name.substr(pos + 1);
    dev.name = dev.name.substr(0, pos);
  }

  return dev;
}

std::unique_ptr<IAESink> CAESinkFactory::Create(const std::string& device,
                                                AEAudioFormat& desiredFormat)
{
  // extract the driver from the device string if it exists
  const AESinkDevice dev = ParseDevice(device);

  AEAudioFormat tmpFormat = desiredFormat;
  std::unique_ptr<IAESink> sink;
  std::string tmpDevice = dev.name;

  for (const auto& reg : m_AESinkRegEntry)
  {
    if (dev.driver != reg.second.sinkName)
      continue;

    sink = reg.second.createFunc(tmpDevice, tmpFormat);
    if (sink)
    {
      desiredFormat = tmpFormat;
      return sink;
    }
  }
  return nullptr;
}

void CAESinkFactory::EnumerateEx(std::vector<AESinkInfo>& list,
                                 bool force,
                                 const std::string& driver)
{
  AESinkInfo info;

  for (const auto& reg : m_AESinkRegEntry)
  {
    if (!driver.empty() && driver != reg.second.sinkName)
      continue;

    info.m_deviceInfoList.clear();
    info.m_sinkName = reg.second.sinkName;
    reg.second.enumerateFunc(info.m_deviceInfoList, force);
    if (!info.m_deviceInfoList.empty())
      list.push_back(info);
  }
}

void CAESinkFactory::Cleanup()
{
  for (const auto& reg : m_AESinkRegEntry)
  {
    if (reg.second.cleanupFunc)
      reg.second.cleanupFunc();
  }
}
