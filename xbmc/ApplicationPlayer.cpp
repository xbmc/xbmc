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

#define VOLUME_MINIMUM 0.0f        // -60dB
#define VOLUME_MAXIMUM 1.0f        // 0dB

CApplicationPlayer::CApplicationPlayer()
{
  m_iPlayerOPSeq = 0;
  m_eCurrentPlayer = EPC_NONE;
}

IPlayer* CApplicationPlayer::GetInternal() const
{
  CSingleLock lock(m_player_lock);
  return m_pPlayer.get();
}

void CApplicationPlayer::ClosePlayer()
{
  IPlayer* player = GetInternal();
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
  IPlayer* player = GetInternal();
  if (player)
  {
    ++m_iPlayerOPSeq;
    player->CloseFile(reopen);
  }
}

void CApplicationPlayer::ClosePlayerGapless(PLAYERCOREID newCore)
{
  IPlayer* player = GetInternal();
  if (!player)
    return;

  bool gaplessSupported = (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer == EPC_PAPLAYER);
#if defined(HAS_OMXPLAYER)
  gaplessSupported = gaplessSupported || (m_eCurrentPlayer == EPC_OMXPLAYER);
#endif            
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
  IPlayer* player = GetInternal();
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
  IPlayer* player = GetInternal();
  return player != NULL; 
}

void CApplicationPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  IPlayer* player = GetInternal();
  if (player)
    player->RegisterAudioCallback(pCallback);
}

void CApplicationPlayer::UnRegisterAudioCallback()
{
  IPlayer* player = GetInternal();
  if (player)
    player->UnRegisterAudioCallback();
}

int CApplicationPlayer::GetChapter()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetChapter();
  else 
    return -1;
}

int CApplicationPlayer::GetChapterCount()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetChapterCount();
  else 
    return 0;
}

void CApplicationPlayer::GetChapterName(CStdString& strChapterName)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetChapterName(strChapterName);
}

bool CApplicationPlayer::HasAudio() const
{
  IPlayer* player = GetInternal();
  return (player && player->HasAudio());
}

bool CApplicationPlayer::HasVideo() const
{
  IPlayer* player = GetInternal();
  return (player && player->HasVideo());
}

bool CApplicationPlayer::IsPaused() const
{
  IPlayer* player = GetInternal();
  return (player && player->IsPaused());
}

bool CApplicationPlayer::IsPlaying() const
{
  IPlayer* player = GetInternal();
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
  IPlayer* player = GetInternal();
  if (player)
    player->Pause();
}

bool CApplicationPlayer::ControlsVolume() const
{
  IPlayer* player = GetInternal();
  return (player && player->ControlsVolume());
}

void CApplicationPlayer::SetMute(bool bOnOff)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetMute(bOnOff);
}

void CApplicationPlayer::SetVolume(float volume)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetVolume(volume);
}

void CApplicationPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
  IPlayer* player = GetInternal();
  if (player)
    player->Seek(bPlus, bLargeStep, bChapterOverride);
}

void CApplicationPlayer::SeekPercentage(float fPercent)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SeekPercentage(fPercent);
}

bool CApplicationPlayer::IsPassthrough() const
{
  IPlayer* player = GetInternal();
  return (player && player->IsPassthrough());
}

bool CApplicationPlayer::CanSeek()
{
  IPlayer* player = GetInternal();
  return (player && player->CanSeek());
}

bool CApplicationPlayer::SeekScene(bool bPlus)
{
  IPlayer* player = GetInternal();
  return (player && player->SeekScene(bPlus));
}

void CApplicationPlayer::SeekTime(int64_t iTime)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SeekTime(iTime);
}

CStdString CApplicationPlayer::GetPlayingTitle()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetPlayingTitle();
  else
    return "";
}

int64_t CApplicationPlayer::GetTime() const
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetTime();
  else
    return 0;
}

bool CApplicationPlayer::IsCaching() const
{
  IPlayer* player = GetInternal();
  return (player && player->IsCaching());
}

bool CApplicationPlayer::IsInMenu() const
{
  IPlayer* player = GetInternal();
  return (player && player->IsInMenu());
}

bool CApplicationPlayer::HasMenu() const
{
  IPlayer* player = GetInternal();
  return (player && player->HasMenu());
}

int CApplicationPlayer::GetCacheLevel() const
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetCacheLevel();
  else
    return 0;
}

int CApplicationPlayer::GetSubtitleCount()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetSubtitleCount();
  else
    return 0;
}

int CApplicationPlayer::GetAudioStream()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetAudioStream();
  else
    return 0;
}

int CApplicationPlayer::GetSubtitle()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetSubtitle();
  else
    return 0;
}

bool CApplicationPlayer::GetSubtitleVisible()
{
  IPlayer* player = GetInternal();
  return (player && player->GetSubtitleVisible());
}

bool CApplicationPlayer::CanRecord()
{
  IPlayer* player = GetInternal();
  return (player && player->CanRecord());
}

bool CApplicationPlayer::CanPause()
{
  IPlayer* player = GetInternal();
  return (player && player->CanPause());
}

bool CApplicationPlayer::IsRecording() const
{
  IPlayer* player = GetInternal();
  return (player && player->IsRecording());
}

