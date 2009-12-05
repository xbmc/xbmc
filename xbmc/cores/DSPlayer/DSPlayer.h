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
//#include <stdio.h>
#include "StdString.h"
#include "StringUtils.h"

#include "DSGraph.h"






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
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual bool CanSeek()                                        { return m_pDsGraph.CanSeek(); }
  virtual void Seek(bool bPlus, bool bLargeStep);//                { m_dshowCmd.Seek(bPlus,bLargeStep);}
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage()                                 { return m_pDsGraph.GetPercentage(); }
  virtual void SetVolume(long nVolume)                          { m_pDsGraph.SetVolume(nVolume); }
  virtual void SetDynamicRangeCompression(long drc)             { m_pDsGraph.SetDynamicRangeCompression(drc); }
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo(CStdString& strGeneralInfo);

//Audio stream selection
  virtual int  GetAudioStreamCount()  { return m_pDsGraph.GetAudioStreamCount(); }
  virtual int  GetAudioStream()       { return m_pDsGraph.GetAudioStream(); }
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName) { m_pDsGraph.GetAudioStreamName(iStream,strStreamName); };
  virtual void SetAudioStream(int iStream) { m_pDsGraph.SetAudioStream(iStream); };
  void Update(bool bPauseDrawing)                               { m_pDsGraph.Update(bPauseDrawing); }
  void GetVideoRect(CRect& SrcRect, CRect& DestRect)  { m_pDsGraph.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR)                  { fAR = m_pDsGraph.GetAspectRatio(); }
  //virtual void SetAVDelay(float fValue = 0.0f);
  //virtual float GetAVDelay();
   
  //virtual bool HasMenu();
  //virtual int GetAudioBitrate();
  //virtual int GetVideoBitrate();
  //virtual int GetSourceBitrate();
  //virtual int GetChannels();
  //virtual CStdString GetAudioCodecName();
  //virtual CStdString GetVideoCodecName();
  virtual __int64 GetTime()                                     { return m_pDsGraph.GetTime(); }
  virtual int GetTotalTime()                                    { return m_pDsGraph.GetTotalTime(); }
  virtual void ToFFRW(int iSpeed);
  virtual bool OnAction(const CAction &action);
  
//CDSPlayer
  virtual void ProcessDsWmCommand(WPARAM wParam, LPARAM lParam) { m_pDsGraph.ProcessDsWmCommand(wParam, lParam); }
  virtual HRESULT HandleGraphEvent()                            { return m_pDsGraph.HandleGraphEvent(); }
  virtual void Stop();
protected:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  int m_currentSpeed;
  double m_currentRate;
  CDSGraph m_pDsGraph;
  CPlayerOptions m_PlayerOptions;
  CURL m_Filename;
  bool m_bAbortRequest;
  HANDLE m_hReadyEvent;
  
};
