/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetInfoTagLoaderFactory.h"

#include "SetTagLoaderNFO.h"

using namespace KODI::VIDEO;

std::unique_ptr<ISetInfoTagLoader> CSetInfoTagLoaderFactory::CreateLoader(const std::string& title)
{
  auto nfo{std::make_unique<CSetTagLoaderNFO>(title)};
  if (nfo->HasInfo())
    return nfo;

  return nullptr;
}
