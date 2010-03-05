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
#include "DSPlayer.h"
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
#include "StreamsManager.h"
#include <streams.h>
#include "DShowUtil/DShowUtil.h"
#include "FgManager.h"
#include "qnetwork.h"

#include "DShowUtil/smartptr.h"

//used to get the same cachces subtitles on start of file
#include "DVDSubtitles/DVDFactorySubtitle.h"

#include "DshowUtil/MediaTypeEx.h"

#include "timeutils.h"
enum 
{
  WM_GRAPHNOTIFY = WM_APP+1,
};


#include "win32exception.h"
#include "Filters/EVRAllocatorPresenter.h"
#include "DSConfig.h"

using namespace std;

CDSGraph::CDSGraph() :m_pGraphBuilder(NULL)
{  
  m_PlaybackRate = 1;
  m_currentSpeed = 0;
  m_iCurrentFrameRefreshCycle = 0;
  g_userId = 0xACDCACDC;
  m_State.Clear();
  m_VideoInfo.Clear();

  m_pMediaControl = NULL;
  m_pMediaEvent = NULL;
  m_pMediaSeeking = NULL;
  m_pBasicAudio = NULL;
  m_pFilterGraph = NULL;

  m_bReachedEnd = false;
  m_bChangingAudioStream = false;
  g_dsconfig.pGraph = this;
  m_Filename = "";
}

CDSGraph::~CDSGraph()
{
}

//This is also creating the Graph
HRESULT CDSGraph::SetFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (CDSPlayer::PlayerState != DSPLAYER_LOADING)
    return E_FAIL;
  HRESULT hr;

  m_VideoInfo.Clear();
  m_Filename = file.m_strPath;
  
  //Reset the g_dsconfig for not getting unwanted interface from last file into the player
  g_dsconfig.ClearConfig();
  m_pGraphBuilder = new CFGManager();
  m_pGraphBuilder->InitManager();

  if (SUCCEEDED(m_pGraphBuilder->AddToROT()))
    CLog::Log(LOGDEBUG, "%s Successfully added XBMC to the Running Object Table", __FUNCTION__);
  else
    CLog::Log(LOGERROR, "%s Failed to add XBMC to the Running Object Table", __FUNCTION__);

  hr = m_pGraphBuilder->RenderFileXbmc(file);
  if (FAILED(hr))
  {
    m_Filename = "";
    return hr;
  }

  hr = m_pGraphBuilder->QueryInterface(__uuidof(m_pMediaSeeking),(void **)&m_pMediaSeeking);
  hr = m_pGraphBuilder->QueryInterface(__uuidof(m_pMediaControl),(void **)&m_pMediaControl);
  hr = m_pGraphBuilder->QueryInterface(__uuidof(m_pMediaEvent),(void **)&m_pMediaEvent);
  hr = m_pGraphBuilder->QueryInterface(__uuidof(m_pBasicAudio),(void **)&m_pBasicAudio);

  // Audio & subtitle streams
  CStreamsManager::getSingleton()->InitManager(this);
  CStreamsManager::getSingleton()->LoadStreams();

  // Chapters
  CChaptersManager::getSingleton()->InitManager(this);
  if (!CChaptersManager::getSingleton()->LoadChapters())
    CLog::Log(LOGNOTICE, "%s No chapters found!", __FUNCTION__);

  UpdateCurrentVideoInfo();

  SetVolume(g_settings.m_nVolumeLevel);
  
  //Get cached subs
  
  //TODO add addsubtitles stream from external file in the stream manager
  //for(unsigned int i=0;i<filenames.size();i++)
  //CStreamsManager::AddSubtitle(CStdString(filenames[i]));
  //For an unknown reason the subtitles are not working for the whole playback if we are using this

  //still need to be added
  //SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
  
  /*if (g_dsconfig.pQualProp)
  {
    m_pQualProp = g_dsconfig.pQualProp;    
  }*/

  CDSPlayer::PlayerState = DSPLAYER_LOADED;  
  
  Play();

  m_currentSpeed = 10000;

  return hr;
}

