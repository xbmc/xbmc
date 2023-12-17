/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "cores/IPlayer.h"
#include "threads/Thread.h"

#include <string>
#include <vector>

class CGUIDialogOK;

class CExternalPlayer : public IPlayer, public CThread
{
public:
  enum WARP_CURSOR { WARP_NONE = 0, WARP_TOP_LEFT, WARP_TOP_RIGHT, WARP_BOTTOM_RIGHT, WARP_BOTTOM_LEFT, WARP_CENTER };

  explicit CExternalPlayer(IPlayerCallback& callback);
  ~CExternalPlayer() override;
  bool Initialize(TiXmlElement* pConfig) override;
  bool OpenFile(const CFileItem& file, const CPlayerOptions &options) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  void Pause() override;
  bool HasVideo() const override;
  bool HasAudio() const override;
  bool CanSeek() const override;
  void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  void SeekPercentage(float iPercent) override;
  void SetVolume(float volume) override {}
  void SetDynamicRangeCompression(long drc) override {}
  void SetAVDelay(float fValue = 0.0f) override;
  float GetAVDelay() override;

  void SetSubTitleDelay(float fValue = 0.0f) override;
  float GetSubTitleDelay() override;

  void SeekTime(int64_t iTime) override;
  void SetSpeed(float speed) override;

  bool SetPlayerState(const std::string& state) override;

#if defined(TARGET_WINDOWS_DESKTOP)
  bool ExecuteAppW32(const char* strPath, const char* strSwitches);
  //static void CALLBACK AppFinished(void* closure, BOOLEAN TimerOrWaitFired);
#elif defined(TARGET_ANDROID)
  bool ExecuteAppAndroid(const char* strSwitches,const char* strPath);
#elif defined(TARGET_POSIX)
  bool ExecuteAppLinux(const char* strSwitches);
#endif

private:
  void GetCustomRegexpReplacers(TiXmlElement *pRootElement, std::vector<std::string>& settings);
  void Process() override;

  bool m_bAbortRequest;
  bool m_bIsPlaying;
  std::chrono::time_point<std::chrono::steady_clock> m_playbackStartTime;
  float m_speed;
  int m_time;
  std::string m_launchFilename;
#if defined(TARGET_WINDOWS_DESKTOP)
  POINT m_ptCursorpos;
  PROCESS_INFORMATION m_processInfo;
#endif
  CGUIDialogOK* m_dialog;
#if defined(TARGET_WINDOWS_DESKTOP)
  int m_xPos;
  int m_yPos;
#endif
  std::string m_filename;
  std::string m_args;
  bool m_hideconsole;
  bool m_hidexbmc;
  bool m_islauncher;
  bool m_playOneStackItem;
  WARP_CURSOR m_warpcursor;
  int m_playCountMinTime;
  std::vector<std::string> m_filenameReplacers;
  CFileItem m_file;
};
