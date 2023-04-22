/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#define _LIBCPP_DISABLE_AVAILABILITY

#include "Variant.h"

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <variant>

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

// helper type for the visitor
template<class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

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
  switch (type)
  {
    case VariantTypeNull:
      m_data = Null{};
      break;
    case VariantTypeConstNull:
      m_data = ConstNull{};
      break;
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
  }
}

CVariant::CVariant(int integer) : m_data(static_cast<int64_t>(integer))
{
}

CVariant::CVariant(int64_t integer) : m_data(integer)
{
}

CVariant::CVariant(unsigned int unsignedinteger) : m_data(static_cast<uint64_t>(unsignedinteger))
{
}

CVariant::CVariant(uint64_t unsignedinteger) : m_data(unsignedinteger)
{
}

CVariant::CVariant(double value) : m_data(value)
{
}

CVariant::CVariant(float value) : m_data(static_cast<double>(value))
{
}

CVariant::CVariant(bool boolean) : m_data(boolean)
{
}

CVariant::CVariant(const char* str) : m_data(std::in_place_type<std::string>, str)
{
}

CVariant::CVariant(const char* str, unsigned int length)
  : m_data(std::in_place_type<std::string>, str, length)
{
}

CVariant::CVariant(const std::string& str) : m_data(std::in_place_type<std::string>, str)
{
}

CVariant::CVariant(std::string&& str) : m_data(std::in_place_type<std::string>, std::move(str))
{
}

CVariant::CVariant(const wchar_t* str) : m_data(std::in_place_type<std::wstring>, str)
{
}

CVariant::CVariant(const wchar_t* str, unsigned int length)
  : m_data(std::in_place_type<std::wstring>, str, length)
{
}

CVariant::CVariant(const std::wstring& str) : m_data(std::in_place_type<std::wstring>, str)
{
}

CVariant::CVariant(std::wstring&& str) : m_data(std::in_place_type<std::wstring>, std::move(str))
{
}

CVariant::CVariant(const std::vector<std::string> &strArray)
{
  VariantArray tmpArray;
  tmpArray.reserve(strArray.size());
  for (const auto& item : strArray)
    tmpArray.emplace_back(item);

  m_data = std::move(tmpArray);
}

CVariant::CVariant(std::vector<std::string>&& strArray)
{
  VariantArray tmpArray;
  tmpArray.reserve(strArray.size());
  for (auto& item : strArray)
    tmpArray.emplace_back(std::move(item));

  m_data = std::move(tmpArray);
}

CVariant::CVariant(const std::map<std::string, std::string> &strMap)
{
  VariantMap tmpMap;
  for (const auto& elem : strMap)
    tmpMap.emplace(elem.first, CVariant(elem.second));

  m_data = std::move(tmpMap);
}

CVariant::CVariant(std::map<std::string, std::string>&& strMap)
{
  VariantMap tmpMap;
  for (auto& elem : strMap)
    tmpMap.emplace(elem.first, CVariant(std::move(elem.second)));

  m_data = std::move(tmpMap);
}

CVariant::CVariant(const std::map<std::string, CVariant>& variantMap)
  : m_data(std::in_place_type<VariantMap>, variantMap)
{
}

CVariant::CVariant(std::map<std::string, CVariant>&& variantMap)
  : m_data(std::in_place_type<VariantMap>, std::move(variantMap))
{
}

CVariant::CVariant(const CVariant& variant) : m_data(variant.m_data)
{
}

CVariant::CVariant(CVariant&& rhs) noexcept : m_data(std::move(rhs.m_data))
{
}

CVariant::~CVariant()
{
  cleanup();
}

void CVariant::cleanup()
{
  m_data = Null{};
}

bool CVariant::isInteger() const
{
  return isSignedInteger() || isUnsignedInteger();
}

bool CVariant::isSignedInteger() const
{
  return std::holds_alternative<int64_t>(m_data);
}

bool CVariant::isUnsignedInteger() const
{
  return std::holds_alternative<uint64_t>(m_data);
}

