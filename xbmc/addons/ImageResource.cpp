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
#include "ImageResource.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "filesystem/XbtManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace ADDON
{

std::unique_ptr<CImageResource> CImageResource::FromExtension(const AddonInfoPtr& addonInfo, const cp_extension_t* ext)
{
  std::string type = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@type");
  return std::unique_ptr<CImageResource>(new CImageResource(addonInfo, std::move(type)));
}

CImageResource::CImageResource(const AddonInfoPtr& addonInfo, std::string type)
    : CResource(addonInfo), m_type(std::move(type))
{
}

void CImageResource::OnPreUnInstall()
{
  CURL xbtUrl;
  if (!HasXbt(xbtUrl))
    return;

  // if there's an XBT we need to remove it from the XBT manager
  XFILE::CXbtManager::GetInstance().Release(xbtUrl);
}

bool CImageResource::IsAllowed(const std::string &file) const
{
  // check if the file path points to a directory
  if (URIUtils::HasSlashAtEnd(file, true))
    return true;

  std::string ext = URIUtils::GetExtension(file);
  return file.empty() ||
         StringUtils::EqualsNoCase(ext, ".png") ||
         StringUtils::EqualsNoCase(ext, ".jpg");
}

std::string CImageResource::GetFullPath(const std::string &filePath) const
{
  // check if there's an XBT file which might contain the file. if not just return the usual full path
  CURL xbtUrl;
  if (!HasXbt(xbtUrl))
    return CResource::GetFullPath(filePath);

  // append the file path to the xbt:// URL
  return URIUtils::AddFileToFolder(xbtUrl.Get(), filePath);
}

bool CImageResource::HasXbt(CURL& xbtUrl) const
{
  std::string resourcePath = GetResourcePath();
  std::string xbtPath = URIUtils::AddFileToFolder(resourcePath, "Textures.xbt");
  if (!XFILE::CFile::Exists(xbtPath))
    return false;

  // translate it into a xbt:// URL
  xbtUrl = URIUtils::CreateArchivePath("xbt", CURL(xbtPath));

  return true;
}

} /* namespace ADDON */
