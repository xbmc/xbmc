/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CodecFactory.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "VideoPlayerCodec.h"
#include "addons/AudioDecoder.h"
#include "utils/StringUtils.h"

using namespace ADDON;

ICodec* CodecFactory::CreateCodec(const std::string &strFileType)
{
  std::string fileType = strFileType;
  StringUtils::ToLower(fileType);

  std::vector<AddonInfoPtr> addonInfos;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, true, ADDON_AUDIODECODER);
  for (const auto& addonInfo : addonInfos)
  {
    auto exts = StringUtils::Split(CAudioDecoder::GetExtensions(addonInfo), "|");
    if (std::find(exts.begin(), exts.end(), "." + fileType) != exts.end())
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

  VideoPlayerCodec *dvdcodec = new VideoPlayerCodec();
  return dvdcodec;
}

ICodec* CodecFactory::CreateCodecDemux(const CFileItem& file, unsigned int filecache)
{
  CURL urlFile(file.GetDynPath());
  std::string content = file.GetMimeType();
  StringUtils::ToLower(content);
  if (!content.empty())
  {
    std::vector<AddonInfoPtr> addonInfos;
    CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, true, ADDON_AUDIODECODER);
    for (const auto& addonInfo : addonInfos)
    {
      auto types = StringUtils::Split(CAudioDecoder::GetMimetypes(addonInfo), "|");
      if (std::find(types.begin(), types.end(), content) != types.end())
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
  }

  if( content == "audio/mpeg"       ||
      content == "audio/mpeg3"      ||
      content == "audio/mp3"        ||
      content == "audio/aac"        ||
      content == "audio/aacp"       ||
      content == "audio/x-ms-wma"   ||
      content == "audio/x-ape"      ||
      content == "audio/ape"        ||
      content == "application/ogg"  ||
      content == "audio/ogg"        ||
      content == "audio/x-xbmc-pcm" ||
      content == "audio/flac"       ||
      content == "audio/x-flac"     ||
      content == "application/x-flac"
      )
  {
    VideoPlayerCodec *dvdcodec = new VideoPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else if (urlFile.IsProtocol("shout"))
  {
    VideoPlayerCodec *dvdcodec = new VideoPlayerCodec();
    dvdcodec->SetContentType("audio/mp3");
    return dvdcodec; // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }
  else if (urlFile.IsFileType("wav") ||
      content == "audio/wav" ||
      content == "audio/x-wav")
  {
    VideoPlayerCodec *dvdcodec = new VideoPlayerCodec();
    dvdcodec->SetContentType("audio/x-spdif-compressed");
    if (dvdcodec->Init(file, filecache))
    {
      return dvdcodec;
    }

    dvdcodec = new VideoPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else
    return CreateCodec(urlFile.GetFileType());
}

