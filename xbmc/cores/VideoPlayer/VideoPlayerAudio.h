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

#pragma once
#include <list>
#include <utility>

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "IVideoPlayer.h"
#include "threads/Thread.h"
#include "utils/BitstreamStats.h"


class CVideoPlayer;
class CDVDAudioCodec;
class CDVDAudioCodec;

class CVideoPlayerAudio : public CThread, public IDVDStreamPlayerAudio
{
public:
  CVideoPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent, CProcessInfo &processInfo);
  virtual ~CVideoPlayerAudio();

  bool OpenStream(CDVDStreamInfo &hints) override;
  void CloseStream(bool bWaitForBuffers) override;

  void SetSpeed(int speed) override;
  void Flush(bool sync) override;

  // waits until all available data has been rendered
  bool AcceptsData() const override;
  bool HasData() const override                                  { return m_messageQueue.GetDataSize() > 0; }
  int  GetLevel() const override                                 { return m_messageQueue.GetLevel(); }
  bool IsInited() const override                                 { return m_messageQueue.IsInited(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0)  override    { m_messageQueue.Put(pMsg, priority); }
  void FlushMessages() override                                  { m_messageQueue.Flush(); }

  void SetDynamicRangeCompression(long drc)  override            { m_dvdAudio.SetDynamicRangeCompression(drc); }
  float GetDynamicRangeAmplification() const override            { return 0.0f; }


  std::string GetPlayerInfo() const override;
  int GetAudioBitrate() const override;
  int GetAudioChannels() const override;

  // holds stream information for current playing stream
  CDVDStreamInfo m_streaminfo;

  double GetCurrentPts() const override                           { CSingleLock lock(m_info_section); return m_info.pts; }

  bool IsStalled() const override                           { return m_stalled;  }
  bool IsPassthrough() const override;

protected:

  virtual void OnStartup() override;
  virtual void OnExit() override;
  virtual void Process() override;

  void UpdatePlayerInfo();
  void OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec* codec);
  //! Switch codec if needed. Called when the sample rate gotten from the
  //! codec changes, in which case we may want to switch passthrough on/off.
  bool SwitchCodecIfNeeded();
  float GetCurrentAttenuation()                         { return m_dvdAudio.GetCurrentAttenuation(); }

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;

  double m_audioClock;

  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  CDVDAudioCodec* m_pAudioCodec; // audio codec
  BitstreamStats m_audioStats;

  int m_speed;
  bool m_stalled;
  bool m_silence;
  bool m_paused;
  IDVDStreamPlayer::ESyncState m_syncState;
  XbmcThreads::EndTime m_syncTimer;

  bool OutputPacket(DVDAudioFrame &audioframe);

  //SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
  int    m_synctype;
  int    m_setsynctype;
  int    m_prevsynctype; //so we can print to the log

  void   SetSyncType(bool passthrough);

  bool   m_prevskipped;
  double m_maxspeedadjust;

  struct SInfo
  {
    SInfo()
    : pts(DVD_NOPTS_VALUE)
    , passthrough(false)
    {}

    std::string      info;
    double           pts;
    bool             passthrough;
  };

  CCriticalSection m_info_section;
  SInfo            m_info;
};

