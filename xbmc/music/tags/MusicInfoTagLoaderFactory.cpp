/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderFactory.h"
#include "TagLoaderTagLib.h"
#include "MusicInfoTagLoaderCDDA.h"
#include "MusicInfoTagLoaderShn.h"
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicInfoTagLoaderFFmpeg.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "ServiceBroker.h"

#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/AudioDecoder.h"

using namespace ADDON;

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory() = default;

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory() = default;

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CFileItem& item)
{
  // dont try to read the tags for streams & shoutcast
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  std::string strExtension = URIUtils::GetExtension(item.GetPath());
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
    return NULL;

  BinaryAddonBaseList addonInfos;
  CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, true, ADDON_AUDIODECODER);
  for (const auto& addonInfo : addonInfos)
  {
    if (CAudioDecoder::HasTags(addonInfo) &&
        CAudioDecoder::GetExtensions(addonInfo).find("."+strExtension) != std::string::npos)
    {
      CAudioDecoder* result = new CAudioDecoder(addonInfo);
      if (!result->CreateDecoder())
      {
        delete result;
        return nullptr;
      }
      return result;
    }
  }

  if (strExtension == "aac" ||
      strExtension == "ape" || strExtension == "mac" ||
      strExtension == "mp3" ||
      strExtension == "wma" ||
      strExtension == "flac" ||
      strExtension == "m4a" || strExtension == "mp4" || strExtension == "m4b" ||
      strExtension == "m4v" ||
      strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+" ||
      strExtension == "ogg" || strExtension == "oga" || strExtension == "oggstream" ||
      strExtension == "opus" ||
      strExtension == "aif" || strExtension == "aiff" ||
      strExtension == "wav" ||
      strExtension == "mod" ||
      strExtension == "s3m" || strExtension == "it" || strExtension == "xm" ||
      strExtension == "wv")
  {
    CTagLoaderTagLib *pTagLoader = new CTagLoaderTagLib();
    return pTagLoader;
  }
#ifdef HAS_DVD_DRIVE
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
