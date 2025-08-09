/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace ADDON
{

class CAddonInfoBuilder;
class CAddonDatabaseSerializer;

struct SExtValue
{
  SExtValue() = default;
  explicit SExtValue(const std::string& strValue) : str(strValue) {}

  const std::string& asString() const { return str; }
  bool asBoolean() const;
  int asInteger() const { return std::atoi(str.c_str()); }
  float asFloat() const { return static_cast<float>(std::atof(str.c_str())); }
  bool empty() const { return str.empty(); }

  const std::string str;
};

class CExtValues;
class CAddonExtensions;

using EXT_ELEMENTS = std::vector<std::pair<std::string, CAddonExtensions>>;
using EXT_VALUE = std::vector<std::pair<std::string, SExtValue>>;
using EXT_VALUES = std::vector<std::pair<std::string, CExtValues>>;

class CExtValues : public EXT_VALUE
{
public:
  explicit CExtValues(const EXT_VALUE& values) : EXT_VALUE(values) {}

  SExtValue GetValue(std::string_view id) const
  {
    for (const auto& [valueId, valueValue] : *this)
    {
      if (valueId == id)
        return valueValue;
    }
    return {};
  }
};

class CAddonExtensions
{
public:
  CAddonExtensions() = default;
  ~CAddonExtensions() = default;

  SExtValue GetValue(std::string_view id) const;
  const EXT_VALUES& GetValues() const;
  const CAddonExtensions* GetElement(std::string_view id) const;
  EXT_ELEMENTS GetElements(std::string_view id = "") const;

  void Insert(const std::string& id, const std::string& value);

private:
  friend class CAddonInfoBuilder;
  friend class CAddonDatabaseSerializer;

  std::string m_point;
  EXT_VALUES m_values;
  EXT_ELEMENTS m_children;
};

} /* namespace ADDON */
