/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "SystemOperations.h"
#include "messaging/ApplicationMessenger.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/Variant.h"
#include "powermanagement/PowerManager.h"
#include "ServiceBroker.h"

using namespace JSONRPC;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CSystemOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(client->GetPermissionFlags(), propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CSystemOperations::EjectOpticalDrive(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return CBuiltins::GetInstance().Execute("EjectTray") == 0 ? ACK : FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Shutdown(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanPowerdown())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_POWERDOWN);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Suspend(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanSuspend())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SUSPEND);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Hibernate(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanHibernate())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_HIBERNATE);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Reboot(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanReboot())
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_RESTART);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::GetPropertyValue(int permissions, const std::string &property, CVariant &result)
{
  if (property == "canshutdown")
    result = CServiceBroker::GetPowerManager().CanPowerdown() && (permissions & ControlPower);
  else if (property == "cansuspend")
    result = CServiceBroker::GetPowerManager().CanSuspend() && (permissions & ControlPower);
  else if (property == "canhibernate")
    result = CServiceBroker::GetPowerManager().CanHibernate() && (permissions & ControlPower);
  else if (property == "canreboot")
    result = CServiceBroker::GetPowerManager().CanReboot() && (permissions & ControlPower);
  else
    return InvalidParams;

  return OK;
}
