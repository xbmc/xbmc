
#pragma once

#include "DVDVideoCodec.h"

#include "DllLibMpeg2.h"

class CDVDVideoCodecLibMpeg2 : public CDVDVideoCodec
{
public:
  CDVDVideoCodecLibMpeg2();
  virtual ~CDVDVideoCodecLibMpeg2();
  virtual bool Open(CodecID codecID, int iWidth, int iHeight);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual bool Flush();
  virtual void Reset();
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);

protected:
  DVDVideoPicture* GetBuffer(unsigned int width, unsigned int height);
  inline void ReleaseBuffer(DVDVideoPicture* pPic);
  inline void DeleteBuffer(DVDVideoPicture* pPic);

  int GuessAspect(const mpeg2_sequence_t *sequence, unsigned int *pixel_width, unsigned int *pixel_height);

  mpeg2dec_t* m_pHandle;
  const mpeg2_info_t* m_pInfo;
  DllLibMpeg2 m_dll;

  unsigned int m_irffpattern;
  bool m_bFilm; //Signals that we have film material

  //The buffer of pictures we need
  DVDVideoPicture m_pVideoBuffer[3];
  DVDVideoPicture* m_pCurrentBuffer;
};
