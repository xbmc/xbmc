/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Variant.h"

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#ifndef strtoll
#ifdef TARGET_WINDOWS
#define strtoll  _strtoi64
#define strtoull _strtoui64
#define wcstoll  _wcstoi64
#define wcstoull _wcstoui64
#else // TARGET_WINDOWS
#if !defined(TARGET_DARWIN)
#define strtoll(str, endptr, base)  (int64_t)strtod(str, endptr)
#define strtoull(str, endptr, base) (uint64_t)strtod(str, endptr)
#define wcstoll(str, endptr, base)  (int64_t)wcstod(str, endptr)
#define wcstoull(str, endptr, base) (uint64_t)wcstod(str, endptr)
#endif
#endif // TARGET_WINDOWS
#endif // strtoll

std::string_view trim(std::string_view str)
{
  str.remove_prefix(std::min(str.find_first_not_of(" \n\r\t"), str.size()));
  str.remove_suffix(str.size() - std::min(str.find_last_not_of(" \n\r\t") + 1, str.size()));
  return str;
}

std::wstring_view trim(std::wstring_view str)
{
  str.remove_prefix(std::min(str.find_first_not_of(L" \n\r\t"), str.size()));
  str.remove_suffix(str.size() - std::min(str.find_last_not_of(L" \n\r\t") + 1, str.size()));
  return str;
}

int64_t str2int64(std::string_view str, int64_t fallback /* = 0 */)
{
  std::string_view tmp = trim(str);
  int64_t result{};
  const std::from_chars_result res = std::from_chars(tmp.data(), tmp.data() + tmp.size(), result);
  if (res.ec == std::errc())
    return result;

  return fallback;
}

int64_t str2int64(std::wstring_view str, int64_t fallback /* = 0 */)
{
  wchar_t *end = NULL;
  std::wstring tmp(trim(str));
  int64_t result = wcstoll(tmp.c_str(), &end, 0);
  if (end == NULL || *end == '\0')
    return result;

  return fallback;
}

uint64_t str2uint64(std::string_view str, uint64_t fallback /* = 0 */)
{
  std::string_view tmp = trim(str);
  uint64_t result{};
  const std::from_chars_result res = std::from_chars(tmp.data(), tmp.data() + tmp.size(), result);
  if (res.ec == std::errc())
    return result;

  return fallback;
}

uint64_t str2uint64(std::wstring_view str, uint64_t fallback /* = 0 */)
{
  wchar_t *end = NULL;
  std::wstring tmp(trim(str));
  uint64_t result = wcstoull(tmp.c_str(), &end, 0);
  if (end == NULL || *end == '\0')
    return result;

  return fallback;
}

double str2double(std::string_view str, double fallback /* = 0.0 */)
{
  std::string_view trimmedStr = trim(str);
  char cStr[512]; // definitely big enough to hold any floating point number
  const size_t copySize = std::min(trimmedStr.size(), sizeof(cStr) - 1);
  std::memcpy(cStr, trimmedStr.data(), copySize);
  cStr[copySize] = '\0';
  char* end = nullptr;
  double result = strtod(cStr, &end);
  if (end == NULL || *end == '\0')
    return result;

  // Use this once std::from_char with double is supported on all platform
  //std::string_view tmp = trim(str);
  //double result{};
  //const std::from_chars_result res = std::from_chars(tmp.data(), tmp.data() + tmp.size(), result);
  //if (res.ec == std::errc() && res.ptr == tmp.data() + tmp.size())
  //  return result;

  return fallback;
}

double str2double(std::wstring_view str, double fallback /* = 0.0 */)
{
  std::wstring_view trimmedStr = trim(str);
  wchar_t wcStr[512]; // definitely big enough to hold any floating point number
  const size_t copySize = std::min(trimmedStr.size(), (sizeof(wcStr) / sizeof(wchar_t)) - 1);
  std::memcpy(wcStr, trimmedStr.data(), copySize * sizeof(wchar_t));
  wcStr[copySize] = '\0';
  wchar_t* end = nullptr;
  double result = wcstod(wcStr, &end);
  if (end == NULL || *end == '\0')
    return result;

  return fallback;
}

