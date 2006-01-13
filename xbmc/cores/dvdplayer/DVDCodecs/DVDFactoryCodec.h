
#pragma once

enum CodecID;
class CDVDVideoCodec;
class CDVDAudioCodec;

class CDemuxStreamVideo;
class CDemuxStreamAudio;


class CDVDFactoryCodec
{
public:
  static CDVDVideoCodec* CreateVideoCodec(CDemuxStreamVideo *pDemuxStream );
  static CDVDAudioCodec* CreateAudioCodec(CDemuxStreamAudio *pDemuxStream );

  static CDVDAudioCodec* OpenCodec(CDVDAudioCodec* pCodec,  CDemuxStreamAudio *pDemuxStream );
  static CDVDVideoCodec* OpenCodec(CDVDVideoCodec* pCodec,  CDemuxStreamVideo *pDemuxStream );
};
