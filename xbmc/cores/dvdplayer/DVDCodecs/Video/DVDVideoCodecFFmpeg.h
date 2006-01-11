
#pragma once

#include "DVDVideoCodec.h"
#include "..\DllAvCodec.h"
#include "..\..\DVDDemuxers\DllAvFormat.h"

class CDVDVideoCodecFFmpeg : public CDVDVideoCodec
{
public:
  CDVDVideoCodecFFmpeg();
  virtual ~CDVDVideoCodecFFmpeg();
  virtual bool Open(CodecID codecID, int iWidth, int iHeight, void* ExtraData, unsigned int ExtraSize);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
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
  
  DllAvCodec m_dllAvCodec;
  DllAvFormat m_dllAvFormat;
};
