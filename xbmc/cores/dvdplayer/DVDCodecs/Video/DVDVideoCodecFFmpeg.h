
#pragma once

#include "DVDVideoCodec.h"
#include "../DllAvCodec.h"
#include "../../DVDDemuxers/DllAvFormat.h"

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);  
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double pts);
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return "FFmpeg"; };

protected:
  void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);

  AVFrame* m_pFrame;
  AVCodecContext* m_pCodecContext;

  AVFrame* m_pConvertFrame;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;
  
  std::map<int, double> m_timestamps;
  DllAvCodec m_dllAvCodec;
  DllAvUtil m_dllAvUtil;
};
