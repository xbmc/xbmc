/*
 *  Copyright (C) 2004, Leo Seib, Hannover
 *
 *  Project: C++ Dynamic Library
 *  Module: FieldValue class realisation file
 *  Author: Leo Seib      E-Mail: leoseib@web.de
 *  Begin: 5/04/2002
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

/**********************************************************************
 * 2005-03-29 - Minor modifications to allow get_asBool to function on
 *              on string values that are 1 or 0
 **********************************************************************/

#include "qry_dat.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __GNUC__
#pragma warning(disable : 4800)
#pragma warning(disable : 4715)
#endif

namespace dbiplus
{

//Constructors
field_value::field_value()
{
  field_type = ft_String;
  is_null = false;
}

field_value::field_value(const char* s) : str_value(s)
{
  field_type = ft_String;
  is_null = false;
}

field_value::field_value(const bool b)
{
  bool_value = b;
  field_type = ft_Boolean;
  is_null = false;
}

field_value::field_value(const char c)
{
  char_value = c;
  field_type = ft_Char;
  is_null = false;
}

field_value::field_value(const short s)
{
  short_value = s;
  field_type = ft_Short;
  is_null = false;
}

field_value::field_value(const unsigned short us)
{
  ushort_value = us;
  field_type = ft_UShort;
  is_null = false;
}

field_value::field_value(const int i)
{
  int_value = i;
  field_type = ft_Int;
  is_null = false;
}

field_value::field_value(const unsigned int ui)
{
  uint_value = ui;
  field_type = ft_UInt;
  is_null = false;
}

field_value::field_value(const float f)
{
  float_value = f;
  field_type = ft_Float;
  is_null = false;
}

field_value::field_value(const double d)
{
  double_value = d;
  field_type = ft_Double;
  is_null = false;
}

field_value::field_value(const int64_t i)
{
  int64_value = i;
  field_type = ft_Int64;
  is_null = false;
}

field_value::field_value(const char* s, std::size_t len)
  : field_type(ft_String), str_value(s, len), is_null(false)
{
}

field_value::field_value(const field_value& fv)
{
  switch (fv.get_fType())
  {
    case ft_String:
    {
      set_asString(fv.get_asString());
      break;
    }
    case ft_Boolean:
    {
      set_asBool(fv.get_asBool());
      break;
    }
    case ft_Char:
    {
      set_asChar(fv.get_asChar());
      break;
    }
    case ft_Short:
    {
      set_asShort(fv.get_asShort());
      break;
    }
    case ft_UShort:
    {
      set_asUShort(fv.get_asUShort());
      break;
    }
    case ft_Int:
    {
      set_asInt(fv.get_asInt());
      break;
    }
    case ft_UInt:
    {
      set_asUInt(fv.get_asUInt());
      break;
    }
    case ft_Float:
    {
      set_asFloat(fv.get_asFloat());
      break;
    }
    case ft_Double:
    {
      set_asDouble(fv.get_asDouble());
      break;
    }
    case ft_Int64:
    {
      set_asInt64(fv.get_asInt64());
      break;
    }
    default:
      break;
  }
  is_null = fv.get_isNull();
}

field_value::field_value(field_value&& fv) noexcept
{
  *this = std::move(fv);
}

//empty destructor
field_value::~field_value() = default;

//Conversations functions
std::string field_value::get_asString() const&
{
  switch (field_type)
  {
    case ft_String:
    {
      return str_value;
    }
    case ft_Boolean:
    {
      if (bool_value)
        return "True";
      else
        return "False";
    }
    case ft_Char:
    {
      return {char_value};
    }
    case ft_Short:
    {
      char t[10];
      snprintf(t, sizeof(t), "%i", short_value);
      return t;
    }
    case ft_UShort:
    {
      char t[10];
      snprintf(t, sizeof(t), "%i", ushort_value);
      return t;
    }
    case ft_Int:
    {
      char t[12];
      snprintf(t, sizeof(t), "%d", int_value);
      return t;
    }
    case ft_UInt:
    {
      char t[12];
      snprintf(t, sizeof(t), "%u", uint_value);
      return t;
    }
    case ft_Float:
    {
      char t[16];
      snprintf(t, sizeof(t), "%f", static_cast<double>(float_value));
      return t;
    }
    case ft_Double:
    {
      char t[32];
      snprintf(t, sizeof(t), "%f", double_value);
      return t;
    }
    case ft_Int64:
    {
      char t[23];
      snprintf(t, sizeof(t), "%" PRId64, int64_value);
      return t;
    }
    default:
      return "";
  }
}

std::string field_value::get_asString() &&
{
  switch (field_type)
  {
    case ft_String:
      return std::move(str_value);
    default:
      return get_asString();
  }
}

bool field_value::get_asBool() const
{
  switch (field_type)
  {
    case ft_String:
    {
      if (str_value == "True" || str_value == "true" || str_value == "1")
        return true;
      else
        return false;
    }
    case ft_Boolean:
    {
      return bool_value;
    }
    case ft_Char:
    {
      if (char_value == 'T' || char_value == 't')
        return true;
      else
        return false;
    }
    case ft_Short:
    {
      return (bool)short_value;
    }
    case ft_UShort:
    {
      return (bool)ushort_value;
    }
    case ft_Int:
    {
      return (bool)int_value;
    }
    case ft_UInt:
    {
      return (bool)uint_value;
    }
    case ft_Float:
    {
      return (bool)float_value;
    }
    case ft_Double:
    {
      return (bool)double_value;
    }
    case ft_Int64:
    {
      return (bool)int64_value;
    }
    default:
      return false;
  }
}

char field_value::get_asChar() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return str_value[0];
    }
    case ft_Boolean:
    {
      if (bool_value)
        return 'T';
      else
        return 'F';
    }
    case ft_Char:
    {
      return char_value;
    }
    case ft_Short:
    {
      char t[10];
      snprintf(t, sizeof(t), "%i", short_value);
      return t[0];
    }
    case ft_UShort:
    {
      char t[10];
      snprintf(t, sizeof(t), "%i", ushort_value);
      return t[0];
    }
    case ft_Int:
    {
      char t[12];
      snprintf(t, sizeof(t), "%d", int_value);
      return t[0];
    }
    case ft_UInt:
    {
      char t[12];
      snprintf(t, sizeof(t), "%u", uint_value);
      return t[0];
    }
    case ft_Float:
    {
      char t[16];
      snprintf(t, sizeof(t), "%f", static_cast<double>(float_value));
      return t[0];
    }
    case ft_Double:
    {
      char t[32];
      snprintf(t, sizeof(t), "%f", double_value);
      return t[0];
    }
    case ft_Int64:
    {
      char t[24];
      snprintf(t, sizeof(t), "%" PRId64, int64_value);
      return t[0];
    }
    default:
      return '\0';
  }
}

