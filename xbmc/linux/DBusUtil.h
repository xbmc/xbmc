#pragma once
#include "system.h"
#ifdef HAS_DBUS
#include "DBusMessage.h"
#include <map>

typedef std::map<CStdString,  CStdString> PropertyMap;
typedef std::pair<CStdString, CStdString> Property;
class CDBusUtil
{
public:
  static bool GetBoolean(const char *destination, const char *object, const char *interface, const char *property);
  static void GetAll(PropertyMap& properties, const char *destination, const char *object, const char *interface);

  static CStdString GetVariant(const char *destination, const char *object, const char *interface, const char *property);
private:
  static CStdString ParseType(DBusMessageIter *itr);
  static CStdString ParseVariant(DBusMessageIter *itr);
};
#endif
