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
#include "MP3codec.h"
#include "OGGcodec.h"
#include "FLACcodec.h"
#include "WAVcodec.h"
#include "ModplugCodec.h"
#include "NSFCodec.h"
#ifdef HAS_SPC_CODEC
#include "SPCCodec.h"
#endif
#include "SIDCodec.h"
#include "VGMCodec.h"
#include "YMCodec.h"
#include "ADPCMCodec.h"
#include "TimidityCodec.h"
#ifdef HAS_ASAP_CODEC
#include "ASAPCodec.h"
#endif
#include "URL.h"
#include "DVDPlayerCodec.h"
#include "PCMCodec.h"
#include "utils/StringUtils.h"

using namespace std;

ICodec* CodecFactory::CreateCodec(const CStdString& strFileType)
{
  if (StringUtils::EqualsNoCase(strFileType, "mp3") || StringUtils::EqualsNoCase(strFileType, "mp2"))
    return new MP3Codec();
  else if (StringUtils::EqualsNoCase(strFileType, "pcm") || StringUtils::EqualsNoCase(strFileType, "l16"))
    return new PCMCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "ape") || StringUtils::EqualsNoCase(strFileType, "mac"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "cdda"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "mpc") || StringUtils::EqualsNoCase(strFileType, "mp+") || StringUtils::EqualsNoCase(strFileType, "mpp"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "shn"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "mka"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "flac"))
    return new FLACCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "wav"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "dts") || StringUtils::EqualsNoCase(strFileType, "ac3") ||
           StringUtils::EqualsNoCase(strFileType, "m4a") || StringUtils::EqualsNoCase(strFileType, "aac") ||
           StringUtils::EqualsNoCase(strFileType, "pvr"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "wv"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "669")  ||  StringUtils::EqualsNoCase(strFileType, "abc") ||
           StringUtils::EqualsNoCase(strFileType, "amf")  ||  StringUtils::EqualsNoCase(strFileType, "ams") ||
           StringUtils::EqualsNoCase(strFileType, "dbm")  ||  StringUtils::EqualsNoCase(strFileType, "dmf") ||
           StringUtils::EqualsNoCase(strFileType, "dsm")  ||  StringUtils::EqualsNoCase(strFileType, "far") ||
           StringUtils::EqualsNoCase(strFileType, "it")   ||  StringUtils::EqualsNoCase(strFileType, "j2b") ||
           StringUtils::EqualsNoCase(strFileType, "mdl")  ||  StringUtils::EqualsNoCase(strFileType, "med") ||
           StringUtils::EqualsNoCase(strFileType, "mod")  ||  StringUtils::EqualsNoCase(strFileType, "itgz")||
           StringUtils::EqualsNoCase(strFileType, "mt2")  ||  StringUtils::EqualsNoCase(strFileType, "mtm") ||
           StringUtils::EqualsNoCase(strFileType, "okt")  ||  StringUtils::EqualsNoCase(strFileType, "pat") ||
           StringUtils::EqualsNoCase(strFileType, "psm")  ||  StringUtils::EqualsNoCase(strFileType, "ptm") ||
           StringUtils::EqualsNoCase(strFileType, "s3m")  ||  StringUtils::EqualsNoCase(strFileType, "stm") ||
           StringUtils::EqualsNoCase(strFileType, "ult")  ||  StringUtils::EqualsNoCase(strFileType, "umx") ||
           StringUtils::EqualsNoCase(strFileType, "xm")   || StringUtils::EqualsNoCase(strFileType, "mdgz") ||
           StringUtils::EqualsNoCase(strFileType, "s3gz") || StringUtils::EqualsNoCase(strFileType, "xmgz"))
    return new ModplugCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "nsf") || StringUtils::EqualsNoCase(strFileType, "nsfstream"))
    return new NSFCodec();
#ifdef HAS_SPC_CODEC
  else if (StringUtils::EqualsNoCase(strFileType, "spc"))
    return new SPCCodec();
