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
#ifdef HAS_DBUS
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

CDBusMessage::~CDBusMessage()
{
  Close();
}

bool CDBusMessage::AppendObjectPath(const char *object)
{
  PrepareArgument();
  return dbus_message_iter_append_basic(&m_args, DBUS_TYPE_OBJECT_PATH, &object);
}

bool CDBusMessage::AppendArgument(const char *string)
{
  PrepareArgument();
  return dbus_message_iter_append_basic(&m_args, DBUS_TYPE_STRING, &string);
}

bool CDBusMessage::AppendArgument(const bool b)
{
  dbus_bool_t arg = (b == true);
  PrepareArgument();
  return dbus_message_iter_append_basic(&m_args, DBUS_TYPE_BOOLEAN, &arg);
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

DBusMessage *CDBusMessage::SendSystem()
{
  return Send(DBUS_BUS_SYSTEM);
}

DBusMessage *CDBusMessage::SendSession()
{
  return Send(DBUS_BUS_SESSION);
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
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(type, &error);

  DBusMessage *returnMessage = Send(con, &error);

  if (dbus_error_is_set(&error))
    CLog::Log(LOGERROR, "DBus: Error %s - %s", error.name, error.message);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return returnMessage;
}

bool CDBusMessage::SendAsync(DBusBusType type)
{
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(type, &error);

  bool result;
  if (con && m_message)
    result = dbus_connection_send(con, m_message, NULL);
  else
    result = false;

  dbus_error_free (&error);
  dbus_connection_unref(con);
  return result;
}

DBusMessage *CDBusMessage::Send(DBusConnection *con, DBusError *error)
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
#endif
