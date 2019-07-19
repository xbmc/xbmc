/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonExtensions.h"
#include "utils/StringUtils.h"

#include <stdlib.h>
#include <string>
#include <vector>

class TiXmlElement;

namespace ADDON
{

  class CBinaryAddonExtensions;
  typedef std::vector<std::pair<std::string, CBinaryAddonExtensions>> EXT_BIN_ELEMENTS;

  class CBinaryAddonExtensions
  {
  public:
    CBinaryAddonExtensions() = default;
    ~CBinaryAddonExtensions() = default;

    bool ParseExtension(const TiXmlElement* element);

    const SExtValue GetValue(const std::string& id) const;
    const EXT_VALUES& GetValues() const;
    const CBinaryAddonExtensions* GetElement(const std::string& id) const;
    const EXT_BIN_ELEMENTS GetElements(const std::string& id = "") const;

    void Insert(const std::string& id, const std::string& value);

  private:
    std::string m_point;
    EXT_VALUES m_values;
    EXT_BIN_ELEMENTS m_children;
  };

} /* namespace ADDON */
