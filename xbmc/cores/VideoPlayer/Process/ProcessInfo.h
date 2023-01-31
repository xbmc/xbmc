/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/VideoRenderers/RenderInfo.h"
#include "cores/VideoSettings.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <list>
#include <map>
#include <string>

class CProcessInfo;
class CDataCacheCore;

using CreateProcessControl = CProcessInfo* (*)();

class CProcessInfo
{
public:
  static CProcessInfo* CreateInstance();
  static void RegisterProcessControl(const std::string& id, CreateProcessControl createFunc);
  virtual ~CProcessInfo() = default;
  void SetDataCache(CDataCacheCore *cache);

  // player video
  void ResetVideoCodecInfo();
  void SetVideoDecoderName(const std::string &name, bool isHw);
  std::string GetVideoDecoderName();
  bool IsVideoHwDecoder();
  void SetVideoDeintMethod(const std::string &method);
  std::string GetVideoDeintMethod();
  void SetVideoPixelFormat(const std::string &pixFormat);
  std::string GetVideoPixelFormat();
  void SetVideoStereoMode(const std::string &mode);
  std::string GetVideoStereoMode();
  void SetVideoDimensions(int width, int height);
  void GetVideoDimensions(int &width, int &height);
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();
  void SetVideoInterlaced(bool interlaced);
  bool GetVideoInterlaced();
  virtual EINTERLACEMETHOD GetFallbackDeintMethod();
  virtual void SetSwDeinterlacingMethods();
  void UpdateDeinterlacingMethods(std::list<EINTERLACEMETHOD> &methods);
  bool Supports(EINTERLACEMETHOD method) const;
  void SetDeinterlacingMethodDefault(EINTERLACEMETHOD method);
  EINTERLACEMETHOD GetDeinterlacingMethodDefault() const;
  CVideoBufferManager& GetVideoBufferManager();
  std::vector<AVPixelFormat> GetPixFormats();
  void SetPixFormats(std::vector<AVPixelFormat> &formats);

  // player audio info
  void ResetAudioCodecInfo();
  void SetAudioDecoderName(const std::string &name);
  std::string GetAudioDecoderName();
  void SetAudioChannels(const std::string &channels);
  std::string GetAudioChannels();
  void SetAudioSampleRate(int sampleRate);
  int GetAudioSampleRate();
  void SetAudioBitsPerSample(int bitsPerSample);
  int GetAudioBitsPerSample();
  virtual bool AllowDTSHDDecode();
  virtual bool WantsRawPassthrough() { return false; }

  // render info
  void SetRenderClockSync(bool enabled);
  bool IsRenderClockSync();
  void UpdateRenderInfo(CRenderInfo &info);
  void UpdateRenderBuffers(int queued, int discard, int free);
  void GetRenderBuffers(int &queued, int &discard, int &free);
  virtual std::vector<AVPixelFormat> GetRenderFormats();

  // player states
  /*!
   * @brief Notifies that a seek operation has finished
   * @param offset - the seek offset
  */
  void SeekFinished(int64_t offset);

  void SetStateSeeking(bool active);
  bool IsSeeking();
  void SetStateRealtime(bool state);
  bool IsRealtimeStream();
  void SetSpeed(float speed);
  void SetNewSpeed(float speed);
  float GetNewSpeed();
  void SetFrameAdvance(bool fa);
  bool IsFrameAdvance();
  void SetTempo(float tempo);
  void SetNewTempo(float tempo);
  float GetNewTempo();
  bool IsTempoAllowed(float tempo);
  virtual float MinTempoPlatform();
  virtual float MaxTempoPlatform();
  void SetLevelVQ(int level);
  int GetLevelVQ();
  void SetGuiRender(bool gui);
  bool GetGuiRender();
  void SetVideoRender(bool video);
  bool GetVideoRender();
  unsigned int GetMaxPassthroughOffSyncDuration() const;

  void SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max);
  int64_t GetMaxTime();

  // settings
  CVideoSettings GetVideoSettings();
  void SetVideoSettings(CVideoSettings &settings);
  CVideoSettingsLocked& GetVideoSettingsLocked();

protected:
  CProcessInfo();
  static std::map<std::string, CreateProcessControl> m_processControls;
  CDataCacheCore *m_dataCache = nullptr;

  // player video info
  bool m_videoIsHWDecoder;
  std::string m_videoDecoderName;
  std::string m_videoDeintMethod;
  std::string m_videoPixelFormat;
  std::string m_videoStereoMode;
  int m_videoWidth;
  int m_videoHeight;
  float m_videoFPS;
  float m_videoDAR;
  bool m_videoIsInterlaced;
  std::list<EINTERLACEMETHOD> m_deintMethods;
  EINTERLACEMETHOD m_deintMethodDefault;
  mutable CCriticalSection m_videoCodecSection;
  CVideoBufferManager m_videoBufferManager;
  std::vector<AVPixelFormat> m_pixFormats;

  // player audio info
  std::string m_audioDecoderName;
  std::string m_audioChannels;
  int m_audioSampleRate;
  int m_audioBitsPerSample;
  CCriticalSection m_audioCodecSection;

  // render info
  CCriticalSection m_renderSection;
  bool m_isClockSync;
  CRenderInfo m_renderInfo;
  int m_renderBufQueued = 0;
  int m_renderBufFree = 0;
  int m_renderBufDiscard = 0;

  // player states
  CCriticalSection m_stateSection;
  bool m_stateSeeking;
  std::atomic_int m_levelVQ;
  std::atomic_bool m_renderGuiLayer;
  std::atomic_bool m_renderVideoLayer;
  float m_tempo;
  float m_newTempo;
  float m_speed;
  float m_newSpeed;
  bool m_frameAdvance;
  time_t m_startTime;
  int64_t m_time;
  int64_t m_timeMax;
  int64_t m_timeMin;
  bool m_realTimeStream;

  // settings
  CCriticalSection m_settingsSection;
  CVideoSettings m_videoSettings;
  std::unique_ptr<CVideoSettingsLocked> m_videoSettingsLocked;
};
