
#pragma once

enum CodecID;
class CDVDVideoCodec;
class CDVDAudioCodec;

class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CodecID codecID);
  static CDVDAudioCodec* CreateAudioCodec(CodecID CodecID);
};
