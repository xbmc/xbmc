/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayer.h"

#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "video/VideoFileItemClassify.h"

#include <mutex>

using namespace KODI;
using namespace std::chrono_literals;

std::shared_ptr<const IPlayer> CApplicationPlayer::GetInternal() const
{
  std::unique_lock<CCriticalSection> lock(m_playerLock);
  return m_pPlayer;
}

std::shared_ptr<IPlayer> CApplicationPlayer::GetInternal()
{
  std::unique_lock<CCriticalSection> lock(m_playerLock);
  return m_pPlayer;
}

void CApplicationPlayer::ClosePlayer()
{
  m_nextItem.pItem.reset();
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CloseFile();
    ResetPlayer();
  }
}

void CApplicationPlayer::ResetPlayer()
{
  // we need to do this directly on the member
  std::unique_lock<CCriticalSection> lock(m_playerLock);
  m_pPlayer.reset();
}

void CApplicationPlayer::CloseFile(bool reopen)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->CloseFile(reopen);
  }
}

void CApplicationPlayer::CreatePlayer(const CPlayerCoreFactory &factory, const std::string &player, IPlayerCallback& callback)
{
  std::unique_lock<CCriticalSection> lock(m_playerLock);
  if (!m_pPlayer)
  {
    CDataCacheCore::GetInstance().Reset();
    m_pPlayer = factory.CreatePlayer(player, callback);
  }
}

std::string CApplicationPlayer::GetCurrentPlayer() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->m_name;
  }
  return "";
}

bool CApplicationPlayer::OpenFile(const CFileItem& item, const CPlayerOptions& options,
                                  const CPlayerCoreFactory &factory,
                                  const std::string &playerName, IPlayerCallback& callback)
{
  // get player type
  std::string newPlayer;
  if (!playerName.empty())
    newPlayer = playerName;
  else
    newPlayer = factory.GetDefaultPlayer(item);

  // check if we need to close current player
  // VideoPlayer can open a new file while playing
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player && player->IsPlaying())
  {
    bool needToClose = false;

    if (item.IsDiscImage() || VIDEO::IsDVDFile(item))
      needToClose = true;

    if (player->m_name != newPlayer)
      needToClose = true;

    if (player->m_type != "video" && player->m_type != "remote")
      needToClose = true;

    if (needToClose)
    {
      m_nextItem.pItem = std::make_shared<CFileItem>(item);
      m_nextItem.options = options;
      m_nextItem.playerName = newPlayer;
      m_nextItem.callback = &callback;

      CloseFile();
      if (player->m_name != newPlayer)
      {
        std::unique_lock<CCriticalSection> lock(m_playerLock);
        m_pPlayer.reset();
      }
      return true;
    }
  }
  else if (player && player->m_name != newPlayer)
  {
    CloseFile();
    {
      std::unique_lock<CCriticalSection> lock(m_playerLock);
      m_pPlayer.reset();
      player.reset();
    }
  }

  if (!player)
  {
    CreatePlayer(factory, newPlayer, callback);
    player = GetInternal();
    if (!player)
      return false;
  }

  bool ret = player->OpenFile(item, options);

  m_nextItem.pItem.reset();

  // reset caching timers
  m_audioStreamUpdate.SetExpired();
  m_videoStreamUpdate.SetExpired();
  m_subtitleStreamUpdate.SetExpired();

  return ret;
}

void CApplicationPlayer::OpenNext(const CPlayerCoreFactory &factory)
{
  if (m_nextItem.pItem)
  {
    OpenFile(*m_nextItem.pItem, m_nextItem.options,
             factory,
             m_nextItem.playerName, *m_nextItem.callback);
    m_nextItem.pItem.reset();
  }
}

bool CApplicationPlayer::HasPlayer() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return player != nullptr;
}

int CApplicationPlayer::GetChapter() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapter();
  else
    return -1;
}

int CApplicationPlayer::GetChapterCount() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapterCount();
  else
    return 0;
}

void CApplicationPlayer::GetChapterName(std::string& strChapterName, int chapterIdx) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetChapterName(strChapterName, chapterIdx);
}

int64_t CApplicationPlayer::GetChapterPos(int chapterIdx) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetChapterPos(chapterIdx);

  return -1;
}

bool CApplicationPlayer::HasAudio() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasAudio());
}

bool CApplicationPlayer::HasVideo() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasVideo());
}

bool CApplicationPlayer::HasGame() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasGame());
}

PLAYLIST::Id CApplicationPlayer::GetPreferredPlaylist() const
{
  if (IsPlayingVideo())
    return PLAYLIST::TYPE_VIDEO;

  if (IsPlayingAudio())
    return PLAYLIST::TYPE_MUSIC;

  return PLAYLIST::TYPE_NONE;
}

