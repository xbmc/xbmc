#include "../../stdafx.h"
#include "../../XBAudioConfig.h"
#include "CodecFactory.h"
#include "MP3Codec.h"
#include "APECodec.h"
#include "CDDACodec.h"
#include "OGGCodec.h"
#include "MPCCodec.h"
#include "SHNCodec.h"
#include "FLACCodec.h"
#include "WAVCodec.h"
#include "AACCodec.h"
#include "WAVPackCodec.h"
#include "ModuleCodec.h"
#include "NSFCodec.h"
#include "DTSCodec.h"
#include "DTSCDDACodec.h"
#include "AC3Codec.h"
#include "AC3CDDACodec.h"
#include "SPCCodec.h"
#include "GYMCodec.h"
#include "SIDCodec.h"
#include "AdplugCodec.h"
#include "CubeCodec.h"

ICodec* CodecFactory::CreateCodec(const CStdString& strFileType)
{
  if (strFileType.Equals("mp3") || strFileType.Equals("mp2"))
    return new MP3Codec();
  else if (strFileType.Equals("ape") || strFileType.Equals("mac"))
    return new APECodec();
  else if (strFileType.Equals("cdda"))
    return new CDDACodec();
  else if (strFileType.Equals("ogg") || strFileType.Equals("oggstream"))
    return new OGGCodec();
  else if (strFileType.Equals("mpc") || strFileType.Equals("mp+") || strFileType.Equals("mpp"))
    return new MPCCodec();
  else if (strFileType.Equals("shn"))
    return new SHNCodec();
  else if (strFileType.Equals("flac"))
    return new FLACCodec();
  else if (strFileType.Equals("wav"))
    return new WAVCodec();
  else if (strFileType.Equals("dts"))
    return new DTSCodec();
  else if (strFileType.Equals("ac3"))
    return new AC3Codec();
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
  else if (CubeCodec::IsSupportedFormat(strFileType))
    return new CubeCodec();

  return NULL;
}

ICodec* CodecFactory::CreateCodecDemux(const CStdString& strFile, unsigned int filecache)
{
  CURL urlFile(strFile);
  if (urlFile.GetProtocol() == "lastfm")
  {
    return new MP3Codec();
  }
  if (urlFile.GetFileType().Equals("wav"))
  {
    //lets see what it contains...
    //this kinda sucks 'cause if it's a plain wav file the file
    //will be opened, sniffed and closed 2 times before it is opened *again* for wav
    //would be better if the papcodecs could work with bitstreams instead of filenames.
    ICodec* codec = new DTSCodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
    codec = new AC3Codec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  if (urlFile.GetFileType().Equals("cdda"))
  {
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
    codec = new AC3CDDACodec();
    if (codec->Init(strFile, filecache))
    {
      return codec;
    }
    delete codec;
  }
  //default
  return CreateCodec(urlFile.GetFileType());
}