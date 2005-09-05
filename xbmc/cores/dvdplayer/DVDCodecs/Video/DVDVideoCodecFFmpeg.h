
#pragma once

#include "DVDVideoCodec.h"

struct AVCodecContext;
struct AVFrame;

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CodecID codecID, int iWidth, int iHeight);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual bool Flush();
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);

protected:
  void GetVideoAspect(AVCodecContext* CodecContext, unsigned int& iWidth, unsigned int& iHeight);

  AVFrame* m_pFrame;
  AVCodecContext* m_pCodecContext;

  AVFrame* m_pConvertFrame;

  int m_iPictureWidth;
  int m_iPictureHeight;

  int m_iScreenWidth;
  int m_iScreenHeight;
  
  bool m_bDllLoaded;
};
