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

#include <locale>

#include "JSONVariantWriter.h"

using namespace std;

string CJSONVariantWriter::Write(const CVariant &value, bool compact)
{
  string output;

#if YAJL_MAJOR == 2
  yajl_gen g = yajl_gen_alloc(NULL);
  yajl_gen_config(g, yajl_gen_beautify, compact ? 0 : 1);
  yajl_gen_config(g, yajl_gen_indent_string, "\t");
#else
  yajl_gen_config conf = { compact ? 0 : 1, "\t" };
  yajl_gen g = yajl_gen_alloc(&conf, NULL);
#endif

  // Set locale to classic ("C") to ensure valid JSON numbers
  std::string currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  if (InternalWrite(g, value))
  {
    const unsigned char * buffer;

#if YAJL_MAJOR == 2
    size_t length;
    yajl_gen_get_buf(g, &buffer, &length);
#else
    unsigned int length;
    yajl_gen_get_buf(g, &buffer, &length);
#endif
    output = string((const char *)buffer, length);
  }

  // Re-set locale to what it was before using yajl
  setlocale(LC_NUMERIC, currentLocale.c_str());

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
#if YAJL_MAJOR == 2
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long long int)value.asInteger());
#else
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long int)value.asInteger());
#endif
    break;
  case CVariant::VariantTypeUnsignedInteger:
#if YAJL_MAJOR == 2
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long long int)value.asUnsignedInteger());
#else
    success = yajl_gen_status_ok == yajl_gen_integer(g, (long int)value.asUnsignedInteger());
#endif
    break;
  case CVariant::VariantTypeDouble:
    success = yajl_gen_status_ok == yajl_gen_double(g, value.asDouble());
    break;
  case CVariant::VariantTypeBoolean:
    success = yajl_gen_status_ok == yajl_gen_bool(g, value.asBoolean() ? 1 : 0);
    break;
  case CVariant::VariantTypeString:
#if YAJL_MAJOR == 2
    success = yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)value.c_str(), (size_t)value.size());
#else
    success = yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)value.c_str(), value.size());
#endif
    break;
  case CVariant::VariantTypeArray:
    success = yajl_gen_status_ok == yajl_gen_array_open(g);

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array() && success; itr++)
      success &= InternalWrite(g, *itr);

    if (success)
      success = yajl_gen_status_ok == yajl_gen_array_close(g);

    break;
  case CVariant::VariantTypeObject:
    success = yajl_gen_status_ok == yajl_gen_map_open(g);

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map() && success; itr++)
    {
#if YAJL_MAJOR == 2
      success &= yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)itr->first.c_str(), (size_t)itr->first.length());
#else
      success &= yajl_gen_status_ok == yajl_gen_string(g, (const unsigned char*)itr->first.c_str(), itr->first.length());
#endif
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
