/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

namespace XFILE
{
  class CDAVCommon
  {
    public:
      static bool ValueWithoutNamespace(const TiXmlNode *pNode, const std::string& value);
      static std::string GetStatusTag(const TiXmlElement *pElement);
  };
}
