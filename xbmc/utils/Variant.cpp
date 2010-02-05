#include "Variant.h"
#include <string.h>
#include <assert.h>

using namespace std;

CVariant::CVariant(VariantType type)
{
  m_type = type;

  if (isString())
    m_data.string = NULL;
  else if (isArray())
    m_data.array = new VariantArray();
  else if (isObject())
    m_data.map = new VariantMap();
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

CVariant::CVariant(const CVariant &variant)
{
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
  return m_type == VariantTypeNull;
}

int64_t CVariant::asInteger() const
{
  assert(isInteger());
  return m_data.integer;
}

uint64_t CVariant::asUnsignedInteger() const
{
  assert(isUnsignedInteger());
  return m_data.unsignedinteger;
}

bool CVariant::asBoolean() const
{
  assert(isBoolean());
  return m_data.integer;
}

const char *CVariant::asString() const
{
  assert(isString());
  return m_data.string->c_str();
}

CVariant &CVariant::operator[](string key)
{
  assert(isObject() || isNull());
  if (isNull())
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }
  return (*m_data.map)[key];
}

CVariant &CVariant::operator[](unsigned int position)
{
  assert(isArray() && size() > position);
  return (*m_data.array)[position];
}

CVariant &CVariant::operator=(const CVariant &rhs)
{
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
  assert(isArray() || isNull());
  if (isNull())
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }
  m_data.array->push_back(variant);
}

unsigned int CVariant::size() const
{
  assert(isNull() || isObject() || isArray());

  if (isObject())
    return m_data.map->size();
  else if (isArray())
    return m_data.array->size();
  else
    return 0;
}

bool CVariant::empty() const
{
  assert(isNull() || isObject() || isArray());

  if (isObject())
    return m_data.map->empty();
  else if (isArray())
    return m_data.array->empty();
  else
    return true;
}

void CVariant::clear()
{
  assert(isNull() || isObject() || isArray());

  if (isObject())
    m_data.map->clear();
  else if (isArray())
    m_data.array->clear();
}

void CVariant::erase(std::string key)
{
  assert(isObject() || isNull());
  if (isNull())
  {
    m_type = VariantTypeObject;
    m_data.map = new VariantMap();
  }
  
  m_data.map->erase(key);
}

void CVariant::erase(unsigned int position)
{
  assert(isArray() || isNull());
  if (isNull())
  {
    m_type = VariantTypeArray;
    m_data.array = new VariantArray();
  }

  if (position < size())
    m_data.array->erase(m_data.array->begin() + position);
}

#include <stdio.h>

void CVariant::debug()
{
  internaldebug();
  printf("\n");
}

void CVariant::internaldebug()
{
  switch (m_type)
  {
  case VariantTypeInteger:
    printf("int: %lld", m_data.integer);
    break;
  case VariantTypeUnsignedInteger:
    printf("uint: %lld", m_data.unsignedinteger);
    break;
  case VariantTypeBoolean:
    printf("bool: %s", m_data.boolean ? "true" : "false");
    break;
  case VariantTypeString:
    printf("string: \"%s\"", m_data.string->c_str());
    break;
  case VariantTypeArray:
    printf("array: [");
    for (unsigned int i = 0; i < size(); i++)
    {
      (*m_data.array)[i].internaldebug();
      if (i < size() - 1)
        printf(", ");
    }
    printf("]");
    break;
  case VariantTypeObject:
    printf("map: [");
    for (VariantMap::iterator itr = m_data.map->begin(); itr != m_data.map->end(); itr++)
    {
      printf("key='%s' value='", itr->first.c_str());
      itr->second.internaldebug();
      printf("' ");
    }
    printf("]");
    break;
  default:
    break;
  }
}