short field_value::get_asShort() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return (short)atoi(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (short)bool_value;
    }
    case ft_Char:
    {
      return (short)char_value;
    }
    case ft_Short:
    {
      return short_value;
    }
    case ft_UShort:
    {
      return (short)ushort_value;
    }
    case ft_Int:
    {
      return (short)int_value;
    }
    case ft_UInt:
    {
      return (short)uint_value;
    }
    case ft_Float:
    {
      return (short)float_value;
    }
    case ft_Double:
    {
      return (short)double_value;
    }
    case ft_Int64:
    {
      return (short)int64_value;
    }
    default:
      return 0;
  }
}

unsigned short field_value::get_asUShort() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return (unsigned short)atoi(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (unsigned short)bool_value;
    }
    case ft_Char:
    {
      return (unsigned short)char_value;
    }
    case ft_Short:
    {
      return (unsigned short)short_value;
    }
    case ft_UShort:
    {
      return ushort_value;
    }
    case ft_Int:
    {
      return (unsigned short)int_value;
    }
    case ft_UInt:
    {
      return (unsigned short)uint_value;
    }
    case ft_Float:
    {
      return (unsigned short)float_value;
    }
    case ft_Double:
    {
      return (unsigned short)double_value;
    }
    case ft_Int64:
    {
      return (unsigned short)int64_value;
    }
    default:
      return 0;
  }
}

