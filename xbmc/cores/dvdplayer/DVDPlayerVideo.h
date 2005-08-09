#pragma once
#include "../../utils/thread.h"

#include "../VideoRenderers/RenderManager.h"
#include "DVDDemuxers\DVDPacketQueue.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "DVDDemuxers\DVDDemux.h"
#include "DVDCodecs\DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlay.h"

enum CodecID;
class CDVDDemuxSPU;

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

  void Update(bool bPauseDrawing); // called form xbmc
  void UpdateMenuPicture();
 
  void GetVideoRect(RECT& SrcRect, RECT& DestRect);
  float GetAspectRatio();

  //Set a forced aspect ratio
  void SetAspectRatio(float fAspectRatio);

  __int64 GetDelay();
    void SetDelay(__int64 delay);
  __int64 GetDiff();
  int GetNrOfDroppedFrames() { return m_iDroppedFrames; }

  bool InitializedOutputDevice();
  CDVDPacketQueue m_packetQueue;

  CodecID m_codec;    // codec id of the current active stream

  CRITICAL_SECTION m_critSection;
  HANDLE m_hEvent;

  bool m_bRunningVideo;
  CDVDClock* m_pClock;

  CDVDOverlay m_overlay;

  DVDVideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
  int pictq_size, pictq_rindex, pictq_windex;

  int m_iSpeed;
  bool m_bRenderSubs;
  int m_iNrOfPicturesNotToSkip;
  __int64 m_iVideoDelay;
  
  int m_iDroppedFrames;
  
  CDVDVideoCodec* m_pVideoCodec;
  
protected:  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool OutputPicture(DVDVideoPicture* pPicture, __int64 pts1);

  CRITICAL_SECTION m_critCodecSection;

  
  bool m_bInitializedOutputDevice;
  float m_fForcedAspectRatio;
  DVDVideoPicture* m_pOverlayPicture;

  CDVDDemuxSPU* m_pDVDSpu;

  CDemuxStreamVideo* m_pDemuxStreamVideo;
};
