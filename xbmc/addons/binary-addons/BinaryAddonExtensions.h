/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <stdlib.h>
#include <string>
#include <vector>

class TiXmlElement;

namespace ADDON
{

  struct SExtValue
  {
    explicit SExtValue(const std::string& strValue) : str(strValue) { }
    const std::string& asString() const { return str; }
    bool asBoolean() const { return StringUtils::EqualsNoCase(str, "true"); }
    int asInteger() const { return atoi(str.c_str()); }
    float asFloat() const { return static_cast<float>(atof(str.c_str())); }
    bool empty() const { return str.empty(); }
    const std::string str;
  };

  class CBinaryAddonExtensions;
  typedef std::vector<std::pair<std::string, CBinaryAddonExtensions>> EXT_ELEMENTS;
  typedef std::vector<std::pair<std::string, SExtValue>> EXT_VALUE;
  typedef std::vector<std::pair<std::string, EXT_VALUE>> EXT_VALUES;

  class CBinaryAddonExtensions
  {
  public:
    CBinaryAddonExtensions() = default;
    ~CBinaryAddonExtensions() = default;

    bool ParseExtension(const TiXmlElement* element);

    const SExtValue GetValue(const std::string& id) const;
    const EXT_VALUES& GetValues() const;
    const CBinaryAddonExtensions* GetElement(const std::string& id) const;
    const EXT_ELEMENTS GetElements(const std::string& id = "") const;

    void Insert(const std::string& id, const std::string& value);

  private:
    std::string m_point;
    EXT_VALUES m_values;
    EXT_ELEMENTS m_children;
  };

} /* namespace ADDON */
