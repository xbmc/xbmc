/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _OMX_PLAYERVIDEO_H_
#define _OMX_PLAYERVIDEO_H_

#include <deque>
#include <sys/types.h>

#include "utils/StdString.h"

#include "OMXClock.h"
#include "DVDStreamInfo.h"
#include "OMXVideo.h"
#include "threads/Thread.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDOverlayContainer.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "linux/DllBCM.h"
#include "cores/VideoRenderers/RenderManager.h"

using namespace std;

class OMXPlayerVideo : public CThread
{
protected:
  CDVDMessageQueue          m_messageQueue;
  int                       m_stream_id;
  bool                      m_open;
  CDVDStreamInfo            m_hints;
  double                    m_iCurrentPts;
  double                    m_nextOverlay;
  OMXClock                  *m_av_clock;
  COMXVideo                 m_omxVideo;
  float                     m_fFrameRate;
  bool                      m_hdmi_clock_sync;
  double                    m_iVideoDelay;
  int                       m_speed;
  bool                      m_stalled;
  bool                      m_started;
  bool                      m_flush;
  std::string               m_codecname;
  double                    m_iSubtitleDelay;
  bool                      m_bRenderSubs;
  bool                      m_bAllowFullscreen;

  float                     m_fForcedAspectRatio;
  unsigned                  m_flags;

  CRect                     m_src_rect;
  CRect                     m_dst_rect;

  uint32_t                  m_history_valid_pts;
  DllBcmHost                m_DllBcmHost;

  CDVDOverlayContainer  *m_pOverlayContainer;
  CDVDMessageQueue      &m_messageParent;

  BitstreamStats m_videoStats;

  void ProcessOverlays(double pts);
  double NextOverlay(double pts);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
private:
public:
  OMXPlayerVideo(OMXClock *av_clock, CDVDOverlayContainer* pOverlayContainer, CDVDMessageQueue& parent);
  ~OMXPlayerVideo();
  bool OpenStream(CDVDStreamInfo &hints);
  bool OpenStream(CDVDStreamInfo &hints, COMXVideo *codec);
  void SendMessage(CDVDMsg* pMsg, int priority = 0) { m_messageQueue.Put(pMsg, priority); }
  bool AcceptsData() const                          { return !m_messageQueue.IsFull(); }
  bool HasData() const                              { return m_messageQueue.GetDataSize() > 0; }
  bool IsInited() const                             { return m_messageQueue.IsInited(); }
  void WaitForBuffers()                             { m_messageQueue.WaitUntilEmpty(); }
  int  GetLevel() const                             { return m_messageQueue.GetLevel(); }
  bool IsStalled()                                  { return m_stalled;  }
  bool IsEOS();
  bool CloseStream(bool bWaitForBuffers);
  void Output(double pts, bool bDropPacket);
  void Flush();
  bool OpenDecoder();
  int  GetDecoderBufferSize();
  int  GetDecoderFreeSpace();
  double GetCurrentPts() { return m_iCurrentPts; };
  double GetFPS() { return m_fFrameRate; };
  void  SubmitEOS();
  bool SubmittedEOS();
  void SetDelay(double delay) { m_iVideoDelay = delay; }
  double GetDelay() { return m_iVideoDelay; }
  void SetSpeed(int iSpeed);
  std::string GetPlayerInfo();
  int GetVideoBitrate();
  std::string GetStereoMode();
  double GetOutputDelay();
  double GetSubtitleDelay()                         { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay)               { m_iSubtitleDelay = delay; }
  void EnableSubtitle(bool bEnable)                 { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled()                          { return m_bRenderSubs; }
  void EnableFullscreen(bool bEnable)               { m_bAllowFullscreen = bEnable; }
  float GetAspectRatio()                            { return g_renderManager.GetAspectRatio(); }
  void SetFlags(unsigned flags)                     { m_flags = flags; };
  int GetFreeSpace();
  void  SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);
  void ResolutionUpdateCallBack(uint32_t width, uint32_t height, float pixel_aspect);
  static void ResolutionUpdateCallBack(void *ctx, uint32_t width, uint32_t height, float pixel_aspect);
};
#endif
