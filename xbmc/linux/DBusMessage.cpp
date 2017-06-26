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
  m_reply = nullptr;
  m_message = dbus_message_new_method_call (destination, object, interface, method);
  if (!m_message)
  {
    // Fails only due to memory allocation failure
    throw std::runtime_error("dbus_message_new_method_call");
  }
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

void CDBusMessage::AppendObjectPath(const char *object)
{
  AppendWithType(DBUS_TYPE_OBJECT_PATH, &object);
}

template<>
void CDBusMessage::AppendArgument<bool>(const bool arg)
{
  // dbus_bool_t width might not match C++ bool width
  dbus_bool_t convArg = (arg == true);
  AppendWithType(DBUS_TYPE_BOOLEAN, &convArg);
}

void CDBusMessage::AppendArgument(const char **arrayString, unsigned int length)
{
  PrepareArgument();
  DBusMessageIter sub;
  if (!dbus_message_iter_open_container(&m_args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &sub))
  {
    throw std::runtime_error("dbus_message_iter_open_container");
  }

  for (unsigned int i = 0; i < length; i++)
  {
    if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &arrayString[i]))
    {
      throw std::runtime_error("dbus_message_iter_append_basic");
    }
  }

  if (!dbus_message_iter_close_container(&m_args, &sub))
  {
    throw std::runtime_error("dbus_message_iter_close_container");
  }
}

void CDBusMessage::AppendWithType(int type, const void* value)
{
  PrepareArgument();
  if (!dbus_message_iter_append_basic(&m_args, type, value))
  {
    throw std::runtime_error("dbus_message_iter_append_basic");
  }
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
  CDBusConnection con;
  if (!con.Connect(type))
    return nullptr;

  DBusMessage *returnMessage = Send(con, error);

  if (error)
    error.Log();

  return returnMessage;
}

DBusMessage *CDBusMessage::Send(DBusBusType type, CDBusError& error)
{
  CDBusConnection con;
  if (!con.Connect(type, error))
    return nullptr;
  
  DBusMessage *returnMessage = Send(con, error);

  return returnMessage;
}

bool CDBusMessage::SendAsync(DBusBusType type)
{
  CDBusConnection con;
  if (!con.Connect(type))
    return false;

  return dbus_connection_send(con, m_message, nullptr);
}

DBusMessage *CDBusMessage::Send(DBusConnection *con, CDBusError& error)
{
  if (m_reply)
    dbus_message_unref(m_reply);

  m_reply = dbus_connection_send_with_reply_and_block(con, m_message, -1, error);
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
