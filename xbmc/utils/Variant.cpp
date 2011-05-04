/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "Variant.h"
#include <string.h>

using namespace std;

CVariant CVariant::ConstNullVariant = CVariant::VariantTypeConstNull;

CVariant::CVariant(VariantType type)
{
  m_type = type;

  switch (type)
  {
    case VariantTypeInteger:
      m_data.integer = 0;
      break;
    case VariantTypeUnsignedInteger:
      m_data.unsignedinteger = 0;
      break;
    case VariantTypeBoolean:
      m_data.boolean = false;
      break;
    case VariantTypeString:
      m_data.string = NULL;
      break;
    case VariantTypeDouble:
      m_data.dvalue = 0.0;
      break;
    case VariantTypeArray:
      m_data.array = new VariantArray();
      break;
    case VariantTypeObject:
      m_data.map = new VariantMap();
      break;
    default:
      memset(&m_data, 0, sizeof(m_data));
      break;
  }
}

CVariant::CVariant(int integer)
{
  m_type = VariantTypeInteger;
  m_data.integer = integer;
}

CVariant::CVariant(int64_t integer)
{
  m_type = VariantTypeInteger;
  m_data.integer = integer;
}

CVariant::CVariant(unsigned int unsignedinteger)
{
  m_type = VariantTypeUnsignedInteger;
  m_data.unsignedinteger = unsignedinteger;
}

CVariant::CVariant(uint64_t unsignedinteger)
{
  m_type = VariantTypeUnsignedInteger;
  m_data.unsignedinteger = unsignedinteger;
}

CVariant::CVariant(double value)
{
  m_type = VariantTypeDouble;
  m_data.dvalue = value;
}

CVariant::CVariant(float value)
{
  m_type = VariantTypeDouble;
  m_data.dvalue = (double)value;
}

CVariant::CVariant(bool boolean)
{
  m_type = VariantTypeBoolean;
  m_data.boolean = boolean;
}

CVariant::CVariant(const char *str)
{
  m_type = VariantTypeString;
  m_data.string = new string(str);
}

CVariant::CVariant(const char *str, unsigned int length)
{
  m_type = VariantTypeString;
  m_data.string = new string(str, length);
}

CVariant::CVariant(const string &str)
{
  m_type = VariantTypeString;
  m_data.string = new string(str);
}

CVariant::CVariant(const CVariant &variant)
{
  m_type = variant.m_type;
  *this = variant;
}

CVariant::~CVariant()
{
  if (m_type == VariantTypeString && m_data.string)
  {
    delete m_data.string;
    m_data.string = NULL;
  }
  else if (m_type == VariantTypeArray && m_data.array)
  {
    delete m_data.array;
    m_data.array = NULL;
  }
  else if (m_type == VariantTypeObject && m_data.map)
  {
    delete m_data.map;
    m_data.map = NULL;
  }
}

bool CVariant::isInteger() const
{
  return m_type == VariantTypeInteger;
}

bool CVariant::isUnsignedInteger() const
{
  return m_type == VariantTypeUnsignedInteger;
}

bool CVariant::isBoolean() const
{
  return m_type == VariantTypeBoolean;
}

bool CVariant::isDouble() const
{
  return m_type == VariantTypeDouble;
}

bool CVariant::isString() const
{
  return m_type == VariantTypeString;
}

bool CVariant::isArray() const
{
  return m_type == VariantTypeArray;
}

bool CVariant::isObject() const
{
  return m_type == VariantTypeObject;
}

bool CVariant::isNull() const
{
  return m_type == VariantTypeNull || m_type == VariantTypeConstNull;
}

CVariant::VariantType CVariant::type() const
{
  return m_type;
}

int64_t CVariant::asInteger(int64_t fallback) const
{
  if (m_type == VariantTypeInteger)
    return m_data.integer;
  else
    return fallback;
}

uint64_t CVariant::asUnsignedInteger(uint64_t fallback) const
{
  if (m_type == VariantTypeUnsignedInteger)
    return m_data.unsignedinteger;
  else
    return fallback;
}

double CVariant::asDouble(double fallback) const
{
  if (m_type == VariantTypeDouble)
    return m_data.dvalue;
  else
    return fallback;
}

float CVariant::asFloat(float fallback) const
{
  if (m_type == VariantTypeDouble)
    return (float)m_data.dvalue;
  else
    return fallback;
}

bool CVariant::asBoolean(bool fallback) const
{
  if (m_type == VariantTypeBoolean)
    return m_data.boolean;
  else
    return fallback;
}

const char *CVariant::asString(const char *fallback) const
{
  if (m_type == VariantTypeString)
    return m_data.string->c_str();
  else
    return fallback;
}

CVariant &CVariant::operator[](string key)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }

  if (m_type == VariantTypeObject)
    return (*m_data.map)[key];
  else
    return ConstNullVariant;
}

const CVariant &CVariant::operator[](std::string key) const
{
  if (m_type == VariantTypeObject)
    return (*m_data.map)[key];
  else
    return ConstNullVariant;
}

CVariant &CVariant::operator[](unsigned int position)
{
  if (m_type == VariantTypeArray && size() > position)
    return (*m_data.array)[position];
  else
    return ConstNullVariant;
}

