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
#include "PowerManager.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CSystemOperations::Shutdown(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (g_powerManager.CanPowerdown())
  {
    g_application.getApplicationMessenger().Powerdown();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Suspend(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (g_powerManager.CanSuspend())
  {
    g_application.getApplicationMessenger().Suspend();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Hibernate(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (g_powerManager.CanHibernate())
  {
    g_application.getApplicationMessenger().Hibernate();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::Reboot(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (g_powerManager.CanReboot())
  {
    g_application.getApplicationMessenger().Restart();
    return ACK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CSystemOperations::GetInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!parameterObject.isArray())
    return InvalidParams;

  bool CanControlPower = (client->GetPermissionFlags() & ControlPower) > 0;

  for (unsigned int i = 0; i < parameterObject.size(); i++)
  {
    if (!parameterObject[i].isString())
      continue;

    CStdString field = parameterObject[i].asString();
    field = field.ToLower();

    if (field.Equals("canshutdown"))
      result["CanShutdown"] = (g_powerManager.CanPowerdown() && CanControlPower);
    else if (field.Equals("cansuspend"))
      result["CanSuspend"] = (g_powerManager.CanSuspend() && CanControlPower);
    else if (field.Equals("canhibernate"))
      result["CanHibernate"] = (g_powerManager.CanHibernate() && CanControlPower);
    else if (field.Equals("canreboot"))
      result["CanReboot"] = (g_powerManager.CanReboot() && CanControlPower);
  }

  return OK;
}
