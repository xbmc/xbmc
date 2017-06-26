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
#include <string>

#include <dbus/dbus.h>

#include "DBusMessage.h"
#include "utils/Variant.h"

class CDBusUtil
{
public:
  static CVariant GetAll(const char *destination, const char *object, const char *interface);

  static CVariant GetVariant(const char *destination, const char *object, const char *interface, const char *property);
private:
  static CVariant ParseType(DBusMessageIter *itr);
  static CVariant ParseVariant(DBusMessageIter *itr);
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

#endif