const CVariant &CVariant::operator[](unsigned int position) const
{
  if (m_type == VariantTypeArray && size() > position)
    return (*m_data.array)[position];
  else
    return ConstNullVariant;
}

CVariant &CVariant::operator=(const CVariant &rhs)
{
  if (m_type == VariantTypeConstNull)
    return *this;

  m_type = rhs.m_type;

  switch (m_type)
  {
  case VariantTypeInteger:
    m_data.integer = rhs.m_data.integer;
    break;
  case VariantTypeUnsignedInteger:
    m_data.integer = rhs.m_data.unsignedinteger;
    break;
  case VariantTypeBoolean:
    m_data.boolean = rhs.m_data.boolean;
    break;
  case VariantTypeDouble:
    m_data.dvalue = rhs.m_data.dvalue;
    break;
  case VariantTypeString:
    m_data.string = new string(rhs.m_data.string->c_str());
    break;
  case VariantTypeArray:
    m_data.array = new VariantArray(rhs.m_data.array->begin(), rhs.m_data.array->end());
    break;
  case VariantTypeObject:
    m_data.map = new VariantMap(rhs.m_data.map->begin(), rhs.m_data.map->end());
    break;
  default:
    break;
  }

  return *this;
}

bool CVariant::operator==(const CVariant &rhs) const
{
  if (m_type == rhs.m_type)
  {
    switch (m_type)
    {
    case VariantTypeInteger:
      return m_data.integer == rhs.m_data.integer;
      break;
    case VariantTypeUnsignedInteger:
      return m_data.unsignedinteger == rhs.m_data.unsignedinteger;
      break;
    case VariantTypeBoolean:
      return m_data.boolean == rhs.m_data.boolean;
      break;
    case VariantTypeDouble:
      return m_data.dvalue == rhs.m_data.dvalue;
      break;
    case VariantTypeString:
      return (*m_data.string) == (*rhs.m_data.string);
      break;
    case VariantTypeArray:
      return (*m_data.array) == (*rhs.m_data.array);
      break;
    case VariantTypeObject:
      return (*m_data.map) == (*rhs.m_data.map);
      break;
    default:
      break;
    }
  }

  return false;
}

void CVariant::push_back(CVariant variant)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }

  if (m_type == VariantTypeArray)
    m_data.array->push_back(variant);
}

void CVariant::append(CVariant variant)
{
  push_back(variant);
}

const char *CVariant::c_str() const
{
  if (m_type == VariantTypeString)
    return m_data.string->c_str();
  else
    return NULL;
}

void CVariant::swap(CVariant &rhs)
{
  VariantType  temp_type = m_type;
  VariantUnion temp_data = m_data;

  m_type = rhs.m_type;
  m_data = rhs.m_data;

  rhs.m_type = temp_type;
  rhs.m_data = temp_data;
}

CVariant::iterator_array CVariant::begin_array()
{
  if (m_type == VariantTypeArray)
    return m_data.array->begin();
  else
    return iterator_array();
}

CVariant::const_iterator_array CVariant::begin_array() const
{
  if (m_type == VariantTypeArray)
    return m_data.array->begin();
  else
    return const_iterator_array();
}

CVariant::iterator_array CVariant::end_array()
{
  if (m_type == VariantTypeArray)
    return m_data.array->end();
  else
    return iterator_array();
}

CVariant::const_iterator_array CVariant::end_array() const
{
  if (m_type == VariantTypeArray)
    return m_data.array->end();
  else
    return const_iterator_array();
}

CVariant::iterator_map CVariant::begin_map()
{
  if (m_type == VariantTypeObject)
    return m_data.map->begin();
  else
    return iterator_map();
}

CVariant::const_iterator_map CVariant::begin_map() const
{
  if (m_type == VariantTypeObject)
    return m_data.map->begin();
  else
    return const_iterator_map();
}

CVariant::iterator_map CVariant::end_map()
{
  if (m_type == VariantTypeObject)
    return m_data.map->end();
  else
    return iterator_map();
}

CVariant::const_iterator_map CVariant::end_map() const
{
  if (m_type == VariantTypeObject)
    return m_data.map->end();
  else
    return const_iterator_map();
}

unsigned int CVariant::size() const
{
  if (m_type == VariantTypeObject)
    return m_data.map->size();
  else if (m_type == VariantTypeArray)
    return m_data.array->size();
  else if (m_type == VariantTypeString)
    return m_data.string->size();
  else
    return 0;
}

bool CVariant::empty() const
{
  if (m_type == VariantTypeObject)
    return m_data.map->empty();
  else if (m_type == VariantTypeArray)
    return m_data.array->empty();
  else
    return true;
}

void CVariant::clear()
{
  if (m_type == VariantTypeObject)
    m_data.map->clear();
  else if (m_type == VariantTypeArray)
    m_data.array->clear();
}

void CVariant::erase(std::string key)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }
  else if (m_type == VariantTypeObject)
    m_data.map->erase(key);
}

void CVariant::erase(unsigned int position)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }

  if (m_type == VariantTypeArray && position < size())
    m_data.array->erase(m_data.array->begin() + position);
}

bool CVariant::isMember(string key) const
{
  if (m_type == VariantTypeObject)
    return m_data.map->find(key) != m_data.map->end();

  return false;
}
