/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SetInfoTagLoaderFactory.h"

#include "FileItem.h"
#include "SetTagLoaderNFO.h"

using namespace VIDEO;

std::unique_ptr<ISetInfoTagLoader> CSetInfoTagLoaderFactory::CreateLoader(const CFileItem& item)
{
  auto nfo{std::make_unique<CSetTagLoaderNFO>(item)};
  if (nfo->HasInfo())
    return nfo;

  return nullptr;
}
