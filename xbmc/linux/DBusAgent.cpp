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

template <class T>
CDBusAgent<T>::CDBusAgent(DBusConnection *conn, const char *path)
{
  m_connection = dbus_connection_ref(conn);
  m_path = path;

  if (!m_path.empty())
  {
    static const DBusObjectPathVTable vtable = { 0 };
    DBusError error;
    dbus_error_init(&error);
    dbus_connection_register_object_path(m_connection, path, &vtable, &error);
    if (dbus_error_is_set(&error))
      CLog::Log(LOGERROR, "DBus: Error %s - %s", error.name, error.message);
    dbus_error_free(&error);
  }
}

template <class T>
CDBusAgent<T>::~CDBusAgent()
{
  if (!m_path.empty())
  {
    dbus_connection_unregister_object_path(m_connection, m_path.c_str());
  }
  dbus_connection_unref(m_connection);
}

template <class T>
bool CDBusAgent<T>::ProcessMessage(DBusMessage *msg)
{
  m_msg = msg;
  unsigned int i;
  for (i = 0; i < m_handlers.size(); i++)
  {
    if (dbus_message_is_method_call(msg, m_handlers[i].iface.c_str(), m_handlers[i].name.c_str()))
    {
      CLog::Log(LOGDEBUG, "DBus: Agent %s handler %s.%s called.", m_path.c_str(),
        m_handlers[i].iface.c_str(), m_handlers[i].name.c_str());
      std::vector<CVariant> args;
      DBusMessageIter iter;
      if (dbus_message_iter_init(msg, &iter))
      {
        do
        {
          args.push_back(CDBusUtil::Parse(&iter));
        } while (dbus_message_iter_next(&iter));
      }
      ((T*)this->*(m_handlers[i].handler))(args);
      m_msg = NULL;
      return true;
    }
  }
  m_msg = NULL;
  return false;
}

template <class T>
void CDBusAgent<T>::RegisterMethod(const char *iface, const char *name, DBusAgentHandler method)
{
  DBusAgentHandlerInfo info = {iface, name, method};
  m_handlers.push_back(info);
}

template <class T>
void CDBusAgent<T>::Reply(int type, const void *value)
{
  if (m_msg != NULL)
  {
    DBusMessage *reply = dbus_message_new_method_return(m_msg);
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply, &iter);
    if (dbus_message_iter_append_basic(&iter, type, value))
    {
      dbus_connection_send(m_connection, reply, NULL);
      dbus_connection_flush(m_connection);
    }
    dbus_message_unref(reply);
    CLog::Log(LOGDEBUG, "DBus: method reply sent.");
  }
}

template <class T>
void CDBusAgent<T>::Reply(const char *string)
{
  Reply(DBUS_TYPE_STRING, &string);
  CLog::Log(LOGDEBUG, "DBus: method reply string %s", string);
}

template <class T>
void CDBusAgent<T>::Reply(bool b)
{
  dbus_bool_t b2 = b;
  Reply(DBUS_TYPE_BOOLEAN, &b2);
  CLog::Log(LOGDEBUG, "DBus: method reply boolean %s", b ? "true" : "false");
}

template <class T>
void CDBusAgent<T>::Reply(unsigned int i)
{
  dbus_uint32_t i2 = i;
  Reply(DBUS_TYPE_UINT32, &i2);
  CLog::Log(LOGDEBUG, "DBus: method reply uint32 %u", i);
}

template <class T>
void CDBusAgent<T>::Reply()
{
  if (m_msg != NULL)
  {
    DBusMessage *reply = dbus_message_new_method_return(m_msg);
    dbus_connection_send(m_connection, reply, NULL);
    dbus_connection_flush(m_connection);
    dbus_message_unref(reply);
    CLog::Log(LOGDEBUG, "DBus: method reply empty");
  }
}

template <class T>
void CDBusAgent<T>::ReplyError(const char *name, const char *message)
{
  if (m_msg != NULL)
  {
    DBusMessage *reply = dbus_message_new_error(m_msg, name, message);
    dbus_connection_send(m_connection, reply, NULL);
    dbus_connection_flush(m_connection);
    dbus_message_unref(reply);
    CLog::Log(LOGDEBUG, "DBus: method reply error %s: %s", name, message);
  }
}

