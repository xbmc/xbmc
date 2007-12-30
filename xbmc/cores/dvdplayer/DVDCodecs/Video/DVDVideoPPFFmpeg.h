
#pragma once

#include "DVDVideoCodec.h"
#include "../../../ffmpeg/DllPostProc.h"

class CDVDVideoPPFFmpeg
{
public:

  enum EPPTYPE
  {
    ED_DEINT_FFMPEG,
    ED_DEINT_CUBICIPOL,
    ED_DEINT_LINBLEND
  };

  CDVDVideoPPFFmpeg(EPPTYPE mType);
  ~CDVDVideoPPFFmpeg();


  void SetTarget(DVDVideoPicture *pPicture){ m_pTarget = pPicture; };
  void Process(DVDVideoPicture *pPicture);
  bool GetPicture(DVDVideoPicture *pPicture);

protected:
  EPPTYPE m_eType;

  void *m_pContext;
  void *m_pMode;

  DVDVideoPicture m_FrameBuffer;
  DVDVideoPicture *m_pSource;
  DVDVideoPicture *m_pTarget;

  void Dispose();
  
  int m_iInitWidth, m_iInitHeight;
  bool CheckInit(int iWidth, int iHeight);
  bool CheckFrameBuffer(const DVDVideoPicture* pSource);
  
  DllPostProc m_dll;
};

