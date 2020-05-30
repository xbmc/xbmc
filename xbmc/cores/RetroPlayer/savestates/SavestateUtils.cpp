/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateUtils.h"

#include "utils/URIUtils.h"

#define SAVESTATE_EXTENSION ".sav"

using namespace KODI;
using namespace RETRO;

std::string CSavestateUtils::MakePath(const std::string& gamePath)
{
  return URIUtils::ReplaceExtension(gamePath, SAVESTATE_EXTENSION);
}
