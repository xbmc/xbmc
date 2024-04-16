/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoFileItemClassify.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace KODI::VIDEO
{

bool IsVideo(const CFileItem& item)
{
  /* check preset mime type */
  if (StringUtils::StartsWithNoCase(item.GetMimeType(), "video/"))
    return true;

  if (item.HasVideoInfoTag())
    return true;

  if (item.HasGameInfoTag())
    return false;

  if (item.HasMusicInfoTag())
    return false;

  if (item.HasPictureInfoTag())
    return false;

  // TV recordings are videos...
  if (!item.m_bIsFolder && URIUtils::IsPVRTVRecordingFileOrFolder(item.GetPath()))
    return true;

  // ... all other PVR items are not.
  if (item.IsPVR())
    return false;

  if (URIUtils::IsDVD(item.GetPath()))
    return true;

  std::string extension;
  if (StringUtils::StartsWithNoCase(item.GetMimeType(), "application/"))
  { /* check for some standard types */
    extension = item.GetMimeType().substr(12);
    if (StringUtils::EqualsNoCase(extension, "ogg") ||
        StringUtils::EqualsNoCase(extension, "mp4") || StringUtils::EqualsNoCase(extension, "mxf"))
      return true;
  }

  //! @todo If the file is a zip file, ask the game clients if any support this
  // file before assuming it is video.

  return URIUtils::HasExtension(item.GetPath(),
                                CServiceBroker::GetFileExtensionProvider().GetVideoExtensions());
}

} // namespace KODI::VIDEO
