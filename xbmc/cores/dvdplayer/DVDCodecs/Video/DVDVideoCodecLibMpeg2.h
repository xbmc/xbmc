
#pragma once

#include "DVDVideoCodec.h"

// some typedef's here to prevent mpeg.h include
typedef struct mpeg2dec_s mpeg2dec_t;
typedef struct mpeg2_info_s mpeg2_info_t;
typedef struct mpeg2_sequence_s mpeg2_sequence_t;

class DllLoader;

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
  bool  m_bDllLoaded;

  unsigned int m_irffpattern;
  bool m_bFilm; //Signals that we have film material

  //The buffer of pictures we need
  DVDVideoPicture m_pVideoBuffer[3];
  DVDVideoPicture* m_pCurrentBuffer;
};
