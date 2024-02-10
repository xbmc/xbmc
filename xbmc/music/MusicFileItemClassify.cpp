/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/MusicFileItemClassify.h"

#include "FileItem.h"
#include "utils/URIUtils.h"

namespace KODI::MUSIC
{

bool IsCUESheet(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(), ".cue");
}

} // namespace KODI::MUSIC
