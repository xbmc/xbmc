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
#include "PlatformDefs.h"
#include <string.h>
#include "jsoncpp/include/json/value.h"

using namespace std;

CVariant CVariant::ConstNullVariant = CVariant::VariantTypeConstNull;

CVariant::CVariant(VariantType type)
{
  m_type = type;

  if (isString())
    m_data.string = NULL;
  else if (isArray())
    m_data.array = new VariantArray();
  else if (isObject())
    m_data.map = new VariantMap();
  else
    memset(&m_data, 0, sizeof(m_data));
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

CVariant::CVariant(float fFloat)
{
  m_type = VariantTypeFloat;
  m_data.fFloat = fFloat;
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
  if (isString() && m_data.string)
  {
    delete m_data.string;
    m_data.string = NULL;
  }
  else if (isArray() && m_data.array)
  {
    delete m_data.array;
    m_data.array = NULL;
  }
  else if (isObject() && m_data.map)
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

bool CVariant::isFloat() const
{
  return m_type == VariantTypeFloat;
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

int64_t CVariant::asInteger(int64_t fallback) const
{
  if (isInteger())
    return m_data.integer;
  else
    return fallback;
}

uint64_t CVariant::asUnsignedInteger(uint64_t fallback) const
{
  if (isUnsignedInteger())
    return m_data.unsignedinteger;
  else
    return fallback;
}

float CVariant::asFloat(float fallback) const
{
  if (isFloat())
    return m_data.fFloat;
  else
    return fallback;
}

bool CVariant::asBoolean(bool fallback) const
{
  if (isBoolean())
    return m_data.boolean;
  else
    return fallback;
}

const char *CVariant::asString(const char *fallback) const
{
  if (isString())
    return m_data.string->c_str();
  else
    return fallback;
}

void CVariant::toJsonValue(Json::Value& value) const
{
  switch (m_type)
  {
  case VariantTypeInteger:
    value = (int32_t) m_data.integer;
    break;
  case VariantTypeUnsignedInteger:
    value = (uint32_t) m_data.unsignedinteger;
    break;
  case VariantTypeBoolean:
    value = m_data.boolean;
    break;
  case VariantTypeFloat:
    value = m_data.fFloat;
    break;
  case VariantTypeString:
    value = (*m_data.string);
    break;
  case VariantTypeArray:
    for (unsigned int i = 0; i < size(); i++)
    {
      Json::Value array;
      (*m_data.array)[i].toJsonValue(array);
      value.append(array);
    }
    break;
  case VariantTypeObject:
    for (VariantMap::iterator itr = m_data.map->begin(); itr != m_data.map->end(); itr++)
    {
      Json::Value object;
      itr->second.toJsonValue(object);
      value[itr->first] = object;
    }
    break;
  default:
    break;
  }
}

CVariant &CVariant::operator[](string key)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }

  if (isObject())
    return (*m_data.map)[key];
  else
    return ConstNullVariant;
}

const CVariant &CVariant::operator[](std::string key) const
{
  if (isObject())
    return (*m_data.map)[key];
  else
    return ConstNullVariant;
}

CVariant &CVariant::operator[](unsigned int position)
{
  if (isArray() && size() > position)
    return (*m_data.array)[position];
  else
    return ConstNullVariant;
}

const CVariant &CVariant::operator[](unsigned int position) const
{
  if (isArray() && size() > position)
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
  case VariantTypeFloat:
    m_data.fFloat = rhs.m_data.fFloat;
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

void CVariant::push_back(CVariant variant)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }

  if (isArray())
    m_data.array->push_back(variant);
}

unsigned int CVariant::size() const
{
  if (isObject())
    return m_data.map->size();
  else if (isArray())
    return m_data.array->size();
  else
    return 0;
}

bool CVariant::empty() const
{
  if (isObject())
    return m_data.map->empty();
  else if (isArray())
    return m_data.array->empty();
  else
    return true;
}

void CVariant::clear()
{
  if (isObject())
    m_data.map->clear();
  else if (isArray())
    m_data.array->clear();
}

void CVariant::erase(std::string key)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }
  else if (isObject())
    m_data.map->erase(key);
}

void CVariant::erase(unsigned int position)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }

  if (isArray() && position < size())
    m_data.array->erase(m_data.array->begin() + position);
}
