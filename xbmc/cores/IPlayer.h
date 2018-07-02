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

#include <vector>
#include <string>
#include <memory>

#include "IPlayerCallback.h"
#include "VideoSettings.h"
#include "Interface/StreamInfo.h"

#define CURRENT_STREAM -1
#define CAPTUREFLAG_CONTINUOUS  0x01 //after a render is done, render a new one immediately
#define CAPTUREFLAG_IMMEDIATELY 0x02 //read out immediately after render, this can cause a busy wait
#define CAPTUREFORMAT_BGRA 0x01

struct TextCacheStruct_t;
class TiXmlElement;
class CStreamDetails;
class CAction;

class CPlayerOptions
{
public:
  CPlayerOptions()
  {
    starttime = 0LL;
    startpercent = 0LL;
    fullscreen = false;
    videoOnly = false;
    preferStereo = false;
  }
  double starttime; /* start time in seconds */
  double startpercent; /* start time in percent */
  std::string state;  /* potential playerstate to restore to */
  bool fullscreen; /* player is allowed to switch to fullscreen */
  bool videoOnly; /* player is not allowed to play audio streams, video streams only */
  bool preferStereo; /* prefer stereo streams when selecting initial audio stream*/
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

enum ERENDERFEATURE
{
  RENDERFEATURE_GAMMA,
  RENDERFEATURE_BRIGHTNESS,
  RENDERFEATURE_CONTRAST,
  RENDERFEATURE_NOISE,
  RENDERFEATURE_SHARPNESS,
  RENDERFEATURE_NONLINSTRETCH,
  RENDERFEATURE_ROTATION,
  RENDERFEATURE_STRETCH,
  RENDERFEATURE_ZOOM,
  RENDERFEATURE_VERTICAL_SHIFT,
  RENDERFEATURE_PIXEL_RATIO,
  RENDERFEATURE_POSTPROCESS,
  RENDERFEATURE_TONEMAP
};

class IPlayer
{
public:
  explicit IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer() = default;
  virtual bool Initialize(TiXmlElement* pConfig) { return true; };
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options){ return false;}
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile(bool reopen = false) = 0;
  virtual bool IsPlaying() const { return false;}
  virtual bool CanPause() { return true; };
  virtual void Pause() = 0;
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool HasGame() const { return false; }
  virtual bool HasRDS() const { return false; }
  virtual bool IsPassthrough() const { return false;}
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetCachePercentage(){ return 0;}
  virtual void SetMute(bool bOnOff){}
  virtual void SetVolume(float volume){}
  virtual void SetDynamicRangeCompression(long drc){}

  virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  virtual float GetAVDelay()                    { return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay()    { return 0.0f; }
  virtual int  GetSubtitleCount()     { return 0; }
  virtual int  GetSubtitle()          { return -1; }
  virtual void GetSubtitleStreamInfo(int index, SubtitleStreamInfo &info){};
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
  virtual void GetAudioStreamInfo(int index, AudioStreamInfo &info){};

  virtual int GetVideoStream() const { return -1; }
  virtual int GetVideoStreamCount() const { return 0; }
  virtual void GetVideoStreamInfo(int streamId, VideoStreamInfo &info) {}
  virtual void SetVideoStream(int iStream) {}

  virtual int GetPrograms(std::vector<ProgramInfo>& programs) { return 0; }
  virtual void SetProgram(int progId) {}
  virtual int GetProgramsCount() { return 0; }

  virtual std::shared_ptr<TextCacheStruct_t> GetTeletextCache() { return NULL; };
  virtual void LoadPage(int p, int sp, unsigned char* buffer) {};

  virtual std::string GetRadioText(unsigned int line) { return ""; };

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) { return; }
  virtual int64_t GetChapterPos(int chapterIdx=-1)             { return 0; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }
//  virtual bool GetChapterInfo(int chapter, SChapterInfo &info) { return false; }

  virtual void SeekTime(int64_t iTime = 0){};
  /*
   \brief seek relative to current time, returns false if not implemented by player
   \param iTime The time in milliseconds to seek. A positive value will seek forward, a negative backward.
   \return True if the player supports relative seeking, otherwise false
   */
  virtual bool SeekTimeRelative(int64_t iTime) { return false; }

  /*!
   \brief Sets the current time. This
   can be used for injecting the current time.
   This is not to be confused with a seek. It just
   can be used if endless streams contain multiple
   tracks in reality (like with airtunes)
   */
  virtual void SetTime(int64_t time) { }

  /*!
   \brief Set the total time  in milliseconds
   this can be used for injecting the duration in case
   its not available in the underlaying decoder (airtunes for example)
   */
  virtual void SetTotalTime(int64_t time) { }
  virtual void SetSpeed(float speed) = 0;
  virtual void SetTempo(float tempo) { };
  virtual bool SupportsTempo() { return false; }
  virtual void FrameAdvance(int frames) { };

  //Returns true if not playback (paused or stopped being filled)
  virtual bool IsCaching() const {return false;};
  //Cache filled in Percent
  virtual int GetCacheLevel() const {return -1;};

  virtual bool IsInMenu() const {return false;};
  virtual bool HasMenu() const { return false; };

  virtual void DoAudioWork(){};
  virtual bool OnAction(const CAction &action) { return false; };

  //returns a state that is needed for resuming from a specific time
  virtual std::string GetPlayerState() { return ""; };
  virtual bool SetPlayerState(const std::string& state) { return false;};

  virtual void GetAudioCapabilities(std::vector<int> &audioCaps) { audioCaps.assign(1,IPC_AUD_ALL); };
  /*!
   \brief define the subtitle capabilities of the player
   */
  virtual void GetSubtitleCapabilities(std::vector<int> &subCaps) { subCaps.assign(1,IPC_SUBS_ALL); };

  /*!
   \brief hook into render loop of render thread
   */
  virtual void Render(bool clear, uint32_t alpha = 255, bool gui = true) {};
  virtual void FlushRenderer() {};
  virtual void SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch) {};
  virtual float GetRenderAspectRatio() { return 1.0; };
  virtual void TriggerUpdateResolution() {};
  virtual bool IsRenderingVideo() { return false; };

  virtual bool Supports(EINTERLACEMETHOD method) { return false; };
  virtual EINTERLACEMETHOD GetDeinterlacingMethodDefault() { return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE; }
  virtual bool Supports(ESCALINGMETHOD method) { return false; };
  virtual bool Supports(ERENDERFEATURE feature) { return false; };

  virtual unsigned int RenderCaptureAlloc() { return 0; };
  virtual void RenderCaptureRelease(unsigned int captureId) {};
  virtual void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags) {};
  virtual bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size) { return false; };

  // video and audio settings
  virtual CVideoSettings GetVideoSettings() { return CVideoSettings(); };
  virtual void SetVideoSettings(CVideoSettings& settings) {};

  std::string m_name;
  std::string m_type;

protected:
  IPlayerCallback& m_callback;
};
