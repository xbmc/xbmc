/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinResource.h"

#include "addons/addoninfo/AddonType.h"

#include <array>
#include <string_view>

using namespace ADDON;

CSkinResource::CSkinResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_SKIN)
{
}

CResource::Published CSkinResource::PublishedFiles() const
{
  static constexpr std::array<std::string_view, 11> extensions{
      ".xml", ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".dds", ".tga", ".ttf", ".otf", ".ttc"};
  return {.extensions = extensions};
}
