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

#include "DSGraph.h"
#include "winsystemwin32.h" //Important needed to get the right hwnd
#include "WindowingFactory.h" //important needed to get d3d object and device
#include "Util.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include <iomanip>
#include "Log.h"
#include "URL.h"
#include "AdvancedSettings.h"

#include <streams.h>
#include "DShowUtil/DShowUtil.h"
#include "FgManager.h"
#include "qnetwork.h"

#include "DshowUtil/MediaTypeEx.h"
#include "MediaInfoDll/MediaInfoDLL.h"
#include "Subtitles/DsSubtitleManager.h"

#include "timeutils.h"
enum 
{
	WM_GRAPHNOTIFY = WM_APP+1,
};


#include "win32exception.h"

using namespace std;
using namespace MediaInfoDLL;

CDSGraph::CDSGraph() :
m_pGraphBuilder(NULL),
m_pDsConfig(NULL)
{
  
  m_PlaybackRate = 1;
  m_currentSpeed = 0;
  g_userId = 0xACDCACDC;
  m_State.Clear();
  m_VideoInfo.Clear();
  HRESULT hr;
  hr = CoInitialize(0);
}

CDSGraph::~CDSGraph()
{
  CloseFile();
  //CoUninitialize();
}

//This is alo creating the Graph
HRESULT CDSGraph::SetFile(const CFileItem& file, const CPlayerOptions &options)
{
  HRESULT hr;
  if (m_pGraphBuilder)
	  CloseFile();

  m_pGraphBuilder = new CFGManagerPlayer(_T("CFGManagerPlayer"), NULL, g_hWnd);
  hr = m_pGraphBuilder->AddToROT();
  //Adding every filters required for this file into the igraphbuilder
  hr = m_pGraphBuilder->RenderFileXbmc(file);
  if (FAILED(hr))
    return hr;
  //This
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaSeeking);
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaControl);
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaEvent);
  hr = m_pGraphBuilder.QueryInterface(&m_pBasicAudio);
  hr = m_pGraphBuilder.QueryInterface(&m_pBasicVideo);
  m_pDsConfig = new CDSConfig();
  //Get all custom interface
  m_pDsConfig->LoadGraph(m_pGraphBuilder);
  LONGLONG tmestamp;
  tmestamp = CTimeUtils::GetTimeMS();
  CLog::Log(LOGDEBUG,"%s Timestamp before loading video info with mediainfo.dll %I64d",__FUNCTION__,tmestamp);
  UpdateCurrentVideoInfo(file.GetAsUrl().GetFileName());
  tmestamp = CTimeUtils::GetTimeMS();
  CLog::Log(LOGDEBUG,"%s Timestamp after loading video info with mediainfo.dll  %I64d",__FUNCTION__,tmestamp);
  

  SetVolume(g_stSettings.m_nVolumeLevel);
  UpdateState();

  bool ok;
  ok = g_dllMpcSubs.Load(); //return true if loaded
  ok = g_dllMpcSubs.LoadSubtitles(m_Filename.c_str(),m_pGraphBuilder,"H:\\Downloads\\Two.and.a.Half.Men.S07E11.Warning.Its.Dirty.HDTV.XviD-FQM.srt");
  g_dllMpcSubs.EnableSubtitle(true);
  g_dllMpcSubs.GetSubtitlesList();

  if (m_pMediaControl)
    m_pMediaControl->Run();
  m_currentSpeed = 10000;

  return hr;
}

void CDSGraph::CloseFile()
{
  OnPlayStop();
  if (m_pDsConfig)
    m_pDsConfig = NULL;
  HRESULT hr;
  if (m_pGraphBuilder)
    hr = m_pGraphBuilder->RemoveFromROT();
  BeginEnumFilters(m_pGraphBuilder,pEF,pBF)
  {
    m_pGraphBuilder->RemoveFilter(pBF);
  }
  EndEnumFilters
  
  if (m_pGraphBuilder)
    m_pGraphBuilder.Release();
  
}

bool CDSGraph::InitializedOutputDevice()
{
#ifdef HAS_VIDEO_PLAYBACK
  return g_renderManager.IsStarted();
#else
  return false;
#endif
}

