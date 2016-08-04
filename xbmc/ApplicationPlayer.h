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

#include <memory>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "guilib/Resolution.h"
#include "cores/IPlayer.h"

typedef enum
{
  PLAYBACK_CANCELED = -1,
  PLAYBACK_FAIL = 0,
  PLAYBACK_OK = 1,
} PlayBackRet;

namespace PVR
{
  class CPVRChannel;
  typedef std::shared_ptr<PVR::CPVRChannel> CPVRChannelPtr;
}

class CAction;
class CPlayerOptions;
class CStreamDetails;

struct SPlayerAudioStreamInfo;
struct SPlayerVideoStreamInfo;
struct SPlayerSubtitleStreamInfo;
struct TextCacheStruct_t;

class CApplicationPlayer
{
  std::shared_ptr<IPlayer> m_pPlayer;
  unsigned int m_iPlayerOPSeq;  // used to detect whether an OpenFile request on player is canceled by us.

  CCriticalSection  m_player_lock;

  // cache player state
  XbmcThreads::EndTime m_audioStreamUpdate;
  int m_iAudioStream;
  XbmcThreads::EndTime m_videoStreamUpdate;
  int m_iVideoStream;
  XbmcThreads::EndTime m_subtitleStreamUpdate;
  int m_iSubtitleStream;
  XbmcThreads::EndTime m_speedUpdate;
  float m_fPlaySpeed;

public:
  CApplicationPlayer();

  // player management
  void CloseFile(bool reopen = false);
  void ClosePlayer();
  void ClosePlayerGapless(std::string &playername);
  void CreatePlayer(const std::string &player, IPlayerCallback& callback);
  std::string GetCurrentPlayer();
  float  GetPlaySpeed();
  bool HasPlayer() const;
  PlayBackRet OpenFile(const CFileItem& item, const CPlayerOptions& options);
  void SetPlaySpeed(float speed);

  void FrameMove();
  bool HasFrame();
  void Render(bool clear, uint32_t alpha = 255, bool gui = true);
  void FlushRenderer();
  void SetRenderViewMode(int mode);
  float GetRenderAspectRatio();
  void TriggerUpdateResolution();
  bool IsRenderingVideo();
  bool IsRenderingGuiLayer();
  bool IsRenderingVideoLayer();
  bool Supports(EINTERLACEMETHOD method);
  bool Supports(ESCALINGMETHOD method);
  bool Supports(ERENDERFEATURE feature);
  unsigned int RenderCaptureAlloc();
  void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags = 0);
  void RenderCaptureRelease(unsigned int captureId);
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size);
  bool IsExternalPlaying();

  // proxy calls
  void   AddSubtitle(const std::string& strSubPath);
  bool  CanPause();
  bool  CanRecord();
  bool  CanSeek();
  void  DoAudioWork();
  void  GetAudioCapabilities(std::vector<int> &audioCaps);
  int   GetAudioStream();
  int   GetAudioStreamCount();
  void  GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info);
  int   GetCacheLevel() const;
  float GetCachePercentage() const;
  int   GetChapterCount();
  int   GetChapter();  
  void  GetChapterName(std::string& strChapterName, int chapterIdx=-1);
  int64_t GetChapterPos(int chapterIdx=-1);
  void  GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods);
  float GetPercentage() const;
  std::string GetPlayerState();
  std::string GetPlayingTitle();
  int   GetPreferredPlaylist() const;
  void  GetRenderFeatures(std::vector<int> &renderFeatures);
  void  GetScalingMethods(std::vector<int> &scalingMethods);
  bool  GetStreamDetails(CStreamDetails &details);
  int   GetSubtitle();
  void  GetSubtitleCapabilities(std::vector<int> &subCaps);
  int   GetSubtitleCount();
  void  GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info);
  bool  GetSubtitleVisible();
  TextCacheStruct_t* GetTeletextCache();
  std::string GetRadioText(unsigned int line);
  int64_t GetTime() const;
  int64_t GetTotalTime() const;
  int   GetVideoStream();
  int   GetVideoStreamCount();
  void  GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo &info);
  bool  HasAudio() const;
  bool  HasMenu() const;
  bool  HasVideo() const;
  bool  HasRDS() const;
  bool  IsCaching() const;
  bool  IsInMenu() const;
  bool  IsPaused();
  bool  IsPausedPlayback();
  bool  IsPassthrough() const;
  bool  IsPlaying() const;
  bool  IsPlayingAudio() const;
  bool  IsPlayingVideo() const;
  bool  IsPlayingRDS() const;
  bool  IsRecording() const;
  void  LoadPage(int p, int sp, unsigned char* buffer);
  bool  OnAction(const CAction &action);
  void  OnNothingToQueueNotify();
  void  Pause();
  bool  QueueNextFile(const CFileItem &file);
  bool  Record(bool bOnOff);
  void  Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false);
  int   SeekChapter(int iChapter);
  void  SeekPercentage(float fPercent = 0);
  bool  SeekScene(bool bPlus = true);
  void  SeekTime(int64_t iTime = 0);
  void  SeekTimeRelative(int64_t iTime = 0);
  void  SetAudioStream(int iStream);
  void  SetAVDelay(float fValue = 0.0f);
  void  SetDynamicRangeCompression(long drc);
  void  SetMute(bool bOnOff);
  bool  SetPlayerState(const std::string& state);
  void  SetSubtitle(int iStream);
  void  SetSubTitleDelay(float fValue = 0.0f);
  void  SetSubtitleVisible(bool bVisible);
  void  SetTime(int64_t time);
  void  SetTotalTime(int64_t time);
  void  SetVideoStream(int iStream);
  void  SetVolume(float volume);
  bool  SwitchChannel(const PVR::CPVRChannelPtr &channel);
  void  SetSpeed(float speed);
  bool SupportsTempo();

  protected:
    std::shared_ptr<IPlayer> GetInternal() const;
};
