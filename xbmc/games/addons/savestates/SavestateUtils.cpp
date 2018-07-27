/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateUtils.h"
#include "Savestate.h"
#include "utils/URIUtils.h"

#define SAVESTATE_EXTENSION      ".sav"
#define METADATA_EXTENSION       ".xml"

using namespace KODI;
using namespace GAME;

std::string CSavestateUtils::MakePath(const CSavestate& save)
{
  return URIUtils::ReplaceExtension(save.GamePath(), SAVESTATE_EXTENSION);
}

std::string CSavestateUtils::MakeMetadataPath(const std::string &gamePath)
{
  return URIUtils::ReplaceExtension(gamePath, METADATA_EXTENSION);
}
