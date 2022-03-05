/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SystemOperations.h"

#include "ServiceBroker.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/ApplicationMessenger.h"
#include "powermanagement/PowerManager.h"
#include "utils/Variant.h"

using namespace JSONRPC;

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
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_POWERDOWN);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Suspend(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanSuspend())
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SUSPEND);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Hibernate(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanHibernate())
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_HIBERNATE);
    return ACK;
  }
  else
    return FailedToExecute;
}

JSONRPC_STATUS CSystemOperations::Reboot(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CServiceBroker::GetPowerManager().CanReboot())
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTART);
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