CVariant::CVariant()
  : CVariant(VariantTypeNull)
{
}

CVariant CVariant::ConstNullVariant = CVariant::VariantTypeConstNull;
CVariant::VariantArray CVariant::EMPTY_ARRAY;
CVariant::VariantMap CVariant::EMPTY_MAP;

CVariant::CVariant(VariantType type)
{
  m_type = type;

  switch (type)
  {
    case VariantTypeInteger:
      m_data = int64_t{};
      break;
    case VariantTypeUnsignedInteger:
      m_data = uint64_t{};
      break;
    case VariantTypeBoolean:
      m_data = bool{};
      break;
    case VariantTypeDouble:
      m_data = double{};
      break;
    case VariantTypeString:
      m_data = std::string{};
      break;
    case VariantTypeWideString:
      m_data = std::wstring{};
      break;
    case VariantTypeArray:
      m_data = VariantArray{};
      break;
    case VariantTypeObject:
      m_data = VariantMap{};
      break;
    default:
      m_data = std::monostate{};
      break;
  }
}

CVariant::CVariant(int integer) : m_type(VariantTypeInteger), m_data(static_cast<int64_t>(integer))
{
}

CVariant::CVariant(int64_t integer) : m_type(VariantTypeInteger), m_data(integer)
{
}

CVariant::CVariant(unsigned int unsignedinteger)
  : m_type(VariantTypeUnsignedInteger), m_data(static_cast<uint64_t>(unsignedinteger))
{
}

CVariant::CVariant(uint64_t unsignedinteger)
  : m_type(VariantTypeUnsignedInteger), m_data(unsignedinteger)
{
}

CVariant::CVariant(double value) : m_type(VariantTypeDouble), m_data(value)
{
}

CVariant::CVariant(float value) : m_type(VariantTypeDouble), m_data(static_cast<double>(value))
{
}

CVariant::CVariant(bool boolean) : m_type(VariantTypeBoolean), m_data(boolean)
{
}

CVariant::CVariant(const char* str)
  : m_type(VariantTypeString), m_data(std::in_place_type<std::string>, str)
{
}

CVariant::CVariant(const char* str, unsigned int length)
  : m_type(VariantTypeString), m_data(std::in_place_type<std::string>, str, length)
{
}

CVariant::CVariant(const std::string& str)
  : m_type(VariantTypeString), m_data(std::in_place_type<std::string>, str)
{
}

CVariant::CVariant(std::string&& str)
  : m_type(VariantTypeString), m_data(std::in_place_type<std::string>, std::move(str))
{
}

CVariant::CVariant(const wchar_t* str)
  : m_type(VariantTypeWideString), m_data(std::in_place_type<std::wstring>, str)
{
}

CVariant::CVariant(const wchar_t* str, unsigned int length)
  : m_type(VariantTypeWideString), m_data(std::in_place_type<std::wstring>, str, length)
{
}

CVariant::CVariant(const std::wstring& str)
  : m_type(VariantTypeWideString), m_data(std::in_place_type<std::wstring>, str)
{
}

CVariant::CVariant(std::wstring&& str)
  : m_type(VariantTypeWideString), m_data(std::in_place_type<std::wstring>, std::move(str))
{
}

CVariant::CVariant(const std::vector<std::string> &strArray)
{
  VariantArray tmpArray;
  tmpArray.reserve(strArray.size());
  for (const auto& item : strArray)
    tmpArray.emplace_back(item);

  m_type = VariantTypeArray;
  m_data = std::move(tmpArray);
}

CVariant::CVariant(const std::map<std::string, std::string> &strMap)
{
  VariantMap tmpMap;
  for (const auto& elem : strMap)
    tmpMap.emplace(elem.first, CVariant(elem.second));

  m_type = VariantTypeObject;
  m_data = std::move(tmpMap);
}

