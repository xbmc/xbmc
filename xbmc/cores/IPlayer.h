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
#include <vector>
#include "IPlayerCallback.h"
#include "guilib/Geometry.h"
#include "guilib/Resolution.h"
#include <string>

#define CURRENT_STREAM -1
#define CAPTUREFLAG_CONTINUOUS  0x01 //after a render is done, render a new one immediately
#define CAPTUREFLAG_IMMEDIATELY 0x02 //read out immediately after render, this can cause a busy wait
#define CAPTUREFORMAT_BGRA 0x01

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
  bool valid;
  int bitrate;
  int channels;
  int samplerate;
  int bitspersample;
  std::string language;
  std::string name;
  std::string audioCodecName;

  SPlayerAudioStreamInfo()
  {
    valid = false;
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
  bool valid;
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
    valid = false;
    bitrate = 0;
    videoAspectRatio = 1.0f;
    height = 0;
    width = 0;
  }
};

enum EINTERLACEMETHOD
{
  VS_INTERLACEMETHOD_NONE=0,
  VS_INTERLACEMETHOD_AUTO=1,
  VS_INTERLACEMETHOD_RENDER_BLEND=2,

  VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED=3,
  VS_INTERLACEMETHOD_RENDER_WEAVE=4,

  VS_INTERLACEMETHOD_RENDER_BOB_INVERTED=5,
  VS_INTERLACEMETHOD_RENDER_BOB=6,

  VS_INTERLACEMETHOD_DEINTERLACE=7,

  VS_INTERLACEMETHOD_VDPAU_BOB=8,
  VS_INTERLACEMETHOD_INVERSE_TELECINE=9,

  VS_INTERLACEMETHOD_VDPAU_INVERSE_TELECINE=11,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL=12,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_HALF=13,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL=14,
  VS_INTERLACEMETHOD_VDPAU_TEMPORAL_SPATIAL_HALF=15,
  VS_INTERLACEMETHOD_DEINTERLACE_HALF=16,

  VS_INTERLACEMETHOD_AUTO_ION = 21,

  VS_INTERLACEMETHOD_VAAPI_BOB = 22,
  VS_INTERLACEMETHOD_VAAPI_MADI = 23,
  VS_INTERLACEMETHOD_VAAPI_MACI = 24,

  VS_INTERLACEMETHOD_MMAL_ADVANCED = 25,
  VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF = 26,
  VS_INTERLACEMETHOD_MMAL_BOB = 27,
  VS_INTERLACEMETHOD_MMAL_BOB_HALF = 28,

  VS_INTERLACEMETHOD_IMX_FASTMOTION = 29,
  VS_INTERLACEMETHOD_IMX_FASTMOTION_DOUBLE = 30,

  VS_INTERLACEMETHOD_MAX // do not use and keep as last enum value.
};

enum ESCALINGMETHOD
{
  VS_SCALINGMETHOD_NEAREST=0,
  VS_SCALINGMETHOD_LINEAR,

  VS_SCALINGMETHOD_CUBIC,
  VS_SCALINGMETHOD_LANCZOS2,
  VS_SCALINGMETHOD_LANCZOS3_FAST,
  VS_SCALINGMETHOD_LANCZOS3,
  VS_SCALINGMETHOD_SINC8,
  VS_SCALINGMETHOD_NEDI,

  VS_SCALINGMETHOD_BICUBIC_SOFTWARE,
  VS_SCALINGMETHOD_LANCZOS_SOFTWARE,
  VS_SCALINGMETHOD_SINC_SOFTWARE,
  VS_SCALINGMETHOD_VDPAU_HARDWARE,
  VS_SCALINGMETHOD_DXVA_HARDWARE,

  VS_SCALINGMETHOD_AUTO,

  VS_SCALINGMETHOD_SPLINE36_FAST,
  VS_SCALINGMETHOD_SPLINE36,

  VS_SCALINGMETHOD_MAX // do not use and keep as last enum value.
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
  RENDERFEATURE_POSTPROCESS
};

enum ViewMode {
  ViewModeNormal = 0,
  ViewModeZoom,
  ViewModeStretch4x3,
  ViewModeWideZoom,
  ViewModeStretch16x9,
  ViewModeOriginal,
  ViewModeCustom,
  ViewModeStretch16x9Nonlin
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
  virtual bool HasVideo() const = 0;
  virtual bool HasAudio() const = 0;
  virtual bool HasRDS() const { return false; }
  virtual bool IsPassthrough() const { return false;}
  virtual bool CanSeek() {return true;}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) = 0;
  virtual bool SeekScene(bool bPlus = true) {return false;}
  virtual void SeekPercentage(float fPercent = 0){}
  virtual float GetPercentage(){ return 0;}
  virtual float GetCachePercentage(){ return 0;}
  virtual void SetMute(bool bOnOff){}
  virtual void SetVolume(float volume){}
  virtual void SetDynamicRangeCompression(long drc){}
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

  virtual int GetVideoStream() const { return -1; }
  virtual int GetVideoStreamCount() const { return 0; }
  virtual void GetVideoStreamInfo(int streamId, SPlayerVideoStreamInfo &info) {}
  virtual void SetVideoStream(int iStream) {}

  virtual TextCacheStruct_t* GetTeletextCache() { return NULL; };
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
   \brief current time in milliseconds
   */
  virtual int64_t GetTime() { return 0; }
  /*!
   \brief Sets the current time. This 
   can be used for injecting the current time. 
   This is not to be confused with a seek. It just
   can be used if endless streams contain multiple
   tracks in reality (like with airtunes)
   */
  virtual void SetTime(int64_t time) { }
  /*!
   \brief total time in milliseconds
   */
  virtual int64_t GetTotalTime() { return 0; }
  /*!
   \brief Set the total time  in milliseconds
   this can be used for injecting the duration in case
   its not available in the underlaying decoder (airtunes for example)
   */
  virtual void SetTotalTime(int64_t time) { }
  virtual int GetSourceBitrate(){ return 0;}
  virtual bool GetStreamDetails(CStreamDetails &details){ return false;}
  virtual void SetSpeed(float speed) = 0;
  virtual float GetSpeed() = 0;
  virtual bool SupportsTempo() { return false; }

  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  //Returns true if not playback (paused or stopped beeing filled)
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

  /*!
   \breif hook into render loop of render thread
   */
  virtual void FrameMove() {};

  virtual bool HasFrame() { return false; };

  virtual void Render(bool clear, uint32_t alpha = 255, bool gui = true) {};

  virtual void FlushRenderer() {};

  virtual void SetRenderViewMode(int mode) {};

  virtual float GetRenderAspectRatio() { return 1.0; };

  virtual void TriggerUpdateResolution() {};

  virtual bool IsRenderingVideo() { return false; };

  virtual bool IsRenderingGuiLayer() { return false; };

  virtual bool IsRenderingVideoLayer() { return false; };

  virtual bool Supports(EINTERLACEMETHOD method) { return false; };
  virtual bool Supports(ESCALINGMETHOD method) { return false; };
  virtual bool Supports(ERENDERFEATURE feature) { return false; };

  virtual unsigned int RenderCaptureAlloc() { return 0; };
  virtual void RenderCaptureRelease(unsigned int captureId) {};
  virtual void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags) {};
  virtual bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size) { return false; };

  std::string m_name;
  std::string m_type;

protected:
  IPlayerCallback& m_callback;
};
