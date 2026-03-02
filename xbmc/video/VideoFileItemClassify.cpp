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
  return URIUtils::IsBDFile(item.GetDynPath());
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

bool IsVideoDb(const CFileItem& item)
{
  return URIUtils::IsVideoDb(item.GetPath());
}

} // namespace KODI::VIDEO