int field_value::get_asInt() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return atoi(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (int)bool_value;
    }
    case ft_Char:
    {
      return (int)char_value;
    }
    case ft_Short:
    {
      return (int)short_value;
    }
    case ft_UShort:
    {
      return (int)ushort_value;
    }
    case ft_Int:
    {
      return int_value;
    }
    case ft_UInt:
    {
      return (int)uint_value;
    }
    case ft_Float:
    {
      return (int)float_value;
    }
    case ft_Double:
    {
      return (int)double_value;
    }
    case ft_Int64:
    {
      return (int)int64_value;
    }
    default:
      return 0;
  }
}

unsigned int field_value::get_asUInt() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return (unsigned int)atoi(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (unsigned int)bool_value;
    }
    case ft_Char:
    {
      return (unsigned int)char_value;
    }
    case ft_Short:
    {
      return (unsigned int)short_value;
    }
    case ft_UShort:
    {
      return (unsigned int)ushort_value;
    }
    case ft_Int:
    {
      return (unsigned int)int_value;
    }
    case ft_UInt:
    {
      return uint_value;
    }
    case ft_Float:
    {
      return (unsigned int)float_value;
    }
    case ft_Double:
    {
      return (unsigned int)double_value;
    }
    case ft_Int64:
    {
      return (unsigned int)int64_value;
    }
    default:
      return 0;
  }
}

float field_value::get_asFloat() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return (float)atof(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (float)bool_value;
    }
    case ft_Char:
    {
      return (float)char_value;
    }
    case ft_Short:
    {
      return (float)short_value;
    }
    case ft_UShort:
    {
      return (float)ushort_value;
    }
    case ft_Int:
    {
      return (float)int_value;
    }
    case ft_UInt:
    {
      return (float)uint_value;
    }
    case ft_Float:
    {
      return float_value;
    }
    case ft_Double:
    {
      return (float)double_value;
    }
    case ft_Int64:
    {
      return (float)int64_value;
    }
    default:
      return 0.0;
  }
}

double field_value::get_asDouble() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return atof(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (double)bool_value;
    }
    case ft_Char:
    {
      return (double)char_value;
    }
    case ft_Short:
    {
      return (double)short_value;
    }
    case ft_UShort:
    {
      return (double)ushort_value;
    }
    case ft_Int:
    {
      return (double)int_value;
    }
    case ft_UInt:
    {
      return (double)uint_value;
    }
    case ft_Float:
    {
      return (double)float_value;
    }
    case ft_Double:
    {
      return (double)double_value;
    }
    case ft_Int64:
    {
      return (double)int64_value;
    }
    default:
      return 0.0;
  }
}

int64_t field_value::get_asInt64() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return std::atoll(str_value.c_str());
    }
    case ft_Boolean:
    {
      return (int64_t)bool_value;
    }
    case ft_Char:
    {
      return (int64_t)char_value;
    }
    case ft_Short:
    {
      return (int64_t)short_value;
    }
    case ft_UShort:
    {
      return (int64_t)ushort_value;
    }
    case ft_Int:
    {
      return (int64_t)int_value;
    }
    case ft_UInt:
    {
      return (int64_t)uint_value;
    }
    case ft_Float:
    {
      return (int64_t)float_value;
    }
    case ft_Double:
    {
      return (int64_t)double_value;
    }
    case ft_Int64:
    {
      return int64_value;
    }
    default:
      return 0;
  }
}

