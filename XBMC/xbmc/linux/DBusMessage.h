#pragma once
#include "system.h"
#ifdef HAS_DBUS
#include "StdString.h"
#include "log.h"
#include <dbus/dbus.h>
#include <stdio.h>

class CDBusMessage
{
public:
  CDBusMessage(const char *destination, const char *object, const char *interface, const char *method);
  ~CDBusMessage();

  bool AppendObjectPath(const char *object);
  bool AppendArgument(const char *string);
  bool AppendArgument(const char **arrayString, int length);

  DBusMessage *SendSystem();
  DBusMessage *SendSession();
  DBusMessage *Send(DBusBusType type);
  DBusMessage *Send(DBusConnection *con, DBusError *error);

  void Close();

private:
  DBusMessage *m_message;
  DBusMessage *m_reply;
};
#endif
