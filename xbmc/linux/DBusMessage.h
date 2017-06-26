#pragma once
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
#include "system.h"
#ifdef HAS_DBUS
#include <cstdint>
#include <memory>
#include <string>

#include <dbus/dbus.h>

class CDBusError;

template<typename T>
struct ToDBusType;
template<> struct ToDBusType<bool> { static constexpr int TYPE = DBUS_TYPE_BOOLEAN; };
template<> struct ToDBusType<char*> { static constexpr int TYPE = DBUS_TYPE_STRING; };
template<> struct ToDBusType<const char*> { static constexpr int TYPE = DBUS_TYPE_STRING; };
template<> struct ToDBusType<std::uint8_t> { static constexpr int TYPE = DBUS_TYPE_BYTE; };
template<> struct ToDBusType<std::int16_t> { static constexpr int TYPE = DBUS_TYPE_INT16; };
template<> struct ToDBusType<std::uint16_t> { static constexpr int TYPE = DBUS_TYPE_UINT16; };
template<> struct ToDBusType<std::int32_t> { static constexpr int TYPE = DBUS_TYPE_INT32; };
template<> struct ToDBusType<std::uint32_t> { static constexpr int TYPE = DBUS_TYPE_UINT32; };
template<> struct ToDBusType<std::int64_t> { static constexpr int TYPE = DBUS_TYPE_INT64; };
template<> struct ToDBusType<std::uint64_t> { static constexpr int TYPE = DBUS_TYPE_UINT64; };
template<> struct ToDBusType<double> { static constexpr int TYPE = DBUS_TYPE_DOUBLE; };

struct DBusMessageDeleter
{
  void operator()(DBusMessage* message) const;
};
using DBusMessagePtr = std::unique_ptr<DBusMessage, DBusMessageDeleter>;

class CDBusMessage
{
public:
  CDBusMessage(const char *destination, const char *object, const char *interface, const char *method);
  CDBusMessage(std::string const& destination, std::string const& object, std::string const& interface, std::string const& method);

  void AppendObjectPath(const char *object);

  template<typename T>
  void AppendArgument(const T arg)
  {
    AppendWithType(ToDBusType<T>::TYPE, &arg);
  }
  void AppendArgument(const char **arrayString, unsigned int length);

  DBusMessage *SendSystem();
  DBusMessage *SendSession();
  DBusMessage *SendSystem(CDBusError& error);
  DBusMessage *SendSession(CDBusError& error);

  bool SendAsyncSystem();
  bool SendAsyncSession();

  DBusMessage *Send(DBusBusType type);
  DBusMessage *Send(DBusBusType type, CDBusError& error);
  DBusMessage *Send(DBusConnection *con, CDBusError& error);
private:
  void AppendWithType(int type, const void* value);
  bool SendAsync(DBusBusType type);

  void PrepareArgument();

  DBusMessagePtr m_message;
  DBusMessagePtr m_reply;
  DBusMessageIter m_args;
  bool m_haveArgs;
};

template<>
void CDBusMessage::AppendArgument<bool>(const bool arg);
#endif
