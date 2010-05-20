#pragma once

/*
*      Copyright (C) 2005-2009 Team XBMC
*      http://www.xbmc.org
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

#include "cores/IPlayer.h"
#include "utils/Thread.h"
#include "SingleLock.h"
#include "StdString.h"
#include "DSGraph.h"
#include "DSClock.h"

#include "StreamsManager.h"
#include "ChaptersManager.h"

#include "TimeUtils.h"
#include "Event.h"
#include "GUIDialogBoxBase.h"

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

#define START_PERFORMANCE_COUNTER { int64_t start = CurrentHostCounter();
#define END_PERFORMANCE_COUNTER int64_t end = CurrentHostCounter(); \
  CLog::Log(LOGINFO, "%s Elapsed time: %.2fms", __FUNCTION__, 1000.f * (end - start) / CurrentHostFrequency()); }

enum DSPLAYER_STATE
{
  DSPLAYER_LOADING,
  DSPLAYER_LOADED,
  DSPLAYER_PLAYING,
  DSPLAYER_PAUSED,
  DSPLAYER_STOPPED,
  DSPLAYER_CLOSING,
  DSPLAYER_CLOSED,
  DSPLAYER_ERROR
};

class CDSPlayer : public IPlayer, public CThread
{
public:
//IPlayer
  CDSPlayer(IPlayerCallback& callback);
  virtual ~CDSPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) {}
  virtual void UnRegisterAudioCallback()                        {}
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual bool IsCaching() const { return false; };
  virtual bool IsPaused() const { return g_dsGraph->IsPaused(); };
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual bool HasMenu() { return g_dsGraph->IsDvd(); };
  bool IsInMenu() const { return g_dsGraph->IsInMenu(); };
  virtual void Pause();
  virtual bool CanSeek()                                        { return g_dsGraph->CanSeek(); }
  virtual void Seek(bool bPlus, bool bLargeStep);//                { m_dshowCmd.Seek(bPlus,bLargeStep);}
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage()                                 { return g_dsGraph->GetPercentage(); }
  virtual void SetVolume(long nVolume)                          { g_dsGraph->SetVolume(nVolume); }
  //virtual void SetDynamicRangeCompression(long drc)             { m_pDsGraph.SetDynamicRangeCompression(drc); }
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo(CStdString& strGeneralInfo);
  virtual bool Closing()                                      { return PlayerState == DSPLAYER_CLOSING; }

//Audio stream selection
  virtual int  GetAudioStreamCount() { return CStreamsManager::Get()->GetAudioStreamCount(); }
  virtual int  GetAudioStream() { return CStreamsManager::Get()->GetAudioStream(); }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName) { CStreamsManager::Get()->GetAudioStreamName(iStream,strStreamName); };
  virtual void SetAudioStream(int iStream) { CStreamsManager::Get()->SetAudioStream(iStream); };

  virtual int  GetSubtitleCount() { return CStreamsManager::Get()->SubtitleManager->GetSubtitleCount(); }
  virtual int  GetSubtitle() { return CStreamsManager::Get()->SubtitleManager->GetSubtitle(); }
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName) { CStreamsManager::Get()->SubtitleManager->GetSubtitleName(iStream, strStreamName); }
  virtual void SetSubtitle(int iStream) { CStreamsManager::Get()->SubtitleManager->SetSubtitle(iStream); }
  virtual bool GetSubtitleVisible() { return CStreamsManager::Get()->SubtitleManager->GetSubtitleVisible(); }
  virtual void SetSubtitleVisible( bool bVisible ) { CStreamsManager::Get()->SubtitleManager->SetSubtitleVisible(bVisible); }

  virtual int AddSubtitle(const CStdString& strSubPath) { return CStreamsManager::Get()->SubtitleManager->AddSubtitle(strSubPath); };
  virtual void SetSubTitleDelay(float fValue = 0.0f) { CStreamsManager::Get()->SubtitleManager->SetSubtitleDelay(fValue); };
  virtual float GetSubTileDelay(void) { return CStreamsManager::Get()->SubtitleManager->GetSubtitleDelay(); }
  // Chapters

  virtual int  GetChapterCount() { CSingleLock lock(m_StateSection); return CChaptersManager::Get()->GetChapterCount(); }
  virtual int  GetChapter() { CSingleLock lock(m_StateSection); return CChaptersManager::Get()->GetChapter(); }
  virtual void GetChapterName(CStdString& strChapterName) { CSingleLock lock(m_StateSection); CChaptersManager::Get()->GetChapterName(strChapterName); }
  virtual int  SeekChapter(int iChapter) { return CChaptersManager::Get()->SeekChapter(iChapter); }

  void Update(bool bPauseDrawing) { g_renderManager.Update(bPauseDrawing); }
  void GetVideoRect(CRect& SrcRect, CRect& DestRect) { g_renderManager.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR) { fAR = g_renderManager.GetAspectRatio(); }

  virtual int GetChannels() { return CStreamsManager::Get()->GetChannels();  };
  virtual int GetBitsPerSample() { return CStreamsManager::Get()->GetBitsPerSample(); };
  virtual int GetSampleRate() { return CStreamsManager::Get()->GetSampleRate(); };
  virtual int GetPictureWidth() { return CStreamsManager::Get()->GetPictureWidth(); }
  virtual CStdString GetAudioCodecName() { return CStreamsManager::Get()->GetAudioCodecName(); }
  virtual CStdString GetVideoCodecName() { return CStreamsManager::Get()->GetVideoCodecName(); }

  virtual __int64 GetTime() { CSingleLock lock(m_StateSection); return g_dsGraph->GetTime(); }
  virtual int GetTotalTime() { CSingleLock lock(m_StateSection); return g_dsGraph->GetTotalTime(); }
  virtual void ToFFRW(int iSpeed);
  virtual bool OnAction(const CAction &action);
  
//CDSPlayer
  virtual void ProcessMessage(WPARAM wParam, LPARAM lParam) { g_dsGraph->ProcessMessage(wParam, lParam); }
  virtual HRESULT HandleGraphEvent() { return g_dsGraph->HandleGraphEvent(); }
  virtual void HandleStart();
  virtual void Stop();

  static DSPLAYER_STATE PlayerState;
  static CFileItem currentFileItem;
  static CGUIDialogBoxBase* errorWindow;

  CCriticalSection m_StateSection;
protected:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  int  m_currentRate;
  bool m_bSpeedChanged;

  CDSClock m_pDsClock;
  CPlayerOptions m_PlayerOptions;
  CURL m_Filename;
  CEvent m_hReadyEvent;
private:
  bool m_callSetFileFromThread;
};
