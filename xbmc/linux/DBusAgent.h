#pragma once

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

#include "system.h"
#ifdef HAS_DBUS

#include "DBusUtil.h"
#include <string>
#include <vector>

template <class T>
class CDBusAgent
{
public:
  CDBusAgent(DBusConnection *conn, const char *path);
  ~CDBusAgent();
  const char* GetPath() { return m_path.c_str(); }

  // Return true if a registered method or signal handler that matched the dbus message is invoked
  bool ProcessMessage(DBusMessage *msg);

protected:
  typedef void (T::*DBusAgentHandler) (std::vector<CVariant> &args);
  typedef struct
  {
    std::string iface;
    std::string name;
    DBusAgentHandler handler;
  } DBusAgentHandlerInfo;

  std::string m_path;
  DBusConnection *m_connection;
  DBusMessage *m_msg;
  std::vector<DBusAgentHandlerInfo> m_handlers;

  void RegisterMethod(const char *iface, const char *name, DBusAgentHandler method);

  void Reply(int type, const void *value);
  void Reply(const char *string);
  void Reply(bool b);
  void Reply(unsigned int i);
  void Reply();
  void ReplyError(const char *name, const char *message);
};

#include "DBusAgent.cpp"

#endif

