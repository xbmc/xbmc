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

#include "system.h" // until we get sane int types used here
#include <memory>
#include "IPlayerCallback.h"
#include "guilib/Geometry.h"
#include <string>

#define CURRENT_STREAM -1

struct TextCacheStruct_t;
class TiXmlElement;
class CStreamDetails;
class CAction;

namespace PVR
{
  class CPVRChannel;
  typedef std::shared_ptr<PVR::CPVRChannel> CPVRChannelPtr;
}

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
  std::string state;  /* potential playerstate to restore to */
  bool    fullscreen; /* player is allowed to switch to fullscreen */
  bool    video_only; /* player is not allowed to play audio streams, video streams only */
};

class CFileItem;

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

struct SPlayerAudioStreamInfo
{
  int bitrate;
  int channels;
  int samplerate;
  int bitspersample;
  std::string language;
  std::string name;
  std::string audioCodecName;

  SPlayerAudioStreamInfo()
  {
    bitrate = 0;
    channels = 0;
    samplerate = 0;
    bitspersample = 0;
  }
};

struct SPlayerSubtitleStreamInfo
{
  std::string language;
  std::string name;
};

struct SPlayerVideoStreamInfo
{
  int bitrate;
  float videoAspectRatio;
  int height;
  int width;
  std::string language;
  std::string name;
  std::string videoCodecName;
  CRect SrcRect;
  CRect DestRect;
  std::string stereoMode;

  SPlayerVideoStreamInfo()
  {
    bitrate = 0;
    videoAspectRatio = 1.0f;
    height = 0;
    width = 0;
  }
};

class IPlayer
{
public:
  IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer(){};
  virtual bool Initialize(TiXmlElement* pConfig) { return true; };
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options){ return false;}
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile(bool reopen = false) = 0;
  virtual bool IsPlaying() const { return false;}
  virtual bool CanPause() { return true; };
  virtual void Pause() = 0;
  virtual bool IsPaused() const = 0;
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool IsPassthrough() const { return false;}
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetPercentage(){ return 0;}
  virtual float GetCachePercentage(){ return 0;}
  virtual void SetMute(bool bOnOff){}
  virtual void SetVolume(float volume){}
  virtual bool ControlsVolume(){ return false;}
  virtual void SetDynamicRangeCompression(long drc){}
  virtual void GetAudioInfo(std::string& strAudioInfo) = 0;
  virtual void GetVideoInfo(std::string& strVideoInfo) = 0;
  virtual void GetGeneralInfo(std::string& strGeneralInfo) = 0;
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};

  virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  virtual float GetAVDelay()                    { return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay()    { return 0.0f; }
  virtual int  GetSubtitleCount()     { return 0; }
  virtual int  GetSubtitle()          { return -1; }
  virtual void GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info){};
  virtual void SetSubtitle(int iStream){};
  virtual bool GetSubtitleVisible(){ return false;};
  virtual void SetSubtitleVisible(bool bVisible){};

  /** \brief Adds the subtitle(s) provided by the given file to the available player streams
  *          and actives the first of the added stream(s). E.g., vob subs can contain multiple streams.
  *   \param[in] strSubPath The full path of the subtitle file.
  */
  virtual void  AddSubtitle(const std::string& strSubPath) {};

  virtual int  GetAudioStreamCount()  { return 0; }
  virtual int  GetAudioStream()       { return -1; }
  virtual void SetAudioStream(int iStream){};
  virtual void GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info){};

  virtual TextCacheStruct_t* GetTeletextCache() { return NULL; };
  virtual void LoadPage(int p, int sp, unsigned char* buffer) {};

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) { return; }
  virtual int64_t GetChapterPos(int chapterIdx=-1)             { return 0; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }
//  virtual bool GetChapterInfo(int chapter, SChapterInfo &info) { return false; }

  virtual float GetActualFPS() { return 0.0f; };
  virtual void SeekTime(int64_t iTime = 0){};
  /*
   \brief seek relative to current time, returns false if not implemented by player
   \param iTime The time in milliseconds to seek. A positive value will seek forward, a negative backward.
   \return True if the player supports relative seeking, otherwise false
   */
  virtual bool SeekTimeRelative(int64_t iTime) { return false; }
  /*!
   \brief current time in milliseconds
   */
  virtual int64_t GetTime() { return 0; }
  /*!
   \brief time of frame on screen in milliseconds
   */
  virtual int64_t GetDisplayTime() { return GetTime(); }
  /*!
   \brief total time in milliseconds
   */
  virtual int64_t GetTotalTime() { return 0; }
  virtual void GetVideoStreamInfo(SPlayerVideoStreamInfo &info){};
  virtual int GetSourceBitrate(){ return 0;}
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

  //returns a state that is needed for resuming from a specific time
  virtual std::string GetPlayerState() { return ""; };
  virtual bool SetPlayerState(const std::string& state) { return false;};
  
  virtual std::string GetPlayingTitle() { return ""; };

  virtual bool SwitchChannel(const PVR::CPVRChannelPtr &channel) { return false; }

  // Note: the following "OMX" methods are deprecated and will be removed in the future
  // They should be handled by the video renderer, not the player
  /*!
   \brief If the player uses bypass mode, define its rendering capabilities
   */
  virtual void OMXGetRenderFeatures(std::vector<int> &renderFeatures) {};
  /*!
   \brief If the player uses bypass mode, define its deinterlace algorithms
   */
  virtual void OMXGetDeinterlaceMethods(std::vector<int> &deinterlaceMethods) {};
  /*!
   \brief If the player uses bypass mode, define how deinterlace is set
   */
  virtual void OMXGetDeinterlaceModes(std::vector<int> &deinterlaceModes) {};
  /*!
   \brief If the player uses bypass mode, define its scaling capabilities
   */
  virtual void OMXGetScalingMethods(std::vector<int> &scalingMethods) {};
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
