/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include <memory>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "windowing/Resolution.h"
#include "cores/IPlayer.h"
#include "SeekHandler.h"

class CAction;
class CPlayerCoreFactory;
class CPlayerOptions;
class CStreamDetails;

struct AudioStreamInfo;
struct VideoStreamInfo;
struct SubtitleStreamInfo;
struct TextCacheStruct_t;

class CApplicationPlayer
{
public:
  CApplicationPlayer();

  // player management
  void ClosePlayer();
  std::string GetCurrentPlayer();
  float GetPlaySpeed();
  float GetPlayTempo();
  bool HasPlayer() const;
  bool OpenFile(const CFileItem& item, const CPlayerOptions& options,
                const CPlayerCoreFactory &factory,
                const std::string &playerName, IPlayerCallback& callback);
  void OpenNext(const CPlayerCoreFactory &factory);
  void SetPlaySpeed(float speed);
  void SetTempo(float tempo);
  void FrameAdvance(int frames);

  void FrameMove();
  void Render(bool clear, uint32_t alpha = 255, bool gui = true);
  void FlushRenderer();
  void SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch);
  float GetRenderAspectRatio();
  void TriggerUpdateResolution();
  bool IsRenderingVideo();
  bool IsRenderingGuiLayer();
  bool IsRenderingVideoLayer();
  bool Supports(EINTERLACEMETHOD method);
  EINTERLACEMETHOD GetDeinterlacingMethodDefault();
  bool Supports(ESCALINGMETHOD method);
  bool Supports(ERENDERFEATURE feature);
  unsigned int RenderCaptureAlloc();
  void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags = 0);
  void RenderCaptureRelease(unsigned int captureId);
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size);
  bool IsExternalPlaying();

  // proxy calls
  void AddSubtitle(const std::string& strSubPath);
  bool CanPause();
  bool CanSeek();
  void DoAudioWork();
  void GetAudioCapabilities(std::vector<int> &audioCaps);
  int GetAudioStream();
  int GetAudioStreamCount();
  void GetAudioStreamInfo(int index, AudioStreamInfo &info);
  int GetCacheLevel() const;
  float GetCachePercentage() const;
  int GetChapterCount();
  int GetChapter();
  void GetChapterName(std::string& strChapterName, int chapterIdx=-1);
  int64_t GetChapterPos(int chapterIdx=-1);
  float GetPercentage() const;
  std::string GetPlayerState();
  int GetPreferredPlaylist() const;
  int GetSubtitle();
  void GetSubtitleCapabilities(std::vector<int> &subCaps);
  int GetSubtitleCount();
  void GetSubtitleStreamInfo(int index, SubtitleStreamInfo &info);
  bool GetSubtitleVisible();
  std::shared_ptr<TextCacheStruct_t> GetTeletextCache();
  std::string GetRadioText(unsigned int line);
  int64_t GetTime() const;
  int64_t GetMinTime() const;
  int64_t GetMaxTime() const;
  time_t GetStartTime() const;
  int64_t GetTotalTime() const;
  int GetVideoStream();
  int GetVideoStreamCount();
  void GetVideoStreamInfo(int streamId, VideoStreamInfo &info);
  int GetPrograms(std::vector<ProgramInfo>& programs);
  void SetProgram(int progId);
  int GetProgramsCount();
  bool HasAudio() const;
  bool HasMenu() const;
  bool HasVideo() const;
  bool HasGame() const;
  bool HasRDS() const;
  bool IsCaching() const;
  bool IsInMenu() const;
  bool IsPaused();
  bool IsPausedPlayback();
  bool IsPassthrough() const;
  bool IsPlaying() const;
  bool IsPlayingAudio() const;
  bool IsPlayingVideo() const;
  bool IsPlayingGame() const;
  bool IsPlayingRDS() const;
  void LoadPage(int p, int sp, unsigned char* buffer);
  bool OnAction(const CAction &action);
  void OnNothingToQueueNotify();
  void Pause();
  bool QueueNextFile(const CFileItem &file);
  void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false);
  int SeekChapter(int iChapter);
  void SeekPercentage(float fPercent = 0);
  bool SeekScene(bool bPlus = true);
  void SeekTime(int64_t iTime = 0);
  void SeekTimeRelative(int64_t iTime = 0);
  void SetAudioStream(int iStream);
  void SetAVDelay(float fValue = 0.0f);
  void SetDynamicRangeCompression(long drc);
  void SetMute(bool bOnOff);
  bool SetPlayerState(const std::string& state);
  void SetSubtitle(int iStream);
  void SetSubTitleDelay(float fValue = 0.0f);
  void SetSubtitleVisible(bool bVisible);
  void SetTime(int64_t time);
  void SetTotalTime(int64_t time);
  void SetVideoStream(int iStream);
  void SetVolume(float volume);
  void SetSpeed(float speed);
  bool SupportsTempo();

  CVideoSettings GetVideoSettings();
  void SetVideoSettings(CVideoSettings& settings);

  CSeekHandler& GetSeekHandler();

  void SetUpdateStreamDetails();

private:
  std::shared_ptr<IPlayer> GetInternal() const;
  void CreatePlayer(const CPlayerCoreFactory &factory, const std::string &player, IPlayerCallback& callback);
  void CloseFile(bool reopen = false);

  std::shared_ptr<IPlayer> m_pPlayer;
  CCriticalSection m_playerLock;
  CSeekHandler m_seekHandler;

  // cache player state
  XbmcThreads::EndTime m_audioStreamUpdate;
  int m_iAudioStream;
  XbmcThreads::EndTime m_videoStreamUpdate;
  int m_iVideoStream;
  XbmcThreads::EndTime m_subtitleStreamUpdate;
  int m_iSubtitleStream;

  struct SNextItem
  {
    std::shared_ptr<CFileItem> pItem;
    CPlayerOptions options = {};
    std::string playerName;
    IPlayerCallback *callback = nullptr;
  } m_nextItem;
};
