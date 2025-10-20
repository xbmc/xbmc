/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinResource.h"

#include "addons/addoninfo/AddonType.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace ADDON;

CSkinResource::CSkinResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_SKIN)
{
}

bool CSkinResource::IsAllowed(const std::string& file) const
{
  if (URIUtils::HasSlashAtEnd(file, true))
    return true;

  const std::string ext = URIUtils::GetExtension(file);

  return file.empty() || StringUtils::EqualsNoCase(ext, ".xml") ||
         StringUtils::EqualsNoCase(ext, ".png") || StringUtils::EqualsNoCase(ext, ".jpg") ||
         StringUtils::EqualsNoCase(ext, ".jpeg") || StringUtils::EqualsNoCase(ext, ".gif") ||
         StringUtils::EqualsNoCase(ext, ".bmp") || StringUtils::EqualsNoCase(ext, ".dds") ||
         StringUtils::EqualsNoCase(ext, ".tga") || StringUtils::EqualsNoCase(ext, ".ttf") ||
         StringUtils::EqualsNoCase(ext, ".otf") || StringUtils::EqualsNoCase(ext, ".ttc");
}
