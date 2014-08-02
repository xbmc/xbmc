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

#include "ApplicationPlayer.h"
#include "cores/IPlayer.h"
#include "Application.h"
#include "settings/MediaSettings.h"

CApplicationPlayer::CApplicationPlayer()
{
  m_iPlayerOPSeq = 0;
  m_eCurrentPlayer = EPC_NONE;
}

boost::shared_ptr<IPlayer> CApplicationPlayer::GetInternal() const
{
  CSingleLock lock(m_player_lock);
  return m_pPlayer;
}

void CApplicationPlayer::ClosePlayer()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    CloseFile();
    // we need to do this directly on the member
    CSingleLock lock(m_player_lock);
    m_pPlayer.reset();
  }
}

void CApplicationPlayer::CloseFile(bool reopen)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    ++m_iPlayerOPSeq;
    player->CloseFile(reopen);
  }
}

void CApplicationPlayer::ClosePlayerGapless(PLAYERCOREID newCore)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (!player)
    return;

  bool gaplessSupported = (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer == EPC_PAPLAYER);
  gaplessSupported = gaplessSupported && (m_eCurrentPlayer == newCore);
  if (!gaplessSupported)
  {
    ClosePlayer();
  }
  else
  {
    // XXX: we had to stop the previous playing item, it was done in dvdplayer::OpenFile.
    // but in paplayer::OpenFile, it sometimes just fade in without call CloseFile.
    // but if we do not stop it, we can not distingush callbacks from previous
    // item and current item, it will confused us then we can not make correct delay
    // callback after the starting state.
    CloseFile(true);
  }
}

void CApplicationPlayer::CreatePlayer(PLAYERCOREID newCore, IPlayerCallback& callback)
{
  CSingleLock lock(m_player_lock);
  if (!m_pPlayer)
  {
    m_eCurrentPlayer = newCore;
    m_pPlayer.reset(CPlayerCoreFactory::Get().CreatePlayer(newCore, callback));
  }
}

PlayBackRet CApplicationPlayer::OpenFile(const CFileItem& item, const CPlayerOptions& options)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  PlayBackRet iResult = PLAYBACK_FAIL;
  if (player)
  {
    // op seq for detect cancel (CloseFile be called or OpenFile be called again) during OpenFile.
    unsigned int startingSeq = ++m_iPlayerOPSeq;

    iResult = player->OpenFile(item, options) ? PLAYBACK_OK : PLAYBACK_FAIL;
    // check whether the OpenFile was canceled by either CloseFile or another OpenFile.
    if (m_iPlayerOPSeq != startingSeq)
      iResult = PLAYBACK_CANCELED;
  }
  return iResult;
}

bool CApplicationPlayer::HasPlayer() const 
{ 
  boost::shared_ptr<IPlayer> player = GetInternal();
  return player != NULL; 
}

int CApplicationPlayer::GetChapter()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetChapter();
  else 
    return -1;
}

int CApplicationPlayer::GetChapterCount()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetChapterCount();
  else 
    return 0;
}

void CApplicationPlayer::GetChapterName(std::string& strChapterName)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetChapterName(strChapterName);
}

bool CApplicationPlayer::HasAudio() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->HasAudio());
}

bool CApplicationPlayer::HasVideo() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->HasVideo());
}

bool CApplicationPlayer::IsPaused() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsPaused());
}

bool CApplicationPlayer::IsPlaying() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsPlaying());
}

bool CApplicationPlayer::IsPausedPlayback() const
{
  return (IsPlaying() && IsPaused());
}

bool CApplicationPlayer::IsPlayingAudio() const
{
  return (IsPlaying() && !HasVideo() && HasAudio());
}

bool CApplicationPlayer::IsPlayingVideo() const
{
  return (IsPlaying() && HasVideo());
}

void CApplicationPlayer::Pause()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->Pause();
}

bool CApplicationPlayer::ControlsVolume() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->ControlsVolume());
}

