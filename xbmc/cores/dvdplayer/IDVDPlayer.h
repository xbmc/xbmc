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

template <typename T> class CRectGen;
typedef CRectGen<float>  CRect;

class DVDNavResult;

class IDVDPlayer
{
public:
  virtual int OnDVDNavResult(void* pData, int iMessage) = 0;
  virtual ~IDVDPlayer() { }
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
};

class CDVDVideoCodec;

class IDVDStreamPlayerVideo : public IDVDStreamPlayer
{
public:
  ~IDVDStreamPlayerVideo() {}
  float GetRelativeUsage() { return 0.0f; }
  virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual bool StepFrame() = 0;
  virtual void Flush() = 0;
  virtual void WaitForBuffers() = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  virtual bool IsInited() const = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
  virtual void EnableSubtitle(bool bEnable) = 0;
  virtual bool IsSubtitleEnabled() = 0;
  virtual void EnableFullscreen(bool bEnable) = 0;
#ifdef HAS_VIDEO_PLAYBACK
  virtual void GetVideoRect(CRect& SrcRect, CRect& DestRect, CRect& ViewRect) const = 0;
  virtual float GetAspectRatio() = 0;
#endif
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
  virtual void Flush() = 0;
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
  virtual double GetDelay() = 0;
  virtual double GetCacheTotal() = 0;
  virtual float GetDynamicRangeAmplification() const = 0;
  virtual bool IsEOS() = 0;
};
