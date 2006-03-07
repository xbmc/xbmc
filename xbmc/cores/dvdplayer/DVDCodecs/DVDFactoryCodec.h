
#pragma once

enum CodecID;
class CDVDVideoCodec;
class CDVDAudioCodec;

class CDemuxStreamVideo;
class CDVDStreamInfo;


class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint );
  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint );

  static CDVDAudioCodec* OpenCodec(CDVDAudioCodec* pCodec,  CDVDStreamInfo &hint );
  static CDVDVideoCodec* OpenCodec(CDVDVideoCodec* pCodec,  CDVDStreamInfo &hint );
};
