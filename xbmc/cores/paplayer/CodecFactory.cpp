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
#include "addons/ExtsMimeSupportList.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/StringUtils.h"

using namespace KODI::ADDONS;

ICodec* CodecFactory::CreateCodec(const CURL& urlFile)
{
  std::string fileType = urlFile.GetFileType();
  StringUtils::ToLower(fileType);

  auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetExtensionSupportedAddonInfos(
      "." + fileType, CExtsMimeSupportList::FilterSelect::all);
  for (const auto& addonInfo : addonInfos)
  {
    // Check asked and given extension is supported by only for here allowed audiodecoder addons.
    if (addonInfo.first == ADDON::AddonType::AUDIODECODER)
    {
      std::unique_ptr<CAudioDecoder> result = std::make_unique<CAudioDecoder>(addonInfo.second);
      if (!result->CreateDecoder())
        continue;

      return result.release();
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
    auto addonInfos = CServiceBroker::GetExtsMimeSupportList().GetMimetypeSupportedAddonInfos(
        content, CExtsMimeSupportList::FilterSelect::all);
    for (const auto& addonInfo : addonInfos)
    {
      // Check asked and given mime type is supported by only for here allowed audiodecoder addons.
      if (addonInfo.first == ADDON::AddonType::AUDIODECODER)
      {
        std::unique_ptr<CAudioDecoder> result = std::make_unique<CAudioDecoder>(addonInfo.second);
        if (!result->CreateDecoder() && result->SupportsFile(file.GetPath()))
          continue;

        return result.release();
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
    return CreateCodec(urlFile);
}

