/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "ImageResource.h"
#include "URL.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "filesystem/XbtManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

namespace ADDON
{

std::unique_ptr<CImageResource> CImageResource::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  std::string type = CServiceBroker::GetAddonMgr().GetExtValue(ext->configuration, "@type");
  return std::unique_ptr<CImageResource>(new CImageResource(std::move(addonInfo), std::move(type)));
}

CImageResource::CImageResource(CAddonInfo addonInfo, std::string type)
    : CResource(std::move(addonInfo)), m_type(std::move(type))
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