void CApplicationPlayer::SetMute(bool bOnOff)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetMute(bOnOff);
}

void CApplicationPlayer::SetVolume(float volume)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetVolume(volume);
}

void CApplicationPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->Seek(bPlus, bLargeStep, bChapterOverride);
}

void CApplicationPlayer::SeekPercentage(float fPercent)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekPercentage(fPercent);
}

bool CApplicationPlayer::IsPassthrough() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsPassthrough());
}

bool CApplicationPlayer::CanSeek()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->CanSeek());
}

bool CApplicationPlayer::SeekScene(bool bPlus)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SeekScene(bPlus));
}

void CApplicationPlayer::SeekTime(int64_t iTime)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SeekTime(iTime);
}

std::string CApplicationPlayer::GetPlayingTitle()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetPlayingTitle();
  else
    return "";
}

int64_t CApplicationPlayer::GetTime() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetTime();
  else
    return 0;
}

bool CApplicationPlayer::IsCaching() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsCaching());
}

bool CApplicationPlayer::IsInMenu() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsInMenu());
}

bool CApplicationPlayer::HasMenu() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->HasMenu());
}

int CApplicationPlayer::GetCacheLevel() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetCacheLevel();
  else
    return 0;
}

int CApplicationPlayer::GetSubtitleCount()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetSubtitleCount();
  else
    return 0;
}

int CApplicationPlayer::GetAudioStream()
{
  if (!m_audioStreamUpdate.IsTimePast())
    return m_iAudioStream;

  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iAudioStream = player->GetAudioStream();
    m_audioStreamUpdate.Set(1000);
    return m_iAudioStream;
  }
  else
    return 0;
}

int CApplicationPlayer::GetSubtitle()
{
  if (!m_subtitleStreamUpdate.IsTimePast())
    return m_iSubtitleStream;

  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    m_iSubtitleStream = player->GetSubtitle();
    m_subtitleStreamUpdate.Set(1000);
    return m_iSubtitleStream;
  }
  else
    return 0;
}

bool CApplicationPlayer::GetSubtitleVisible()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->GetSubtitleVisible());
}

bool CApplicationPlayer::CanRecord()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->CanRecord());
}

bool CApplicationPlayer::CanPause()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->CanPause());
}

bool CApplicationPlayer::IsRecording() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->IsRecording());
}

TextCacheStruct_t* CApplicationPlayer::GetTeletextCache()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetTeletextCache();
  else
    return NULL;
}

int64_t CApplicationPlayer::GetTotalTime() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetTotalTime();
  else
    return 0;
}

float CApplicationPlayer::GetPercentage() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetPercentage();
  else
    return 0.0;
}

float CApplicationPlayer::GetCachePercentage() const
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetCachePercentage();
  else
    return 0.0;
}

void CApplicationPlayer::ToFFRW(int iSpeed)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->ToFFRW(iSpeed);
}

void CApplicationPlayer::DoAudioWork()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->DoAudioWork();
}

std::string CApplicationPlayer::GetPlayerState()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetPlayerState();
  else
    return "";
}

bool CApplicationPlayer::QueueNextFile(const CFileItem &file)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->QueueNextFile(file));
}

bool CApplicationPlayer::GetStreamDetails(CStreamDetails &details)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->GetStreamDetails(details));
}

bool CApplicationPlayer::SetPlayerState(const std::string& state)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SetPlayerState(state));
}

void CApplicationPlayer::OnNothingToQueueNotify()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OnNothingToQueueNotify();
}

void CApplicationPlayer::GetVideoStreamInfo(SPlayerVideoStreamInfo &info)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetVideoStreamInfo(info);
}

void CApplicationPlayer::GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetAudioStreamInfo(index, info);
}

bool CApplicationPlayer::OnAction(const CAction &action)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->OnAction(action));
}

bool CApplicationPlayer::Record(bool bOnOff)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->Record(bOnOff));
}

