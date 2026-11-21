/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace KODI::SMART_HOME
{
enum class SmartHomePropertyType
{
  BOOL,
  INT8,
  INT16,
  INT32,
  INT64,
  UINT8,
  UINT16,
  UINT32,
  UINT64,
  FLOAT32,
  FLOAT64,
  STRING,
};

using SmartHomePropertyValue = std::variant<bool,
                                            int8_t,
                                            int16_t,
                                            int32_t,
                                            int64_t,
                                            uint8_t,
                                            uint16_t,
                                            uint32_t,
                                            uint64_t,
                                            float,
                                            double,
                                            std::string>;

std::optional<SmartHomePropertyType> SmartHomePropertyTypeFromString(std::string_view type);
std::string_view SmartHomePropertyTypeToString(SmartHomePropertyType type);
bool FormatSmartHomeProperty(const SmartHomePropertyValue& value,
                             const std::string& format,
                             std::string& result);
bool IsValidSmartHomePropertyFormat(SmartHomePropertyType type, const std::string& format);
bool FormatSmartHomeNumber(double value, const std::string& format, std::string& result);
bool IsValidSmartHomeNumberFormat(const std::string& format);
} // namespace KODI::SMART_HOME