CVariant::CVariant(const std::map<std::string, CVariant>& variantMap)
  : m_type(VariantTypeObject), m_data(std::in_place_type<VariantMap>, variantMap)
{
}

CVariant::CVariant(const CVariant& variant) : m_type(variant.m_type), m_data(variant.m_data)
{
}

CVariant::CVariant(CVariant&& rhs) noexcept : m_type(rhs.m_type), m_data(std::move(rhs.m_data))
{
}

CVariant::~CVariant()
{
  cleanup();
}

void CVariant::cleanup()
{
  m_data = std::monostate{};
  m_type = VariantTypeNull;
}

bool CVariant::isInteger() const
{
  return isSignedInteger() || isUnsignedInteger();
}

bool CVariant::isSignedInteger() const
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

bool CVariant::isWideString() const
{
  return m_type == VariantTypeWideString;
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
  switch (m_type)
  {
    case VariantTypeInteger:
      return std::get<int64_t>(m_data);
    case VariantTypeUnsignedInteger:
      return static_cast<int64_t>(std::get<uint64_t>(m_data));
    case VariantTypeDouble:
      return static_cast<int64_t>(std::get<double>(m_data));
    case VariantTypeString:
      return str2int64(std::get<std::string>(m_data), fallback);
    case VariantTypeWideString:
      return str2int64(std::get<std::wstring>(m_data), fallback);
    default:
      return fallback;
  }
}

int32_t CVariant::asInteger32(int32_t fallback) const
{
  return static_cast<int32_t>(asInteger(fallback));
}

uint64_t CVariant::asUnsignedInteger(uint64_t fallback) const
{
  switch (m_type)
  {
    case VariantTypeUnsignedInteger:
      return std::get<uint64_t>(m_data);
    case VariantTypeInteger:
      return static_cast<uint64_t>(std::get<int64_t>(m_data));
    case VariantTypeDouble:
      return static_cast<uint64_t>(std::get<double>(m_data));
    case VariantTypeString:
      return str2uint64(std::get<std::string>(m_data), fallback);
    case VariantTypeWideString:
      return str2uint64(std::get<std::wstring>(m_data), fallback);
    default:
      return fallback;
  }
}

uint32_t CVariant::asUnsignedInteger32(uint32_t fallback) const
{
  return static_cast<uint32_t>(asUnsignedInteger(fallback));
}

double CVariant::asDouble(double fallback) const
{
  switch (m_type)
  {
    case VariantTypeDouble:
      return std::get<double>(m_data);
    case VariantTypeInteger:
      return static_cast<double>(std::get<int64_t>(m_data));
    case VariantTypeUnsignedInteger:
      return static_cast<double>(std::get<uint64_t>(m_data));
    case VariantTypeString:
      return str2double(std::get<std::string>(m_data), fallback);
    case VariantTypeWideString:
      return str2double(std::get<std::wstring>(m_data), fallback);
    default:
      return fallback;
  }
}

float CVariant::asFloat(float fallback) const
{

  return static_cast<float>(asDouble(static_cast<double>(fallback)));
}

bool CVariant::asBoolean(bool fallback) const
{
  switch (m_type)
  {
    case VariantTypeBoolean:
      return std::get<bool>(m_data);
    case VariantTypeInteger:
      return (std::get<int64_t>(m_data) != 0);
    case VariantTypeUnsignedInteger:
      return (std::get<uint64_t>(m_data) != 0);
    case VariantTypeDouble:
      return (std::get<double>(m_data) != 0);
    case VariantTypeString:
    {
      const auto& str = std::get<std::string>(m_data);
      if (str.empty() || str == "0" || str == "false")
        return false;
      return true;
    }
    case VariantTypeWideString:
    {
      const auto& wstr = std::get<std::wstring>(m_data);
      if (wstr.empty() || wstr == L"0" || wstr == L"false")
        return false;
      return true;
    }
    default:
      return fallback;
  }
}