int  CApplicationPlayer::GetAudioStreamCount()
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->GetAudioStreamCount();
  else
    return 0;
}

void CApplicationPlayer::SetAudioStream(int iStream)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetAudioStream(iStream);
    m_iAudioStream = iStream;
    m_audioStreamUpdate.Set(1000);
    CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream = iStream;
  }
}

void CApplicationPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetSubtitleStreamInfo(index, info);
}

void CApplicationPlayer::SetSubtitle(int iStream)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitle(iStream);
    m_iSubtitleStream = iStream;
    m_subtitleStreamUpdate.Set(1000);
    CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleStream = iStream;
  }
}

void CApplicationPlayer::SetSubtitleVisible(bool bVisible)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
  {
    player->SetSubtitleVisible(bVisible);
    CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn = bVisible;
  }
}

int  CApplicationPlayer::AddSubtitle(const std::string& strSubPath)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->AddSubtitle(strSubPath);
  else
    return 0;
}

void CApplicationPlayer::SetSubTitleDelay(float fValue)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetSubTitleDelay(fValue);
}

void CApplicationPlayer::SetAVDelay(float fValue)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetAVDelay(fValue);
}

void CApplicationPlayer::SetDynamicRangeCompression(long drc)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->SetDynamicRangeCompression(drc);
}

bool CApplicationPlayer::SwitchChannel(const PVR::CPVRChannel &channel)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  return (player && player->SwitchChannel(channel));
}

void CApplicationPlayer::LoadPage(int p, int sp, unsigned char* buffer)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->LoadPage(p, sp, buffer);
}

void CApplicationPlayer::GetAudioCapabilities(std::vector<int> &audioCaps)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetAudioCapabilities(audioCaps);
}

void CApplicationPlayer::GetSubtitleCapabilities(std::vector<int> &subCaps)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetSubtitleCapabilities(subCaps);
}

void CApplicationPlayer::GetAudioInfo(std::string& strAudioInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetAudioInfo(strAudioInfo);
}

void CApplicationPlayer::GetVideoInfo(std::string& strVideoInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetVideoInfo(strVideoInfo);
}

void CApplicationPlayer::GetGeneralInfo(std::string& strVideoInfo)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->GetGeneralInfo(strVideoInfo);
}

int  CApplicationPlayer::SeekChapter(int iChapter)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    return player->SeekChapter(iChapter);
  else
    return 0;
}

void CApplicationPlayer::GetRenderFeatures(std::vector<int> &renderFeatures)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OMXGetRenderFeatures(renderFeatures);
}

void CApplicationPlayer::GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OMXGetDeinterlaceMethods(deinterlaceMethods);
}

void CApplicationPlayer::GetDeinterlaceModes(std::vector<int> &deinterlaceModes)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OMXGetDeinterlaceModes(deinterlaceModes);
}

void CApplicationPlayer::GetScalingMethods(std::vector<int> &scalingMethods)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (player)
    player->OMXGetScalingMethods(scalingMethods);
}

void CApplicationPlayer::SetPlaySpeed(int iSpeed, bool bApplicationMuted)
{
  boost::shared_ptr<IPlayer> player = GetInternal();
  if (!player)
    return;

  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
  if (!CanSeek())
    return;
  if (IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (iSpeed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (iSpeed < m_iPlaySpeed))
    )
    {
      iSpeed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    Pause();
  }
  m_iPlaySpeed = iSpeed;

  ToFFRW(m_iPlaySpeed);

  // if player has volume control, set it.
  if (ControlsVolume())
  {
    if (m_iPlaySpeed == 1)
    { // restore volume
      player->SetVolume(g_application.GetVolume(false));
    }
    else
    { // mute volume
      player->SetVolume(VOLUME_MINIMUM);
    }
    player->SetMute(bApplicationMuted);
  }
}

int CApplicationPlayer::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}
