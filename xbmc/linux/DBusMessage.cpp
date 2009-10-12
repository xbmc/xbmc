#include "DBusMessage.h"
#include "log.h"
#ifdef HAS_DBUS

CDBusMessage::CDBusMessage(const char *destination, const char *object, const char *interface, const char *method)
{
  m_reply = NULL;
  m_message = dbus_message_new_method_call (destination, object, interface, method);
  CLog::Log(LOGDEBUG, "DBus: Creating message to %s on %s with interface %s and method %s\n", destination, object, interface, method);
}

CDBusMessage::~CDBusMessage()
{
  Close();
}

void CDBusMessage::AppendObjectPath(const char *object)
{
  dbus_message_append_args(m_message, DBUS_TYPE_OBJECT_PATH, &object, DBUS_TYPE_INVALID);
}

void CDBusMessage::AppendArgument(const char *string)
{
  dbus_message_append_args(m_message, DBUS_TYPE_STRING, &string, DBUS_TYPE_INVALID);
}

DBusMessage *CDBusMessage::SendSystem()
{
  return Send(DBUS_BUS_SYSTEM);
}

DBusMessage *CDBusMessage::SendSession()
{
  return Send(DBUS_BUS_SESSION);
}

DBusMessage *CDBusMessage::Send(DBusBusType type)
{
  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(type, &error);

  DBusMessage *returnMessage = Send(con, &error);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return returnMessage;
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
#endif
