/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "SFTPDirectory.h"

#include "SFTPFile.h"
#include "utils/log.h"
#include "URL.h"

using namespace XFILE;

CSFTPDirectory::CSFTPDirectory(void) = default;

CSFTPDirectory::~CSFTPDirectory(void) = default;

bool CSFTPDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url);
  return session->GetDirectory(url.GetWithoutFilename().c_str(), url.GetFileName().c_str(), items);
}

bool CSFTPDirectory::Exists(const CURL& url)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url);
  if (session)
    return session->DirectoryExists(url.GetFileName().c_str());
  else
  {
    CLog::Log(LOGERROR, "SFTPDirectory: Failed to create session to check exists");
    return false;
  }
}
