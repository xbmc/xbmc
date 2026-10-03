/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscsUtils.h"

#include "FileItem.h"
#include "URIUtils.h"
#include "URL.h"

//! @todo it's wrong to include videoplayer scoped files, refactor
// dvd inputstream so they can be used by other components. Or just use libdvdnav directly.
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamNavigator.h"
#include "filesystem/File.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

UTILS::DISCS::DiscInfo UTILS::DISCS::ProbeDVDDiscInfo(const std::string& mediaPath)
{
  DiscInfo info;
  CFileItem item{mediaPath, false};
  CDVDInputStreamNavigator dvdNavigator{nullptr, item};
  if (dvdNavigator.Open())
  {
    info.type = DiscType::DVD;
    info.name = dvdNavigator.GetDVDTitleString();
    // fallback to DVD volume id
    if (info.name.empty())
    {
      info.name = dvdNavigator.GetDVDVolIdString();
    }
    info.serial = dvdNavigator.GetDVDSerialString();
  }
  return info;
}

bool UTILS::DISCS::IsBlurayDiscImage(const CFileItem& item)
{
  return IsBlurayDiscImage(item.GetDynPath());
}

bool UTILS::DISCS::IsBlurayDiscImage(const std::string& path)
{
  if (!URIUtils::IsDiscImage(path))
    return false;

  static constexpr std::array<std::string_view, 4> blurayFiles = {
      "index.bdmv",
      "INDEX.BDM",
      "BDMV/index.bdmv",
      "BDMV/INDEX.BDM",
  };
  CURL url("udf://");
  url.SetHostName(path);

  return std::ranges::any_of(blurayFiles,
                             [&url](std::string_view file)
                             {
                               url.SetFileName(std::string(file));
                               return XFILE::CFile::Exists(url.Get());
                             });
}