void CDSGraph::UpdateTime()
{
  if (!m_pMediaSeeking)
    return;
  REFTIME rt = (REFTIME) 0;
  LONGLONG Position;
// Should we return a media position
  if(m_VideoInfo.time_format == TIME_FORMAT_MEDIA_TIME)
  {
  if(SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
    m_State.time = double(Position) / TIME_FORMAT_TO_MS;
  }
  else
  {
  if(SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
    m_State.time = double(Position);
  }
}

void CDSGraph::UpdateState()
{
  HRESULT hr;
  LONGLONG Duration;
  hr = m_pMediaSeeking->GetTimeFormatA(&m_VideoInfo.time_format);
  // Should we seek using IMediaSelection
  if(m_VideoInfo.time_format == TIME_FORMAT_MEDIA_TIME)
  {
    if(SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
      m_State.time_total = (double) Duration / TIME_FORMAT_TO_MS;
  }
  else
  {
    if(SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
       m_State.time_total =  (double) Duration;
  }
  hr = m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);
  if (m_State.current_filter_state != State_Running)
    m_PlaybackRate = 0;
  else
  {
    double newRate;
    hr = m_pMediaSeeking->GetRate(&newRate);
    m_PlaybackRate=(int)newRate;
  }
  
}
void CDSGraph::UpdateCurrentVideoInfo(CStdString currentFile)
{

  m_VideoInfo.dxva_info= m_pDsConfig->GetDxvaMode();
  
  MediaInfo MI;
  MI.Open(currentFile.c_str());
  MI.Option(_T("Complete"));
  m_VideoInfo.codec_video.AppendFormat("Codec video:%s",MI.Get(Stream_Video,0,_T("CodecID/Hint")).c_str());
  m_VideoInfo.codec_video.AppendFormat("@%i Kbps",((int)MI.Get(Stream_Video,0,_T("BitRate")).c_str()) / 1024);
  m_VideoInfo.codec_video.AppendFormat(" Res:%sx%s @ %s",MI.Get(Stream_Video,0,_T("Width")).c_str(),
                                                         MI.Get(Stream_Video,0,_T("Height")).c_str(),
                                                         MI.Get(Stream_Video,0,_T("FrameRate/String")).c_str());
  CLog::Log(LOGDEBUG,"%s",m_VideoInfo.codec_video.c_str());
  m_VideoInfo.codec_audio.AppendFormat("Codec audio:%s",MI.Get(Stream_Audio,0,_T("CodecID/Hint")).c_str());
  m_VideoInfo.codec_audio.AppendFormat("@%i Kbps,",((int)MI.Get(Stream_Audio,0,_T("BitRate")).c_str()) / 1024);
  m_VideoInfo.codec_audio.AppendFormat(" %s chn, %s, %s",MI.Get(Stream_Audio,0,_T("Channel(s)")).c_str(),
                                                         MI.Get(Stream_Audio,0,_T("Resolution/String")).c_str(),
                                                         MI.Get(Stream_Audio,0,_T("SamplingRate/String")).c_str());
  CLog::Log(LOGDEBUG,"%s",m_VideoInfo.codec_audio.c_str());
//Codec:XviD@1196 Kbps Res:624x352 @ 23.976 fps
//Codec:MP3@1195 Kbps, 2 chn, 16 bits, 48.0 KHz
//need too fix the audio kbps
}


HRESULT CDSGraph::HandleGraphEvent(void)
{
  LONG evCode;
  LONG_PTR evParam1, evParam2;
  HRESULT hr=S_OK;
    // Make sure that we don't access the media event interface
    // after it has already been released.
  if (!m_pMediaEvent)
    return S_OK;

  // Process all queued events
  while(SUCCEEDED(m_pMediaEvent->GetEvent(&evCode, &evParam1, &evParam2, 0)))
  {
// Free memory associated with callback, since we're not using it
  hr = m_pMediaEvent->FreeEventParams(evCode, evParam1, evParam2);
  switch(evCode)
  {
    case EC_STEP_COMPLETE:
		  CLog::Log(LOGDEBUG,"%s EC_STEP_COMPLETE",__FUNCTION__);
      g_application.m_pPlayer->CloseFile();
      break;
    case EC_COMPLETE:
			CLog::Log(LOGDEBUG,"%s EC_COMPLETE",__FUNCTION__);
			g_application.m_pPlayer->CloseFile();
			break;
    case EC_USERABORT:
			CLog::Log(LOGDEBUG,"%s EC_USERABORT",__FUNCTION__);
			g_application.m_pPlayer->CloseFile();
			break;
    case EC_ERRORABORT:
		  CLog::Log(LOGDEBUG,"%s EC_ERRORABORT",__FUNCTION__);
      g_application.m_pPlayer->CloseFile();
      break;
		case EC_STATE_CHANGE:
      CLog::Log(LOGDEBUG,"%s EC_STATE_CHANGE",__FUNCTION__);
      break;
		case EC_DEVICE_LOST:
      CLog::Log(LOGDEBUG,"%s EC_DEVICE_LOST",__FUNCTION__);
      break;
		case EC_VMR_RECONNECTION_FAILED:
      CLog::Log(LOGDEBUG,"%s EC_VMR_RECONNECTION_FAILED",__FUNCTION__);
      break;
      default:
      break;
    }
    }

    return hr;
}

//USER ACTIONS
void CDSGraph::SetVolume(long nVolume)
{
  if (m_pBasicAudio)
    m_pBasicAudio->put_Volume(nVolume);
}
void CDSGraph::SetDynamicRangeCompression(long drc)
{
  if (m_pBasicAudio)
    m_pBasicAudio->put_Volume(drc);
}

void CDSGraph::OnPlayStop()
{
  LONGLONG pos = 0;
  
  if (m_pMediaSeeking)
    m_pMediaSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
  if (m_pMediaControl)
    m_pMediaControl->Stop();
  BeginEnumFilters(m_pGraphBuilder, pEF, pBF)
  {
    CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF;
    CComQIPtr<IFileSourceFilter> pFSF = pBF;
    if(pAMNS && pFSF)
    {
      WCHAR* pFN = NULL;
      AM_MEDIA_TYPE mt;
      if(SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN)
      {
        pFSF->Load(pFN, NULL);
        CoTaskMemFree(pFN);
      }
      break;
    }
  }
  EndEnumFilters

}
void CDSGraph::Play()
{
  UpdateState();
  if (m_State.current_filter_state != State_Running)
    m_pMediaControl->Run();
}
void CDSGraph::Pause()
{
  UpdateState();
  if (m_currentSpeed == 0)
  {
    if (m_State.current_filter_state != State_Running)
      m_pMediaControl->Run();
    m_currentSpeed = 10000;
	//SetPlaySpeed(1);
  }
  else
  {
    if (m_pMediaControl)
	  {
	  if ( SUCCEEDED( m_pMediaControl->Pause() ) )
	    m_currentSpeed = 0;
	}
  
  }
}

//The fastforward is based on mediaportal fastforward
void CDSGraph::DoFFRW(int currentSpeed)
{
  if (!m_pMediaSeeking)
    return;
  m_currentSpeed = currentSpeed;
  HRESULT hr;
  LONGLONG earliest, latest, current, stop, rewind, pStop;
  m_pMediaSeeking->GetAvailable(&earliest,&latest);
  m_pMediaSeeking->GetPositions(&current,&stop);
  
  LONGLONG lTimerInterval = 300;
  rewind = (LONGLONG)(current + (2 * (LONGLONG)(lTimerInterval) * currentSpeed));
  pStop = 0;
  if ((rewind < earliest) && (currentSpeed < 0))
  {
    currentSpeed = 10000;
    rewind = earliest;
    hr = m_pMediaSeeking->SetPositions(&earliest, AM_SEEKING_AbsolutePositioning, (LONGLONG) 0, AM_SEEKING_NoPositioning);
	  m_State.time = double(earliest) / TIME_FORMAT_TO_MS;;
    m_pMediaControl->Run();
    return;
  }

  if ((rewind > (latest - 100000)) && (currentSpeed > 0))
  {
    currentSpeed = 10000;
    rewind = latest - 100000;

    hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning, &pStop, AM_SEEKING_NoPositioning);

		m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
    m_pMediaControl->Run();
    return;
  }
  //seek to new moment in time
  hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame,&pStop, AM_SEEKING_NoPositioning);
	m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
  m_pMediaControl->StopWhenReady();

}

bool CDSGraph::IsPaused() const
{
  if (!m_pMediaControl)
    return true;

  return (m_currentSpeed == 0);
  /*m_pMediaControl->GetState(INFINITE, (OAFilterState *)&m_State.current_filter_state);
  if (m_State.current_filter_state == State_Running )
    return false;
  else if (m_State.current_filter_state == State_Paused)
    return true;
  else
    return true;*/
}

void CDSGraph::SeekInMilliSec(double sec)
{
  HRESULT hr;
  LONGLONG seekrequest,earliest,latest;
  //Really need to verify those com interface before using them 
  //even if the player finished playback those function can still be called
  if ((!m_pMediaControl) || (!m_pMediaSeeking))
    return;
  m_pMediaSeeking->GetAvailable(&earliest,&latest);
  hr = m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);

  seekrequest = ( LONGLONG )( sec * 10000 );
  if ( seekrequest < 0 )
    seekrequest = 0;
  m_pMediaSeeking->SetPositions(&seekrequest, AM_SEEKING_AbsolutePositioning, NULL,AM_SEEKING_NoPositioning);
  if(m_State.current_filter_state == State_Stopped)
  {
    m_pMediaControl->Pause();
    m_pMediaControl->GetState(INFINITE, (OAFilterState *)&m_State.current_filter_state);
    m_pMediaControl->Stop();
  }
}
void CDSGraph::Seek(bool bPlus, bool bLargeStep)
{
  if (!m_pMediaSeeking || !m_pMediaControl)
    return;
  __int64 seek;
  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;
    seek = (__int64)(GetTotalTimeInMsec()*(GetPercentage()+percent)/100);
  }

  
  UpdateTime();
  SeekInMilliSec(seek);
}

