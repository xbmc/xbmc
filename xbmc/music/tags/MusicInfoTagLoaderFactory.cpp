/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderFactory.h"

#include "FileItem.h"
#include "MusicInfoTagLoaderCDDA.h"
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicInfoTagLoaderFFmpeg.h"
#include "MusicInfoTagLoaderShn.h"
#include "ServiceBroker.h"
#include "TagLoaderTagLib.h"
#include "addons/AudioDecoder.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/addoninfo/AddonType.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace KODI::ADDONS;
using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory() = default;

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory() = default;

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CFileItem& item)
{
  // dont try to read the tags for streams & shoutcast
  if (NETWORK::IsInternetStream(item))
    return NULL;

  if (MUSIC::IsMusicDb(item))
    return new CMusicInfoTagLoaderDatabase();

  std::string strExtension = URIUtils::GetExtension(item.GetPath());
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
    return NULL;

  const auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetExtensionSupportedAddonInfos(
      "." + strExtension, CExtsMimeSupportList::FilterSelect::hasTags);
  for (const auto& addonInfo : addonInfos)
  {
    if (addonInfo.first == ADDON::AddonType::AUDIODECODER)
    {
      std::unique_ptr<CAudioDecoder> result = std::make_unique<CAudioDecoder>(addonInfo.second);
      if (!result->CreateDecoder() && result->SupportsFile(item.GetPath()))
        continue;

      return result.release();
    }
  }

  if (strExtension == "aac" || strExtension == "ape" || strExtension == "mac" ||
      strExtension == "mp3" || strExtension == "wma" || strExtension == "flac" ||
      strExtension == "m4a" || strExtension == "mp4" || strExtension == "m4b" ||
      strExtension == "m4v" || strExtension == "mpc" || strExtension == "mpp" ||
      strExtension == "mp+" || strExtension == "ogg" || strExtension == "oga" ||
      strExtension == "opus" || strExtension == "aif" || strExtension == "aiff" ||
      strExtension == "wav" || strExtension == "mod" || strExtension == "s3m" ||
      strExtension == "it" || strExtension == "xm" || strExtension == "wv")
  {
    CTagLoaderTagLib *pTagLoader = new CTagLoaderTagLib();
    return pTagLoader;
  }
#ifdef HAS_OPTICAL_DRIVE
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return pTagLoader;
  }
#endif
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return pTagLoader;
  }
  else if (strExtension == "mka" || strExtension == "dsf" ||
           strExtension == "dff")
    return new CMusicInfoTagLoaderFFmpeg();

  return NULL;
}
