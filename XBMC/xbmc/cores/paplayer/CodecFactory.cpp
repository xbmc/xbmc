#include "../../stdafx.h"
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
  else if (strFileType.Equals("m4a") || strFileType.Equals("aac"))
    return new AACCodec();
  else if (strFileType.Equals("wv"))
    return new WAVPackCodec();
  else if (ModuleCodec::IsSupportedFormat(strFileType))
    return new ModuleCodec();

  return NULL;
}
