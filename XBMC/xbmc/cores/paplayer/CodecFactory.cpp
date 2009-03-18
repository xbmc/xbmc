/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "XBAudioConfig.h"
#include "CodecFactory.h"
#include "MP3codec.h"
#include "APEcodec.h"
#include "CDDAcodec.h"
#include "OGGcodec.h"
#include "MPCcodec.h"
#include "SHNcodec.h"
#include "FLACcodec.h"
#include "WAVcodec.h"
#include "AACcodec.h"
#include "WAVPackcodec.h"
#include "ModuleCodec.h"
#include "NSFCodec.h"
#include "AC3Codec.h"
#include "AC3CDDACodec.h"
#include "SPCCodec.h"
#include "GYMCodec.h"
#include "SIDCodec.h"
#include "AdplugCodec.h"
#include "VGMCodec.h"
#include "YMCodec.h"
#include "WMACodec.h"
#include "AIFFcodec.h"
#include "ADPCMCodec.h"
#include "TimidityCodec.h"
#include "ASAPCodec.h"
#include "URL.h"
#include "DVDPlayerCodec.h"
#ifdef HAS_DTS_CODEC
#include "DTSCodec.h"
#include "DTSCDDACodec.h"
#endif

ICodec* CodecFactory::CreateCodec(const CStdString& strFileType)
{
  if (strFileType.Equals("mp3") || strFileType.Equals("mp2"))
    return new MP3Codec();
  else if (strFileType.Equals("ape") || strFileType.Equals("mac"))
    return new APECodec();
  else if (strFileType.Equals("cdda"))
    return new CDDACodec();
  else if (strFileType.Equals("mpc") || strFileType.Equals("mp+") || strFileType.Equals("mpp"))
    return new MPCCodec();
  else if (strFileType.Equals("shn"))
#ifdef _XBOX
    return new SHNCodec();
#else
    return new DVDPlayerCodec();
#endif
  else if (strFileType.Equals("flac"))
    return new FLACCodec();
  else if (strFileType.Equals("wav"))
    return new WAVCodec();
#ifdef HAS_DTS_CODEC
  else if (strFileType.Equals("dts"))
    return new DTSCodec();
#endif
#ifdef HAS_AC3_CODEC
  else if (strFileType.Equals("ac3"))
    return new AC3Codec();
#endif
  else if (strFileType.Equals("m4a") || strFileType.Equals("aac"))
    return new AACCodec();
  else if (strFileType.Equals("wv"))
    return new WAVPackCodec();
  else if (ModuleCodec::IsSupportedFormat(strFileType))
    return new ModuleCodec();
  else if (strFileType.Equals("nsf") || strFileType.Equals("nsfstream"))
    return new NSFCodec();
  else if (strFileType.Equals("spc"))
    return new SPCCodec();
  else if (strFileType.Equals("gym"))
    return new GYMCodec();
  else if (strFileType.Equals("sid") || strFileType.Equals("sidstream"))
    return new SIDCodec();
  else if (AdplugCodec::IsSupportedFormat(strFileType))
    return new AdplugCodec();
  else if (VGMCodec::IsSupportedFormat(strFileType))
    return new VGMCodec();
  else if (strFileType.Equals("ym"))
    return new YMCodec();
  else if (strFileType.Equals("wma"))
    return new DVDPlayerCodec();
  else if (strFileType.Equals("aiff") || strFileType.Equals("aif"))
    return new AIFFCodec();
  else if (strFileType.Equals("xwav"))
    return new ADPCMCodec();
  else if (TimidityCodec::IsSupportedFormat(strFileType))
    return new TimidityCodec();
  else if (ASAPCodec::IsSupportedFormat(strFileType) || strFileType.Equals("asapstream"))
    return new ASAPCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const CStdString& strFile, const CStdString& strContent, unsigned int filecache)
{
  CURL urlFile(strFile);
  if( strContent.Equals("audio/mpeg") )
    return new MP3Codec();
  else if( strContent.Equals("audio/aac")
    || strContent.Equals("audio/aacp") )
  {
    if (urlFile.GetProtocol() == "shout" )
    {
      DVDPlayerCodec *pCodec = new DVDPlayerCodec;
      pCodec->SetContentType(strContent);
      return pCodec;
    }

    return new AACCodec();
  }
  else if( strContent.Equals("audio/x-ms-wma") )
    return new DVDPlayerCodec();

  if (urlFile.GetProtocol() == "lastfm" || urlFile.GetProtocol() == "shout")
  {
    return new MP3Codec(); // if we got this far with internet radio - content-type was wrong. gamble on mp3.
  }

  if (urlFile.GetFileType().Equals("wav"))
  {
    ICodec* codec;
#ifdef HAS_DTS_CODEC
    //lets see what it contains...
    //this kinda sucks 'cause if it's a plain wav file the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for wav
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    codec = new DTSCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
#ifdef HAS_AC3_CODEC
    codec = new AC3Codec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
    codec = new ADPCMCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  if (urlFile.GetFileType().Equals("cdda"))
  {
#ifdef HAS_DTS_CODEC
    //lets see what it contains...
    //this kinda sucks 'cause if it's plain cdda the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for cdda
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    ICodec* codec = new DTSCDDACodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
#ifdef HAS_AC3_CDDA_CODEC
    codec = new AC3CDDACodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
#endif
  }
  else if (urlFile.GetFileType().Equals("ogg") || urlFile.GetFileType().Equals("oggstream") || urlFile.GetFileType().Equals("oga"))
  {
    // oldnemesis: we want to use OGGCodec() for OGG music since unlike DVDCodec it provides better
    //  timings for Karaoke. However OGGCodec() cannot handle ogg-flac and ogg videos, that's why this block.
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
  //default
  return CreateCodec(urlFile.GetFileType());
}
