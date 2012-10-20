/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "DVDStreamInfo.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDOverlayContainer.h"
#include "DVDMessageQueue.h"
#include "utils/BitstreamStats.h"
#include "linux/DllBCM.h"

using namespace std;

class OMXPlayerVideo : public CThread
{
protected:
  CDVDMessageQueue          m_messageQueue;
  int                       m_stream_id;
  bool                      m_open;
  CDVDStreamInfo            m_hints;
  double                    m_iCurrentPts;
  OMXClock                  *m_av_clock;
  COMXVideo                 m_omxVideo;
  float                     m_fFrameRate;
  bool                      m_Deinterlace;
  bool                      m_flush;
  bool                      m_hdmi_clock_sync;
  double                    m_iVideoDelay;
  int                       m_speed;
  double                    m_FlipTimeStamp; // time stamp of last flippage. used to play at a forced framerate
  int                       m_audio_count;
  bool                      m_stalled;
  bool                      m_started;
  std::string               m_codecname;
  double                    m_droptime;
  double                    m_dropbase;
  unsigned int              m_autosync;
  double                    m_iSubtitleDelay;
  bool                      m_bRenderSubs;
  bool                      m_bAllowFullscreen;

  float                     m_fForcedAspectRatio;
  unsigned int              m_width;
  unsigned int              m_height;
  unsigned int              m_video_width;
  unsigned int              m_video_height;
  unsigned                  m_flags;
  float                     m_fps;

  CRect                     m_dst_rect;
  int                       m_view_mode;

  DllBcmHost                m_DllBcmHost;
  bool                      m_send_eos;

  CDVDOverlayContainer  *m_pOverlayContainer;
  CDVDMessageQueue      &m_messageParent;

  BitstreamStats m_videoStats;

  DVDVideoPicture* m_pTempOverlayPicture;

  void ProcessOverlays(int iGroupId, double pts);

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
  bool IsEOS()                                      { return m_send_eos; };
  bool CloseStream(bool bWaitForBuffers);
  void Output(int iGroupId, double pts, bool bDropPacket);
  void Flush();
  bool OpenDecoder();
  int  GetDecoderBufferSize();
  int  GetDecoderFreeSpace();
  double GetCurrentPTS() { return m_iCurrentPts; };
  double GetFPS() { return m_fFrameRate; };
  void  WaitCompletion();
  void SetDelay(double delay) { m_iVideoDelay = delay; }
  double GetDelay() { return m_iVideoDelay; }
  void SetSpeed(int iSpeed);
  std::string GetPlayerInfo();
  int GetVideoBitrate();
  double GetOutputDelay();
  double GetSubtitleDelay()                         { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay)               { m_iSubtitleDelay = delay; }
  void EnableSubtitle(bool bEnable)                 { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled()                          { return m_bRenderSubs; }
  void EnableFullscreen(bool bEnable)               { m_bAllowFullscreen = bEnable; }
  void SetFlags(unsigned flags)                     { m_flags = flags; };
  int GetFreeSpace();
  void  SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);
};
#endif
