/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonType.h"

#include "addons/addoninfo/AddonInfo.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace ADDON
{
static const std::set<AddonType> dependencyTypes = {
    AddonType::SCRAPER_LIBRARY,
    AddonType::SCRIPT_LIBRARY,
    AddonType::SCRIPT_MODULE,
};
} /* namespace ADDON */

using namespace ADDON;

std::string CAddonType::LibPath() const
{
  if (m_libname.empty())
    return "";
  return URIUtils::AddFileToFolder(m_path, m_libname);
}

void CAddonType::SetProvides(const std::string& content)
{
  if (!content.empty())
  {
    /*
     * Normally the "provides" becomes added from xml scan, but for add-ons
     * stored in the database (e.g. repository contents) it might not be
     * available. Since this information is available in add-on metadata for the
     * main type (see extrainfo) we take the function contents and insert it if
     * empty.
     */
    if (GetValue("provides").empty())
      Insert("provides", content);

    for (const auto& provide : StringUtils::Split(content, ' '))
    {
      AddonType content = CAddonInfo::TranslateSubContent(provide);
      if (content != AddonType::UNKNOWN)
        m_providedSubContent.insert(content);
    }
  }
}

bool CAddonType::IsDependencyType(AddonType type)
{
  return dependencyTypes.find(type) != dependencyTypes.end();
}
