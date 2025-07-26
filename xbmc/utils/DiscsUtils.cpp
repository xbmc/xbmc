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
#ifdef HAVE_LIBBLURAY
//! @todo it's wrong to include vfs scoped files in a utils class, refactor
// to use libbluray directly.
#include "filesystem/BlurayDirectory.h"
#endif

#include "filesystem/File.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

bool UTILS::DISCS::GetDiscInfo(UTILS::DISCS::DiscInfo& info, const std::string& mediaPath)
{
  // try to probe as a DVD
  info = ProbeDVDDiscInfo(mediaPath);
  if (!info.empty())
    return true;

  // try to probe as Blu-ray
  info = ProbeBlurayDiscInfo(mediaPath);
  if (!info.empty())
    return true;

  return false;
}

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

UTILS::DISCS::DiscInfo UTILS::DISCS::ProbeBlurayDiscInfo(const std::string& mediaPath)
{
  DiscInfo info;
#ifdef HAVE_LIBBLURAY
  XFILE::CBlurayDirectory bdDir;
  if (!bdDir.InitializeBluray(mediaPath))
    return info;

  info.type = DiscType::BLURAY;
  info.name = bdDir.GetBlurayTitle();
  info.serial = bdDir.GetBlurayID();
#endif
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
