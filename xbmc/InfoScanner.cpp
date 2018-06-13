/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InfoScanner.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

bool CInfoScanner::HasNoMedia(const std::string &strDirectory) const
{
  std::string noMediaFile = URIUtils::AddFileToFolder(strDirectory, ".nomedia");
  return XFILE::CFile::Exists(noMediaFile);
}

bool CInfoScanner::IsFileExcluded(const std::string& file, const std::vector<std::string>& regexps) const
{
  return CUtil::ExcludeFileOrFolder(file, regexps) || IsDirectoryExcluded(URIUtils::GetDirectory(file), regexps);
}

bool CInfoScanner::IsDirectoryExcluded(const std::string& directory, const std::vector<std::string>& regexps) const
{
  if (CUtil::ExcludeFileOrFolder(directory, regexps))
    return true;

  if (!URIUtils::IsPlugin(directory) && HasNoMedia(directory))
  {
    CLog::Log(LOGWARNING, "Skipping items in '%s' due to '.nomedia' file in directory, they won't be added to the library.", CURL::GetRedacted(directory).c_str());
    return true;
  }
  return false;
}
