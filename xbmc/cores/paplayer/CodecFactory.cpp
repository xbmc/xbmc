#include "../../stdafx.h"
#include "CodecFactory.h"
#include "MP3Codec.h"
#include "APECodec.h"
#include "CDDACodec.h"
#include "OGGCodec.h"

ICodec* CodecFactory::CreateCodec(const CStdString& strFileType)
{
  if (strFileType.Equals("mp3"))
    return new MP3Codec;
  else if (strFileType.Equals("ape") || strFileType.Equals("mac"))
    return new APECodec;
  else if (strFileType.Equals("cdda"))
    return new CDDACodec;
  else if (strFileType.Equals("ogg"))
    return new OGGCodec;

  return NULL;
}