// return time in ms
__int64 CDSGraph::GetTime()
{
  return llrint(m_State.time);
}

// return length in msec
__int64 CDSGraph::GetTotalTimeInMsec()
{
  return llrint(m_State.time_total);
}

int CDSGraph::GetTotalTime()
{
  return (int)(GetTotalTimeInMsec() / 1000);
}

float CDSGraph::GetPercentage()
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

std::string CDSGraph::GetGeneralInfo()
{
  return m_VideoInfo.dxva_info.c_str();
}

std::string CDSGraph::GetVideoInfo()
{
  return m_VideoInfo.codec_video.c_str();
}

bool CDSGraph::CanSeek()
{
//Its not doing any error
  HRESULT hr;
  DWORD seekcaps;//AM_SEEKING_SEEKING_CAPABILITIES 
  seekcaps = AM_SEEKING_CanSeekForwards;
  seekcaps |= AM_SEEKING_CanSeekBackwards;
  seekcaps |= AM_SEEKING_CanSeekAbsolute;
  hr = m_pMediaSeeking->CheckCapabilities(&seekcaps);
  if (SUCCEEDED(hr))
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CDSGraph::ProcessDsWmCommand(WPARAM wParam, LPARAM lParam)
{
  switch(wParam)
  {
    case ID_PLAY_PLAY:
      if (m_pMediaControl)
	    m_pMediaControl->Run();
	  break;
	case ID_SEEK_TO:
      SeekInMilliSec((LPARAM)lParam);
      break;
    case ID_SEEK_FORWARDSMALL:
      Seek(true,false);
	  CLog::Log(LOGDEBUG,"%s ID_SEEK_FORWARDSMALL",__FUNCTION__);
	  break;
	case ID_SEEK_FORWARDLARGE:
      Seek(true,true);
	  CLog::Log(LOGDEBUG,"%s ID_SEEK_FORWARDLARGE",__FUNCTION__);
	  break;
    case ID_SEEK_BACKWARDSMALL:
      Seek(false,false);
	  CLog::Log(LOGDEBUG,"%s ID_SEEK_BACKWARDSMALL",__FUNCTION__);
	  break;
	case ID_SEEK_BACKWARDLARGE:
	  Seek(false,true);
	  CLog::Log(LOGDEBUG,"%s ID_SEEK_BACKWARDLARGE",__FUNCTION__);
	  break;
	case ID_PLAY_PAUSE:
      Pause();
	  CLog::Log(LOGDEBUG,"%s ID_PLAY_PAUSE",__FUNCTION__);
	  break;
	case ID_PLAY_STOP:
      OnPlayStop();
	  CLog::Log(LOGDEBUG,"%s ID_PLAY_STOP",__FUNCTION__);
      break;
    case ID_STOP_DSPLAYER:
      OnPlayStop();
	  CLog::Log(LOGDEBUG,"%s ID_STOP_DSPLAYER",__FUNCTION__);
      break;
  }
}
