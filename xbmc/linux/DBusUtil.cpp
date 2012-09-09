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
#include "DBusUtil.h"
#ifdef HAS_DBUS
#include "utils/log.h"

CVariant CDBusUtil::GetVariant(const char *destination, const char *object, const char *interface, const char *property)
{
//dbus-send --system --print-reply --dest=destination object org.freedesktop.DBus.Properties.Get string:interface string:property
  CDBusMessage message(destination, object, "org.freedesktop.DBus.Properties", "Get");
  CVariant result;

  if (message.AppendArgument(interface) && message.AppendArgument(property))
  {
    DBusMessage *reply = message.SendSystem();

    if (reply)
    {
      DBusMessageIter iter;

      if (dbus_message_iter_init(reply, &iter))
      {
        if (!dbus_message_has_signature(reply, "v"))
          CLog::Log(LOGERROR, "DBus: wrong signature on Get - should be \"v\" but was %s", dbus_message_iter_get_signature(&iter));
        else
          result = ParseVariant(&iter);
      }
    }
  }
  else
    CLog::Log(LOGERROR, "DBus: append arguments failed");

  return result;
}

CVariant CDBusUtil::GetAll(const char *destination, const char *object, const char *interface)
{
  CDBusMessage message(destination, object, "org.freedesktop.DBus.Properties", "GetAll");
  CVariant properties;
  message.AppendArgument(interface);
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter))
    {
      if (!dbus_message_has_signature(reply, "a{sv}"))
        CLog::Log(LOGERROR, "DBus: wrong signature on GetAll - should be \"a{sv}\" but was %s", dbus_message_iter_get_signature(&iter));
      else
      {
        do
        {
          DBusMessageIter sub;
          dbus_message_iter_recurse(&iter, &sub);
          do
          {
            DBusMessageIter dict;
            dbus_message_iter_recurse(&sub, &dict);
            do
            {
              const char * key = NULL;

              dbus_message_iter_get_basic(&dict, &key);
              if (!dbus_message_iter_next(&dict))
                break;

              CVariant value = ParseVariant(&dict);

              if (!value.isNull())
                properties[key] = value;

            } while (dbus_message_iter_next(&dict));

          } while (dbus_message_iter_next(&sub));

        } while (dbus_message_iter_next(&iter));
      }
    }
  }

  return properties;
}

CVariant CDBusUtil::ParseVariant(DBusMessageIter *itr)
{
  DBusMessageIter variant;
  dbus_message_iter_recurse(itr, &variant);

  return ParseType(&variant);
}

CVariant CDBusUtil::ParseType(DBusMessageIter *itr)
{
  CVariant value;
  const char *    string  = NULL;
  dbus_int32_t    int32   = 0;
  dbus_uint32_t   uint32  = 0;
  dbus_int64_t    int64   = 0;
  dbus_uint64_t   uint64  = 0;
  dbus_bool_t     boolean = false;
  double          doublev = 0;

  int type = dbus_message_iter_get_arg_type(itr);
  switch (type)
  {
  case DBUS_TYPE_OBJECT_PATH:
  case DBUS_TYPE_STRING:
    dbus_message_iter_get_basic(itr, &string);
    value = string;
    break;
  case DBUS_TYPE_UINT32:
    dbus_message_iter_get_basic(itr, &uint32);
    value = (uint64_t)uint32;
    break;
  case DBUS_TYPE_BYTE:
  case DBUS_TYPE_INT32:
    dbus_message_iter_get_basic(itr, &int32);
    value = (int64_t)int32;
    break;
  case DBUS_TYPE_UINT64:
    dbus_message_iter_get_basic(itr, &uint64);
    value = (uint64_t)uint64;
    break;
  case DBUS_TYPE_INT64:
    dbus_message_iter_get_basic(itr, &int64);
    value = (int64_t)int64;
    break;
  case DBUS_TYPE_BOOLEAN:
    dbus_message_iter_get_basic(itr, &boolean);
    value = (bool)boolean;
    break;
  case DBUS_TYPE_DOUBLE:
    dbus_message_iter_get_basic(itr, &doublev);
    value = (double)doublev;
    break;
  case DBUS_TYPE_ARRAY:
    DBusMessageIter array;
    dbus_message_iter_recurse(itr, &array);

    value = CVariant::VariantTypeArray;

    do
    {
      CVariant item = ParseType(&array);
      if (!item.isNull())
        value.push_back(item);
    } while (dbus_message_iter_next(&array));
    break;
  }

  return value;
}
#endif
