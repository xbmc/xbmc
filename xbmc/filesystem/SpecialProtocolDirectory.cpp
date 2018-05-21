/*
 *      Copyright (C) 2005-present Team Kodi
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

#include "SpecialProtocolDirectory.h"
#include "SpecialProtocol.h"
#include "Directory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "URL.h"

using namespace XFILE;

CSpecialProtocolDirectory::CSpecialProtocolDirectory(void) = default;

CSpecialProtocolDirectory::~CSpecialProtocolDirectory(void) = default;

bool CSpecialProtocolDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const std::string pathToUrl(url.Get());
  std::string translatedPath = CSpecialProtocol::TranslatePath(url);
  if (CDirectory::GetDirectory(translatedPath, items, m_strFileMask, m_flags | DIR_FLAG_GET_HIDDEN))
  { // replace our paths as necessary
    items.SetURL(url);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (URIUtils::PathHasParent(item->GetPath(), translatedPath))
        item->SetPath(URIUtils::AddFileToFolder(pathToUrl, item->GetPath().substr(translatedPath.size())));
    }
    return true;
  }
  return false;
}

std::string CSpecialProtocolDirectory::TranslatePath(const CURL &url)
{
  return CSpecialProtocol::TranslatePath(url);
}
