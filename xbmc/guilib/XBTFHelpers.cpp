/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "XBTFHelpers.h"

#include "guilib/XBTF.h"
#include "guilib/XBTFReader.h"
#include "URL.h"

namespace KODI
{
namespace GUILIB
{
bool HasTextureFiles (const CURL& path)
{
  CXBTFReader reader;
  if (!reader.Open(path))
    return false;

  return reader.HasFiles();
}

bool GetTextureFiles(const CURL& path, std::vector<CXBTFFile>& files)
{
  CXBTFReader reader;
  if (!reader.Open(path))
    return false;

  files = reader.GetFiles();

  return !files.empty();
}
}
}