#endif
  else if (StringUtils::EqualsNoCase(strFileType, "sid") || StringUtils::EqualsNoCase(strFileType, "sidstream"))
    return new SIDCodec();
  else if (VGMCodec::IsSupportedFormat(strFileType))
    return new VGMCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "ym"))
    return new YMCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "wma"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "aiff") || StringUtils::EqualsNoCase(strFileType, "aif"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "xwav"))
    return new ADPCMCodec();
  else if (TimidityCodec::IsSupportedFormat(strFileType))
    return new TimidityCodec();
#ifdef HAS_ASAP_CODEC
  else if (ASAPCodec::IsSupportedFormat(strFileType) || StringUtils::EqualsNoCase(strFileType, "asapstream"))
    return new ASAPCodec();
#endif
  else if (StringUtils::EqualsNoCase(strFileType, "tta"))
    return new DVDPlayerCodec();
  else if (StringUtils::EqualsNoCase(strFileType, "tak"))
    return new DVDPlayerCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const CStdString& strFile, const CStdString& strContent, unsigned int filecache)
{
  CURL urlFile(strFile);
  if( StringUtils::EqualsNoCase(strContent, "audio/mpeg")
  ||  StringUtils::EqualsNoCase(strContent, "audio/mpeg3")
  ||  StringUtils::EqualsNoCase(strContent, "audio/mp3") )
    return new MP3Codec();
  else if (StringUtils::StartsWithNoCase(strContent, "audio/l16"))
  {
    PCMCodec * pcm_codec = new PCMCodec();
    pcm_codec->SetMimeParams(strContent);
    return pcm_codec;
  }
  else if (StringUtils::EqualsNoCase(strContent, "audio/aac") ||
           StringUtils::EqualsNoCase(strContent, "audio/aacp") ||
           StringUtils::EqualsNoCase(strContent, "audio/x-ms-wma") ||
           StringUtils::EqualsNoCase(strContent, "audio/x-ape") ||
           StringUtils::EqualsNoCase(strContent, "audio/ape"))
  {
    DVDPlayerCodec *pCodec = new DVDPlayerCodec;
    pCodec->SetContentType(strContent);
    return pCodec;
  }
  else if (StringUtils::EqualsNoCase(strContent, "application/ogg") ||
           StringUtils::EqualsNoCase(strContent, "audio/ogg"))
    return CreateOGGCodec(strFile,filecache);
  else if (StringUtils::EqualsNoCase(strContent, "audio/x-xbmc-pcm"))
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    DVDPlayerCodec *dvdcodec = new DVDPlayerCodec();
    dvdcodec->SetContentType(strContent);
    return dvdcodec;
  }
  else if (StringUtils::EqualsNoCase(strContent, "audio/flac") ||
           StringUtils::EqualsNoCase(strContent, "audio/x-flac") ||
           StringUtils::EqualsNoCase(strContent, "application/x-flac"))
    return new FLACCodec();

  if (urlFile.GetProtocol() == "shout")
  {
    return new MP3Codec(); // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }

  if (StringUtils::EqualsNoCase(urlFile.GetFileType(), "wav") ||
      StringUtils::EqualsNoCase(strContent, "audio/wav") ||
      StringUtils::EqualsNoCase(strContent, "audio/x-wav"))
  {
    ICodec* codec;
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
    delete dvdcodec;
    codec = new ADPCMCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;

    codec = new WAVCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  else if (StringUtils::EqualsNoCase(urlFile.GetFileType(), "ogg") ||
           StringUtils::EqualsNoCase(urlFile.GetFileType(), "oggstream") ||
           StringUtils::EqualsNoCase(urlFile.GetFileType(), "oga"))
    return CreateOGGCodec(strFile,filecache);

  //default
  return CreateCodec(urlFile.GetFileType());
}

ICodec* CodecFactory::CreateOGGCodec(const CStdString& strFile,
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

