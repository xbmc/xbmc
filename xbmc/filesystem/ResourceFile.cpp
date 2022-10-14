/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResourceFile.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/Resource.h"

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
  const std::string& addonId = url.GetShareName();
  std::string filePath;
  if (url.GetFileName().length() > addonId.length())
    filePath = url.GetFileName().substr(addonId.size() + 1);

  if (addonId.empty())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, OnlyEnabled::CHOICE_YES) ||
      addon == NULL)
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
