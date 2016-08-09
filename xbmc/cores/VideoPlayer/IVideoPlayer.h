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

#include <string>
#include <utility>
#include <vector>

#include "DVDClock.h"

#define VideoPlayer_AUDIO    1
#define VideoPlayer_VIDEO    2
#define VideoPlayer_SUBTITLE 3
#define VideoPlayer_TELETEXT 4
#define VideoPlayer_RDS      5


template <typename T> class CRectGen;
typedef CRectGen<float>  CRect;

class DVDNavResult;
class CDVDMsg;
class CDVDStreamInfo;
class CProcessInfo;

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
  virtual void GetVideoResolution(unsigned int &width, unsigned int &height) = 0;
  virtual ~IVideoPlayer() { }
};

class IDVDStreamPlayer
{
public:
  IDVDStreamPlayer(CProcessInfo &processInfo) : m_processInfo(processInfo) {};
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
protected:
  CProcessInfo &m_processInfo;
};

class CDVDVideoCodec;

class IDVDStreamPlayerVideo : public IDVDStreamPlayer
{
public:
  IDVDStreamPlayerVideo(CProcessInfo &processInfo) : IDVDStreamPlayer(processInfo) {};
  ~IDVDStreamPlayerVideo() {}
  virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void Flush(bool sync) = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  virtual bool IsInited() const = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void EnableSubtitle(bool bEnable) = 0;
  virtual bool IsSubtitleEnabled() = 0;
  virtual void EnableFullscreen(bool bEnable) = 0;
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
  virtual bool IsEOS() { return false; };
};

class CDVDAudioCodec;
class IDVDStreamPlayerAudio : public IDVDStreamPlayer
{
public:
  IDVDStreamPlayerAudio(CProcessInfo &processInfo) : IDVDStreamPlayer(processInfo) {};
  ~IDVDStreamPlayerAudio() {}
  virtual bool OpenStream(CDVDStreamInfo &hints) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void SetSpeed(int speed) = 0;
  virtual void Flush(bool sync) = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  virtual bool IsInited() const = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void SetVolume(float fVolume) {};
  virtual void SetMute(bool bOnOff) {};
  virtual void SetDynamicRangeCompression(long drc) = 0;
  virtual std::string GetPlayerInfo() = 0;
  virtual int GetAudioBitrate() = 0;
  virtual int GetAudioChannels() = 0;
  virtual double GetCurrentPts() = 0;
  virtual bool IsStalled() const = 0;
  virtual bool IsPassthrough() const = 0;
  virtual float GetDynamicRangeAmplification() const = 0;
  virtual bool IsEOS() { return false; };
};
