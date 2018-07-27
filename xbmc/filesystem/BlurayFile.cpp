/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayFile.h"
#include "URL.h"
#include <assert.h>

namespace XFILE
{

  CBlurayFile::CBlurayFile(void)
    : COverrideFile(false)
  { }

  CBlurayFile::~CBlurayFile(void) = default;

  std::string CBlurayFile::TranslatePath(const CURL& url)
  {
    assert(url.IsProtocol("bluray"));

    std::string host = url.GetHostName();
    std::string filename = url.GetFileName();
    if (host.empty() || filename.empty())
      return "";

    return host.append(filename);
  }
} /* namespace XFILE */
