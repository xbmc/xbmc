/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SeekHandler.h"
#include "application/IApplicationComponent.h"
#include "cores/IPlayer.h"
#include "cores/MenuType.h"
#include "playlists/PlayListTypes.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"

#include <memory>
#include <string>
#include <vector>

class CAction;
class CPlayerCoreFactory;
class CPlayerOptions;
class CStreamDetails;

struct AudioStreamInfo;
struct VideoStreamInfo;
struct SubtitleStreamInfo;
struct TextCacheStruct_t;

class CApplicationPlayer : public IApplicationComponent
{
public:
  CApplicationPlayer() = default;

  // player management
  void ClosePlayer();
  void ResetPlayer();
  std::string GetCurrentPlayer() const;
  float GetPlaySpeed() const;
  float GetPlayTempo() const;
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
  float GetRenderAspectRatio() const;
  bool GetRects(CRect& source, CRect& dest, CRect& view) const;
  unsigned int GetOrientation() const;
  void TriggerUpdateResolution();
  bool IsRenderingVideo() const;
  bool IsRenderingGuiLayer() const;
  bool IsRenderingVideoLayer() const;
  bool Supports(EINTERLACEMETHOD method) const;
  EINTERLACEMETHOD GetDeinterlacingMethodDefault() const;
  bool Supports(ESCALINGMETHOD method) const;
  bool Supports(ERENDERFEATURE feature) const;
  unsigned int RenderCaptureAlloc();
  void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags = 0);
  void RenderCaptureRelease(unsigned int captureId);
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size);
  bool IsExternalPlaying() const;
  bool IsRemotePlaying() const;

  /*!
   * \brief Get the name of the player in use
   * \return the player name if a player is active, otherwise it returns an empty string
   */
  std::string GetName() const;

  // proxy calls
  void AddSubtitle(const std::string& strSubPath);
  bool CanPause() const;
  bool CanSeek() const;
  int GetAudioDelay() const;
  void GetAudioCapabilities(std::vector<int>& audioCaps) const;
  int GetAudioStream();
  int GetAudioStreamCount() const;
  void GetAudioStreamInfo(int index, AudioStreamInfo& info) const;
  int GetCacheLevel() const;
  float GetCachePercentage() const;
  int GetChapterCount() const;
  int GetChapter() const;
  void GetChapterName(std::string& strChapterName, int chapterIdx = -1) const;
  int64_t GetChapterPos(int chapterIdx = -1) const;
  float GetPercentage() const;
  std::string GetPlayerState();
  PLAYLIST::Id GetPreferredPlaylist() const;
  int GetSubtitleDelay() const;
  int GetSubtitle();
  void GetSubtitleCapabilities(std::vector<int>& subCaps) const;
  int GetSubtitleCount() const;
  void GetSubtitleStreamInfo(int index, SubtitleStreamInfo& info) const;
  bool GetSubtitleVisible() const;
  bool HasTeletextCache() const;
  std::shared_ptr<TextCacheStruct_t> GetTeletextCache();
  int64_t GetTime() const;
  int64_t GetMinTime() const;
  int64_t GetMaxTime() const;
  time_t GetStartTime() const;
  int64_t GetTotalTime() const;
  int GetVideoStream();
  int GetVideoStreamCount() const;
  void GetVideoStreamInfo(int streamId, VideoStreamInfo& info) const;
  int GetPrograms(std::vector<ProgramInfo>& programs);
  void SetProgram(int progId);
  int GetProgramsCount() const;
  bool HasAudio() const;

  /*!
   * \brief Get the supported menu type
   * \return The supported menu type
  */
  MenuType GetSupportedMenuType() const;

  bool HasVideo() const;
  bool HasGame() const;
  bool HasRDS() const;
  bool IsCaching() const;
  bool IsInMenu() const;
  bool IsPaused() const;
  bool IsPausedPlayback() const;
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

  /*!
   * \brief Set the subtitle vertical position,
   * it depends on current screen resolution
   * \param value The subtitle position in pixels
   * \param save If true, the value will be saved to resolution info
   */
  void SetSubtitleVerticalPosition(const int value, bool save);

  void SetTime(int64_t time);
  void SetTotalTime(int64_t time);
  void SetVideoStream(int iStream);
  void SetVolume(float volume);
  void SetSpeed(float speed);
  bool SupportsTempo() const;

  CVideoSettings GetVideoSettings() const;
  void SetVideoSettings(CVideoSettings& settings);

  CSeekHandler& GetSeekHandler();
  const CSeekHandler& GetSeekHandler() const;

  void SetUpdateStreamDetails();

  /*!
   * \copydoc IPlayer::HasGameAgent
   */
  bool HasGameAgent() const;

private:
  std::shared_ptr<const IPlayer> GetInternal() const;
  std::shared_ptr<IPlayer> GetInternal();
  void CreatePlayer(const CPlayerCoreFactory &factory, const std::string &player, IPlayerCallback& callback);
  void CloseFile(bool reopen = false);

  std::shared_ptr<IPlayer> m_pPlayer;
  mutable CCriticalSection m_playerLock;
  CSeekHandler m_seekHandler;

  // cache player state
  XbmcThreads::EndTime<> m_audioStreamUpdate;
  int m_iAudioStream;
  XbmcThreads::EndTime<> m_videoStreamUpdate;
  int m_iVideoStream;
  XbmcThreads::EndTime<> m_subtitleStreamUpdate;
  int m_iSubtitleStream;

  struct SNextItem
  {
    std::shared_ptr<CFileItem> pItem;
    CPlayerOptions options = {};
    std::string playerName;
    IPlayerCallback *callback = nullptr;
  } m_nextItem;
};
