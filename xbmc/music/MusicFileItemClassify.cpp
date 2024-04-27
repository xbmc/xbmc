/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/MusicFileItemClassify.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace KODI::MUSIC
{

bool IsAudio(const CFileItem& item)
{
  /* check preset mime type */
  if (StringUtils::StartsWithNoCase(item.GetMimeType(), "audio/"))
    return true;

  if (item.HasMusicInfoTag())
    return true;

  if (item.HasVideoInfoTag())
    return false;

  if (item.HasPictureInfoTag())
    return false;

  if (item.HasGameInfoTag())
    return false;

  if (IsCDDA(item))
    return true;

  if (StringUtils::StartsWithNoCase(item.GetMimeType(), "application/"))
  { /* check for some standard types */
    std::string extension = item.GetMimeType().substr(12);
    if (StringUtils::EqualsNoCase(extension, "ogg") ||
        StringUtils::EqualsNoCase(extension, "mp4") || StringUtils::EqualsNoCase(extension, "mxf"))
      return true;
  }

  //! @todo If the file is a zip file, ask the game clients if any support this
  // file before assuming it is audio

  return URIUtils::HasExtension(item.GetPath(),
                                CServiceBroker::GetFileExtensionProvider().GetMusicExtensions());
}

bool IsAudioBook(const CFileItem& item)
{
  return item.IsType(".m4b") || item.IsType(".mka");
}

bool IsCDDA(const CFileItem& item)
{
  return URIUtils::IsCDDA(item.GetPath());
}

bool IsCUESheet(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(), ".cue");
}

bool IsLyrics(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(), ".cdg|.lrc");
}

bool IsMusicDb(const CFileItem& item)
{
  return URIUtils::IsMusicDb(item.GetPath());
}

} // namespace KODI::MUSIC
