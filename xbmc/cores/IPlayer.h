#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h" // until we get sane int types used here
#include "IAudioCallback.h"
#include "utils/StdString.h"

struct TextCacheStruct_t;
class TiXmlElement;
class CStreamDetails;
class CAction;

namespace PVR
{
  class CPVRChannel;
}

class IPlayerCallback
{
public:
  virtual ~IPlayerCallback() {}
  virtual void OnPlayBackEnded() = 0;
  virtual void OnPlayBackStarted() = 0;
  virtual void OnPlayBackPaused() {};
  virtual void OnPlayBackResumed() {};
  virtual void OnPlayBackStopped() = 0;
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackSeek(int iTime, int seekOffset) {};
  virtual void OnPlayBackSeekChapter(int iChapter) {};
  virtual void OnPlayBackSpeedChanged(int iSpeed) {};
};

class CPlayerOptions
{
public:
  CPlayerOptions()
  {
    starttime = 0LL;
    startpercent = 0LL;
    identify = false;
    fullscreen = false;
    video_only = false;
  }
  double  starttime; /* start time in seconds */
  double  startpercent; /* start time in percent */  
  bool    identify;  /* identify mode, used for checking format and length of a file */
  CStdString state;  /* potential playerstate to restore to */
  bool    fullscreen; /* player is allowed to switch to fullscreen */
  bool    video_only; /* player is not allowed to play audio streams, video streams only */
};

class CFileItem;
class CRect;

enum IPlayerAudioCapabilities
{
  IPC_AUD_ALL,
  IPC_AUD_OFFSET,
  IPC_AUD_AMP,
  IPC_AUD_SELECT_STREAM,
  IPC_AUD_OUTPUT_STEREO,
  IPC_AUD_SELECT_OUTPUT
};

enum IPlayerSubtitleCapabilities
{
  IPC_SUBS_ALL,
  IPC_SUBS_SELECT,
  IPC_SUBS_EXTERNAL,
  IPC_SUBS_OFFSET
};

class IPlayer
{
public:
  IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer(){};
  virtual bool Initialize(TiXmlElement* pConfig) { return true; };
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {};
  virtual void UnRegisterAudioCallback() {};
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options){ return false;}
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile(){ return true;}
  virtual bool IsPlaying() const { return false;}
  virtual bool CanPause() { return true; };
  virtual void Pause() = 0;
  virtual bool IsPaused() const = 0;
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool IsPassthrough() const { return false;}
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetPercentage(){ return 0;}
  virtual float GetCachePercentage(){ return 0;}
  virtual bool ControlsVolume(){ return false;}
  virtual void SetMute(bool bOnOff){}
  virtual void SetVolume(float volume){}
  virtual void SetDynamicRangeCompression(long drc){}
  virtual void GetAudioInfo( CStdString& strAudioInfo) = 0;
  virtual void GetVideoInfo( CStdString& strVideoInfo) = 0;
  virtual void GetGeneralInfo( CStdString& strVideoInfo) = 0;
  virtual void Update(bool bPauseDrawing = false) = 0;
  virtual void GetVideoRect(CRect& SrcRect, CRect& DestRect) {}
  virtual void GetVideoAspectRatio(float& fAR) { fAR = 1.0f; }
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};

  virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  virtual float GetAVDelay()                    { return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay()    { return 0.0f; }
  virtual int  GetSubtitleCount()     { return 0; }
  virtual int  GetSubtitle()          { return -1; }
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName){};
  virtual void GetSubtitleLanguage(int iStream, CStdString &strStreamLang){};
  virtual void SetSubtitle(int iStream){};
  virtual bool GetSubtitleVisible(){ return false;};
  virtual void SetSubtitleVisible(bool bVisible){};
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension){ return false;};
  virtual int  AddSubtitle(const CStdString& strSubPath) {return -1;};

  virtual int  GetAudioStreamCount()  { return 0; }
  virtual int  GetAudioStream()       { return -1; }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName){};
  virtual void SetAudioStream(int iStream){};
  virtual void GetAudioStreamLanguage(int iStream, CStdString &strLanguage){};

  virtual TextCacheStruct_t* GetTeletextCache() { return NULL; };
  virtual void LoadPage(int p, int sp, unsigned char* buffer) {};

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(CStdString& strChapterName)      { return; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }
//  virtual bool GetChapterInfo(int chapter, SChapterInfo &info) { return false; }

  virtual float GetActualFPS() { return 0.0f; };
  virtual void SeekTime(int64_t iTime = 0){};
  /*!
   \brief current time in milliseconds
   */
  virtual int64_t GetTime() { return 0; }
  /*!
   \brief total time in milliseconds
   */
  virtual int64_t GetTotalTime() { return 0; }
  virtual int GetAudioBitrate(){ return 0;}
  virtual int GetVideoBitrate(){ return 0;}
  virtual int GetSourceBitrate(){ return 0;}
  virtual int GetChannels(){ return 0;};
  virtual int GetBitsPerSample(){ return 0;};
  virtual int GetSampleRate(){ return 0;};
  virtual CStdString GetAudioCodecName(){ return "";}
  virtual CStdString GetVideoCodecName(){ return "";}
  virtual int GetPictureWidth(){ return 0;}
  virtual int GetPictureHeight(){ return 0;}
  virtual bool GetStreamDetails(CStreamDetails &details){ return false;}
  virtual void ToFFRW(int iSpeed = 0){};
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool IsCaching() const {return false;};
  //Cache filled in Percent
  virtual int GetCacheLevel() const {return -1;};

  virtual bool IsInMenu() const {return false;};
  virtual bool HasMenu() { return false; };

  virtual void DoAudioWork(){};
  virtual bool OnAction(const CAction &action) { return false; };

  virtual bool GetCurrentSubtitle(CStdString& strSubtitle) { strSubtitle = ""; return false; }
  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState() { return ""; };
  virtual bool SetPlayerState(CStdString state) { return false;};
  
  virtual CStdString GetPlayingTitle() { return ""; };

  virtual bool SwitchChannel(const PVR::CPVRChannel &channel) { return false; }

  /*!
   \brief If the player uses bypass mode, define its rendering capabilities
   */
  virtual void GetRenderFeatures(std::vector<int> &renderFeatures) {};
  /*!
   \brief If the player uses bypass mode, define its deinterlace algorithms
   */
  virtual void GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods) {};
  /*!
   \brief If the player uses bypass mode, define how deinterlace is set
   */
  virtual void GetDeinterlaceModes(std::vector<int> &deinterlaceModes) {};
  /*!
   \brief If the player uses bypass mode, define its scaling capabilities
   */
  virtual void GetScalingMethods(std::vector<int> &scalingMethods) {};
  /*!
   \brief define the audio capabilities of the player (default=all)
   */

  virtual void GetAudioCapabilities(std::vector<int> &audioCaps) { audioCaps.assign(1,IPC_AUD_ALL); };
  /*!
   \brief define the subtitle capabilities of the player
   */
  virtual void GetSubtitleCapabilities(std::vector<int> &subCaps) { subCaps.assign(1,IPC_SUBS_ALL); };

protected:
  IPlayerCallback& m_callback;
};
