/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "ConnmanNetworkManager.h"

#ifdef HAS_DBUS
#include "ConnmanConnection.h"
#include "linux/DBusUtil.h"
#include "linux/DBusMessage.h"
#include "utils/log.h"

using namespace std;

CConnmanNetworkManager::CConnmanNetworkManager()
{
  // should use CONNMAN_SERVICE here (in connman/dbus.h)
  CDBusMessage message("net.connman", "/", "net.connman.Manager", "GetProperties");
  CDBusReplyPtr reply = message.SendSystem();
  m_properties = reply->GetNextArgument();

  UpdateNetworkManager();

  dbus_error_init (&m_error);
  // TODO: do not use dbus_connection_pop_message() that requires the use of a
  // private connection
  m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &m_error);
  if (m_connection)
  {
    dbus_connection_set_exit_on_disconnect(m_connection, false);

    dbus_bus_add_match(m_connection, "type='signal',interface='net.connman.Manager'", &m_error);
    dbus_connection_flush(m_connection);
    if (dbus_error_is_set(&m_error))
    {
      CLog::Log(LOGERROR, "ConnmanNetworkManager: Failed to attach to signal %s", m_error.message);
      dbus_connection_close(m_connection);
      dbus_connection_unref(m_connection);
      m_connection = NULL;
    }
  }
  else
    CLog::Log(LOGERROR, "ConnmanNetworkManager: Failed to get a DBus connection %s", m_error.message);
}

CConnmanNetworkManager::~CConnmanNetworkManager()
{
  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

bool CConnmanNetworkManager::CanManageConnections()
{
  // TODO Only return true if we are registered as agent
  return true;
}

ConnectionList CConnmanNetworkManager::GetConnections()
{
  return m_connections;
}

bool CConnmanNetworkManager::PumpNetworkEvents(INetworkEventsCallback *callback)
{
  bool result = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      CDBusReplyPtr reply = CDBusReplyPtr(new CDBusReply(msg));

      if (dbus_message_is_signal(msg, "net.connman.Manager", "PropertyChanged"))
      {
        CVariant key = reply->GetNextArgument();
        m_properties[key.asString()] = reply->GetNextArgument();

        UpdateNetworkManager();

        if (strcmp(key.asString().c_str(), "Services") == 0)
          callback->OnConnectionListChange(m_connections);

        result = true;
      }
      else if (dbus_message_is_signal(msg, "net.connman.Manager", "StateChanged"))
      {
        CVariant stateString = reply->GetNextArgument();
        result = true;
        callback->OnConnectionStateChange(CConnmanConnection::ParseConnectionState(stateString.asString().c_str()));
      }
      else if (dbus_message_is_signal(msg, "net.connman.Manager", "NameAcquired"))
      {
      }
      else
        CLog::Log(LOGINFO, "ConnmanNetworkManager: Recieved an unknown signal %s", dbus_message_get_member(msg));

      dbus_message_unref(msg);
    }
  }

  for (size_t i = 0; i < m_connections.size(); i++)
  {
    if (((CConnmanConnection *)m_connections[i].get())->PumpNetworkEvents())
    {
      callback->OnConnectionChange(m_connections[i]);
      result = true;
    }
  }

  return result;
}

bool CConnmanNetworkManager::HasConnman()
{
  CDBusMessage message("net.connman", "/", "net.connman.Manager", "GetProperties");

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  message.Send(connection, &error);

  if (!dbus_error_is_set(&error))
    return true;
  else
  {
    CLog::Log(LOGDEBUG, "ConnmanNetworkManager: %s - %s", error.name, error.message);
    return false;
  }
}

void CConnmanNetworkManager::UpdateNetworkManager()
{
  m_connections.clear();

  CVariant services = m_properties["Services"];

  for (unsigned int i = 0; i < services.size(); i++)
  {
    if (strcmp(services[i].asString().c_str(), "") == 0)
      continue;

    IConnection *connection = new CConnmanConnection(services[i].asString().c_str());
    m_connections.push_back(CConnectionPtr(connection));
  }
}
#endif