void CDSGraph::CloseFile()
{
  CAutoLock lock(&m_ObjectLock);
  HRESULT hr;
  m_Filename = "";

  if (m_pGraphBuilder)
  {
    if (CStreamsManager::getSingleton()->IsChangingStream())
      return;

    Stop(true);

    // No more need to release interface with CComPtr

    hr = m_pGraphBuilder->RemoveFromROT();

    CStreamsManager::Destroy();
    CChaptersManager::Destroy();

    UnloadGraph();

    m_VideoInfo.Clear();
    m_State.Clear();

    m_PlaybackRate = 1;
    m_currentSpeed = 0;
    g_userId = 0xACDCACDC;
    m_bReachedEnd = false;
    m_bChangingAudioStream = false;

    SAFE_DELETE(m_pGraphBuilder);
    CDSGraph::m_pFilterGraph = NULL;

  } else {
    CLog::Log(LOGDEBUG, "%s CloseFile called more than one time!", __FUNCTION__);
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

  if ( m_State.time == m_State.time_total )
    m_bReachedEnd = true;
  //with the Current frame refresh cycle at 5 the framerate is requested every 1000ms
  // pQualProp is queried in CFGLoader::InsertVideoRenderer
  if ((CFGLoader::Filters.VideoRenderer.pQualProp) && m_iCurrentFrameRefreshCycle <= 0)
  {
    int avgRate;
    CFGLoader::Filters.VideoRenderer.pQualProp->get_AvgFrameRate(&avgRate);
    if (CFGLoader::GetCurrentRenderer() == DIRECTSHOW_RENDERER_EVR)
      m_pStrCurrentFrameRate = "Real FPS: Not implemented"; // Avoid people complain on forum while IQualProp not implemented in the custom EVR renderer
    else
      m_pStrCurrentFrameRate.Format("Real FPS: %4.2f", (float) avgRate / 100);
    m_iCurrentFrameRefreshCycle = 5;
  }
  m_iCurrentFrameRefreshCycle--;
}

void CDSGraph::UpdateState()
{
  HRESULT hr = S_OK;
  LONGLONG Duration = 0;

  if (! m_pMediaSeeking || CDSPlayer::PlayerState == DSPLAYER_CLOSING
                        || CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    return;

  hr = m_pMediaSeeking->GetTimeFormatA(&m_VideoInfo.time_format);

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

  if (CDSPlayer::PlayerState == DSPLAYER_CLOSING ||
    CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    return;

  switch (m_State.current_filter_state)
  {
    case State_Running:
      CDSPlayer::PlayerState = DSPLAYER_PLAYING;
      break;
    case State_Paused:
      CDSPlayer::PlayerState = DSPLAYER_PAUSED;
      break;
    case State_Stopped:
      CDSPlayer::PlayerState = DSPLAYER_STOPPED;
      break;
  }
}
void CDSGraph::UpdateCurrentVideoInfo()
{
  
  m_VideoInfo.dxva_info = g_dsconfig.GetDxvaMode();
  m_pGraphBuilder->GetFileInfo(&m_VideoInfo.filter_source,
    &m_VideoInfo.filter_splitter, &m_VideoInfo.filter_audio_dec,
    &m_VideoInfo.filter_video_dec, &m_VideoInfo.filter_audio_renderer);
}


HRESULT CDSGraph::HandleGraphEvent()
{
  LONG evCode;
  LONG_PTR evParam1, evParam2;
  HRESULT hr=S_OK;
    // Make sure that we don't access the media event interface
    // after it has already been released.
  if (!m_pMediaEvent)
    return S_OK;

  // Process all queued events
  while((CDSPlayer::PlayerState != DSPLAYER_CLOSING && CDSPlayer::PlayerState != DSPLAYER_CLOSED)
    &&  SUCCEEDED(m_pMediaEvent->GetEvent(&evCode, &evParam1, &evParam2, 0)))
  {
    switch(evCode)
    {
      case EC_STEP_COMPLETE:
        CLog::Log(LOGDEBUG,"%s EC_STEP_COMPLETE", __FUNCTION__);
        g_application.m_pPlayer->CloseFile();
        break;
      case EC_COMPLETE:
        CLog::Log(LOGDEBUG,"%s EC_COMPLETE", __FUNCTION__);
        g_application.m_pPlayer->CloseFile();
        break;
      case EC_USERABORT:
        CLog::Log(LOGDEBUG,"%s EC_USERABORT", __FUNCTION__);
        g_application.m_pPlayer->CloseFile();
        break;
      case EC_ERRORABORT:
        CLog::Log(LOGDEBUG,"%s EC_ERRORABORT. Error code: %X", __FUNCTION__, evParam1);
        g_application.m_pPlayer->CloseFile();
        break;
      case EC_STATE_CHANGE:
        CLog::Log(LOGDEBUG,"%s EC_STATE_CHANGE", __FUNCTION__);
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
    if (m_pMediaEvent)
      hr = m_pMediaEvent->FreeEventParams(evCode, evParam1, evParam2);
  }

    return hr;
}

//USER ACTIONS
void CDSGraph::SetVolume(long nVolume)
{
  if (m_pBasicAudio)
    m_pBasicAudio->put_Volume(nVolume);
}
/*void CDSGraph::SetDynamicRangeCompression(long drc)
{
  if (m_pBasicAudio)
    m_pBasicAudio->put_Volume(drc);
}*/

void CDSGraph::Stop(bool rewind)
{
  LONGLONG pos = 0;  
  
  if (rewind && m_pMediaSeeking)
    m_pMediaSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

  if (m_pMediaControl)
  {
    if (m_pMediaControl->Stop() == S_FALSE)
    {
      do 
      {
        m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);
      } while (m_State.current_filter_state != State_Stopped);    
    }
  }

  UpdateState();

  /*if (! m_pGraphBuilder)
    return;

  BeginEnumFilters(CDSGraph::m_pFilterGraph, pEF, pBF)
  {
    Com::SmartQIPtr<IFileSourceFilter> pFSF;
    pFSF = Com::SmartQIPtr<IAMNetworkStatus>(pBF);
    if(pFSF)
    {
      WCHAR* pFN = NULL;
      AM_MEDIA_TYPE mt;
      if(SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN)
      {
        pFSF->Load(pFN, NULL);
        CoTaskMemFree(pFN);
        DeleteMediaType(&mt);
      }
      break;
    }
  }
  EndEnumFilters*/
}

void CDSGraph::Play()
{
  if (m_State.current_filter_state != State_Running)
    m_pMediaControl->Run();

  UpdateState();
}

void CDSGraph::Pause()
{
  if (CDSPlayer::PlayerState == DSPLAYER_PAUSED)
  {
    if (m_State.current_filter_state != State_Running)
      m_pMediaControl->Run();
    
    m_currentSpeed = 10000;
  }
  else
  {
    if (m_pMediaControl)
      if ( m_pMediaControl->Pause() == S_FALSE )
      {
        /* the graph may need some time */
        do 
        {
          m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);
        } while (m_State.current_filter_state != State_Paused);        
        
        m_currentSpeed = 0;
      }
  }

  UpdateState();
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
    UpdateState();
    return;
  }

  if ((rewind > (latest - 100000)) && (currentSpeed > 0))
  {
    currentSpeed = 10000;
    rewind = latest - 100000;

    hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning, &pStop, AM_SEEKING_NoPositioning);

    m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
    m_pMediaControl->Run();
    UpdateState();
    return;
  }
  //seek to new moment in time
  hr = m_pMediaSeeking->SetPositions(&rewind, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame,&pStop, AM_SEEKING_NoPositioning);
  m_State.time = double(rewind) / TIME_FORMAT_TO_MS;
  m_pMediaControl->StopWhenReady();

  UpdateState();
}

