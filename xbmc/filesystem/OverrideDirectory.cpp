/*
 *      Copyright (C) 2014 Team XBMC
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

#include "OverrideDirectory.h"
#include "URL.h"
#include "filesystem/Directory.h"

using namespace XFILE;


COverrideDirectory::COverrideDirectory()
{ }


COverrideDirectory::~COverrideDirectory()
{ }

bool COverrideDirectory::Create(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Create(translatedPath.c_str());
}

bool COverrideDirectory::Remove(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Remove(translatedPath.c_str());
}

bool COverrideDirectory::Exists(const CURL& url)
{
  std::string translatedPath = TranslatePath(url);

  return CDirectory::Exists(translatedPath.c_str());
}
