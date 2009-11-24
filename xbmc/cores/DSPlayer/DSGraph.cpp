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

#include "DshowUtil/MediaTypeEx.h"
#include "MediaInfoDll/MediaInfoDLL.h"
enum 
{
	WM_GRAPHNOTIFY = WM_APP+1,
};


#include "win32exception.h"

using namespace std;
using namespace MediaInfoDLL;

CDSGraph::CDSGraph() :
                 m_pGraphBuilder(NULL)
{
  m_PlaybackRate = 1;
  g_userId = 0xACDCACDC;
  m_State.Clear();
  m_VideoInfo.Clear();
  CoInitialize(0);
}

CDSGraph::~CDSGraph()
{
  CloseFile();
  CoUninitialize();
}

//This is alo creating the Graph
HRESULT CDSGraph::SetFile(const CFileItem& file)
{
  HRESULT hr;
  if (m_pGraphBuilder)
	  CloseFile();
  CStdString homepath;
  CUtil::GetHomePath(homepath);
  
  m_pGraphBuilder = new CFGManagerPlayer(_T("CFGManagerPlayer"), NULL, g_hWnd,homepath);
  hr = m_pGraphBuilder->AddToROT();
  //Adding every filters required for this file into the igraphbuilder
  hr = m_pGraphBuilder->RenderFileXbmc(file);
  
  //This 
  hr = m_pGraphBuilder->GetXbmcVideoDecFilter(&m_pIMpcDecFilter);
  hr = m_pGraphBuilder->GetFfdshowVideoDecFilter(&m_pIffdDecFilter);
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaSeeking);

  //hr = m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
  //-->> TIME_FORMAT_FRAME for frame by frame
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaControl);
  hr = m_pGraphBuilder.QueryInterface(&m_pMediaEvent);
  hr = m_pGraphBuilder.QueryInterface(&m_pBasicAudio);
  hr = m_pGraphBuilder.QueryInterface(&m_pBasicVideo);
  UpdateCurrentVideoInfo(file.GetAsUrl().GetFileName());

  SetVolume(g_stSettings.m_nVolumeLevel);
  UpdateState();
  if (m_pMediaControl)
    m_pMediaControl->Run();
  
  
  //m_pGraphBuilder
  return hr;
}

void CDSGraph::CloseFile()
{
  OnPlayStop();
  m_pMediaControl.Release();
  m_pMediaEvent.Release();
  m_pMediaSeeking.Release();
  m_pBasicAudio.Release();
  if (m_pGraphBuilder)
  {
    m_pGraphBuilder->RemoveFromROT();
    m_pGraphBuilder.Release();
  }
}

bool CDSGraph::InitializedOutputDevice()
{
#ifdef HAS_VIDEO_PLAYBACK
  return g_renderManager.IsStarted();
#else
  return false;
#endif
}

HRESULT CDSGraph::CloseGraph()
{
  try
  {
  HRESULT hr;
  if (!m_pGraphBuilder)
    return S_OK;
  m_pMediaControl->Stop();
  hr = m_pGraphBuilder->Abort();
  if FAILED(m_pGraphBuilder->RemoveFromROT())
    CLog::Log(LOGERROR,"%s m_pGraphBuilder->RemoveFromROT",__FUNCTION__);  
  }
  catch (...)
  {
	  CLog::Log(LOGERROR,"%s error while closing graph",__FUNCTION__);
  }
  m_File.Close();
  m_pMediaControl.Release();
  m_pBasicAudio.Release();
  m_pGraphBuilder.Release();
  return S_OK;
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
  if (m_pIffdDecFilter)
  {
    //Im not sure if its the best interface to work with from ffdshow but at least the interface is correctly queried
  }
  if (m_pIMpcDecFilter)
    m_VideoInfo.dxva_info=DShowUtil::GetDXVAMode(m_pIMpcDecFilter->GetDXVADecoderGuid());
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

}

