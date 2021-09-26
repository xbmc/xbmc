/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DBusMessage.h"
#include "utils/Variant.h"

#include <memory>
#include <string>

#include <dbus/dbus.h>

class CDBusUtil
{
public:
  static CVariant GetAll(const char *destination, const char *object, const char *interface);
  static CVariant GetVariant(const char *destination, const char *object, const char *interface, const char *property);
  /**
   * Try to call a DBus method and return whether the call succeeded
   */
  static bool TryMethodCall(DBusBusType bus, const char* destination, const char* object, const char* interface, const char* method);
  /**
   * Try to call a DBus method and return whether the call succeeded
   */
  static bool TryMethodCall(DBusBusType bus, std::string const& destination, std::string const& object, std::string const& interface, std::string const& method);

private:
  static CVariant ParseType(DBusMessageIter *itr);
  static CVariant ParseVariant(DBusMessageIter *itr);
};

class CDBusConnection
{
public:
  CDBusConnection();
  bool Connect(DBusBusType bus, bool openPrivate = false);
  bool Connect(DBusBusType bus, CDBusError& error, bool openPrivate = false);
  void Destroy();
  operator DBusConnection*();

private:
  CDBusConnection(CDBusConnection const& other) = delete;
  CDBusConnection& operator=(CDBusConnection const& other) = delete;

  struct DBusConnectionDeleter
  {
    DBusConnectionDeleter() {}
    bool closeBeforeUnref = false;
    void operator()(DBusConnection* connection) const;
  };
  std::unique_ptr<DBusConnection, DBusConnectionDeleter> m_connection;
};

class CDBusError
{
public:
  CDBusError();
  ~CDBusError();
  operator DBusError*();
  bool IsSet() const;
  /**
   * Reset this error wrapper
   *
   * If there was an error, it was handled and this error wrapper should be used
   * again in a new call, it must be reset before that call.
   */
  void Reset();
  // Keep because operator DBusError* would be used for if-statements on
  // non-const CDBusError instead
  operator bool();
  operator bool() const;
  std::string Name() const;
  std::string Message() const;
  void Log(std::string const& message = "DBus error") const;
  void Log(int level, std::string const& message = "DBus error") const;

private:
  CDBusError(CDBusError const& other) = delete;
  CDBusError& operator=(CDBusError const& other) = delete;

  DBusError m_error;
};