TextCacheStruct_t* CApplicationPlayer::GetTeletextCache()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetTeletextCache();
  else
    return NULL;
}

int64_t CApplicationPlayer::GetTotalTime() const
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetTotalTime();
  else
    return 0;
}

float CApplicationPlayer::GetPercentage() const
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetPercentage();
  else
    return 0.0;
}

float CApplicationPlayer::GetCachePercentage() const
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetCachePercentage();
  else
    return 0.0;
}

void CApplicationPlayer::ToFFRW(int iSpeed)
{
  IPlayer* player = GetInternal();
  if (player)
    player->ToFFRW(iSpeed);
}

void CApplicationPlayer::DoAudioWork()
{
  IPlayer* player = GetInternal();
  if (player)
    player->DoAudioWork();
}

CStdString CApplicationPlayer::GetPlayerState()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetPlayerState();
  else
    return "";
}

bool CApplicationPlayer::QueueNextFile(const CFileItem &file)
{
  IPlayer* player = GetInternal();
  return (player && player->QueueNextFile(file));
}

bool CApplicationPlayer::GetStreamDetails(CStreamDetails &details)
{
  IPlayer* player = GetInternal();
  return (player && player->GetStreamDetails(details));
}

bool CApplicationPlayer::SetPlayerState(CStdString state)
{
  IPlayer* player = GetInternal();
  return (player && player->SetPlayerState(state));
}

void CApplicationPlayer::OnNothingToQueueNotify()
{
  IPlayer* player = GetInternal();
  if (player)
    player->OnNothingToQueueNotify();
}

void CApplicationPlayer::GetVideoStreamInfo(SPlayerVideoStreamInfo &info)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetVideoStreamInfo(info);
}

void CApplicationPlayer::GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetAudioStreamInfo(index, info);
}

bool CApplicationPlayer::OnAction(const CAction &action)
{
  IPlayer* player = GetInternal();
  return (player && player->OnAction(action));
}

bool CApplicationPlayer::Record(bool bOnOff)
{
  IPlayer* player = GetInternal();
  return (player && player->Record(bOnOff));
}

int  CApplicationPlayer::GetAudioStreamCount()
{
  IPlayer* player = GetInternal();
  if (player)
    return player->GetAudioStreamCount();
  else
    return 0;
}

void CApplicationPlayer::SetAudioStream(int iStream)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetAudioStream(iStream);
}

void CApplicationPlayer::GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetSubtitleStreamInfo(index, info);
}

void CApplicationPlayer::SetSubtitle(int iStream)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetSubtitle(iStream);
}

void CApplicationPlayer::SetSubtitleVisible(bool bVisible)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetSubtitleVisible(bVisible);
}

int  CApplicationPlayer::AddSubtitle(const CStdString& strSubPath)
{
  IPlayer* player = GetInternal();
  if (player)
    return player->AddSubtitle(strSubPath);
  else
    return 0;
}

void CApplicationPlayer::SetSubTitleDelay(float fValue)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetSubTitleDelay(fValue);
}

void CApplicationPlayer::SetAVDelay(float fValue)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetAVDelay(fValue);
}

void CApplicationPlayer::SetDynamicRangeCompression(long drc)
{
  IPlayer* player = GetInternal();
  if (player)
    player->SetDynamicRangeCompression(drc);
}

bool CApplicationPlayer::SwitchChannel(const PVR::CPVRChannel &channel)
{
  IPlayer* player = GetInternal();
  return (player && player->SwitchChannel(channel));
}

void CApplicationPlayer::LoadPage(int p, int sp, unsigned char* buffer)
{
  IPlayer* player = GetInternal();
  if (player)
    player->LoadPage(p, sp, buffer);
}

void CApplicationPlayer::GetAudioCapabilities(std::vector<int> &audioCaps)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetAudioCapabilities(audioCaps);
}

void CApplicationPlayer::GetSubtitleCapabilities(std::vector<int> &subCaps)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetSubtitleCapabilities(subCaps);
}

void CApplicationPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetAudioInfo(strAudioInfo);
}

void CApplicationPlayer::GetVideoInfo( CStdString& strVideoInfo)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetVideoInfo(strVideoInfo);
}

void CApplicationPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetGeneralInfo(strVideoInfo);
}

int  CApplicationPlayer::SeekChapter(int iChapter)
{
  IPlayer* player = GetInternal();
  if (player)
    return player->SeekChapter(iChapter);
  else
    return 0;
}

void CApplicationPlayer::GetRenderFeatures(std::vector<int> &renderFeatures)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetRenderFeatures(renderFeatures);
}

void CApplicationPlayer::GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetDeinterlaceMethods(deinterlaceMethods);
}

void CApplicationPlayer::GetDeinterlaceModes(std::vector<int> &deinterlaceModes)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetDeinterlaceModes(deinterlaceModes);
}

void CApplicationPlayer::GetScalingMethods(std::vector<int> &scalingMethods)
{
  IPlayer* player = GetInternal();
  if (player)
    player->GetScalingMethods(scalingMethods);
}

void CApplicationPlayer::SetPlaySpeed(int iSpeed, bool bApplicationMuted)
{
  IPlayer* player = GetInternal();
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