bool CApplicationPlayer::HasRDS() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->HasRDS());
}

bool CApplicationPlayer::IsPaused() const
{
  return (GetPlaySpeed() == 0);
}

bool CApplicationPlayer::IsPlaying() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsPlaying());
}

bool CApplicationPlayer::IsPausedPlayback() const
{
  return (IsPlaying() && (GetPlaySpeed() == 0));
}

bool CApplicationPlayer::IsPlayingAudio() const
{
  return (IsPlaying() && !HasVideo() && HasAudio());
}

bool CApplicationPlayer::IsPlayingVideo() const
{
  return (IsPlaying() && HasVideo());
}

bool CApplicationPlayer::IsPlayingGame() const
{
  return (IsPlaying() && HasGame());
}

bool CApplicationPlayer::IsPlayingRDS() const
{
  return (IsPlaying() && HasRDS());
}

void CApplicationPlayer::Pause()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->Pause();
  }
}

void CApplicationPlayer::SetMute(bool bOnOff)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetMute(bOnOff);
}

void CApplicationPlayer::SetVolume(float volume)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetVolume(volume);
}

void CApplicationPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->Seek(bPlus, bLargeStep, bChapterOverride);
}

void CApplicationPlayer::SeekPercentage(float fPercent)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekPercentage(fPercent);
}

bool CApplicationPlayer::IsPassthrough() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsPassthrough());
}

bool CApplicationPlayer::CanSeek() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->CanSeek());
}

bool CApplicationPlayer::SeekScene(bool bPlus)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SeekScene(bPlus));
}

void CApplicationPlayer::SeekTime(int64_t iTime)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekTime(iTime);
}

void CApplicationPlayer::SeekTimeRelative(int64_t iTime)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    // use relative seeking if implemented by player
    if (!player->SeekTimeRelative(iTime))
    {
      int64_t abstime = GetTime() + iTime;
      player->SeekTime(abstime);
    }
  }
}

int64_t CApplicationPlayer::GetTime() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CDataCacheCore::GetInstance().GetPlayTime();
  else
    return 0;
}

int64_t CApplicationPlayer::GetMinTime() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CDataCacheCore::GetInstance().GetMinTime();
  else
    return 0;
}

int64_t CApplicationPlayer::GetMaxTime() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CDataCacheCore::GetInstance().GetMaxTime();
  else
    return 0;
}

time_t CApplicationPlayer::GetStartTime() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CDataCacheCore::GetInstance().GetStartTime();
  else
    return 0;
}

int64_t CApplicationPlayer::GetTotalTime() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    int64_t total = CDataCacheCore::GetInstance().GetMaxTime() - CDataCacheCore::GetInstance().GetMinTime();
    return total;
  }
  else
    return 0;
}

bool CApplicationPlayer::IsCaching() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsCaching());
}

bool CApplicationPlayer::IsInMenu() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->IsInMenu());
}

MenuType CApplicationPlayer::GetSupportedMenuType() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (!player)
  {
    return MenuType::NONE;
  }
  return player->GetSupportedMenuType();
}

int CApplicationPlayer::GetCacheLevel() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetCacheLevel();
  else
    return 0;
}

int CApplicationPlayer::GetSubtitleCount() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetSubtitleCount();
  else
    return 0;
}

int CApplicationPlayer::GetAudioStream()
{
  if (!m_audioStreamUpdate.IsTimePast())
    return m_iAudioStream;

  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iAudioStream = player->GetAudioStream();
    m_audioStreamUpdate.Set(1000ms);
    return m_iAudioStream;
  }
  else
    return 0;
}

int CApplicationPlayer::GetSubtitle()
{
  if (!m_subtitleStreamUpdate.IsTimePast())
    return m_iSubtitleStream;

  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iSubtitleStream = player->GetSubtitle();
    m_subtitleStreamUpdate.Set(1000ms);
    return m_iSubtitleStream;
  }
  else
    return 0;
}

bool CApplicationPlayer::GetSubtitleVisible() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  return player && player->GetSubtitleVisible();
}

bool CApplicationPlayer::CanPause() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  return (player && player->CanPause());
}

bool CApplicationPlayer::HasTeletextCache() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->HasTeletextCache();
  else
    return false;
}

std::shared_ptr<TextCacheStruct_t> CApplicationPlayer::GetTeletextCache()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetTeletextCache();
  else
    return {};
}

float CApplicationPlayer::GetPercentage() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    float fPercent = CDataCacheCore::GetInstance().GetPlayPercentage();
    return std::max(0.0f, std::min(fPercent, 100.0f));
  }
  else
    return 0.0;
}

float CApplicationPlayer::GetCachePercentage() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetCachePercentage();
  else
    return 0.0;
}

