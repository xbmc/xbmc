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
#include "URL.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

namespace KODI::VIDEO
{

bool IsBDFile(const CFileItem& item)
{
  const std::string strFileName = URIUtils::GetFileName(item.GetDynPath());
  return (StringUtils::EqualsNoCase(strFileName, "index.bdmv") ||
          StringUtils::EqualsNoCase(strFileName, "MovieObject.bdmv") ||
          StringUtils::EqualsNoCase(strFileName, "INDEX.BDM") ||
          StringUtils::EqualsNoCase(strFileName, "MOVIEOBJ.BDM"));
}

bool IsDiscStub(const CFileItem& item)
{
  if (IsVideoDb(item) && item.HasVideoInfoTag())
  {
    CFileItem dbItem(item.m_bIsFolder ? item.GetVideoInfoTag()->m_strPath
                                      : item.GetVideoInfoTag()->m_strFileNameAndPath,
                     item.m_bIsFolder);
    return IsDiscStub(dbItem);
  }

  return URIUtils::HasExtension(item.GetPath(),
                                CServiceBroker::GetFileExtensionProvider().GetDiscStubExtensions());
}

bool IsDVDFile(const CFileItem& item, bool bVobs /*= true*/, bool bIfos /*= true*/)
{
  const std::string strFileName = URIUtils::GetFileName(item.GetDynPath());
  if (bIfos)
  {
    if (StringUtils::EqualsNoCase(strFileName, "video_ts.ifo"))
      return true;
    if (StringUtils::StartsWithNoCase(strFileName, "vts_") &&
        StringUtils::EndsWithNoCase(strFileName, "_0.ifo") && strFileName.length() == 12)
      return true;
  }
  if (bVobs)
  {
    if (StringUtils::EqualsNoCase(strFileName, "video_ts.vob"))
      return true;
    if (StringUtils::StartsWithNoCase(strFileName, "vts_") &&
        StringUtils::EndsWithNoCase(strFileName, ".vob"))
      return true;
  }

  return false;
}

bool IsProtectedBlurayDisc(const CFileItem& item)
{
  const std::string path = URIUtils::AddFileToFolder(item.GetPath(), "AACS", "Unit_Key_RO.inf");
  return CFileUtils::Exists(path);
}

bool IsBlurayPlaylist(const CFileItem& item)
{
  return StringUtils::EqualsNoCase(URIUtils::GetExtension(item.GetDynPath()), ".mpls");
}

bool IsSubtitle(const CFileItem& item)
{
  return URIUtils::HasExtension(item.GetPath(),
                                CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions());
}

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

bool IsVideoAssetFile(const CFileItem& item)
{
  if (item.m_bIsFolder || !IsVideoDb(item))
    return false;

  // @todo maybe in the future look for prefix videodb://movies/videoversions in path instead
  // @todo better encoding of video assets as path, they won't always be tied with movies.
  return CURL(item.GetPath()).HasOption("videoversionid");
}

bool IsVideoDb(const CFileItem& item)
{
  return URIUtils::IsVideoDb(item.GetPath());
}

bool IsVideoExtrasFolder(const CFileItem& item)
{
  return item.m_bIsFolder &&
         StringUtils::EqualsNoCase(URIUtils::GetFileOrFolderName(item.GetPath()), "extras");
}

} // namespace KODI::VIDEO
