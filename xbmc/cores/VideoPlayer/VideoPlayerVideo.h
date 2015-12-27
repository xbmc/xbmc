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
#include "Interfaces/IVPClockCallback.h"
#include "DVDMessageQueue.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlayContainer.h"
#include "DVDTSCorrection.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#endif
#include "utils/BitstreamStats.h"

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
  void AddOutputDropGain(double pts, double frametime);
  struct CGain
  {
    double gain;
    double pts;
  };
  std::deque<CGain> m_gain;
  double m_totalGain;
  double m_lastDecoderPts;
  double m_lastPts;
  unsigned int m_lateFrames;
  unsigned int m_dropRequests;
};

class CVideoPlayerVideo : public CThread, public IDVDStreamPlayerVideo, public IVPClockCallback
{
public:
  CVideoPlayerVideo(CDVDClock* pClock
                 ,CDVDOverlayContainer* pOverlayContainer
                 ,CDVDMessageQueue& parent
                 ,CRenderManager& renderManager);
  virtual ~CVideoPlayerVideo();

  bool OpenStream(CDVDStreamInfo &hint);
  void CloseStream(bool bWaitForBuffers);

  void Flush(bool sync);
  bool AcceptsData() const;
  bool HasData() const { return m_messageQueue.GetDataSize() > 0; }
  int  GetLevel() const { return m_messageQueue.GetLevel(); }
  bool IsInited() const { return m_messageQueue.IsInited(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0) { m_messageQueue.Put(pMsg, priority); }
  void FlushMessages() { m_messageQueue.Flush(); }

  void EnableSubtitle(bool bEnable) { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled() { return m_bRenderSubs; }
  void EnableFullscreen(bool bEnable) { m_bAllowFullscreen = bEnable; }
  double GetDelay() { return m_iVideoDelay; }
  void SetDelay(double delay) { m_iVideoDelay = delay; }
  double GetSubtitleDelay() { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay) { m_iSubtitleDelay = delay; }
  bool IsStalled() const { return m_stalled; }
  bool IsEOS() { return false; }
  bool SubmittedEOS() const { return false; }
  double GetCurrentPts();
  double GetOutputDelay(); /* returns the expected delay, from that a packet is put in queue */
  int GetDecoderFreeSpace() { return 0; }
  std::string GetPlayerInfo();
  int GetVideoBitrate();
  std::string GetStereoMode();
  void SetSpeed(int iSpeed);

  // IVPClockCallback interface
  virtual double GetInterpolatedClock();

  CDVDOverlayContainer* m_pOverlayContainer;
  CDVDClock* m_pClock;

protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  bool ProcessDecoderOutput(int &decoderState, double &frametime, double &pts);

  int OutputPicture(const DVDVideoPicture* src, double pts);
  void ProcessOverlays(DVDVideoPicture* pSource, double pts);
  void OpenStream(CDVDStreamInfo &hint, CDVDVideoCodec* codec);

  // waits until all available data has been rendered
  // just waiting for packetqueue should be enough for video
  void WaitForBuffers()  { m_messageQueue.WaitUntilEmpty(); }

  void ResetFrameRateCalc();
  void CalcFrameRate();
  int CalcDropRequirement(double pts, bool updateOnly);

  double m_iVideoDelay;
  double m_iSubtitleDelay;
  double m_FlipTimeStamp; // time stamp of last flippage. used to play at a forced framerate
  double m_FlipTimePts;   // pts of the last flipped page

  int m_iLateFrames;
  int m_iDroppedFrames;
  int m_iDroppedRequest;

  double m_fFrameRate;       //framerate of the video currently playing
  bool m_bCalcFrameRate;     //if we should calculate the framerate from the timestamps
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
  int m_iNrOfPicturesNotToSkip;
  int m_speed;
  bool m_stalled;
  IDVDStreamPlayer::ESyncState m_syncState;
  std::string m_codecname;

  BitstreamStats m_videoStats;

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;
  CDVDStreamInfo m_hints;
  CDVDVideoCodec* m_pVideoCodec;
  DVDVideoPicture* m_pTempOverlayPicture;
  CPullupCorrection m_pullupCorrection;
  std::list<DVDMessageListItem> m_packets;
  CDroppingStats m_droppingStats;
  CRenderManager& m_renderManager;
  DVDVideoPicture m_picture;
};

