#pragma once

/*
*      Copyright (C) 2005-2014 Team XBMC
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

#include <atomic>
#include <string>
#include "threads/CriticalSection.h"

class CDataCacheCore
{
public:
  CDataCacheCore();
  static CDataCacheCore& GetInstance();
  void Reset();
  bool HasAVInfoChanges();
  void SignalVideoInfoChange();
  void SignalAudioInfoChange();

  // player video info
  void SetVideoDecoderName(std::string name, bool isHw);
  std::string GetVideoDecoderName();
  bool IsVideoHwDecoder();
  void SetVideoDeintMethod(std::string method);
  std::string GetVideoDeintMethod();
  void SetVideoPixelFormat(std::string pixFormat);
  std::string GetVideoPixelFormat();
  void SetVideoStereoMode(std::string mode);
  std::string GetVideoStereoMode();
  void SetVideoDimensions(int width, int height);
  int GetVideoWidth();
  int GetVideoHeight();
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();

  // player audio info
  void SetAudioDecoderName(std::string name);
  std::string GetAudioDecoderName();
  void SetAudioChannels(std::string channels);
  std::string GetAudioChannels();
  void SetAudioSampleRate(int sampleRate);
  int GetAudioSampleRate();
  void SetAudioBitsPerSample(int bitsPerSample);
  int GetAudioBitsPerSample();

  // render info
  void SetRenderClockSync(bool enabled);
  bool IsRenderClockSync();

  // player states
  void SetStateSeeking(bool active);
  bool IsSeeking();
  void SetSpeed(float tempo, float speed);
  float GetSpeed();
  float GetTempo();
  bool IsPlayerStateChanged();
  void SetGuiRender(bool gui);
  bool GetGuiRender();
  void SetVideoRender(bool video);
  bool GetVideoRender();
  void SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max);

  /*!
   * \brief Get the start time
   *
   * For a typical video this will be zero. For live TV, this is a reference time
   * in units of time_t (UTC) from which time elapsed starts. Ideally this would
   * be the start of the tv show but can be any other time as well.
   */
  time_t GetStartTime();

  /*!
   * \brief Get the current time of playback
   *
   * This is the time elapsed, in ms, since the start time.
   */
  int64_t GetPlayTime();

  /*!
   * \brief Get the minumum time
   *
   * This will be zero for a typical video. With timeshift, this is the time,
   * in ms, that the player can go back. This can be before the start time.
   */
  int64_t GetMinTime();

  /*!
   * \brief Get the maximum time
   *
   * This is the maximun time, in ms, that the player can skip forward. For a
   * typical video, this will be the total length. For live TV without
   * timeshift this is zero, and for live TV with timeshift this will be the
   * buffer ahead.
   */
  int64_t GetMaxTime();

protected:
  std::atomic_bool m_hasAVInfoChanges;

  CCriticalSection m_videoPlayerSection;
  struct SPlayerVideoInfo
  {
    std::string decoderName;
    bool isHwDecoder;
    std::string deintMethod;
    std::string pixFormat;
    std::string stereoMode;
    int width;
    int height;
    float fps;
    float dar;
  } m_playerVideoInfo;

  CCriticalSection m_audioPlayerSection;
  struct SPlayerAudioInfo
  {
    std::string decoderName;
    std::string channels;
    int sampleRate;
    int bitsPerSample;
  } m_playerAudioInfo;

  CCriticalSection m_renderSection;
  struct SRenderInfo
  {
    bool m_isClockSync;
  } m_renderInfo;

  CCriticalSection m_stateSection;
  bool m_playerStateChanged = false;
  struct SStateInfo
  {
    bool m_stateSeeking;
    bool m_renderGuiLayer;
    bool m_renderVideoLayer;
    float m_tempo;
    float m_speed;
  } m_stateInfo;

  struct STimeInfo
  {
    time_t m_startTime;
    int64_t m_time;
    int64_t m_timeMax;
    int64_t m_timeMin;
  } m_timeInfo;
};
