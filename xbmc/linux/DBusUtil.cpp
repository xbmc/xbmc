#include "DBusUtil.h"
#ifdef HAS_DBUS

bool CDBusUtil::GetBoolean(const char *destination, const char *object, const char *interface, const char *property)
{
  return GetVariant(destination, object, interface, property).Equals("true");
}

int CDBusUtil::GetInt32(const char *destination, const char *object, const char *interface, const char *property)
{
  return atoi(GetVariant(destination, object, interface, property).c_str());
}

CStdString CDBusUtil::GetVariant(const char *destination, const char *object, const char *interface, const char *property)
{
//dbus-send --system --print-reply --dest=destination object org.freedesktop.DBus.Properties.Get string:interface string:property
  CDBusMessage message(destination, object, "org.freedesktop.DBus.Properties", "Get");
  message.AppendArgument(interface);
  message.AppendArgument(property);
  DBusMessage *reply = message.SendSystem();
  CStdString result;
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

  return result;
}

void CDBusUtil::GetAll(PropertyMap& properties, const char *destination, const char *object, const char *interface)
{
  CDBusMessage message(destination, object, "org.freedesktop.DBus.Properties", "GetAll");
  message.AppendArgument(interface);
  DBusMessage *reply = message.SendSystem();
  CStdString result;
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
              const char *    key     = NULL;

              dbus_message_iter_get_basic(&dict, &key);
              dbus_message_iter_next(&dict);

              CStdString value = ParseVariant(&dict);

              if (value.length() > 0)
                properties.insert(Property(key, value));

            } while (dbus_message_iter_next(&dict));

          } while (dbus_message_iter_next(&sub));

        } while (dbus_message_iter_next(&iter));
      }
    }
  }
}

CStdString CDBusUtil::ParseVariant(DBusMessageIter *itr)
{
  DBusMessageIter variant;
  dbus_message_iter_recurse(itr, &variant);

  return ParseType(&variant);
}

CStdString CDBusUtil::ParseType(DBusMessageIter *itr)
{
  CStdString value;
  const char *    string  = NULL;
  dbus_int32_t    int32   = 0;
  dbus_int64_t    int64   = 0;
  dbus_bool_t     boolean = false;

  int type = dbus_message_iter_get_arg_type(itr);

  switch (type)
  {
  case DBUS_TYPE_OBJECT_PATH:
  case DBUS_TYPE_STRING:
    dbus_message_iter_get_basic(itr, &string);
    value = string;
    break;

  case DBUS_TYPE_BYTE:
  case DBUS_TYPE_UINT32:
  case DBUS_TYPE_INT32:
    dbus_message_iter_get_basic(itr, &int32);
    value.Format("%i", (int)int32);
    break;
  case DBUS_TYPE_UINT64:
  case DBUS_TYPE_INT64:
    dbus_message_iter_get_basic(itr, &int64);
    value.Format("%li", int64);
    break;
  case DBUS_TYPE_BOOLEAN:
    dbus_message_iter_get_basic(itr, &boolean);
    value = boolean ? "true" : "false";
    break;
  case DBUS_TYPE_ARRAY:
    DBusMessageIter array;
    dbus_message_iter_recurse(itr, &array);

    std::vector<CStdString> strArray;

    do
    {
      strArray.push_back(ParseType(&array));
    } while (dbus_message_iter_next(&array));

    value = strArray.size() > 0 ? strArray[0] : ""; //Only handle first in the array for now.
    break;
  }

  return value;
}
#endif
