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

#include <boost/shared_ptr.hpp>
#include "threads/SingleLock.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"

typedef enum
{
  PLAYBACK_CANCELED = -1,
  PLAYBACK_FAIL = 0,
  PLAYBACK_OK = 1,
} PlayBackRet;

namespace PVR
{
  class CPVRChannel;
}

class IAudioCallback;
class CAction;
class CPlayerOptions;
class CStreamDetails;
class IInputHandler;

struct SPlayerAudioStreamInfo;
struct SPlayerVideoStreamInfo;
struct SPlayerSubtitleStreamInfo;
struct TextCacheStruct_t;

class CApplicationPlayer
{
  boost::shared_ptr<IPlayer> m_pPlayer;
  unsigned int m_iPlayerOPSeq;  // used to detect whether an OpenFile request on player is canceled by us.
  PLAYERCOREID m_eCurrentPlayer;

  CCriticalSection  m_player_lock;
  
public:
  CApplicationPlayer();

  int m_iPlaySpeed;

  // player management
  void CloseFile();
  void ClosePlayer();
  void ClosePlayerGapless(PLAYERCOREID newCore);
  void CreatePlayer(PLAYERCOREID newCore, IPlayerCallback& callback);
  PLAYERCOREID GetCurrentPlayer() const { return m_eCurrentPlayer; }
  boost::shared_ptr<IPlayer> GetInternal() const;
  int  GetPlaySpeed() const;
  bool HasPlayer() const;
  PlayBackRet OpenFile(const CFileItem& item, const CPlayerOptions& options);
  void ResetPlayer() { m_eCurrentPlayer = EPC_NONE; }
  void SetPlaySpeed(int iSpeed, bool bApplicationMuted);

  // proxy calls
  int   AddSubtitle(const CStdString& strSubPath);
  bool  CanPause();
  bool  CanRecord();
  bool  CanSeek();
  bool  ControlsVolume() const;
  void  DoAudioWork();
  void  GetAudioCapabilities(std::vector<int> &audioCaps);
  void  GetAudioInfo( CStdString& strAudioInfo);
  int   GetAudioStream();
  int   GetAudioStreamCount();
  void  GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info);
  int   GetBitsPerSample();
  int   GetCacheLevel() const;
  float GetCachePercentage() const;
  int   GetChapterCount();
  int   GetChapter();  
  void  GetChapterName(CStdString& strChapterName);
  void  GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods);
  void  GetDeinterlaceModes(std::vector<int> &deinterlaceModes);
  bool  GetCurrentSubtitle(CStdString& strSubtitle);
  void  GetGeneralInfo( CStdString& strVideoInfo);
  float GetPercentage() const;
  int   GetPictureHeight();
  int   GetPictureWidth();
  CStdString GetPlayerState();
  CStdString GetPlayingTitle();
  void  GetRenderFeatures(std::vector<int> &renderFeatures);
  int   GetSampleRate();
  void  GetScalingMethods(std::vector<int> &scalingMethods);
  bool  GetStreamDetails(CStreamDetails &details);
  int   GetSubtitle();
  void  GetSubtitleCapabilities(std::vector<int> &subCaps);
  int   GetSubtitleCount();
  void  GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info);
  bool  GetSubtitleVisible();
  TextCacheStruct_t* GetTeletextCache();
  int64_t GetTime() const;
  int64_t GetTotalTime() const;
  void  GetVideoInfo( CStdString& strVideoInfo);
  void  GetVideoStreamInfo(SPlayerVideoStreamInfo &info);
  IInputHandler *GetInputHandler();
  bool  HasAudio() const;
  bool  HasMenu() const;
  bool  HasVideo() const;
  bool  IsCaching() const;
  bool  IsInMenu() const;
  bool  IsPaused() const;
  bool  IsPausedPlayback() const;
  bool  IsPassthrough() const;
  bool  IsPlaying() const;
  bool  IsPlayingAudio() const;
  bool  IsPlayingVideo() const;
  bool  IsPlayingGame() const;
  bool  IsRecording() const;
  void  LoadPage(int p, int sp, unsigned char* buffer);
  bool  OnAction(const CAction &action);
  void  OnNothingToQueueNotify();
  void  Pause();
  bool  QueueNextFile(const CFileItem &file);
  bool  Record(bool bOnOff);
  void  RegisterAudioCallback(IAudioCallback* pCallback);
  void  Seek(bool bPlus = true, bool bLargeStep = false);
  int   SeekChapter(int iChapter);
  void  SeekPercentage(float fPercent = 0);
  bool  SeekScene(bool bPlus = true);
  void  SeekTime(int64_t iTime = 0);
  void  SetAudioStream(int iStream);
  void  SetAVDelay(float fValue = 0.0f);
  void  SetDynamicRangeCompression(long drc);
  void  SetMute(bool bOnOff);
  bool  SetPlayerState(CStdString state);
  void  SetSubtitle(int iStream);
  void  SetSubTitleDelay(float fValue = 0.0f);
  void  SetSubtitleVisible(bool bVisible);
  void  SetVolume(float volume);
  bool  SwitchChannel(const PVR::CPVRChannel &channel);
  void  ToFFRW(int iSpeed = 0);
  void  UnRegisterAudioCallback();
};
