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
#include "OGGcodec.h"
#include "ModplugCodec.h"
#include "NSFCodec.h"
#ifdef HAS_SPC_CODEC
#include "SPCCodec.h"
#endif
#include "SIDCodec.h"
#include "VGMCodec.h"
#include "YMCodec.h"
#include "TimidityCodec.h"
#ifdef HAS_ASAP_CODEC
#include "ASAPCodec.h"
#endif
#include "URL.h"
#include "DVDPlayerCodec.h"
#include "utils/StringUtils.h"

ICodec* CodecFactory::CreateCodec(const std::string& strFileType)
{
  std::string fileType = strFileType;
  StringUtils::ToLower(fileType);
  if (fileType == "mp3" || fileType == "mp2")
    return new DVDPlayerCodec();
  else if (fileType == "pcm" || fileType == "l16")
    return new DVDPlayerCodec();
  else if (fileType == "ape" || fileType == "mac")
    return new DVDPlayerCodec();
  else if (fileType == "cdda")
    return new DVDPlayerCodec();
  else if (fileType == "mpc" || fileType == "mp+" || fileType == "mpp")
    return new DVDPlayerCodec();
  else if (fileType == "shn")
    return new DVDPlayerCodec();
  else if (fileType == "mka")
    return new DVDPlayerCodec();
  else if (fileType == "flac")
    return new DVDPlayerCodec();
  else if (fileType == "wav")
    return new DVDPlayerCodec();
  else if (fileType == "dts" || fileType == "ac3" ||
           fileType == "m4a" || fileType == "aac" ||
           fileType == "pvr")
    return new DVDPlayerCodec();
  else if (fileType == "wv")
    return new DVDPlayerCodec();
  else if (fileType == "669"  ||  fileType == "abc" ||
           fileType == "amf"  ||  fileType == "ams" ||
           fileType == "dbm"  ||  fileType == "dmf" ||
           fileType == "dsm"  ||  fileType == "far" ||
           fileType == "it"   ||  fileType == "j2b" ||
           fileType == "mdl"  ||  fileType == "med" ||
           fileType == "mod"  ||  fileType == "itgz"||
           fileType == "mt2"  ||  fileType == "mtm" ||
           fileType == "okt"  ||  fileType == "pat" ||
           fileType == "psm"  ||  fileType == "ptm" ||
           fileType == "s3m"  ||  fileType == "stm" ||
           fileType == "ult"  ||  fileType == "umx" ||
           fileType == "xm"   || fileType == "mdgz" ||
           fileType == "s3gz" || fileType == "xmgz")
    return new ModplugCodec();
  else if (fileType == "nsf" || fileType == "nsfstream")
    return new NSFCodec();
#ifdef HAS_SPC_CODEC
  else if (fileType == "spc")
    return new SPCCodec();
#endif
  else if (fileType == "sid" || fileType == "sidstream")
    return new SIDCodec();
  else if (VGMCodec::IsSupportedFormat(strFileType))
    return new VGMCodec();
  else if (fileType == "ym")
    return new YMCodec();
  else if (fileType == "wma")
    return new DVDPlayerCodec();
  else if (fileType == "aiff" || fileType == "aif")
    return new DVDPlayerCodec();
  else if (fileType == "xwav")
    return new DVDPlayerCodec();
  else if (TimidityCodec::IsSupportedFormat(strFileType))
    return new TimidityCodec();
#ifdef HAS_ASAP_CODEC
  else if (ASAPCodec::IsSupportedFormat(strFileType) || fileType == "asapstream")
    return new ASAPCodec();
#endif
  else if (fileType == "tta")
    return new DVDPlayerCodec();
  else if (fileType == "tak")
    return new DVDPlayerCodec();
  else if (fileType == "opus")
    return new DVDPlayerCodec();
  else if (fileType == "dff" || fileType == "dsf")
    return new DVDPlayerCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const std::string& strFile, const std::string& strContent, unsigned int filecache)
{
  CURL urlFile(strFile);
  std::string content = strContent;
  StringUtils::ToLower(content);
  if( content == "audio/mpeg"
  ||  content == "audio/mpeg3"
  ||  content == "audio/mp3" )
  {
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else if (StringUtils::StartsWith(content, "audio/l16"))
  {
    DVDPlayerCodec *pCodec = new DVDPlayerCodec;
    pCodec->SetContentType(content);
    return pCodec;
  }
  else if(content == "audio/aac" ||
          content == "audio/aacp" ||
          content == "audio/x-ms-wma" ||
          content == "audio/x-ape" ||
          content == "audio/ape")
  {
    DVDPlayerCodec *pCodec = new DVDPlayerCodec;
    pCodec->SetContentType(content);
    return pCodec;
  }
  else if( content == "application/ogg" || content == "audio/ogg")
    return CreateOGGCodec(strFile,filecache);
  else if (content == "audio/x-xbmc-pcm")
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }
  else if (content == "audio/flac" || content == "audio/x-flac" || content == "application/x-flac")
  {
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(content);
    return dvdcodec;
  }

  if (urlFile.IsProtocol("shout"))
  {
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType("audio/mp3");
    return dvdcodec; // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }

  if (urlFile.IsFileType("wav") ||
      content == "audio/wav" ||
      content == "audio/x-wav")
  {
    //lets see what it contains...
    //this kinda sucks 'cause if it's a plain wav file the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for wav
    //would be better if the papcodecs could work with bitstreams instead of filenames.
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
  else if (urlFile.IsFileType("ogg") || urlFile.IsFileType("oggstream") || urlFile.IsFileType("oga"))
    return CreateOGGCodec(strFile,filecache);

  //default
  return CreateCodec(urlFile.GetFileType());
}

ICodec* CodecFactory::CreateOGGCodec(const std::string& strFile,
                                     unsigned int filecache)
{
  // oldnemesis: we want to use OGGCodec() for OGG music since unlike DVDCodec 
  // it provides better timings for Karaoke. However OGGCodec() cannot handle 
  // ogg-flac and ogg videos, that's why this block.
  ICodec* codec = new OGGCodec();
  try
  {
    if (codec->Init(strFile, filecache))
      return codec;
  }
  catch( ... )
  {
  }
  delete codec;
  return new DVDPlayerCodec();
}

