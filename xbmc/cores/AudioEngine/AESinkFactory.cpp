/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://kodi.tv
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

#include "AESinkFactory.h"
#include "ServiceBroker.h"
#include "Interfaces/AESink.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include <algorithm>

using namespace AE;

std::map<std::string, AESinkRegEntry> CAESinkFactory::m_AESinkRegEntry;

void CAESinkFactory::RegisterSink(AESinkRegEntry regEntry)
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

void CAESinkFactory::ParseDevice(std::string &device, std::string &driver)
{
  int pos = device.find_first_of(':');
  bool found = false;
  if (pos > 0)
  {
    driver = device.substr(0, pos);

    for (auto reg : m_AESinkRegEntry)
    {
      if (!StringUtils::EqualsNoCase(driver, reg.second.sinkName))
        continue;

      device = device.substr(pos + 1, device.length() - pos - 1);
      found = true;
    }
  }

  if (!found)
    driver.clear();
}

IAESink *CAESinkFactory::Create(std::string &device, AEAudioFormat &desiredFormat)
{
  // extract the driver from the device string if it exists
  std::string driver;
  ParseDevice(device, driver);

  AEAudioFormat tmpFormat = desiredFormat;
  IAESink *sink;
  std::string tmpDevice = device;

  for (auto reg : m_AESinkRegEntry)
  {
    if (driver != reg.second.sinkName)
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

void CAESinkFactory::EnumerateEx(std::vector<AESinkInfo> &list, bool force)
{
  AESinkInfo info;

  for(auto reg : m_AESinkRegEntry)
  {
    info.m_deviceInfoList.clear();
    info.m_sinkName = reg.second.sinkName;
    reg.second.enumerateFunc(info.m_deviceInfoList, force);
    if (!info.m_deviceInfoList.empty())
      list.push_back(info);
  }
}

void CAESinkFactory::Cleanup()
{
  for (auto reg : m_AESinkRegEntry)
  {
    if (reg.second.cleanupFunc)
      reg.second.cleanupFunc();
  }
}
