/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "imagefactory.h"

#include "ServiceBroker.h"
#include "addons/ImageDecoder.h"
#include "guilib/FFmpegImage.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"

#include <algorithm>

CCriticalSection ImageFactory::m_createSec;

using namespace ADDON;

IImage* ImageFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IImage* ImageFactory::CreateLoader(const CURL& url)
{
  if(!url.GetFileType().empty())
    return CreateLoaderFromMimeType("image/"+url.GetFileType());

  return CreateLoaderFromMimeType(CMime::GetMimeType(url));
}

IImage* ImageFactory::CreateLoaderFromMimeType(const std::string& strMimeType)
{
  std::vector<AddonInfoPtr> addonInfos;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, true, ADDON_IMAGEDECODER);
  for (const auto& addonInfo : addonInfos)
  {
    std::vector<std::string> mime = StringUtils::Split(addonInfo->Type(ADDON_IMAGEDECODER)->GetValue("@mimetype").asString(), "|");
    if (std::find(mime.begin(), mime.end(), strMimeType) != mime.end())
    {
      CSingleLock lock(m_createSec);
      CImageDecoder* result = new CImageDecoder(addonInfo);
      result->Create(strMimeType);
      return result;
    }
  }

  return new CFFmpegImage(strMimeType);
}
