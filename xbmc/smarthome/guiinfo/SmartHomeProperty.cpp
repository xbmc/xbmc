/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeProperty.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <array>

#include <fmt/format.h>

using namespace KODI::SMART_HOME;

namespace
{
constexpr std::array<std::pair<std::string_view, SmartHomePropertyType>, 12> PROPERTY_TYPES{{
    {"bool", SmartHomePropertyType::BOOL},
    {"int8", SmartHomePropertyType::INT8},
    {"int16", SmartHomePropertyType::INT16},
    {"int32", SmartHomePropertyType::INT32},
    {"int64", SmartHomePropertyType::INT64},
    {"uint8", SmartHomePropertyType::UINT8},
    {"uint16", SmartHomePropertyType::UINT16},
    {"uint32", SmartHomePropertyType::UINT32},
    {"uint64", SmartHomePropertyType::UINT64},
    {"float32", SmartHomePropertyType::FLOAT32},
    {"float64", SmartHomePropertyType::FLOAT64},
    {"string", SmartHomePropertyType::STRING},
}};

SmartHomePropertyValue DefaultValue(SmartHomePropertyType type)
{
  switch (type)
  {
    case SmartHomePropertyType::BOOL:
      return false;
    case SmartHomePropertyType::INT8:
      return int8_t{};
    case SmartHomePropertyType::INT16:
      return int16_t{};
    case SmartHomePropertyType::INT32:
      return int32_t{};
    case SmartHomePropertyType::INT64:
      return int64_t{};
    case SmartHomePropertyType::UINT8:
      return uint8_t{};
    case SmartHomePropertyType::UINT16:
      return uint16_t{};
    case SmartHomePropertyType::UINT32:
      return uint32_t{};
    case SmartHomePropertyType::UINT64:
      return uint64_t{};
    case SmartHomePropertyType::FLOAT32:
      return float{};
    case SmartHomePropertyType::FLOAT64:
      return double{};
    case SmartHomePropertyType::STRING:
      return std::string{};
  }
  return std::string{};
}

template<typename Value>
bool FormatValue(const Value& value,
                 const std::string& format,
                 const char* description,
                 std::string& result)
{
  try
  {
    result = format.empty() ? StringUtils::Format("{}", value) : StringUtils::Format(format, value);
    return true;
  }
  catch (const fmt::format_error& error)
  {
    CLog::Log(LOGERROR, "Smart-home {} format '{}' is invalid: {}", description, format,
              error.what());
    result.clear();
    return false;
  }
}
} // namespace

std::optional<SmartHomePropertyType> KODI::SMART_HOME::SmartHomePropertyTypeFromString(
    std::string_view type)
{
  for (const auto& [name, value] : PROPERTY_TYPES)
  {
    if (name == type)
      return value;
  }
  return std::nullopt;
}

std::string_view KODI::SMART_HOME::SmartHomePropertyTypeToString(SmartHomePropertyType type)
{
  for (const auto& [name, value] : PROPERTY_TYPES)
  {
    if (value == type)
      return name;
  }
  return {};
}

bool KODI::SMART_HOME::FormatSmartHomeProperty(const SmartHomePropertyValue& value,
                                               const std::string& format,
                                               std::string& result)
{
  return std::visit([&format, &result](const auto& typedValue)
                    { return FormatValue(typedValue, format, "property", result); }, value);
}

bool KODI::SMART_HOME::IsValidSmartHomePropertyFormat(SmartHomePropertyType type,
                                                      const std::string& format)
{
  std::string unused;
  return format.empty() || FormatSmartHomeProperty(DefaultValue(type), format, unused);
}

bool KODI::SMART_HOME::FormatSmartHomeNumber(double value,
                                             const std::string& format,
                                             std::string& result)
{
  return FormatValue(value, format, "numeric", result);
}

bool KODI::SMART_HOME::IsValidSmartHomeNumberFormat(const std::string& format)
{
  std::string unused;
  return format.empty() || FormatSmartHomeNumber(0.0, format, unused);
}
