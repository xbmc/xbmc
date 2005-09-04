
#pragma once

#include "../../utils/thread.h"
#include "DVDDemuxers\DVDPacketQueue.h"
#include "DVDCodecs\Video\DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlay.h"
#include "../VideoRenderers/RenderManager.h"

enum CodecID;
class CDVDDemuxSPU;
class CDemuxStreamVideo;

#define VIDEO_PICTURE_QUEUE_SIZE 1

class CDVDPlayerVideo : public CThread
{
public:
  CDVDPlayerVideo(CDVDDemuxSPU* spu, CDVDClock* pClock);
  virtual ~CDVDPlayerVideo();

  bool OpenStream(CodecID codecID, int iWidth, int iHeight, CDemuxStreamVideo* pDemuxStreamVideo);
  void CloseStream(bool bWaitForBuffers);

  void Pause();
  void Resume();
  void Flush();

  void Update(bool bPauseDrawing)                   { g_renderManager.Update(bPauseDrawing); }
  void UpdateMenuPicture();
 
  void EnableSubtitle(bool bEnable)                 { m_bRenderSubs = bEnable; }
  void GetVideoRect(RECT& SrcRect, RECT& DestRect)  { g_renderManager.GetVideoRect(SrcRect, DestRect); }
  float GetAspectRatio()                            { return g_renderManager.GetAspectRatio(); }

  //Set a forced aspect ratio
  void SetAspectRatio(float fAspectRatio);

  __int64 GetDelay();
    void SetDelay(__int64 delay);
  __int64 GetDiff();
  int GetNrOfDroppedFrames()                        { return m_iDroppedFrames; }

  bool InitializedOutputDevice();
  
  // used for picture handling between the class and video_refresh_thread()
  CRITICAL_SECTION m_critSection;
  HANDLE m_hEvent;
  DVDVideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
  int pictq_size, pictq_rindex, pictq_windex;
  
  bool m_bRunningVideo;
  
  int m_iDroppedFrames;
  int m_iSpeed;
  __int64 m_iVideoDelay;

  // classes
  CDVDPacketQueue m_packetQueue;
  CDVDOverlay m_overlay;
  
  CDVDClock* m_pClock;

protected:  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool OutputPicture(DVDVideoPicture* pPicture, __int64 pts1);
  
  bool m_bInitializedOutputDevice;
  bool m_bRenderSubs;
  
  float m_fForcedAspectRatio;
  
  int m_iNrOfPicturesNotToSkip;
  
  // classes
  CDVDDemuxSPU* m_pDVDSpu;
  CDemuxStreamVideo* m_pDemuxStreamVideo;
  CDVDVideoCodec* m_pVideoCodec;
  
  CRITICAL_SECTION m_critCodecSection;
};
