/*
 *      Copyright (C) 2005-2016 Team XBMC
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
#pragma once

#include "VideoBuffer.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/VideoRenderers/RenderInfo.h"
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
  static void RegisterProcessControl(std::string id, CreateProcessControl createFunc);
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
  void SetVideoDimensions(int width, int height);
  void GetVideoDimensions(int &width, int &height);
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();
  virtual EINTERLACEMETHOD GetFallbackDeintMethod();
  virtual void SetSwDeinterlacingMethods();
  void UpdateDeinterlacingMethods(std::list<EINTERLACEMETHOD> &methods);
  bool Supports(EINTERLACEMETHOD method);
  void SetDeinterlacingMethodDefault(EINTERLACEMETHOD method);
  EINTERLACEMETHOD GetDeinterlacingMethodDefault();
  CVideoBufferManager& GetVideoBufferManager();

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

  // render info
  void SetRenderClockSync(bool enabled);
  bool IsRenderClockSync();
  void UpdateRenderInfo(CRenderInfo &info);
  void UpdateRenderBuffers(int queued, int discard, int free);
  void GetRenderBuffers(int &queued, int &discard, int &free);
  virtual std::vector<AVPixelFormat> GetRenderFormats();

  // player states
  void SetStateSeeking(bool active);
  bool IsSeeking();
  void SetSpeed(float speed);
  void SetNewSpeed(float speed);
  float GetNewSpeed();
  void SetTempo(float tempo);
  void SetNewTempo(float tempo);
  float GetNewTempo();
  virtual bool IsTempoAllowed(float tempo);

  void SetLevelVQ(int level);
  int GetLevelVQ();
  void SetGuiRender(bool gui);
  bool GetGuiRender();
  void SetVideoRender(bool video);
  bool GetVideoRender();

protected:
  CProcessInfo() = default;
  static std::map<std::string, CreateProcessControl> m_processControls;
  CDataCacheCore *m_dataCache = nullptr;

  // player video info
  bool m_videoIsHWDecoder;
  std::string m_videoDecoderName;
  std::string m_videoDeintMethod;
  std::string m_videoPixelFormat;
  int m_videoWidth;
  int m_videoHeight;
  float m_videoFPS;
  float m_videoDAR;
  std::list<EINTERLACEMETHOD> m_deintMethods;
  EINTERLACEMETHOD m_deintMethodDefault;
  CCriticalSection m_videoCodecSection;
  CVideoBufferManager m_videoBufferManager;

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
};
