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

#include "cores/IPlayer.h"
#include "threads/Thread.h"
#include <string>
#include <vector>

class CGUIDialogOK;

class CExternalPlayer : public IPlayer, public CThread
{
public:
  enum WARP_CURSOR { WARP_NONE = 0, WARP_TOP_LEFT, WARP_TOP_RIGHT, WARP_BOTTOM_RIGHT, WARP_BOTTOM_LEFT, WARP_CENTER };

  CExternalPlayer(IPlayerCallback& callback);
  virtual ~CExternalPlayer();
  virtual bool Initialize(TiXmlElement* pConfig) override;
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options) override;
  virtual bool CloseFile(bool reopen = false) override;
  virtual bool IsPlaying() const override;
  virtual void Pause() override;
  virtual bool HasVideo() const override;
  virtual bool HasAudio() const override;
  virtual void ToggleOSD() { }; // empty
  virtual void SwitchToNextLanguage();
  virtual void ToggleSubtitles();
  virtual bool CanSeek() const override;
  virtual void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  virtual void SeekPercentage(float iPercent) override;
  virtual float GetPercentage()const override;
  virtual void SetVolume(float volume) override {}
  virtual void SetDynamicRangeCompression(long drc) override {}
  virtual void SetContrast(bool bPlus) {}
  virtual void SetBrightness(bool bPlus) {}
  virtual void SetHue(bool bPlus) {}
  virtual void SetSaturation(bool bPlus) {}
  virtual void SwitchToNextAudioLanguage();
  virtual bool CanRecord() const override{ return false; }
  virtual bool IsRecording() const override{ return false; }
  virtual bool Record(bool bOnOff) override { return false; }
  virtual void SetAVDelay(float fValue = 0.0f) override;
  virtual float GetAVDelay() const override;

  virtual void SetSubTitleDelay(float fValue = 0.0f) override;
  virtual float GetSubTitleDelay() const override;

  virtual void SeekTime(int64_t iTime) override;
  virtual int64_t GetTime() const override;
  virtual int64_t GetTotalTime()const override;
  virtual void SetSpeed(int iSpeed) override;
  virtual int GetSpeed() const override;
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork() override {};
  
  virtual std::string GetPlayerState() const override;
  virtual bool SetPlayerState(const std::string& state) override;
  
#if defined(TARGET_WINDOWS)
  virtual BOOL ExecuteAppW32(const char* strPath, const char* strSwitches);
  //static void CALLBACK AppFinished(void* closure, BOOLEAN TimerOrWaitFired);
#elif defined(TARGET_ANDROID)
  virtual BOOL ExecuteAppAndroid(const char* strSwitches,const char* strPath);
#elif defined(TARGET_POSIX)
  virtual BOOL ExecuteAppLinux(const char* strSwitches);
#endif

private:
  void GetCustomRegexpReplacers(TiXmlElement *pRootElement, std::vector<std::string>& settings);
  virtual void Process() override;

  bool m_bAbortRequest;
  bool m_bIsPlaying;
  bool m_paused;
  int64_t m_playbackStartTime;
  int m_speed;
  int m_totalTime;
  mutable int m_time;
  std::string m_launchFilename;
  HWND m_hwndXbmc; 
#if defined(TARGET_WINDOWS)
  POINT m_ptCursorpos;
  PROCESS_INFORMATION m_processInfo;
#endif 
  CGUIDialogOK* m_dialog;
  int m_xPos;
  int m_yPos;
  std::string m_filename;
  std::string m_args;
  bool m_hideconsole;
  bool m_hidexbmc;
  bool m_islauncher;
  bool m_playOneStackItem;
  WARP_CURSOR m_warpcursor;
  int m_playCountMinTime;
  std::vector<std::string> m_filenameReplacers;
};