HRESULT CDSGraph::UnloadGraph()
{
  HRESULT hr = S_OK;

  BeginEnumFilters(CDSGraph::m_pFilterGraph, pEM, pBF)
  {
    CStdString filterName;
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), filterName);

    try
    {
      hr = RemoveFilter(CDSGraph::m_pFilterGraph, pBF);
    }
    catch (...)
    {
      throw;
      // ffdshow dxva decoder crash here, don't know why!
      hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully removed \"%s\" from the graph", __FUNCTION__, filterName.c_str());
    else 
      CLog::Log(LOGERROR, "%s Failed to remove \"%s\" from the graph", __FUNCTION__, filterName.c_str());

    pEM->Reset();
  }
  EndEnumFilters

  m_pMediaControl = NULL;
  m_pMediaEvent = NULL;
  m_pMediaSeeking = NULL;
  m_pBasicAudio = NULL;

  /* delete filters */

  CLog::Log(LOGDEBUG, "%s Deleting filters ...", __FUNCTION__);

  /* Release config interfaces */
  g_dsconfig.ClearConfig();

  // Should be null if no source filter is used.
  // If not null and if there's no source filter, big crash!
  CFGLoader::Filters.Source.pBF = NULL; //Also delete the filter

  CFGLoader::Filters.Splitter.pBF = NULL;
  CFGLoader::Filters.AudioRenderer.pBF = NULL;

  CFGLoader::Filters.VideoRenderer.pQualProp = NULL;
  IBaseFilter *f = CFGLoader::Filters.VideoRenderer.pBF.Detach();
  int c = 0;
  do 
  {
    c = f->Release(); //TODO: Find why there're still some references!
  } while (c != 0);

  CFGLoader::Filters.Audio.pBF = NULL;
  CFGLoader::Filters.Video.pBF = NULL;
  
  while (! CFGLoader::Filters.Extras.empty())
  {
    CFGLoader::Filters.Extras.back().pBF = NULL;
    CFGLoader::Filters.Extras.pop_back();
  }

  CLog::Log(LOGDEBUG, "%s ... done!", __FUNCTION__);

  return hr;
}

bool CDSGraph::IsPaused() const
{
  return CDSPlayer::PlayerState == DSPLAYER_PAUSED;
}

