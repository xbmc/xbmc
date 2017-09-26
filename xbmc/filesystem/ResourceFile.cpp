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

#include "ResourceFile.h"
#include "URL.h"
#include "Util.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/Resource.h"
#include "utils/URIUtils.h"

using namespace ADDON;
using namespace XFILE;

CResourceFile::CResourceFile()
  : COverrideFile(false)
{ }

CResourceFile::~CResourceFile() = default;

bool CResourceFile::TranslatePath(const std::string &path, std::string &translatedPath)
{
  return TranslatePath(CURL(path), translatedPath);
}

bool CResourceFile::TranslatePath(const CURL &url, std::string &translatedPath)
{
  translatedPath = url.Get();

  // only handle resource:// paths
  if (!url.IsProtocol("resource"))
    return false;

  // the share name represents an identifier that can be mapped to an addon ID
  std::string addonId = url.GetShareName();
  std::string filePath;
  if (url.GetFileName().length() > addonId.length())
    filePath = url.GetFileName().substr(addonId.size() + 1);

  if (addonId.empty())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, ADDON_UNKNOWN, true) || addon == NULL)
    return false;

  std::shared_ptr<CResource> resource = std::dynamic_pointer_cast<ADDON::CResource>(addon);
  if (resource == NULL)
    return false;

  if (!resource->IsAllowed(filePath))
    return false;

  translatedPath = CUtil::ValidatePath(resource->GetFullPath(filePath));
  return true;
}

std::string CResourceFile::TranslatePath(const CURL &url)
{
  std::string translatedPath;
  if (!TranslatePath(url, translatedPath))
    return "";

  return translatedPath;
}
