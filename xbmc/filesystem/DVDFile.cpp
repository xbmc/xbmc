/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFile.h"

#include "URL.h"

#include <assert.h>

namespace XFILE
{

CDVDFile::CDVDFile(void) : COverrideFile(false)
{
}

CDVDFile::~CDVDFile(void) = default;

std::string CDVDFile::TranslatePath(const CURL& url)
{
  assert(url.IsProtocol("dvd"));

  std::string host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  if (host.empty() || filename.empty())
    return "";

  return host.append(filename);
}
} /* namespace XFILE */
