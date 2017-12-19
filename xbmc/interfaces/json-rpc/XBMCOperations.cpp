/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBMCOperations.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/Variant.h"
#include "powermanagement/PowerManager.h"
#include "ServiceBroker.h"

using namespace JSONRPC;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CXBMCOperations::GetInfoLabels(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<std::string> info;

  for (unsigned int i = 0; i < parameterObject["labels"].size(); i++)
  {
    std::string field = parameterObject["labels"][i].asString();
    StringUtils::ToLower(field);

    info.push_back(parameterObject["labels"][i].asString());
  }

  if (!info.empty())
  {
    std::vector<std::string> infoLabels;
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_INFOLABEL, -1, -1, static_cast<void*>(&infoLabels), "", info);

    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i]] = infoLabels[i];
    }
  }

  return OK;
}

JSONRPC_STATUS CXBMCOperations::GetInfoBooleans(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::vector<std::string> info;

  bool CanControlPower = (client->GetPermissionFlags() & ControlPower) > 0;

  for (unsigned int i = 0; i < parameterObject["booleans"].size(); i++)
  {
    std::string field = parameterObject["booleans"][i].asString();
    StringUtils::ToLower(field);

    // Need to override power management of whats in infomanager since jsonrpc
    // have a security layer aswell.
    if (field == "system.canshutdown")
      result[parameterObject["booleans"][i].asString()] = (CServiceBroker::GetPowerManager().CanPowerdown() && CanControlPower);
    else if (field == "system.canpowerdown")
      result[parameterObject["booleans"][i].asString()] = (CServiceBroker::GetPowerManager().CanPowerdown() && CanControlPower);
    else if (field == "system.cansuspend")
      result[parameterObject["booleans"][i].asString()] = (CServiceBroker::GetPowerManager().CanSuspend() && CanControlPower);
    else if (field == "system.canhibernate")
      result[parameterObject["booleans"][i].asString()] = (CServiceBroker::GetPowerManager().CanHibernate() && CanControlPower);
    else if (field == "system.canreboot")
      result[parameterObject["booleans"][i].asString()] = (CServiceBroker::GetPowerManager().CanReboot() && CanControlPower);
    else
      info.push_back(parameterObject["booleans"][i].asString());
  }

  if (!info.empty())
  {
    std::vector<bool> infoLabels;
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_INFOBOOL, -1, -1, static_cast<void*>(&infoLabels), "", info);
    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i].c_str()] = CVariant(infoLabels[i]);
    }
  }

  return OK;
}
