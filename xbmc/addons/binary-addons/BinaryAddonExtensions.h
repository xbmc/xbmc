#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StringUtils.h"

#include <stdlib.h>
#include <string>
#include <vector>

class TiXmlElement;

namespace ADDON
{

  struct SExtValue
  {
    SExtValue(const std::string& strValue) : str(strValue) { }
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