std::string CVariant::asString(std::string_view fallback /* = "" */) const&
{
  switch (m_type)
  {
    case VariantTypeString:
      return std::get<std::string>(m_data);
    case VariantTypeBoolean:
      return std::get<bool>(m_data) ? "true" : "false";
    case VariantTypeInteger:
      return std::to_string(std::get<int64_t>(m_data));
    case VariantTypeUnsignedInteger:
      return std::to_string(std::get<uint64_t>(m_data));
    case VariantTypeDouble:
      return std::to_string(std::get<double>(m_data));
    default:
      return std::string(fallback);
  }
}

std::string CVariant::asString(std::string_view fallback /*= ""*/) &&
{
  if (m_type == VariantTypeString)
    return std::move(std::get<std::string>(m_data));
  else
    return asString(fallback);
}

std::wstring CVariant::asWideString(std::wstring_view fallback /* = L"" */) const&
{
  switch (m_type)
  {
    case VariantTypeWideString:
      return std::get<std::wstring>(m_data);
    case VariantTypeBoolean:
      return std::get<bool>(m_data) ? L"true" : L"false";
    case VariantTypeInteger:
      return std::to_wstring(std::get<int64_t>(m_data));
    case VariantTypeUnsignedInteger:
      return std::to_wstring(std::get<uint64_t>(m_data));
    case VariantTypeDouble:
      return std::to_wstring(std::get<double>(m_data));
    default:
      return std::wstring(fallback);
  }
}

std::wstring CVariant::asWideString(std::wstring_view fallback /*= L""*/) &&
{
  if (m_type == VariantTypeWideString)
    return std::move(std::get<std::wstring>(m_data));
  else
    return asWideString(fallback);
}

CVariant& CVariant::operator[](const std::string& key) &
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data = VariantMap{};
  }

  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data)[key];
  else
    return ConstNullVariant;
}

const CVariant& CVariant::operator[](const std::string& key) const&
{
  VariantMap::const_iterator it;
  if (m_type == VariantTypeObject &&
      (it = std::get<VariantMap>(m_data).find(key)) != std::get<VariantMap>(m_data).end())
    return it->second;
  else
    return ConstNullVariant;
}

CVariant CVariant::operator[](const std::string& key) &&
{
  if (m_type == VariantTypeObject)
    return std::move(std::get<VariantMap>(m_data)[key]);
  else
    return ConstNullVariant;
}

CVariant& CVariant::operator[](unsigned int position) &
{
  if (m_type == VariantTypeArray && size() > position)
    return std::get<VariantArray>(m_data).at(position);
  else
    return ConstNullVariant;
}

const CVariant& CVariant::operator[](unsigned int position) const&
{
  if (m_type == VariantTypeArray && size() > position)
    return std::get<VariantArray>(m_data).at(position);
  else
    return ConstNullVariant;
}

CVariant CVariant::operator[](unsigned int position) &&
{
  if (m_type == VariantTypeArray && size() > position)
    return std::move(std::get<VariantArray>(m_data).at(position));
  else
    return ConstNullVariant;
}

CVariant &CVariant::operator=(const CVariant &rhs)
{
  if (m_type == VariantTypeConstNull || this == &rhs)
    return *this;

  cleanup();

  m_type = rhs.m_type;
  m_data = rhs.m_data;

  return *this;
}

CVariant& CVariant::operator=(CVariant&& rhs) noexcept
{
  if (m_type == VariantTypeConstNull || this == &rhs)
    return *this;

  m_type = rhs.m_type;
  m_data = std::move(rhs.m_data);

  return *this;
}

bool CVariant::operator==(const CVariant &rhs) const
{
  return m_type == rhs.m_type && m_data == rhs.m_data;
}

