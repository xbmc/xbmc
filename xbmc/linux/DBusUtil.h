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
#include "DBusMessage.h"
#include "utils/Variant.h"

class CDBusUtil
{
public:
  static CVariant GetAll(const char *destination, const char *object, const char *arg);
  static CVariant GetAll(const char *destination, const char *object, const char *interface, const char *method, const char *arg);

  static CVariant GetVariant(const char *destination, const char *object, const char *interface, const char *property);

  static CVariant Parse(DBusMessageIter *itr);
  static CVariant ParseDictionary(DBusMessageIter *itr);
  static CVariant ParseType(DBusMessageIter *itr);
};
#endif