bool CVariant::isBoolean() const
{
  return std::holds_alternative<bool>(m_data);
}

bool CVariant::isDouble() const
{
  return std::holds_alternative<double>(m_data);
}

bool CVariant::isString() const
{
  return std::holds_alternative<std::string>(m_data);
}

bool CVariant::isWideString() const
{
  return std::holds_alternative<std::wstring>(m_data);
}

bool CVariant::isArray() const
{
  return std::holds_alternative<VariantArray>(m_data);
}

bool CVariant::isObject() const
{
  return std::holds_alternative<VariantMap>(m_data);
}

bool CVariant::isNull() const
{
  return std::holds_alternative<Null>(m_data) || std::holds_alternative<ConstNull>(m_data);
}

CVariant::VariantType CVariant::type() const
{
  return static_cast<VariantType>(m_data.index());
}

int64_t CVariant::asInteger(int64_t fallback) const
{
  return std::visit(overloaded{[](int64_t i) { return i; },
                               [](uint64_t u) { return static_cast<int64_t>(u); },
                               [](double d) { return static_cast<int64_t>(d); },
                               [=](const std::string& s) { return str2int64(s, fallback); },
                               [=](const std::wstring& ws) { return str2int64(ws, fallback); },
                               [=](const auto&) { return fallback; }},
                    m_data);
}

int32_t CVariant::asInteger32(int32_t fallback) const
{
  return static_cast<int32_t>(asInteger(fallback));
}

uint64_t CVariant::asUnsignedInteger(uint64_t fallback) const
{
  return std::visit(overloaded{[](uint64_t u) { return u; },
                               [](int64_t i) { return static_cast<uint64_t>(i); },
                               [](double d) { return static_cast<uint64_t>(d); },
                               [=](const std::string& s) { return str2uint64(s, fallback); },
                               [=](const std::wstring& ws) { return str2uint64(ws, fallback); },
                               [=](const auto&) { return fallback; }},
                    m_data);
}

uint32_t CVariant::asUnsignedInteger32(uint32_t fallback) const
{
  return static_cast<uint32_t>(asUnsignedInteger(fallback));
}

double CVariant::asDouble(double fallback) const
{
  return std::visit(
      overloaded{
          [](double d) { return d; }, [](int64_t i) { return static_cast<double>(i); },
          [](uint64_t u) { return static_cast<double>(u); },
          [=](const std::string& s) { return static_cast<double>(str2uint64(s, fallback)); },
          [=](const std::wstring& ws) { return static_cast<double>(str2uint64(ws, fallback)); },
          [=](const auto&) { return fallback; }},
      m_data);
}

float CVariant::asFloat(float fallback) const
{
  return static_cast<float>(asDouble(static_cast<double>(fallback)));
}

bool CVariant::asBoolean(bool fallback) const
{
  return std::visit(
      overloaded{
          [](bool b) { return b; }, [](int64_t i) { return i != 0; },
          [](uint64_t u) { return u != 0; }, [](double d) { return d != 0; },
          [](const std::string& s) { return !(s.empty() || s == "0" || s == "false"); },
          [](const std::wstring& ws) { return !(ws.empty() || ws == L"0" || ws == L"false"); },
          [=](const auto&) { return fallback; }},
      m_data);
}

std::string CVariant::asString(std::string_view fallback /* = "" */) const&
{
  return std::visit(overloaded{[](const std::string& s) { return s; },
                               [](bool b) { return std::string(b ? "true" : "false"); },
                               [](int64_t i) { return std::to_string(i); },
                               [](uint64_t u) { return std::to_string(u); },
                               [](double d) { return std::to_string(d); },
                               [=](const auto& a) { return std::string(fallback); }},
                    m_data);
}

std::string CVariant::asString(std::string_view fallback /*= ""*/) &&
{
  return std::visit(overloaded{[](std::string& s) { return std::move(s); },
                               [&](auto&) { return asString(fallback); }},
                    m_data);
}

