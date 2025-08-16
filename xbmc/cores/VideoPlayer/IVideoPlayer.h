/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDClock.h"
#include "cores/MenuType.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"

#include <string>
#include <vector>

#define VideoPlayer_AUDIO    1
#define VideoPlayer_VIDEO    2
#define VideoPlayer_SUBTITLE 3
#define VideoPlayer_TELETEXT 4
#define VideoPlayer_RDS      5
#define VideoPlayer_ID3 6

struct SPlayerState
{
  SPlayerState() { Clear(); }
  void Clear()
  {
    timestamp = 0;
    time = 0;
    startTime = 0;
    timeMin = 0;
    timeMax = 0;
    time_offset = 0;
    dts = DVD_NOPTS_VALUE;
    player_state = "";
    isInMenu = false;
    menuType = MenuType::NONE;
    chapter = 0;
    chapters.clear();
    canpause = false;
    canseek = false;
    cantempo = false;
    caching = false;
    cache_bytes = 0;
    cache_level = 0.0;
    cache_offset = 0.0;
    lastSeek = 0;
    streamsReady = false;
  }

  double timestamp; // last time of update
  double lastSeek; // time of last seek
  double time_offset; // difference between time and pts

  double time; // current playback time
  double timeMax;
  double timeMin;
  time_t startTime;
  double dts; // last known dts

  std::string player_state; // full player state
  bool isInMenu;
  MenuType menuType;
  bool streamsReady;

  int chapter; // current chapter
  std::vector<std::pair<std::string, int64_t>> chapters; // name and position for chapters

  bool canpause; // pvr: can pause the current playing item
  bool canseek; // pvr: can seek in the current playing item
  bool cantempo;
  bool caching;

  int64_t cache_bytes; // number of bytes current's cached
  double cache_level; // current cache level
  double cache_offset; // percentage of file ahead of current position
  double cache_time; // estimated playback time of current cached bytes
};

class CDVDMsg;
class CDVDStreamInfo;
class CProcessInfo;

class IVideoPlayer
{
public:
  virtual int OnDiscNavResult(void* pData, int iMessage) = 0;
  virtual void GetVideoResolution(unsigned int &width, unsigned int &height) = 0;
  virtual ~IVideoPlayer() = default;
};

class IDVDStreamPlayer
{
public:
  explicit IDVDStreamPlayer(CProcessInfo& processInfo) : m_processInfo(processInfo) {}
  virtual ~IDVDStreamPlayer() = default;
  virtual bool OpenStream(CDVDStreamInfo hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) = 0;
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

struct SStartMsg
{
  double timestamp;
  int player;
  double cachetime;
  double cachetotal;
};

struct SStateMsg
{
  IDVDStreamPlayer::ESyncState syncState;
  int player;
};

class CDVDVideoCodec;

class IDVDStreamPlayerVideo : public IDVDStreamPlayer
{
public:
  explicit IDVDStreamPlayerVideo(CProcessInfo& processInfo) : IDVDStreamPlayer(processInfo) {}
  ~IDVDStreamPlayerVideo() override = default;
  bool OpenStream(CDVDStreamInfo hint) override = 0;
  void CloseStream(bool bWaitForBuffers) override = 0;
  virtual void Flush(bool sync) = 0;
  bool AcceptsData() const override = 0;
  virtual bool HasData() const = 0;
  bool IsInited() const override = 0;
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override = 0;
  virtual void EnableSubtitle(bool bEnable) = 0;
  virtual bool IsSubtitleEnabled() = 0;
  virtual double GetSubtitleDelay() = 0;
  virtual void SetSubtitleDelay(double delay) = 0;
  bool IsStalled() const override = 0;
  virtual bool IsRewindStalled() const { return false; }
  virtual double GetCurrentPts() = 0;
  virtual double GetOutputDelay() = 0;
  virtual std::string GetPlayerInfo() = 0;
  virtual int GetVideoBitrate() = 0;
  virtual void SetSpeed(int iSpeed) = 0;
  virtual bool IsEOS() { return false; }
};

class CDVDAudioCodec;
class IDVDStreamPlayerAudio : public IDVDStreamPlayer
{
public:
  explicit IDVDStreamPlayerAudio(CProcessInfo& processInfo) : IDVDStreamPlayer(processInfo) {}
  ~IDVDStreamPlayerAudio() override = default;
  bool OpenStream(CDVDStreamInfo hints) override = 0;
  void CloseStream(bool bWaitForBuffers) override = 0;
  virtual void SetSpeed(int speed) = 0;
  virtual void Flush(bool sync) = 0;
  bool AcceptsData() const override = 0;
  virtual bool HasData() const = 0;
  virtual int  GetLevel() const = 0;
  bool IsInited() const override = 0;
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override = 0;
  virtual void SetVolume(float fVolume) {}
  virtual void SetMute(bool bOnOff) {}
  virtual void SetDynamicRangeCompression(long drc) = 0;
  virtual std::string GetPlayerInfo() = 0;
  virtual int GetAudioChannels() = 0;
  virtual double GetCurrentPts() = 0;
  bool IsStalled() const override = 0;
  virtual bool IsPassthrough() const = 0;
  virtual float GetDynamicRangeAmplification() const = 0;
  virtual bool IsEOS() { return false; }
};
