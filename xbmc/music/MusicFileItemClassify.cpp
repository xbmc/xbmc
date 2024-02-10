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

bool IsAudioBook(const CFileItem& item)
{
  return item.IsType(".m4b") || item.IsType(".mka");
}

bool IsCUESheet(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(), ".cue");
}

bool IsLyrics(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(), ".cdg|.lrc");
}

} // namespace KODI::MUSIC
