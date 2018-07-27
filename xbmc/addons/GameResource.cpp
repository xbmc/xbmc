/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameResource.h"

#include <utility>

using namespace ADDON;

CGameResource::CGameResource(CAddonInfo addonInfo) :
  CResource(std::move(addonInfo))
{
}

std::unique_ptr<CGameResource> CGameResource::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  return std::unique_ptr<CGameResource>(new CGameResource(std::move(addonInfo)));
}
