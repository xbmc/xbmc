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

#include "cores/IPlayer.h"
#include "threads/CriticalSection.h"
#include <list>
#include <string>

class CProcessInfo
{
public:
  static CProcessInfo* CreateInstance();
  virtual ~CProcessInfo();

  // player video info
  void ResetVideoCodecInfo();
  void SetVideoDecoderName(std::string name, bool isHw);
  std::string GetVideoDecoderName();
  bool IsVideoHwDecoder();
  void SetVideoDeintMethod(std::string method);
  std::string GetVideoDeintMethod();
  void SetVideoPixelFormat(std::string pixFormat);
  std::string GetVideoPixelFormat();
  void SetVideoDimensions(int width, int height);
  void GetVideoDimensions(int &width, int &height);
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();
  virtual EINTERLACEMETHOD GetFallbackDeintMethod();
  void UpdateDeinterlacingMethods(std::list<EINTERLACEMETHOD> &methods);
  bool Supports(EINTERLACEMETHOD method);

  // player audio info
  void ResetAudioCodecInfo();
  void SetAudioDecoderName(std::string name);
  std::string GetAudioDecoderName();
  void SetAudioChannels(std::string channels);
  std::string GetAudioChannels();
  void SetAudioSampleRate(int sampleRate);
  int GetAudioSampleRate();
  void SetAudioBitsPerSample(int bitsPerSample);
  int GetAudioBitsPerSample();
  virtual bool AllowDTSHDDecode();

  // render info
  void SetRenderClockSync(bool enabled);
  bool IsRenderClockSync();

protected:
  CProcessInfo();

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
  CCriticalSection m_videoCodecSection;

  // player audio info
  std::string m_audioDecoderName;
  std::string m_audioChannels;
  int m_audioSampleRate;
  int m_audioBitsPerSample;
  CCriticalSection m_audioCodecSection;

  // render info
  CCriticalSection m_renderSection;
  bool m_isClockSync;
};
