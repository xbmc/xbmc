/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "imagefactory.h"

#include "ServiceBroker.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/ImageDecoder.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/FFmpegImage.h"
#include "utils/Mime.h"

#include <mutex>

CCriticalSection ImageFactory::m_createSec;

using namespace KODI::ADDONS;

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
  auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetMimetypeSupportedAddonInfos(
      strMimeType, CExtsMimeSupportList::FilterSelect::all);
  for (const auto& addonInfo : addonInfos)
  {
    // Check asked and given mime type is supported by only for here allowed imagedecoder addons.
    if (addonInfo.first != ADDON::AddonType::IMAGEDECODER)
      continue;

    std::unique_lock<CCriticalSection> lock(m_createSec);
    std::unique_ptr<CImageDecoder> result =
        std::make_unique<CImageDecoder>(addonInfo.second, strMimeType);
    if (!result->IsCreated())
    {
      continue;
    }
    return result.release();
  }

  return new CFFmpegImage(strMimeType);
}
