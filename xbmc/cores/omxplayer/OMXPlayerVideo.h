/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <deque>
#include <sys/types.h>

#include "OMXClock.h"
#include "DVDStreamInfo.h"
#include "OMXVideo.h"
#include "threads/Thread.h"
#include "IVideoPlayer.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDOverlayContainer.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "platform/linux/DllBCM.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include <atomic>

class OMXPlayerVideo : public CThread, public IDVDStreamPlayerVideo
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
  int                       m_speed;
  bool                      m_stalled;
  IDVDStreamPlayer::ESyncState m_syncState;
  bool                      m_flush;
  std::string               m_codecname;
  std::atomic_bool          m_bAbortOutput;
  double                    m_iSubtitleDelay;
  bool                      m_bRenderSubs;

  float                     m_fForcedAspectRatio;

  CRect                     m_src_rect;
  CRect                     m_dst_rect;
  RENDER_STEREO_MODE        m_video_stereo_mode;
  RENDER_STEREO_MODE        m_display_stereo_mode;
  bool                      m_StereoInvert;
  DllBcmHost                m_DllBcmHost;

  CDVDOverlayContainer  *m_pOverlayContainer;
  CDVDMessageQueue      &m_messageParent;

  BitstreamStats m_videoStats;
  CRenderManager& m_renderManager;

  void ProcessOverlays(double pts);
  double NextOverlay(double pts);
  bool OpenStream(CDVDStreamInfo &hints, COMXVideo *codec);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void SendMessageBack(CDVDMsg* pMsg, int priority = 0);
  MsgQueueReturnCode GetMessage(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int &priority);
  std::string GetStereoMode();
private:
public:
  OMXPlayerVideo(OMXClock *av_clock, CDVDOverlayContainer* pOverlayContainer, CDVDMessageQueue& parent, CRenderManager& renderManager, CProcessInfo &processInfo);
  ~OMXPlayerVideo();
  bool OpenStream(CDVDStreamInfo hints) override;
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override;
  void FlushMessages() override;
  bool AcceptsData() const override;
  bool HasData() const override;
  bool IsInited() const override;
  bool IsStalled() const                            override { return m_stalled;  }
  bool IsEOS() override;
  void CloseStream(bool bWaitForBuffers) override;
  void Output(double pts, bool bDropPacket);
  bool StepFrame();
  void Flush(bool sync) override;
  bool OpenDecoder();
  double GetCurrentPts() override { return m_iCurrentPts; };
  void  SubmitEOS();
  bool SubmittedEOS() const { return m_omxVideo.SubmittedEOS(); }
  void SetSpeed(int iSpeed) override;
  std::string GetPlayerInfo() override;
  int GetVideoBitrate() override;
  double GetOutputDelay() override;
  double GetSubtitleDelay()                         override { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay)               override { m_iSubtitleDelay = delay; }
  void EnableSubtitle(bool bEnable)                 override { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled()                          override { return m_bRenderSubs; }
  float GetAspectRatio()                                     { return m_renderManager.GetAspectRatio(); }
  void  SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  void ResolutionUpdateCallBack(uint32_t width, uint32_t height, float framerate, float pixel_aspect);
  static void ResolutionUpdateCallBack(void *ctx, uint32_t width, uint32_t height, float framerate, float pixel_aspect);
};
