/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/NetworkFileItemClassify.h"

#include "FileItem.h"
#include "utils/URIUtils.h"

namespace KODI::NETWORK
{

bool IsInternetStream(const CFileItem& item, const bool bStrictCheck /* = false */)
{
  if (item.HasProperty("IsHTTPDirectory"))
    return bStrictCheck;

  if (!item.GetDynPath().empty())
    return URIUtils::IsInternetStream(item.GetDynPath(), bStrictCheck);

  return URIUtils::IsInternetStream(item.GetPath(), bStrictCheck);
}

bool IsRemote(const CFileItem& item)
{
  return URIUtils::IsRemote(item.GetPath());
}

bool IsStreamedFilesystem(const CFileItem& item)
{
  if (!item.GetDynPath().empty())
    return URIUtils::IsStreamedFilesystem(item.GetDynPath());

  return URIUtils::IsStreamedFilesystem(item.GetPath());
}

} // namespace KODI::NETWORK
