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
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>

namespace
{
//! \brief Folder names holding bonus content rather than a video of their own
constexpr const char* VIDEO_EXTRAS_FOLDER_REGEXP{
    R"(^(extras|bonus[ ._-]*(dis[ck]|content|feature)s?)$)"};
} // namespace

namespace KODI::VIDEO
{

bool IsBDFile(const CFileItem& item)
{
  return URIUtils::IsBDFile(item.GetDynPath());
}

bool IsDiscStub(const CFileItem& item)
{
  if (IsVideoDb(item) && item.HasVideoInfoTag())
  {
    CFileItem dbItem(item.IsFolder() ? item.GetVideoInfoTag()->m_strPath
                                     : item.GetVideoInfoTag()->m_strFileNameAndPath,
                     item.IsFolder());
    return IsDiscStub(dbItem);
  }

  return item.GetURL().HasExtension(
      CServiceBroker::GetFileExtensionProvider().GetDiscStubExtensions());
}

bool IsDVDFile(const CFileItem& item, bool bVobs /*= true*/, bool bIfos /*= true*/)
{
  if (URIUtils::IsContainerPath(item.GetDynPath()))
    return false;

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

bool IsSubtitle(const CFileItem& item)
{
  return item.GetURL().HasExtension(
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
  if (!item.IsFolder() && URIUtils::IsPVRTVRecordingFileOrFolder(item.GetPath()))
    return true;

  // ... all other PVR items are not.
  if (item.IsPVR())
    return false;

  if (URIUtils::IsDVD(item.GetPath()))
    return true;

  // IsDVD() above asks whether the path is on optical media, not whether it names a disc.
  if (URIUtils::IsBlurayPath(item.GetPath()) || URIUtils::IsProtocol(item.GetPath(), "dvd"))
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

  return item.GetURL().HasExtension(
      CServiceBroker::GetFileExtensionProvider().GetVideoExtensions());
}

bool IsVideoAssetFile(const CFileItem& item)
{
  if (item.IsFolder() || !IsVideoDb(item))
    return false;

  // @todo better encoding of video assets as path, they won't always be tied with movies.
  // Info can also be retrieved with CVideoDbUrl::FromString but less efficient
  const CURL url{item.GetPath()};
  return (url.HasOption("videoversionid") || url.HasOption("assetType"));
}

bool IsVideoDb(const CFileItem& item)
{
  return URIUtils::IsVideoDb(item.GetPath());
}

bool IsVideoExtrasFolderName(std::string_view name)
{
  thread_local REGEXP::RegExpCache cache;

  const std::shared_ptr<CRegExp> regexp{
      REGEXP::GetRegExp(VIDEO_EXTRAS_FOLDER_REGEXP, &cache, true, CRegExp::autoUtf8)};
  return regexp && regexp->RegFind(std::string{name}) >= 0;
}

bool IsVideoExtrasFolder(const CFileItem& item)
{
  return item.IsFolder() && IsVideoExtrasFolderName(URIUtils::GetFileOrFolderName(item.GetPath()));
}

} // namespace KODI::VIDEO
