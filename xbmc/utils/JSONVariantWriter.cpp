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

#include <locale>

#include "JSONVariantWriter.h"

using namespace std;

string CJSONVariantWriter::Write(const CVariant &value, bool compact)
{
  string output;

  yajl_gen g = yajl_gen_alloc(NULL);
  yajl_gen_config(g, yajl_gen_beautify, compact ? 0 : 1);
  yajl_gen_config(g, yajl_gen_indent_string, "\t");

  // Set locale to classic ("C") to ensure valid JSON numbers
#ifndef TARGET_WINDOWS
  const char *currentLocale = setlocale(LC_NUMERIC, NULL);
  std::string backupLocale;
  if (currentLocale != NULL && (currentLocale[0] != 'C' || currentLocale[1] != 0))
  {
    backupLocale = currentLocale;
    setlocale(LC_NUMERIC, "C");
  }
#else  // TARGET_WINDOWS
  const wchar_t* const currentLocale = _wsetlocale(LC_NUMERIC, NULL);
  std::wstring backupLocale;
  if (currentLocale != NULL && (currentLocale[0] != L'C' || currentLocale[1] != 0))
  {
    backupLocale = currentLocale;
    _wsetlocale(LC_NUMERIC, L"C");
  }
#endif // TARGET_WINDOWS

  if (InternalWrite(g, value))
  {
    const unsigned char * buffer;

    size_t length;
    yajl_gen_get_buf(g, &buffer, &length);
    output = string((const char *)buffer, length);
  }

  // Re-set locale to what it was before using yajl
#ifndef TARGET_WINDOWS
  if (!backupLocale.empty())
    setlocale(LC_NUMERIC, backupLocale.c_str());
#else  // TARGET_WINDOWS
  if (!backupLocale.empty())
    _wsetlocale(LC_NUMERIC, backupLocale.c_str());
#endif // TARGET_WINDOWS

  yajl_gen_clear(g);
  yajl_gen_free(g);

  return output;
}

bool CJSONVariantWriter::InternalWrite(yajl_gen g, const CVariant &value)
{
  bool success = false;

  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long long int)value.asInteger());
    break;
  case CVariant::VariantTypeUnsignedInteger:
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long long int)value.asUnsignedInteger());
    break;
  case CVariant::VariantTypeDouble:
    success = yajl_gen_status_ok == yajl_gen_double(g, value.asDouble());
    break;
  case CVariant::VariantTypeBoolean:
    success = yajl_gen_status_ok == yajl_gen_bool(g, value.asBoolean() ? 1 : 0);
    break;
  case CVariant::VariantTypeString:
    success = yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)value.c_str(), (size_t)value.size());
    break;
  case CVariant::VariantTypeArray:
    success = yajl_gen_status_ok == yajl_gen_array_open(g);

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array() && success; ++itr)
      success &= InternalWrite(g, *itr);

    if (success)
      success = yajl_gen_status_ok == yajl_gen_array_close(g);

    break;
  case CVariant::VariantTypeObject:
    success = yajl_gen_status_ok == yajl_gen_map_open(g);

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map() && success; ++itr)
    {
      success &= yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)itr->first.c_str(), (size_t)itr->first.length());
      if (success)
        success &= InternalWrite(g, itr->second);
    }

    if (success)
      success &= yajl_gen_status_ok == yajl_gen_map_close(g);

    break;
  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    success = yajl_gen_status_ok == yajl_gen_null(g);
    break;
  }

  return success;
}
