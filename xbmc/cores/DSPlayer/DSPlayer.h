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

#include "Intsafe.h"
#include "cores/IPlayer.h"
#include "utils/Thread.h"
#include "StdString.h"
#include "StringUtils.h"
#include "DSGraph.h"

#include "StreamsManager.h"
#include "ChaptersManager.h"

enum DSPLAYER_STATE
{
  DSPLAYER_LOADING,
  DSPLAYER_LOADED,
  DSPLAYER_PLAYING,
  DSPLAYER_PAUSED,
  DSPLAYER_STOPPED,
  DSPLAYER_CLOSING,
  DSPLAYER_CLOSED
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
  virtual bool IsPaused() const { return m_pDsGraph.IsPaused(); };
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual void Pause();
  virtual bool CanSeek()                                        { return m_pDsGraph.CanSeek(); }
  virtual void Seek(bool bPlus, bool bLargeStep);//                { m_dshowCmd.Seek(bPlus,bLargeStep);}
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage()                                 { return m_pDsGraph.GetPercentage(); }
  virtual void SetVolume(long nVolume)                          { m_pDsGraph.SetVolume(nVolume); }
  virtual void SetDynamicRangeCompression(long drc)             { m_pDsGraph.SetDynamicRangeCompression(drc); }
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo(CStdString& strGeneralInfo);
  virtual bool Closing()                                      { return PlayerState == DSPLAYER_CLOSING; }

//Audio stream selection
  virtual int  GetAudioStreamCount()  { return CStreamsManager::getSingleton()->GetAudioStreamCount(); }
  virtual int  GetAudioStream()       { return CStreamsManager::getSingleton()->GetAudioStream(); }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName) { CStreamsManager::getSingleton()->GetAudioStreamName(iStream,strStreamName); };
  virtual void SetAudioStream(int iStream) { CStreamsManager::getSingleton()->SetAudioStream(iStream); };

/*  virtual int  GetSubtitleCount()     { return m_pDsGraph.GetSubtitleCount(); }
  virtual int  GetSubtitle()          { return m_pDsGraph.GetSubtitle(); }
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName) { return m_pDsGraph.GetSubtitleName(iStream, strStreamName); };
  virtual void SetSubtitle(int iStream) { return m_pDsGraph.SetSubtitle(iStream); }; */
  virtual bool AddSubtitle(const CStdString& strSubPath) { return g_dsconfig.SetSubtitlesFile(strSubPath); };
  // Chapters

  virtual int  GetChapterCount()                                { return CChaptersManager::getSingleton()->GetChapterCount(); }
  virtual int  GetChapter()                                     { return CChaptersManager::getSingleton()->GetChapter(); }
  virtual void GetChapterName(CStdString& strChapterName)       { CChaptersManager::getSingleton()->GetChapterName(strChapterName); }
  virtual int  SeekChapter(int iChapter)                        { return CChaptersManager::getSingleton()->SeekChapter(iChapter); }

  void Update(bool bPauseDrawing)                               { m_pDsGraph.Update(bPauseDrawing); }
  void GetVideoRect(CRect& SrcRect, CRect& DestRect)            { m_pDsGraph.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR)                  { fAR = m_pDsGraph.GetAspectRatio(); }

  virtual int GetChannels()                                     { return CStreamsManager::getSingleton()->GetChannels();  };
  virtual int GetBitsPerSample()                                { return CStreamsManager::getSingleton()->GetBitsPerSample(); };
  virtual int GetSampleRate()                                   { return CStreamsManager::getSingleton()->GetSampleRate(); };
  virtual int GetPictureWidth()                                 { return CStreamsManager::getSingleton()->GetPictureWidth(); }
  virtual CStdString GetAudioCodecName()                        { return CStreamsManager::getSingleton()->GetAudioCodecName(); }
  virtual CStdString GetVideoCodecName()                        { return CStreamsManager::getSingleton()->GetVideoCodecName(); }

  virtual __int64 GetTime()                                     { return m_pDsGraph.GetTime(); }
  virtual int GetTotalTime()                                    { return m_pDsGraph.GetTotalTime(); }
  virtual void ToFFRW(int iSpeed);
  virtual bool OnAction(const CAction &action);
  
//CDSPlayer
  virtual void ProcessDsWmCommand(WPARAM wParam, LPARAM lParam) { m_pDsGraph.ProcessDsWmCommand(wParam, lParam); }
  virtual HRESULT HandleGraphEvent()                            { return m_pDsGraph.HandleGraphEvent(); }
  virtual void Stop();

  static DSPLAYER_STATE PlayerState;

protected:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  int  m_currentSpeed;
  double m_currentRate;
  CDSGraph m_pDsGraph;
  CPlayerOptions m_PlayerOptions;
  CURL m_Filename;
  //bool m_bAbortRequest;
  HANDLE m_hReadyEvent;
  
};