std::wstring CVariant::asWideString(std::wstring_view fallback /* = L"" */) const&
{
  return std::visit(overloaded{[](const std::wstring& ws) { return ws; },
                               [](bool b) { return std::wstring(b ? L"true" : L"false"); },
                               [](int64_t i) { return std::to_wstring(i); },
                               [](uint64_t u) { return std::to_wstring(u); },
                               [](double d) { return std::to_wstring(d); },
                               [=](const auto&) { return std::wstring(fallback); }},
                    m_data);
}

std::wstring CVariant::asWideString(std::wstring_view fallback /*= L""*/) &&
{
  return std::visit(overloaded{[](std::wstring& ws) { return std::move(ws); },
                               [&](auto&) { return asWideString(fallback); }},
                    m_data);
}

CVariant& CVariant::operator[](const std::string& key) &
{
  if (type() == VariantTypeNull)
  {
    m_data = VariantMap{};
  }

  return std::visit(overloaded{[&](VariantMap& m) -> CVariant& { return m[key]; },
                               [](auto&) -> CVariant& { return ConstNullVariant; }},
                    m_data);
}

const CVariant& CVariant::operator[](const std::string& key) const&
{
  return std::visit(overloaded{[&](const VariantMap& m) -> const CVariant& {
                                 auto it = m.find(key);
                                 return it != m.cend() ? it->second : ConstNullVariant;
                               },
                               [](const auto&) -> const CVariant& { return ConstNullVariant; }},
                    m_data);
}

CVariant CVariant::operator[](const std::string& key) &&
{
  return std::visit(overloaded{[&](VariantMap& m) -> CVariant {
                                 auto it = m.find(key);
                                 return it != m.cend() ? std::move(it->second) : ConstNullVariant;
                               },
                               [](auto&) -> CVariant { return ConstNullVariant; }},
                    m_data);
}

CVariant& CVariant::operator[](unsigned int position) &
{
  return std::visit(overloaded{[&](VariantArray& a) -> CVariant& {
                                 return a.size() > position ? a[position] : ConstNullVariant;
                               },
                               [](auto&) -> CVariant& { return ConstNullVariant; }},
                    m_data);
}

const CVariant& CVariant::operator[](unsigned int position) const&
{
  return std::visit(overloaded{[&](const VariantArray& a) -> const CVariant& {
                                 return a.size() > position ? a[position] : ConstNullVariant;
                               },
                               [](const auto&) -> const CVariant& { return ConstNullVariant; }},
                    m_data);
}

CVariant CVariant::operator[](unsigned int position) &&
{
  return std::visit(overloaded{[&](VariantArray& a) -> CVariant {
                                 return a.size() > position ? std::move(a[position])
                                                            : ConstNullVariant;
                               },
                               [](auto&) -> CVariant { return ConstNullVariant; }},
                    m_data);
}

CVariant &CVariant::operator=(const CVariant &rhs)
{
  if (type() == VariantTypeConstNull || this == &rhs)
    return *this;

  cleanup();
  m_data = rhs.m_data;
  return *this;
}

CVariant& CVariant::operator=(CVariant&& rhs) noexcept
{
  if (type() == VariantTypeConstNull || this == &rhs)
    return *this;

  m_data = std::move(rhs.m_data);
  return *this;
}

bool CVariant::operator==(const CVariant &rhs) const
{
  return m_data == rhs.m_data;
}

void CVariant::reserve(size_t length)
{
  if (type() == VariantTypeNull)
  {
    m_data = VariantArray{};
  }
  if (type() == VariantTypeArray)
    std::get<VariantArray>(m_data).reserve(length);
}

void CVariant::push_back(const CVariant &variant)
{
  if (type() == VariantTypeNull)
  {
    m_data = VariantArray{};
  }

  if (type() == VariantTypeArray)
    std::get<VariantArray>(m_data).emplace_back(variant);
}

