#pragma once

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

#include "threads/Thread.h"
#include "IVideoPlayer.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlayContainer.h"
#include "PTSTracker.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "utils/BitstreamStats.h"
#include <atomic>

class CDemuxStreamVideo;

#define VIDEO_PICTURE_QUEUE_SIZE 1

#define EOS_ABORT 1
#define EOS_DROPPED 2
#define EOS_VERYLATE 4
#define EOS_BUFFER_LEVEL 8

class CDroppingStats
{
public:
  void Reset();
  void AddOutputDropGain(double pts, int frames);
  struct CGain
  {
    int frames;
    double pts;
  };
  std::deque<CGain> m_gain;
  int m_totalGain;
  double m_lastPts;
};

class CVideoPlayerVideo : public CThread, public IDVDStreamPlayerVideo
{
public:
  CVideoPlayerVideo(CDVDClock* pClock
                 ,CDVDOverlayContainer* pOverlayContainer
                 ,CDVDMessageQueue& parent
                 ,CRenderManager& renderManager,
                 CProcessInfo &processInfo);
  virtual ~CVideoPlayerVideo();

  bool OpenStream(CDVDStreamInfo hint) override;
  void CloseStream(bool bWaitForBuffers) override;
  void Flush(bool sync) override;
  bool AcceptsData() const override;
  bool HasData() const override;
  bool IsInited() const override;
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override;
  void FlushMessages() override;

  void EnableSubtitle(bool bEnable) override { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled() override { return m_bRenderSubs; }
  void EnableFullscreen(bool bEnable) override { m_bAllowFullscreen = bEnable; }
  double GetSubtitleDelay() override { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay) override { m_iSubtitleDelay = delay; }
  bool IsStalled() const override { return m_stalled; }
  bool IsRewindStalled() const override { return m_rewindStalled; }
  double GetCurrentPts() override;
  double GetOutputDelay() override; /* returns the expected delay, from that a packet is put in queue */
  std::string GetPlayerInfo() override;
  int GetVideoBitrate() override;
  std::string GetStereoMode() override;
  void SetSpeed(int iSpeed) override;

  // classes
  CDVDOverlayContainer* m_pOverlayContainer;
  CDVDClock* m_pClock;

protected:

  virtual void OnExit() override;
  virtual void Process() override;
  bool ProcessDecoderOutput(double &frametime, double &pts);
  void SendMessageBack(CDVDMsg* pMsg, int priority = 0);
  MsgQueueReturnCode GetMessage(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int &priority);

  int OutputPicture(const VideoPicture* src, double pts);
  void ProcessOverlays(VideoPicture* pSource, double pts);
  void OpenStream(CDVDStreamInfo &hint, CDVDVideoCodec* codec);

  void ResetFrameRateCalc();
  void CalcFrameRate();
  int CalcDropRequirement(double pts);

  double m_iSubtitleDelay;

  int m_iLateFrames;
  int m_iDroppedFrames;
  int m_iDroppedRequest;

  double m_fFrameRate;       //framerate of the video currently playing
  double m_fStableFrameRate; //place to store calculated framerates
  int m_iFrameRateCount;     //how many calculated framerates we stored in m_fStableFrameRate
  bool m_bAllowDrop;         //we can't drop frames until we've calculated the framerate
  int m_iFrameRateErr;       //how many frames we couldn't calculate the framerate, we give up after a while
  int m_iFrameRateLength;    //how many seconds we should measure the framerate
                             //this is increased exponentially from CVideoPlayerVideo::CalcFrameRate()

  bool m_bFpsInvalid;        // needed to ignore fps (e.g. dvd stills)
  bool m_bAllowFullscreen;
  bool m_bRenderSubs;
  float m_fForcedAspectRatio;
  int m_speed;
  std::atomic_bool m_stalled;
  std::atomic_bool m_rewindStalled;
  bool m_paused;
  IDVDStreamPlayer::ESyncState m_syncState;
  std::atomic_bool m_bAbortOutput;

  BitstreamStats m_videoStats;

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;
  CDVDStreamInfo m_hints;
  CDVDVideoCodec* m_pVideoCodec;
  CPtsTracker m_ptsTracker;
  std::list<DVDMessageListItem> m_packets;
  CDroppingStats m_droppingStats;
  CRenderManager& m_renderManager;
  VideoPicture m_picture;
};
