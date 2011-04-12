/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SystemOperations.h"
#include "Application.h"
#include "powermanagement/PowerManager.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CSystemOperations::Shutdown(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (g_powerManager.CanPowerdown())
  {
    g_application.getApplicationMessenger().Powerdown();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Suspend(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (g_powerManager.CanSuspend())
  {
    g_application.getApplicationMessenger().Suspend();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Hibernate(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (g_powerManager.CanHibernate())
  {
    g_application.getApplicationMessenger().Hibernate();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Reboot(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (g_powerManager.CanReboot())
  {
    g_application.getApplicationMessenger().Restart();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::GetInfoLabels(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  std::vector<CStdString> info;

  for (unsigned int i = 0; i < parameterObject["labels"].size(); i++)
  {
   CStdString field = parameterObject["labels"][i].asString();
    field = field.ToLower();

    info.push_back(parameterObject["labels"][i].asString());
  }

  if (info.size() > 0)
  {
    std::vector<CStdString> infoLabels = g_application.getApplicationMessenger().GetInfoLabels(info);
    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i].c_str()] = infoLabels[i];
    }
  }

  return OK;
}

JSON_STATUS CSystemOperations::GetInfoBooleans(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  std::vector<CStdString> info;

  bool CanControlPower = (client->GetPermissionFlags() & ControlPower) > 0;

  for (unsigned int i = 0; i < parameterObject["booleans"].size(); i++)
  {
    CStdString field = parameterObject["booleans"][i].asString();
    field = field.ToLower();

    // Need to override power management of whats in infomanager since jsonrpc
    // have a security layer aswell.
    if (field.Equals("system.canshutdown"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanPowerdown() && CanControlPower);
    else if (field.Equals("system.canpowerdown"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanPowerdown() && CanControlPower);
    else if (field.Equals("system.cansuspend"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanSuspend() && CanControlPower);
    else if (field.Equals("system.canhibernate"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanHibernate() && CanControlPower);
    else if (field.Equals("system.canreboot"))
      result[parameterObject["booleans"][i].asString()] = (g_powerManager.CanReboot() && CanControlPower);
    else
      info.push_back(parameterObject["booleans"][i].asString());
  }

  if (info.size() > 0)
  {
    std::vector<bool> infoLabels = g_application.getApplicationMessenger().GetInfoBooleans(info);
    for (unsigned int i = 0; i < info.size(); i++)
    {
      if (i >= infoLabels.size())
        break;
      result[info[i].c_str()] = Value(infoLabels[i]);
    }
  }

  return OK;
}