void CVariant::reserve(size_t length)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data = VariantArray{};
  }
  if (m_type == VariantTypeArray)
    std::get<VariantArray>(m_data).reserve(length);
}

void CVariant::push_back(const CVariant &variant)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data = VariantArray{};
  }

  if (m_type == VariantTypeArray)
    std::get<VariantArray>(m_data).emplace_back(variant);
}

void CVariant::push_back(CVariant &&variant)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data = VariantArray{};
  }

  if (m_type == VariantTypeArray)
    std::get<VariantArray>(m_data).emplace_back(std::move(variant));
}

void CVariant::append(const CVariant &variant)
{
  push_back(variant);
}

void CVariant::append(CVariant&& variant)
{
  push_back(std::move(variant));
}

const char *CVariant::c_str() const
{
  if (m_type == VariantTypeString)
    return std::get<std::string>(m_data).c_str();
  else
    return nullptr;
}

void CVariant::swap(CVariant& rhs) noexcept
{
  std::swap(m_data, rhs.m_data);
  std::swap(m_type, rhs.m_type);
}

CVariant::iterator_array CVariant::begin_array()
{
  if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).begin();
  else
    return EMPTY_ARRAY.begin();
}

CVariant::const_iterator_array CVariant::begin_array() const
{
  if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).cbegin();
  else
    return EMPTY_ARRAY.cbegin();
}

CVariant::iterator_array CVariant::end_array()
{
  if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).end();
  else
    return EMPTY_ARRAY.end();
}

CVariant::const_iterator_array CVariant::end_array() const
{
  if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).cend();
  else
    return EMPTY_ARRAY.cend();
}

CVariant::iterator_map CVariant::begin_map()
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).begin();
  else
    return EMPTY_MAP.begin();
}

CVariant::const_iterator_map CVariant::begin_map() const
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).cbegin();
  else
    return EMPTY_MAP.cbegin();
}

CVariant::iterator_map CVariant::end_map()
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).end();
  else
    return EMPTY_MAP.end();
}

CVariant::const_iterator_map CVariant::end_map() const
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).cend();
  else
    return EMPTY_MAP.end();
}

unsigned int CVariant::size() const
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).size();
  else if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).size();
  else if (m_type == VariantTypeString)
    return std::get<std::string>(m_data).size();
  else if (m_type == VariantTypeWideString)
    return std::get<std::wstring>(m_data).size();
  else
    return 0;
}

bool CVariant::empty() const
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).empty();
  else if (m_type == VariantTypeArray)
    return std::get<VariantArray>(m_data).empty();
  else if (m_type == VariantTypeString)
    return std::get<std::string>(m_data).empty();
  else if (m_type == VariantTypeWideString)
    return std::get<std::wstring>(m_data).empty();
  else if (m_type == VariantTypeNull)
    return true;

  return false;
}

void CVariant::clear()
{
  if (m_type == VariantTypeObject)
    std::get<VariantMap>(m_data).clear();
  else if (m_type == VariantTypeArray)
    std::get<VariantArray>(m_data).clear();
  else if (m_type == VariantTypeString)
    return std::get<std::string>(m_data).clear();
  else if (m_type == VariantTypeWideString)
    std::get<std::wstring>(m_data).clear();
}

void CVariant::erase(const std::string &key)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeObject;
    m_data = VariantMap{};
  }
  else if (m_type == VariantTypeObject)
    std::get<VariantMap>(m_data).erase(key);
}

void CVariant::erase(unsigned int position)
{
  if (m_type == VariantTypeNull)
  {
    m_type = VariantTypeArray;
    m_data = VariantArray{};
  }

  if (m_type == VariantTypeArray && position < size())
    std::get<VariantArray>(m_data).erase(std::get<VariantArray>(m_data).begin() + position);
}

bool CVariant::isMember(const std::string &key) const
{
  if (m_type == VariantTypeObject)
    return std::get<VariantMap>(m_data).find(key) != std::get<VariantMap>(m_data).end();

  return false;
}