void CApplicationPlayer::SetSpeed(float speed)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetSpeed(speed);
}

void CApplicationPlayer::SetTempo(float tempo)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetTempo(tempo);
}

void CApplicationPlayer::FrameAdvance(int frames)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->FrameAdvance(frames);
}

std::string CApplicationPlayer::GetPlayerState()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetPlayerState();
  else
    return "";
}

bool CApplicationPlayer::QueueNextFile(const CFileItem &file)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->QueueNextFile(file));
}

bool CApplicationPlayer::SetPlayerState(const std::string& state)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SetPlayerState(state));
}

void CApplicationPlayer::OnNothingToQueueNotify()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OnNothingToQueueNotify();
}

void CApplicationPlayer::GetVideoStreamInfo(int streamId, VideoStreamInfo& info) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetVideoStreamInfo(streamId, info);
}

void CApplicationPlayer::GetAudioStreamInfo(int index, AudioStreamInfo& info) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetAudioStreamInfo(index, info);
}

int CApplicationPlayer::GetPrograms(std::vector<ProgramInfo> &programs)
{
  int ret = 0;
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    ret = player->GetPrograms(programs);
  return ret;
}

void CApplicationPlayer::SetProgram(int progId)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetProgram(progId);
}

int CApplicationPlayer::GetProgramsCount() const
{
  int ret = 0;
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    ret = player->GetProgramsCount();
  return ret;
}

bool CApplicationPlayer::OnAction(const CAction &action)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->OnAction(action));
}

int CApplicationPlayer::GetAudioStreamCount() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetAudioStreamCount();
  else
    return 0;
}

int CApplicationPlayer::GetVideoStream()
{
  if (!m_videoStreamUpdate.IsTimePast())
    return m_iVideoStream;

  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iVideoStream = player->GetVideoStream();
    m_videoStreamUpdate.Set(1000ms);
    return m_iVideoStream;
  }
  else
    return 0;
}

int CApplicationPlayer::GetVideoStreamCount() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetVideoStreamCount();
  else
    return 0;
}

void CApplicationPlayer::SetAudioStream(int iStream)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetAudioStream(iStream);
    m_iAudioStream = iStream;
    m_audioStreamUpdate.Set(1000ms);
  }
}

void CApplicationPlayer::GetSubtitleStreamInfo(int index, SubtitleStreamInfo& info) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetSubtitleStreamInfo(index, info);
}

void CApplicationPlayer::SetSubtitle(int iStream)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitle(iStream);
    m_iSubtitleStream = iStream;
    m_subtitleStreamUpdate.Set(1000ms);
  }
}

void CApplicationPlayer::SetSubtitleVisible(bool bVisible)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitleVisible(bVisible);
  }
}

void CApplicationPlayer::SetSubtitleVerticalPosition(int value, bool save)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitleVerticalPosition(value, save);
  }
}

void CApplicationPlayer::SetTime(int64_t time)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->SetTime(time);
}

void CApplicationPlayer::SetTotalTime(int64_t time)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetTotalTime(time);
}

void CApplicationPlayer::SetVideoStream(int iStream)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetVideoStream(iStream);
    m_iVideoStream = iStream;
    m_videoStreamUpdate.Set(1000ms);
  }
}

void CApplicationPlayer::AddSubtitle(const std::string& strSubPath)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->AddSubtitle(strSubPath);
}

void CApplicationPlayer::SetSubTitleDelay(float fValue)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetSubTitleDelay(fValue);
}

void CApplicationPlayer::SetAVDelay(float fValue)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetAVDelay(fValue);
}

void CApplicationPlayer::SetDynamicRangeCompression(long drc)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetDynamicRangeCompression(drc);
}

void CApplicationPlayer::LoadPage(int p, int sp, unsigned char* buffer)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->LoadPage(p, sp, buffer);
}

void CApplicationPlayer::GetAudioCapabilities(std::vector<int>& audioCaps) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetAudioCapabilities(audioCaps);
}

void CApplicationPlayer::GetSubtitleCapabilities(std::vector<int>& subCaps) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    player->GetSubtitleCapabilities(subCaps);
}

int  CApplicationPlayer::SeekChapter(int iChapter)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->SeekChapter(iChapter);
  else
    return 0;
}

void CApplicationPlayer::SetPlaySpeed(float speed)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (!player)
    return;

  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;

  SetSpeed(speed);
}

float CApplicationPlayer::GetPlaySpeed() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return CDataCacheCore::GetInstance().GetSpeed();
  }
  else
    return 0;
}

float CApplicationPlayer::GetPlayTempo() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return CDataCacheCore::GetInstance().GetTempo();
  }
  else
    return 0;
}

bool CApplicationPlayer::SupportsTempo() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->SupportsTempo();
  else
    return false;
}

