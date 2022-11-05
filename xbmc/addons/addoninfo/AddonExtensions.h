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
#include <vector>

namespace ADDON
{

class CAddonInfoBuilder;
class CAddonDatabaseSerializer;

struct SExtValue
{
  explicit SExtValue(const std::string& strValue) : str(strValue) { }
  const std::string& asString() const { return str; }
  bool asBoolean() const;
  int asInteger() const { return std::atoi(str.c_str()); }
  float asFloat() const { return static_cast<float>(std::atof(str.c_str())); }
  bool empty() const { return str.empty(); }
  const std::string str;
};

class CExtValues;
class CAddonExtensions;
typedef std::vector<std::pair<std::string, CAddonExtensions>> EXT_ELEMENTS;
typedef std::vector<std::pair<std::string, SExtValue>> EXT_VALUE;
typedef std::vector<std::pair<std::string, CExtValues>> EXT_VALUES;

class CExtValues : public EXT_VALUE
{
public:
  CExtValues(const EXT_VALUE& values) : EXT_VALUE(values) { }

  const SExtValue GetValue(const std::string& id) const
  {
    for (const auto& value : *this)
    {
      if (value.first == id)
        return value.second;
    }
    return SExtValue("");
  }
};

class CAddonExtensions
{
public:
  CAddonExtensions() = default;
  ~CAddonExtensions() = default;

  const SExtValue GetValue(const std::string& id) const;
  const EXT_VALUES& GetValues() const;
  const CAddonExtensions* GetElement(const std::string& id) const;
  const EXT_ELEMENTS GetElements(const std::string& id = "") const;

  void Insert(const std::string& id, const std::string& value);

private:
  friend class CAddonInfoBuilder;
  friend class CAddonDatabaseSerializer;

  std::string m_point;
  EXT_VALUES m_values;
  EXT_ELEMENTS m_children;
};

} /* namespace ADDON */