void CVariant::push_back(CVariant &&variant)
{
  if (type() == VariantTypeNull)
  {
    m_data = VariantArray{};
  }

  if (type() == VariantTypeArray)
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
  return std::visit(overloaded{[](const std::string& s) { return s.c_str(); },
                               [](const auto&) -> const char* { return nullptr; }},
                    m_data);
}

void CVariant::swap(CVariant& rhs) noexcept
{
  std::swap(m_data, rhs.m_data);
}

CVariant::iterator_array CVariant::begin_array()
{
  return std::visit(overloaded{[](VariantArray& a) { return a.begin(); },
                               [](auto&) { return EMPTY_ARRAY.begin(); }},
                    m_data);
}

CVariant::const_iterator_array CVariant::begin_array() const
{
  return std::visit(overloaded{[](const VariantArray& a) { return a.cbegin(); },
                               [](const auto&) { return EMPTY_ARRAY.cbegin(); }},
                    m_data);
}

CVariant::iterator_array CVariant::end_array()
{
  return std::visit(
      overloaded{[](VariantArray& a) { return a.end(); }, [](auto&) { return EMPTY_ARRAY.end(); }},
      m_data);
}

CVariant::const_iterator_array CVariant::end_array() const
{
  return std::visit(overloaded{[](const VariantArray& a) { return a.cend(); },
                               [](const auto&) { return EMPTY_ARRAY.cend(); }},
                    m_data);
}

CVariant::iterator_map CVariant::begin_map()
{
  return std::visit(
      overloaded{[](VariantMap& a) { return a.begin(); }, [](auto&) { return EMPTY_MAP.begin(); }},
      m_data);
}

CVariant::const_iterator_map CVariant::begin_map() const
{
  return std::visit(overloaded{[](const VariantMap& a) { return a.cbegin(); },
                               [](const auto&) { return EMPTY_MAP.cbegin(); }},
                    m_data);
}

CVariant::iterator_map CVariant::end_map()
{
  return std::visit(
      overloaded{[](VariantMap& a) { return a.end(); }, [](auto&) { return EMPTY_MAP.end(); }},
      m_data);
}

CVariant::const_iterator_map CVariant::end_map() const
{
  return std::visit(overloaded{[](const VariantMap& m) { return m.cend(); },
                               [](const auto&) { return EMPTY_MAP.cend(); }},
                    m_data);
}

unsigned int CVariant::size() const
{
  return std::visit(overloaded{[](const VariantMap& m) { return m.size(); },
                               [](const VariantArray& a) { return a.size(); },
                               [](const std::string& s) { return s.size(); },
                               [](const std::wstring& w) { return w.size(); },
                               [](const auto&) { return std::size_t(0); }},
                    m_data);
}

bool CVariant::empty() const
{
  return std::visit(overloaded{[](const VariantMap& m) { return m.empty(); },
                               [](const VariantArray& a) { return a.empty(); },
                               [](const std::string& s) { return s.empty(); },
                               [](const std::wstring& w) { return w.empty(); },
                               [](const Null& n) { return true; },
                               [](const auto&) { return false; }},
                    m_data);
}

void CVariant::clear()
{
  std::visit(overloaded{[](VariantMap& m) { m.clear(); }, [](VariantArray& a) { a.clear(); },
                        [](std::string& s) { s.clear(); }, [](std::wstring& w) { w.clear(); },
                        [](auto&) {}},
             m_data);
}

void CVariant::erase(const std::string &key)
{
  std::visit(overloaded{[&](Null&) { m_data = VariantMap{}; }, [&](VariantMap& m) { m.erase(key); },
                        [](const auto&) {}},
             m_data);
}

void CVariant::erase(unsigned int position)
{
  std::visit(overloaded{[&](Null&) { m_data = VariantArray{}; },
                        [=](VariantArray& a) { a.erase(a.begin() + position); }, [](auto&) {}},
             m_data);
}

bool CVariant::isMember(const std::string &key) const
{
  return std::visit(overloaded{[&](const VariantMap& m) { return m.find(key) != m.end(); },
                               [](const auto&) { return false; }},
                    m_data);
}
