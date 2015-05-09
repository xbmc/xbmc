/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#include "CodecFactory.h"
#include "URL.h"
#include "DVDPlayerCodec.h"
#include "utils/StringUtils.h"
#include "addons/AddonManager.h"
#include "addons/AudioDecoder.h"

using namespace ADDON;

ICodec* CodecFactory::CreateCodec(const std::string &strFileType)
{
  std::string fileType = strFileType;
  StringUtils::ToLower(fileType);
  VECADDONS codecs;
  CAddonMgr::Get().GetAddons(ADDON_AUDIODECODER, codecs);
  for (size_t i=0;i<codecs.size();++i)
  {
    std::shared_ptr<CAudioDecoder> dec(std::static_pointer_cast<CAudioDecoder>(codecs[i]));
    std::vector<std::string> exts = StringUtils::Split(dec->GetExtensions(), "|");
    if (std::find(exts.begin(), exts.end(), "."+fileType) != exts.end())
    {
      CAudioDecoder* result = new CAudioDecoder(*dec);
      static_cast<AudioDecoderDll&>(*result).Create();
      return result;
    }
  }

  DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
  return dvdcodec;
}

ICodec* CodecFactory::CreateCodecDemux(const std::string& strFile, const std::string& strContent, unsigned int filecache)
{
  CURL urlFile(strFile);
  std::string content = strContent;
  StringUtils::ToLower(content);
  if (!content.empty())
  {
    VECADDONS codecs;
    CAddonMgr::Get().GetAddons(ADDON_AUDIODECODER, codecs);
    for (size_t i=0;i<codecs.size();++i)
    {
      std::shared_ptr<CAudioDecoder> dec(std::static_pointer_cast<CAudioDecoder>(codecs[i]));
      std::vector<std::string> mime = StringUtils::Split(dec->GetMimetypes(), "|");
      if (std::find(mime.begin(), mime.end(), content) != mime.end())
      {
        CAudioDecoder* result = new CAudioDecoder(*dec);
        static_cast<AudioDecoderDll&>(*result).Create();
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
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else if (urlFile.IsProtocol("shout"))
  {
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType("audio/mp3");
    return dvdcodec; // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }
  else if (urlFile.IsFileType("wav") ||
      content == "audio/wav" ||
      content == "audio/x-wav")
  {
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType("audio/x-spdif-compressed");
    if (dvdcodec->Init(strFile, filecache))
    {
      return dvdcodec;
    }

    dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else
    return CreateCodec(urlFile.GetFileType());
}