void CDSGraph::SeekInMilliSec(double sec)
{
  LONGLONG seekrequest = 0;

  if ( !m_pMediaSeeking )
    return;

  if( m_VideoInfo.time_format == TIME_FORMAT_MEDIA_TIME )
    seekrequest = ( LONGLONG )( sec * TIME_FORMAT_TO_MS );
  else
    seekrequest = (LONGLONG) sec;

  if ( seekrequest < 0 )
    seekrequest = 0;

  m_pMediaSeeking->SetPositions(&seekrequest, AM_SEEKING_AbsolutePositioning, 
    NULL, AM_SEEKING_NoPositioning);

  /*if(m_State.current_filter_state == State_Stopped)
  {
    m_pMediaControl->Pause();
    m_pMediaControl->GetState(INFINITE, (OAFilterState *)&m_State.current_filter_state);
    m_pMediaControl->Stop();
  }*/

  UpdateState();
}
void CDSGraph::Seek(bool bPlus, bool bLargeStep)
{
  // Chapter support
  if (bLargeStep && CChaptersManager::getSingleton()->GetChapterCount() > 1)
  {
    if (bPlus)
      CChaptersManager::getSingleton()->SeekChapter(
        CChaptersManager::getSingleton()->GetChapter() + 1);
    else
      CChaptersManager::getSingleton()->SeekChapter(
        CChaptersManager::getSingleton()->GetChapter() - 1);

    return;
  }

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
      percent = (float) (bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig);
    else
      percent = (float) (bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward);
    seek = (__int64)(GetTotalTimeInMsec()*(GetPercentage()+percent)/100);
  }

  UpdateTime();
  SeekInMilliSec((double) seek);
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

CStdString CDSGraph::GetGeneralInfo()
{
  CStdString generalInfo = "";

  if (! CFGLoader::Filters.Source.osdname.empty() )
  generalInfo = "Source Filter: " + m_VideoInfo.filter_source;

  if (! CFGLoader::Filters.Splitter.osdname.empty())
  {
    if (generalInfo.empty())
      generalInfo = "Splitter: " + CFGLoader::Filters.Splitter.osdname;
    else
      generalInfo += " | Splitter: " + CFGLoader::Filters.Splitter.osdname;
  }

  if (generalInfo.empty())
    generalInfo = "Video renderer: " + CFGLoader::Filters.VideoRenderer.osdname;
  else
    generalInfo += " | Video renderer: " + CFGLoader::Filters.VideoRenderer.osdname;

  return generalInfo;
}

CStdString CDSGraph::GetAudioInfo()
{
  CStdString audioInfo;
  CStreamsManager *c = CStreamsManager::getSingleton();

  audioInfo.Format("Audio Decoder: %s (%s, %d Hz, %d Channels) | Renderer: %s",
    CFGLoader::Filters.Audio.osdname,
    c->GetAudioCodecName(),
    c->GetSampleRate(),
    c->GetChannels(),
    CFGLoader::Filters.AudioRenderer.osdname);
    
  return audioInfo;
}

CStdString CDSGraph::GetVideoInfo()
{
  CStdString videoInfo = "";
  CStreamsManager *c = CStreamsManager::getSingleton();
  videoInfo.Format("Video Decoder: %s (%s, %dx%d) %s",
    CFGLoader::Filters.Video.osdname,
    c->GetVideoCodecName(),
    c->GetPictureWidth(),
    c->GetPictureHeight(),
    m_pStrCurrentFrameRate.c_str());

  if ( ! m_VideoInfo.dxva_info.empty() )
    videoInfo += " | " + m_VideoInfo.dxva_info;

  return videoInfo;
}

bool CDSGraph::CanSeek()
{
  DWORD seekcaps = AM_SEEKING_CanSeekForwards;
  seekcaps |= AM_SEEKING_CanSeekBackwards;
  seekcaps |= AM_SEEKING_CanSeekAbsolute;
  return SUCCEEDED(m_pMediaSeeking->CheckCapabilities(&seekcaps));  
}

void CDSGraph::ProcessDsWmCommand(WPARAM wParam, LPARAM lParam)
{
  switch(wParam)
  {
    case ID_PLAY_PLAY:
      Play();
      break;
    case ID_SEEK_TO:
      SeekInMilliSec((LPARAM)lParam);
      break;
    case ID_SEEK_FORWARDSMALL:
      Seek(true, false);
      CLog::Log(LOGDEBUG,"%s ID_SEEK_FORWARDSMALL",__FUNCTION__);
      break;
    case ID_SEEK_FORWARDLARGE:
      Seek(true, true);
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
      Stop(true);
      CLog::Log(LOGDEBUG,"%s ID_PLAY_STOP",__FUNCTION__);
      break;
    case ID_STOP_DSPLAYER:
      Stop(true);
      CLog::Log(LOGDEBUG,"%s ID_STOP_DSPLAYER",__FUNCTION__);
      break;
  }
}

Com::SmartPtr<IFilterGraph2> CDSGraph::m_pFilterGraph = NULL;
