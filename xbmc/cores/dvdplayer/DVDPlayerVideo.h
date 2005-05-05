#pragma once
#include "../../utils/thread.h"

#include "DVDVideo.h"
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
  __int64 GetDelay();
  __int64 GetDiff();

  bool InitializedOutputDevice();
  CDVDPacketQueue m_packetQueue;

  CodecID m_codec;    // codec id of the current active stream

  CRITICAL_SECTION m_critSection;
  HANDLE m_hEvent;

  CDVDVideo m_dvdVideo;
  bool m_bRunningVideo;
  CDVDClock* m_pClock;

  CDVDOverlay m_overlay;

  DVDVideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
  int pictq_size, pictq_rindex, pictq_windex;

  int m_iSpeed;
  bool m_bRenderSubs;
  float m_fFPS;
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int OutputPicture(DVDVideoPicture* pPicture, __int64 pts1);

  CRITICAL_SECTION m_critCodecSection;




  CDVDVideoCodec* m_pVideoCodec;
  bool m_bInitializedOutputDevice;

  DVDVideoPicture* m_pOverlayPicture;

  CDVDDemuxSPU* m_pDVDSpu;

  CDemuxStreamVideo* m_pDemuxStreamVideo;

};