field_value& field_value::operator=(const field_value& fv)
{
  if (this == &fv)
    return *this;

  is_null = fv.get_isNull();

  switch (fv.get_fType())
  {
    case ft_String:
    {
      set_asString(fv.get_asString());
      return *this;
      break;
    }
    case ft_Boolean:
    {
      set_asBool(fv.get_asBool());
      return *this;
      break;
    }
    case ft_Char:
    {
      set_asChar(fv.get_asChar());
      return *this;
      break;
    }
    case ft_Short:
    {
      set_asShort(fv.get_asShort());
      return *this;
      break;
    }
    case ft_UShort:
    {
      set_asUShort(fv.get_asUShort());
      return *this;
      break;
    }
    case ft_Int:
    {
      set_asInt(fv.get_asInt());
      return *this;
      break;
    }
    case ft_UInt:
    {
      set_asUInt(fv.get_asUInt());
      return *this;
      break;
    }
    case ft_Float:
    {
      set_asFloat(fv.get_asFloat());
      return *this;
      break;
    }
    case ft_Double:
    {
      set_asDouble(fv.get_asDouble());
      return *this;
      break;
    }
    case ft_Int64:
    {
      set_asInt64(fv.get_asInt64());
      return *this;
      break;
    }
    default:
      return *this;
  }
}

field_value& field_value::operator=(field_value&& fv) noexcept
{
  if (this == &fv)
    return *this;

  is_null = fv.get_isNull();

  switch (fv.get_fType())
  {
    case ft_String:
      set_asString(std::move(fv.str_value));
      return *this;
    default:
      return *this = fv;
  }
}

//Set functions
void field_value::set_asString(const char* s)
{
  str_value = s;
  field_type = ft_String;
}

void field_value::set_asString(const char* s, std::size_t len)
{
  str_value = std::string_view(s, len);
  field_type = ft_String;
}

void field_value::set_asString(const std::string& s)
{
  str_value = s;
  field_type = ft_String;
}

void field_value::set_asString(std::string&& s)
{
  str_value = std::move(s);
  field_type = ft_String;
}

void field_value::set_asBool(const bool b)
{
  bool_value = b;
  field_type = ft_Boolean;
}

void field_value::set_asChar(const char c)
{
  char_value = c;
  field_type = ft_Char;
}

void field_value::set_asShort(const short s)
{
  short_value = s;
  field_type = ft_Short;
}

void field_value::set_asUShort(const unsigned short us)
{
  ushort_value = us;
  field_type = ft_UShort;
}

void field_value::set_asInt(const int i)
{
  int_value = i;
  field_type = ft_Int;
}

void field_value::set_asUInt(const unsigned int ui)
{
  int_value = ui;
  field_type = ft_UInt;
}

void field_value::set_asFloat(const float f)
{
  float_value = f;
  field_type = ft_Float;
}

void field_value::set_asDouble(const double d)
{
  double_value = d;
  field_type = ft_Double;
}

void field_value::set_asInt64(const int64_t i)
{
  int64_value = i;
  field_type = ft_Int64;
}

fType field_value::get_field_type()
{
  return field_type;
}

std::string field_value::gft()
{
  std::string tmp;
  switch (field_type)
  {
    case ft_String:
    {
      tmp.assign("string");
      return tmp;
    }
    case ft_Boolean:
    {
      tmp.assign("bool");
      return tmp;
    }
    case ft_Char:
    {
      tmp.assign("char");
      return tmp;
    }
    case ft_Short:
    {
      tmp.assign("short");
      return tmp;
    }
    case ft_Int:
    {
      tmp.assign("int");
      return tmp;
    }
    case ft_Float:
    {
      tmp.assign("float");
      return tmp;
    }
    case ft_Double:
    {
      tmp.assign("double");
      return tmp;
    }
    case ft_Int64:
    {
      tmp.assign("int64");
      return tmp;
    }
    default:
      break;
  }

  return tmp;
}

} // namespace dbiplus
