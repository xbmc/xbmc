/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoScanner.h"

#include "URL.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

bool CInfoScanner::HasNoMedia(const std::string &strDirectory) const
{
  std::string noMediaFile = URIUtils::AddFileToFolder(strDirectory, ".nomedia");

  if (!URIUtils::IsPlugin(strDirectory) && CFileUtils::Exists(noMediaFile))
  {
    CLog::Log(LOGWARNING,
              "Skipping item '{}' with '.nomedia' file in parent directory, it won't be added to "
              "the library.",
              CURL::GetRedacted(strDirectory));
    return true;
  }

  return false;
}
