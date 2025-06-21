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

#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

#ifndef __GNUC__
#pragma warning(disable : 4800)
#pragma warning(disable : 4715)
#endif

namespace dbiplus
{
using enum fType;

//Constructors
field_value::field_value() : field_type(ft_String)
{
}

field_value::field_value(const char* s) : field_type(ft_String), str_value(s)
{
}

field_value::field_value(const bool b) : field_type(ft_Boolean), bool_value(b)
{
}

field_value::field_value(const char c) : field_type(ft_Char), char_value(c)
{
}

field_value::field_value(const short s) : field_type(ft_Short), short_value(s)
{
}

field_value::field_value(const unsigned short us) : field_type(ft_UShort), ushort_value(us)
{
}

field_value::field_value(const int i) : field_type(ft_Int), int_value(i)
{
}

field_value::field_value(const unsigned int ui) : field_type(ft_UInt), uint_value(ui)
{
}

field_value::field_value(const float f) : field_type(ft_Float), float_value(f)
{
}

field_value::field_value(const double d) : field_type(ft_Double), double_value(d)
{
}

field_value::field_value(const int64_t i) : field_type(ft_Int64), int64_value(i)
{
}

field_value::field_value(const char* s, std::size_t len) : field_type(ft_String), str_value(s, len)
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
      return std::to_string(short_value);
    }
    case ft_UShort:
    {
      return std::to_string(ushort_value);
    }
    case ft_Int:
    {
      return std::to_string(int_value);
    }
    case ft_UInt:
    {
      return std::to_string(uint_value);
    }
    case ft_Float:
    {
      return std::to_string(float_value);
    }
    case ft_Double:
    {
      return std::to_string(double_value);
    }
    case ft_Int64:
    {
      return std::to_string(int64_value);
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
      return static_cast<bool>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<bool>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<bool>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<bool>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<bool>(float_value);
    }
    case ft_Double:
    {
      return static_cast<bool>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<bool>(int64_value);
    }
    default:
      return false;
  }
}

char field_value::get_asChar() const
{
  return get_asString()[0];
}

short field_value::get_asShort() const
{
  switch (field_type)
  {
    case ft_String:
    {
      return static_cast<short>(std::atoi(str_value.c_str()));
    }
    case ft_Boolean:
    {
      return static_cast<short>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<short>(char_value);
    }
    case ft_Short:
    {
      return short_value;
    }
    case ft_UShort:
    {
      return static_cast<short>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<short>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<short>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<short>(float_value);
    }
    case ft_Double:
    {
      return static_cast<short>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<short>(int64_value);
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
      return static_cast<unsigned short>(std::atoi(str_value.c_str()));
    }
    case ft_Boolean:
    {
      return static_cast<unsigned short>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<unsigned short>(char_value);
    }
    case ft_Short:
    {
      return static_cast<unsigned short>(short_value);
    }
    case ft_UShort:
    {
      return ushort_value;
    }
    case ft_Int:
    {
      return static_cast<unsigned short>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<unsigned short>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<unsigned short>(float_value);
    }
    case ft_Double:
    {
      return static_cast<unsigned short>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<unsigned short>(int64_value);
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
      return std::atoi(str_value.c_str());
    }
    case ft_Boolean:
    {
      return static_cast<int>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<int>(char_value);
    }
    case ft_Short:
    {
      return static_cast<int>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<int>(ushort_value);
    }
    case ft_Int:
    {
      return int_value;
    }
    case ft_UInt:
    {
      return static_cast<int>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<int>(float_value);
    }
    case ft_Double:
    {
      return static_cast<int>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<int>(int64_value);
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
      return static_cast<unsigned int>(std::atoi(str_value.c_str()));
    }
    case ft_Boolean:
    {
      return static_cast<unsigned int>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<unsigned int>(char_value);
    }
    case ft_Short:
    {
      return static_cast<unsigned int>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<unsigned int>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<unsigned int>(int_value);
    }
    case ft_UInt:
    {
      return uint_value;
    }
    case ft_Float:
    {
      return static_cast<unsigned int>(float_value);
    }
    case ft_Double:
    {
      return static_cast<unsigned int>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<unsigned int>(int64_value);
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
      return static_cast<float>(std::atof(str_value.c_str()));
    }
    case ft_Boolean:
    {
      return static_cast<float>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<float>(char_value);
    }
    case ft_Short:
    {
      return static_cast<float>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<float>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<float>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<float>(uint_value);
    }
    case ft_Float:
    {
      return float_value;
    }
    case ft_Double:
    {
      return static_cast<float>(double_value);
    }
    case ft_Int64:
    {
      return static_cast<float>(int64_value);
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
      return std::atof(str_value.c_str());
    }
    case ft_Boolean:
    {
      return static_cast<double>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<double>(char_value);
    }
    case ft_Short:
    {
      return static_cast<double>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<double>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<double>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<double>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<double>(float_value);
    }
    case ft_Double:
    {
      return double_value;
    }
    case ft_Int64:
    {
      return static_cast<double>(int64_value);
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
      return static_cast<int64_t>(bool_value);
    }
    case ft_Char:
    {
      return static_cast<int64_t>(char_value);
    }
    case ft_Short:
    {
      return static_cast<int64_t>(short_value);
    }
    case ft_UShort:
    {
      return static_cast<int64_t>(ushort_value);
    }
    case ft_Int:
    {
      return static_cast<int64_t>(int_value);
    }
    case ft_UInt:
    {
      return static_cast<int64_t>(uint_value);
    }
    case ft_Float:
    {
      return static_cast<int64_t>(float_value);
    }
    case ft_Double:
    {
      return static_cast<int64_t>(double_value);
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

  return *this;
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
      break;
    default:
      *this = fv;
      break;
  }
  return *this;
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

void field_value::set_asString(std::string_view s)
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

fType field_value::get_field_type() const
{
  return field_type;
}

std::string_view field_value::gft() const
{
  switch (field_type)
  {
    case ft_String:
      return "string";
    case ft_Boolean:
      return "bool";
    case ft_Char:
      return "char";
    case ft_Short:
      return "short";
    case ft_Int:
      return "int";
    case ft_Float:
      return "float";
    case ft_Double:
      return "double";
    case ft_Int64:
      return "int64";
    default:
      break;
  }
  return "";
}

} // namespace dbiplus
