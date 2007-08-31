
#pragma once

class CDVDVideoCodec;
class CDVDAudioCodec;

class CDemuxStreamVideo;
class CDVDStreamInfo;
class CDVDCodecOption;
typedef std::vector<CDVDCodecOption> CDVDCodecOptions;

class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDVDStreamInfo &hint );
  static CDVDAudioCodec* CreateAudioCodec(CDVDStreamInfo &hint );

  static CDVDAudioCodec* OpenCodec(CDVDAudioCodec* pCodec,  CDVDStreamInfo &hint );
  static CDVDVideoCodec* OpenCodec(CDVDVideoCodec* pCodec,  CDVDStreamInfo &hint, CDVDCodecOptions &options );
};

