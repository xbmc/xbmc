#pragma once

/*
 *      Copyright (C) 2008-2009 Plex
 *      http://www.plexapp.com
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <vector>

#include "cores/IPlayer.h"
#include "HTTP.h"
#include "threads/Thread.h"
#include "dialogs/GUIDialogCache.h"

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace std;
namespace ipc = boost::interprocess;
 
class CPlexMediaServerPlayer : public IPlayer, public CThread
{
public:
  
  CPlexMediaServerPlayer(IPlayerCallback& callback);
  virtual ~CPlexMediaServerPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {}
  virtual void UnRegisterAudioCallback()                        {}
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool OnAction(const CAction &action);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual void ToggleOSD() {}
  virtual void ExecuteKeyCommand(char key, bool shift=false, bool control=false, bool command=false, bool option=false);
  virtual void SwitchToNextLanguage() {}
  virtual void ToggleSubtitles() {}
  virtual void ToggleFrameDrop() {}
  virtual bool CanSeek();
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume); 
  virtual void SetDynamicRangeCompression(long drc) {}
  virtual void SetContrast(bool bPlus) {}
  virtual void SetBrightness(bool bPlus) {}
  virtual void SetHue(bool bPlus) {}
  virtual void SetSaturation(bool bPlus) {}
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       {}
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect)      {}
  virtual void GetVideoAspectRatio(float& fAR)                  {}
  virtual void SwitchToNextAudioLanguage() {}
  virtual bool CanRecord() { return false; }
  virtual bool IsRecording() { return false; }
  virtual bool Record(bool bOnOff) { return false; }
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();
  void Render();

  virtual void SetSubTitleDelay(float fValue = 0.0f) {}
  virtual float GetSubTitleDelay() { return 0.0f; }

  virtual void SeekTime(__int64 iTime);
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual void DoAudioWork() {}
  
  virtual CStdString GetPlayerState() { return ""; }
  virtual bool SetPlayerState(CStdString state) { return true; }

  static void RequireServerRestart() { g_needToRestartMediaServer = true; }
  static bool IsRestartRequired() { return g_needToRestartMediaServer; }
  
  void SetCacheDialog(CGUIDialogCache* cacheDialog) { m_pDlgCache = cacheDialog; }
  
private:

  void OnFrameMap(const string& args);
  void OnPlaybackEnded(const string& args);
  void OnPlaybackStarted();
  void OnNewFrame(); 
  void OnPaused();
  void OnResumed();
  void OnProgress(int nPct); 
  
  virtual void Process();
  
  double getTime();

  bool m_paused;
  bool m_playing;
  
  __int64 m_clock;
  int     m_pct;
  DWORD m_lastTime;
  int m_speed;
  
  int          m_totalTime;
  int          m_width;
  int          m_height;
  
  CGUIDialogCache* m_pDlgCache;
  CHTTP      m_http;
  
  int m_frameCount;
  
  static bool g_needToRestartMediaServer;
  
  ipc::named_mutex     m_frameMutex;
  ipc::named_condition m_frameCond;
  ipc::mapped_region*  m_mappedRegion;
};
