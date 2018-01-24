/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