void CApplicationPlayer::FrameMove()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    if (CDataCacheCore::GetInstance().IsPlayerStateChanged())
      // CApplicationMessenger would be overhead because we are already in gui thread
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_STATE_CHANGED);
  }
}

void CApplicationPlayer::Render(bool clear, uint32_t alpha, bool gui)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->Render(clear, alpha, gui);
}

void CApplicationPlayer::FlushRenderer()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->FlushRenderer();
}

void CApplicationPlayer::SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetRenderViewMode(mode, zoom, par, shift, stretch);
}

float CApplicationPlayer::GetRenderAspectRatio() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetRenderAspectRatio();
  else
    return 1.0;
}

bool CApplicationPlayer::GetRects(CRect& source, CRect& dest, CRect& view) const
{
  const std::shared_ptr<const IPlayer> player{GetInternal()};
  if (player)
  {
    player->GetRects(source, dest, view);
    return true;
  }
  else
    return false;
}

unsigned int CApplicationPlayer::GetOrientation() const
{
  const std::shared_ptr<const IPlayer> player{GetInternal()};
  if (player)
    return player->GetOrientation();
  else
    return 0;
}

void CApplicationPlayer::TriggerUpdateResolution()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->TriggerUpdateResolution();
}

bool CApplicationPlayer::IsRenderingVideo() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->IsRenderingVideo();
  else
    return false;
}

bool CApplicationPlayer::IsRenderingGuiLayer() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CServiceBroker::GetDataCacheCore().GetGuiRender();
  else
    return false;
}

bool CApplicationPlayer::IsRenderingVideoLayer() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return CServiceBroker::GetDataCacheCore().GetVideoRender();
  else
    return false;
}

bool CApplicationPlayer::Supports(EINTERLACEMETHOD method) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->Supports(method);
  else
    return false;
}

EINTERLACEMETHOD CApplicationPlayer::GetDeinterlacingMethodDefault() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->GetDeinterlacingMethodDefault();
  else
    return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE;
}

bool CApplicationPlayer::Supports(ESCALINGMETHOD method) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->Supports(method);
  else
    return false;
}

bool CApplicationPlayer::Supports(ERENDERFEATURE feature) const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->Supports(feature);
  else
    return false;
}

unsigned int CApplicationPlayer::RenderCaptureAlloc()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->RenderCaptureAlloc();
  else
    return 0;
}

void CApplicationPlayer::RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->RenderCapture(captureId, width, height, flags);
}

void CApplicationPlayer::RenderCaptureRelease(unsigned int captureId)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->RenderCaptureRelease(captureId);
}

bool CApplicationPlayer::RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->RenderCaptureGetPixels(captureId, millis, buffer, size);
  else
    return false;
}

bool CApplicationPlayer::IsExternalPlaying() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    if (player->IsPlaying() && player->m_type == "external")
      return true;
  }
  return false;
}

bool CApplicationPlayer::IsRemotePlaying() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    if (player->IsPlaying() && player->m_type == "remote")
      return true;
  }
  return false;
}

std::string CApplicationPlayer::GetName() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->m_name;
  }
  return {};
}

CVideoSettings CApplicationPlayer::GetVideoSettings() const
{
  std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
  {
    return player->GetVideoSettings();
  }
  return CVideoSettings();
}

void CApplicationPlayer::SetVideoSettings(CVideoSettings& settings)
{
  std::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    return player->SetVideoSettings(settings);
  }
}

CSeekHandler& CApplicationPlayer::GetSeekHandler()
{
  return m_seekHandler;
}

const CSeekHandler& CApplicationPlayer::GetSeekHandler() const
{
  return m_seekHandler;
}

void CApplicationPlayer::SetUpdateStreamDetails()
{
  std::shared_ptr<IPlayer> player = GetInternal();
  CVideoPlayer* vp = dynamic_cast<CVideoPlayer*>(player.get());
  if (vp)
    vp->SetUpdateStreamDetails();
}

bool CApplicationPlayer::HasGameAgent() const
{
  const std::shared_ptr<const IPlayer> player = GetInternal();
  if (player)
    return player->HasGameAgent();

  return false;
}

int CApplicationPlayer::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  const auto& advSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const auto delay = this->GetVideoSettings().m_SubtitleDelay;
  const auto range = advSettings->m_videoSubsDelayRange;
  return static_cast<int>(0.5f + (delay + range) / (2.f * range) * 100.0f);
}

int CApplicationPlayer::GetAudioDelay() const
{
  // converts audio delay to a percentage
  const auto& advSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const auto delay = this->GetVideoSettings().m_AudioDelay;
  const auto range = advSettings->m_videoAudioDelayRange;
  return static_cast<int>(0.5f + (delay + range) / (2.f * range) * 100.0f);
}
