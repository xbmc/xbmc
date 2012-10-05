#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#ifdef HAS_DBUS
#include <dbus/dbus.h>

class CDBusMessage
{
public:
  CDBusMessage(const char *destination, const char *object, const char *interface, const char *method);
  ~CDBusMessage();

  bool AppendObjectPath(const char *object);
  bool AppendArgument(const char *string);
  bool AppendArgument(const char **arrayString, unsigned int length);

  DBusMessage *SendSystem();
  DBusMessage *SendSession();

  bool SendAsyncSystem();
  bool SendAsyncSession();

  DBusMessage *Send(DBusBusType type);
  DBusMessage *Send(DBusConnection *con, DBusError *error);
private:

  bool SendAsync(DBusBusType type);

  void Close();
  void PrepareArgument();

  DBusMessage *m_message;
  DBusMessage *m_reply;
  DBusMessageIter m_args;
  bool m_haveArgs;
};
#endif
