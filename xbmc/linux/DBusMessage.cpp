/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "DBusMessage.h"
#include "DBusUtil.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"

CDBusMessage::CDBusMessage(const char *destination, const char *object, const char *interface, const char *method)
{
  m_reply = NULL;
  m_message = dbus_message_new_method_call (destination, object, interface, method);
  m_haveArgs = false;

  if (g_advancedSettings.CanLogComponent(LOGDBUS))
    CLog::Log(LOGDEBUG, "DBus: Creating message to %s on %s with interface %s and method %s\n", destination, object, interface, method);
}

CDBusMessage::CDBusMessage(const std::string& destination, const std::string& object, const std::string& interface, const std::string& method)
: CDBusMessage(destination.c_str(), object.c_str(), interface.c_str(), method.c_str())
{}

CDBusMessage::~CDBusMessage()
{
  Close();
}

bool CDBusMessage::AppendObjectPath(const char *object)
{
  return AppendWithType(DBUS_TYPE_OBJECT_PATH, &object);
}

template<>
bool CDBusMessage::AppendArgument<bool>(const bool arg)
{
  // dbus_bool_t width might not match C++ bool width
  dbus_bool_t convArg = (arg == true);
  return AppendWithType(DBUS_TYPE_BOOLEAN, &convArg);
}

bool CDBusMessage::AppendArgument(const char **arrayString, unsigned int length)
{
  PrepareArgument();
  DBusMessageIter sub;
  bool success = dbus_message_iter_open_container(&m_args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &sub);

  for (unsigned int i = 0; i < length && success; i++)
    success &= dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &arrayString[i]);

  success &= dbus_message_iter_close_container(&m_args, &sub);

  return success;
}

bool CDBusMessage::AppendWithType(int type, const void* value)
{
  PrepareArgument();
  return dbus_message_iter_append_basic(&m_args, type, value);
}

DBusMessage *CDBusMessage::SendSystem()
{
  return Send(DBUS_BUS_SYSTEM);
}

DBusMessage *CDBusMessage::SendSession()
{
  return Send(DBUS_BUS_SESSION);
}

DBusMessage *CDBusMessage::SendSystem(CDBusError& error)
{
  return Send(DBUS_BUS_SYSTEM, error);
}

DBusMessage *CDBusMessage::SendSession(CDBusError& error)
{
  return Send(DBUS_BUS_SESSION, error);
}

bool CDBusMessage::SendAsyncSystem()
{
  return SendAsync(DBUS_BUS_SYSTEM);
}

bool CDBusMessage::SendAsyncSession()
{
  return SendAsync(DBUS_BUS_SESSION);
}

DBusMessage *CDBusMessage::Send(DBusBusType type)
{
  CDBusError error;
  DBusConnection *con = dbus_bus_get(type, error);

  DBusMessage *returnMessage = Send(con, error);

  if (error)
    error.Log();

  dbus_connection_unref(con);

  return returnMessage;
}

DBusMessage *CDBusMessage::Send(DBusBusType type, CDBusError& error)
{
  DBusConnection *con = dbus_bus_get(type, error);
  if (!con)
    return nullptr;
  
  DBusMessage *returnMessage = Send(con, error);

  dbus_connection_unref(con);

  return returnMessage;
}

bool CDBusMessage::SendAsync(DBusBusType type)
{
  if (!m_message)
    return false;

  DBusConnection *con = dbus_bus_get(type, NULL);
  if (!con)
    return false;

  bool result = dbus_connection_send(con, m_message, NULL);
  
  dbus_connection_unref(con);
  return result;
}

DBusMessage *CDBusMessage::Send(DBusConnection *con, CDBusError& error)
{
  if (con && m_message)
  {
    if (m_reply)
      dbus_message_unref(m_reply);

    m_reply = dbus_connection_send_with_reply_and_block(con, m_message, -1, error);
  }

  return m_reply;
}

void CDBusMessage::Close()
{
  if (m_message)
    dbus_message_unref(m_message);

  if (m_reply)
    dbus_message_unref(m_reply);
}

void CDBusMessage::PrepareArgument()
{
  if (!m_haveArgs)
    dbus_message_iter_init_append(m_message, &m_args);

  m_haveArgs = true;
}
