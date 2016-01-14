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

#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include "DVDClock.h"

#define VideoPlayer_AUDIO    1
#define VideoPlayer_VIDEO    2
#define VideoPlayer_SUBTITLE 3
#define VideoPlayer_TELETEXT 4
#define VideoPlayer_RDS      5


template <typename T> class CRectGen;
typedef CRectGen<float>  CRect;

class DVDNavResult;

struct SPlayerState
{
  SPlayerState() { Clear(); }
  void Clear()
  {
    player        = 0;
    timestamp     = 0;
    time          = 0;
    time_total    = 0;
    time_offset   = 0;
    dts           = DVD_NOPTS_VALUE;
    player_state  = "";
    chapter       = 0;
    chapters.clear();
    canrecord     = false;
    recording     = false;
    canpause      = false;
    canseek       = false;
    demux_video   = "";
    demux_audio   = "";
    cache_bytes   = 0;
    cache_level   = 0.0;
    cache_delay   = 0.0;
    cache_offset  = 0.0;
  }

  int    player;            // source of this data

  double timestamp;         // last time of update
  double time_offset;       // difference between time and pts

  double time;              // current playback time
  double time_total;        // total playback time
  double dts;               // last known dts

  std::string player_state;  // full player state

  int         chapter;                   // current chapter
  std::vector<std::pair<std::string, int64_t>> chapters; // name and position for chapters

  bool canrecord;           // can input stream record
  bool recording;           // are we currently recording

  bool canpause;            // pvr: can pause the current playing item
  bool canseek;             // pvr: can seek in the current playing item

  std::string demux_video;
  std::string demux_audio;

  int64_t cache_bytes;   // number of bytes current's cached
  double  cache_level;   // current estimated required cache level
  double  cache_delay;   // time until cache is expected to reach estimated level
  double  cache_offset;  // percentage of file ahead of current position
};

struct SStartMsg
{
  double timestamp;
  int player;
  double cachetime;
  double cachetotal;
};

class IVideoPlayer
{
public:
  virtual int OnDVDNavResult(void* pData, int iMessage) = 0;
  virtual ~IVideoPlayer() { }
};

class IDVDStreamPlayer
{
public:
  virtual ~IDVDStreamPlayer() {}
  virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void FlushMessages() = 0;
  virtual bool IsInited() const = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool IsStalled() const = 0;

  enum ESyncState
  {
    SYNC_STARTING,
    SYNC_WAITSYNC,
    SYNC_INSYNC
  };
};

class CDVDVideoCodec;

class IDVDStreamPlayerVideo : public IDVDStreamPlayer
{
public:
  ~IDVDStreamPlayerVideo() {}
  float GetRelativeUsage() { return 0.0f; }
  virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual bool StepFrame() { return false; };
  virtual void Flush(bool sync) = 0;
  virtual void WaitForBuffers() = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  virtual bool IsInited() const = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void EnableSubtitle(bool bEnable) = 0;
  virtual bool IsSubtitleEnabled() = 0;
  virtual void EnableFullscreen(bool bEnable) = 0;
  virtual double GetDelay() = 0;
  virtual void SetDelay(double delay) = 0;
  virtual double GetSubtitleDelay() = 0;
  virtual void SetSubtitleDelay(double delay) = 0;
  virtual bool IsStalled() const = 0;
  virtual double GetCurrentPts() = 0;
  virtual double GetOutputDelay() = 0;
  virtual std::string GetPlayerInfo() = 0;
  virtual int GetVideoBitrate() = 0;
  virtual std::string GetStereoMode() = 0;
  virtual void SetSpeed(int iSpeed) = 0;
  virtual int  GetDecoderBufferSize() { return 0; }
  virtual int  GetDecoderFreeSpace() = 0;
  virtual bool IsEOS() = 0;
  virtual bool SubmittedEOS() const = 0;
};

class CDVDAudioCodec;
class IDVDStreamPlayerAudio : public IDVDStreamPlayer
{
public:
  ~IDVDStreamPlayerAudio() {}
  float GetRelativeUsage() { return 0.0f; }
  virtual bool OpenStream(CDVDStreamInfo &hints) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void SetSpeed(int speed) = 0;
  virtual void Flush(bool sync) = 0;
  virtual void WaitForBuffers() = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  virtual bool IsInited() const = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void SetVolume(float fVolume) = 0;
  virtual void SetMute(bool bOnOff) = 0;
  virtual void SetDynamicRangeCompression(long drc) = 0;
  virtual std::string GetPlayerInfo() = 0;
  virtual int GetAudioBitrate() = 0;
  virtual int GetAudioChannels() = 0;
  virtual double GetCurrentPts() = 0;
  virtual bool IsStalled() const = 0;
  virtual bool IsPassthrough() const = 0;
  virtual float GetDynamicRangeAmplification() const = 0;
  virtual bool IsEOS() = 0;
};