void CDSGraph::Pause()
{
  UpdateState();
  if (m_PlaybackRate == 0)
  {
    if (m_State.current_filter_state != State_Running)
      m_pMediaControl->Run();
    m_PlaybackRate=1;
	//SetPlaySpeed(1);
  }
  else
  {
    if (m_pMediaControl)
	{
	  if ( SUCCEEDED( m_pMediaControl->Pause() ) )
	    m_PlaybackRate = 0;
	}
  
  }
}

//The fastforward is based on mediaportal fastforward
void CDSGraph::DoFFRW(int currentSpeed)
{
  if (!m_pMediaSeeking)
    return;
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
        //Log.Info(" seek ff:{0}",rewind/10000000);
        hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning, &pStop, AM_SEEKING_NoPositioning);
		//m_State.time = rewind;
		m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
        m_pMediaControl->Run();
        return;
      }
      //seek to new moment in time
      //Log.Info(" seek :{0}",rewind/10000);
      hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame,&pStop, AM_SEEKING_NoPositioning);
	  //m_State.time = rewind;
      m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
      m_pMediaControl->StopWhenReady();

}

double CDSGraph::GetPlaySpeed()
{
  if (!m_pMediaSeeking)
    return 0;
  double pRate;
  m_pMediaSeeking->GetRate(&pRate);
  return pRate;
}

void CDSGraph::SetPlaySpeed(int iSpeed)
{
//TODO
//Not working at all
//its just getting stock when i set the rate
  
  if (!m_pMediaSeeking)
  {
	CLog::Log(LOGERROR,"%s There no IMediaSeeking ", __FUNCTION__);
    return;
  }
  HRESULT hr;
  hr = m_pMediaSeeking->SetRate((double) iSpeed);
  if (SUCCEEDED(hr))
  {
    double playbackRate;
    m_pMediaSeeking->GetRate(&playbackRate);
	m_PlaybackRate= (int) playbackRate;
    return;
  }
  if (hr == E_INVALIDARG)
    CLog::Log(LOGERROR,"%s The specified rate was zero or the filters your currently using cant seek backward.", __FUNCTION__);
  
  if (hr == VFW_E_UNSUPPORTED_AUDIO)
    CLog::Log(LOGERROR,"%s Audio device or filter does not support this rate.", __FUNCTION__);

//http://msdn.microsoft.com/en-us/library/dd407039(VS.85).aspx
//E_INVALIDARG The specified rate was zero or a negative value. (See Remarks.)
//VFW_E_UNSUPPORTED_AUDIO Audio device or filter does not support this rate.
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

HRESULT hr;
//hr = pAllocator->StopPresenting(g_userId);
  LONGLONG earliest,latest;
  m_pMediaSeeking->GetAvailable(&earliest,&latest);
  hr = m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);
  UpdateTime();
  
  LONGLONG seekrequest;
  seekrequest = ( LONGLONG )( seek * 10000 );
  if ( seekrequest < 0 )
    seekrequest = 0;
  hr = m_pMediaSeeking->SetPositions(&seekrequest, AM_SEEKING_AbsolutePositioning, NULL,AM_SEEKING_NoPositioning);
  if(m_State.current_filter_state == State_Stopped)
  {
    hr = m_pMediaControl->Pause();
    hr = m_pMediaControl->GetState(INFINITE, (OAFilterState *)&m_State.current_filter_state);
    hr = m_pMediaControl->Stop();
  }
}

// return the time in milliseconds
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
	case ID_SET_SPEEDRATE:
      SetPlaySpeed((int)lParam);
	  CLog::Log(LOGDEBUG,"%s ID_SET_SPEEDRATE",__FUNCTION__);
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
      //m_pMediaControl->StopWhenReady();
	  CLog::Log(LOGDEBUG,"%s ID_STOP_DSPLAYER",__FUNCTION__);
      break;
  }

  //return m_ImageReady;
}
