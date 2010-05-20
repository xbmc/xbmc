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


#include "FgManager.h"
#include "qnetwork.h"

#include "DShowUtil/smartptr.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"

#include "GUIWindowManager.h"

#include "GUIUserMessages.h"

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

CDSGraph* g_dsGraph = NULL;

CDSGraph::CDSGraph(CDSClock* pClock, IPlayerCallback& callback)
    : m_pGraphBuilder(NULL), m_currentRate(1), m_lAvgTimeToSeek(0),
      m_iCurrentFrameRefreshCycle(0), m_userId(0xACDCACDC),
      m_bReachedEnd(false), m_callback(callback), pDsClock(pClock)
{ 
}

CDSGraph::~CDSGraph()
{
}

//This is also creating the Graph
HRESULT CDSGraph::SetFile(const CFileItem& file, const CPlayerOptions &options)
{
  if (CDSPlayer::PlayerState != DSPLAYER_LOADING)
    return E_FAIL;

  HRESULT hr = S_OK;

  m_VideoInfo.Clear();
  m_DvdState.Clear();

  //Reset the g_dsconfig for not getting unwanted interface from last file into the player
  g_dsconfig.ClearConfig();
  m_pGraphBuilder = new CFGManager();
  m_pGraphBuilder->InitManager();

  if (SUCCEEDED(m_pGraphBuilder->AddToROT()))
    CLog::Log(LOGDEBUG, "%s Successfully added XBMC to the Running Object Table", __FUNCTION__);
  else
    CLog::Log(LOGERROR, "%s Failed to add XBMC to the Running Object Table", __FUNCTION__);

  START_PERFORMANCE_COUNTER
  hr = m_pGraphBuilder->RenderFileXbmc(file);
  END_PERFORMANCE_COUNTER

  if (FAILED(hr))
    return hr;

  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaSeeking), (void**)&m_pMediaSeeking);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaControl), (void**)&m_pMediaControl);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaEvent), (void**)&m_pMediaEvent);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pBasicAudio), (void**)&m_pBasicAudio);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pBasicVideo), (void**)&m_pBasicVideo);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pVideoWindow), (void**)&m_pVideoWindow);
  hr = m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
  m_VideoInfo.time_format = TIME_FORMAT_MEDIA_TIME;
  if (m_pVideoWindow)
  {
    //HRESULT hr;
    //m_pVideoWindow->put_Owner((OAHWND)g_hWnd);
    //m_pVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
    //m_pVideoWindow->put_MessageDrain((OAHWND)g_hWnd);
  }
  //TODO Ti-Ben
  //with the vmr9 we need to add AM_DVD_SWDEC_PREFER  AM_DVD_VMR9_ONLY on the ivmr9config prefs

  m_VideoInfo.isDVD = CFGLoader::Filters.isDVD;
  
  // Audio & subtitle streams
  START_PERFORMANCE_COUNTER
  CStreamsManager::Get()->InitManager();
  CStreamsManager::Get()->LoadStreams();
  END_PERFORMANCE_COUNTER

  // Chapters
  START_PERFORMANCE_COUNTER
  CChaptersManager::Get()->InitManager();
  if (!CChaptersManager::Get()->LoadChapters())
    CLog::Log(LOGNOTICE, "%s No chapters found!", __FUNCTION__);
  END_PERFORMANCE_COUNTER

  SetVolume(g_settings.m_nVolumeLevel);
  
  CDSPlayer::PlayerState = DSPLAYER_LOADED;

  m_currentRate = 1;
  return hr;
}

void CDSGraph::CloseFile()
{
  CSingleLock lock(m_ObjectLock);
  HRESULT hr;

  if (m_pGraphBuilder)
  {
    // TODO: Use an Event.
    // If the user close the file when a stream is changing, we need to wait, and not to return!
    if (CStreamsManager::Get()->IsChangingStream())
      return;

    Stop(true);

    hr = m_pGraphBuilder->RemoveFromROT();

    CStreamsManager::Destroy();
    CChaptersManager::Destroy();

    UnloadGraph();

    m_VideoInfo.Clear();
    m_State.Clear();

    m_currentRate = 1;
    
    m_userId = 0xACDCACDC;
    m_bReachedEnd = false;

    SAFE_DELETE(m_pGraphBuilder);
    g_dsGraph->pFilterGraph = NULL;
  } 
  else 
  {
    CLog::Log(LOGDEBUG, "%s CloseFile called more than one time!", __FUNCTION__);
  }
}

void CDSGraph::UpdateTime()
{
  if (!m_pMediaSeeking)
    return;
  LONGLONG Position;
  if(SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
  {
    m_State.time = DS_TIME_TO_MSEC(Position);//double(Position) / TIME_FORMAT_TO_MS;
  }
  if (m_State.time_total == 0)
  {
    //we dont have the duration of the video yet so try to request it
    UpdateTotalTime();
  }

  if ((CFGLoader::Filters.VideoRenderer.pQualProp) && m_iCurrentFrameRefreshCycle <= 0)
  {
    //this is too slow if we are doing it on every UpdateTime
    int avgRate;
    CFGLoader::Filters.VideoRenderer.pQualProp->get_AvgFrameRate(&avgRate);
    if (CFGLoader::GetCurrentRenderer() == DIRECTSHOW_RENDERER_EVR)
      m_pStrCurrentFrameRate = "";//Dont need to waste the space on the osd
    else
      m_pStrCurrentFrameRate.Format(" | Real FPS: %4.2f", (float) avgRate / 100);
    m_iCurrentFrameRefreshCycle = 5;
  }
  m_iCurrentFrameRefreshCycle--;

  //On dvd playback the current time is received in the handlegraphevent
  if ( m_VideoInfo.isDVD )
    return;

 
  
  if (( m_State.time_total != 0 && m_State.time >= m_State.time_total ))
    m_bReachedEnd = true;

  
}

void CDSGraph::UpdateDvdState()
{
}

void CDSGraph::UpdateTotalTime()
{
  HRESULT hr = S_OK;
  LONGLONG Duration = 0;

  if (m_VideoInfo.isDVD)
  {
    if (!CFGLoader::Filters.DVD.dvdInfo)
      return;

    REFERENCE_TIME rtDur = 0;
    DVD_HMSF_TIMECODE tcDur;
    ULONG ulFlags;
    if(SUCCEEDED(CFGLoader::Filters.DVD.dvdInfo->GetTotalTitleTime(&tcDur, &ulFlags)))
    {
      rtDur = DShowUtil::HMSF2RT(tcDur);
      m_State.time_total = DS_TIME_TO_MSEC(rtDur);
    }
  }
  else
  {
    if (! m_pMediaSeeking)
      return;

    hr = m_pMediaSeeking->GetTimeFormatA(&m_VideoInfo.time_format);

    if(m_VideoInfo.time_format == TIME_FORMAT_MEDIA_TIME)
    {
      if(SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
        m_State.time_total = DS_TIME_TO_MSEC(Duration);
    }
    else
    {
      if(SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
        m_State.time_total = (double) Duration;
    }
  }
}

void CDSGraph::UpdateWindowPosition()
{
  if (m_pVideoWindow && m_pVideoWindow)
  {
    HRESULT hr;
    Com::SmartRect videoRect, windowRect;
    CRect vr;
    vr = g_graphicsContext.GetViewWindow();
    
    hr = m_pBasicVideo->SetDefaultSourcePosition();
    hr = m_pBasicVideo->SetDestinationPosition(videoRect.left, videoRect.top, videoRect.Width(), videoRect.Height());
    hr = m_pVideoWindow->SetWindowPosition(windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height());
  }
}

void CDSGraph::UpdateState()
{
  HRESULT hr = S_OK;
  if (CDSPlayer::PlayerState == DSPLAYER_CLOSING || CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    return;
  
  hr = m_pMediaControl->GetState(100, (OAFilterState *)&m_State.current_filter_state);

  //VFW_S_CANT_CUE is graph paused and failed to request the state
  if ( hr == VFW_S_CANT_CUE )
  {
    CDSPlayer::PlayerState = DSPLAYER_PAUSED;
    m_currentRate = 0;
    return;
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

HRESULT CDSGraph::HandleGraphEvent()
{
  LONG evCode;
  LONG_PTR evParam1, evParam2;
  HRESULT hr = S_OK;
    // Make sure that we don't access the media event interface
    // after it has already been released.
  if (!m_pMediaEvent)
    return E_POINTER;

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
        CLog::Log(LOGDEBUG,"%s EC_ERRORABORT. Error code: 0x%X", __FUNCTION__, evParam1);
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
      case EC_DVD_CURRENT_HMSF_TIME:
        {
        double fps = evParam2 == DVD_TC_FLAG_25fps ? 25.0
        : evParam2 == DVD_TC_FLAG_30fps ? 30.0
        : evParam2 == DVD_TC_FLAG_DropFrame ? 29.97
        : 25.0;
        
        // DOne in UpdateDvdState
        /* REFERENCE_TIME rtDur = 0;
        DVD_HMSF_TIMECODE tcDur;
        ULONG ulFlags;
        if(SUCCEEDED(m_pDvdInfo2->GetTotalTitleTime(&tcDur, &ulFlags)))
          rtDur = DShowUtil::HMSF2RT(tcDur, fps);
        m_State.time_total = (double)rtDur / 10000;*/

        REFERENCE_TIME rtNow = DShowUtil::HMSF2RT(*((DVD_HMSF_TIMECODE*)&evParam1), fps);
        m_State.time = rtNow / 10000.f;

        break;
        }
      case EC_DVD_TITLE_CHANGE:
        {
          m_pDvdStatus.DvdTitleId = (ULONG)evParam1;
        }
        break;
      case EC_DVD_DOMAIN_CHANGE:
        {
          m_pDvdStatus.DvdDomain = (DVD_DOMAIN)evParam1;
          CStdString Domain("-");

          switch(m_pDvdStatus.DvdDomain)
          {
          case DVD_DOMAIN_FirstPlay:
            
            if (CFGLoader::Filters.DVD.dvdInfo && SUCCEEDED (CFGLoader::Filters.DVD.dvdInfo->GetDiscID (NULL, &m_pDvdStatus.DvdGuid)))
            {
              if (m_pDvdStatus.DvdTitleId != 0)
              {
                //s.NewDvd (llDVDGuid);
                // Set command line position
                CFGLoader::Filters.DVD.dvdControl->PlayTitle(m_pDvdStatus.DvdTitleId, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                if (m_pDvdStatus.DvdChapterId > 1)
                  CFGLoader::Filters.DVD.dvdControl->PlayChapterInTitle(m_pDvdStatus.DvdTitleId, m_pDvdStatus.DvdChapterId, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                else
                {
                  // Trick : skip trailers with somes DVDs
                  CFGLoader::Filters.DVD.dvdControl->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                  CFGLoader::Filters.DVD.dvdControl->PlayAtTime(&m_pDvdStatus.DvdTimecode, DVD_CMD_FLAG_Flush, NULL);
                }

                //m_iDVDTitle	  = s.lDVDTitle;
                m_pDvdStatus.DvdTitleId   = 0;
                m_pDvdStatus.DvdChapterId = 0;
              }
              /*else if (!s.NewDvd (llDVDGuid) && s.fRememberDVDPos)
              {
                // Set last remembered position (if founded...)
                DVD_POSITION*	DvdPos = s.CurrentDVDPosition();

                m_pDvdControl2->PlayTitle(DvdPos->lTitle, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                m_pDvdControl2->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                if (SUCCEEDED (hr = m_pDvdControl2->PlayAtTime (&DvdPos->Timecode, DVD_CMD_FLAG_Flush, NULL)))
                {
                  m_iDVDTitle = DvdPos->lTitle;
                }
              }*/
            }
            Domain = _T("First Play"); 
            m_DvdState.isInMenu = false;
            break;
          case DVD_DOMAIN_VideoManagerMenu: 
            Domain = _T("Video Manager Menu"); 
            m_DvdState.isInMenu = true;
            break;
          case DVD_DOMAIN_VideoTitleSetMenu: 
            Domain = _T("Video Title Set Menu"); 
            //Entered menu
            m_DvdState.isInMenu = true;
            break;
          case DVD_DOMAIN_Title: 
            Domain.Format("Title %d", m_pDvdStatus.DvdTitleId);
            //left menu
            m_DvdState.isInMenu = false;
            m_pDvdStatus.DvdTitleId = (ULONG)evParam2;
            break;
          case DVD_DOMAIN_Stop: 
            Domain = "stopped"; 
            break;
          default: Domain = _T("-"); break;
          }
        }
        break;
      case EC_DVD_ERROR:
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

  if (! m_pGraphBuilder)
    return;

  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    Com::SmartQIPtr<IFileSourceFilter> pFSF;
    pFSF = Com::SmartQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus>(pBF);
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
  EndEnumFilters
}

bool CDSGraph::OnMouseClick(tagPOINT pt)
{
  return true;
}

bool CDSGraph::OnMouseMove(tagPOINT pt)
{
  HRESULT hr;
  hr = CFGLoader::Filters.DVD.dvdControl->SelectAtPosition(pt);
  if (SUCCEEDED(hr))
    return true;
  return true;
}

void CDSGraph::Play(bool force/* = false*/)
{
  if (m_pMediaControl && (force || m_State.current_filter_state != State_Running))
    m_pMediaControl->Run();

  UpdateState();
}

void CDSGraph::Pause()
{
  if (CDSPlayer::PlayerState == DSPLAYER_PAUSED)
  {
    if (m_State.current_filter_state != State_Running)
      m_pMediaControl->Run();
    
    m_currentRate = 1;
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
        
        m_currentRate = 0;
      }
  }

  UpdateState();
}

int CDSGraph::DoFFRW(int currentRate)
{
  if ( currentRate == 1 || currentRate == 0 )
    return 250;

  //TIME_FORMAT_MEDIA_TIME is using Reference time (100-nanosecond units).
  //1 sec = 1 000 000 000 nanoseconds
  //so 1 units of TIME_FORMAT_MEDIA_TIME is 0,0000001 sec or 0,0001 millisec
  //If playback speed is at 32x we will try to make it the closest to 32sec per sec

  __int64 stepInMsec = (( currentRate * 1000) / 4 );
  double startTimer = 0;
  if (currentRate != m_currentRate)
  {
    m_currentRate = currentRate;
    m_lAvgTimeToSeek = 0;
  }

  //Get the target in TIME_FORMAT_MEDIA_TIME
  stepInMsec += GetTime();

  //the ajustement is to get an estimate of the time beetween each seek
  startTimer = CDSClock::GetAbsoluteClock();

  stepInMsec += m_lAvgTimeToSeek;

  if (m_VideoInfo.isDVD)
  {

  }
  else
  {
    if (!m_pMediaSeeking)
      return 250;

    HRESULT hr = S_OK;
    __int64 earliest = 0, latest = 0;
    double msec_earliest = 0;
    m_pMediaSeeking->GetAvailable(&earliest,&latest);

    msec_earliest = DS_TIME_TO_MSEC(earliest);
    //if target is under the lowest position possible in the media just make it play from start
    if (stepInMsec < msec_earliest)
    {
      //setting speed at 1x
      m_currentRate = 1;
      //Seeking to earliest position in the video
      SeekInMilliSec(msec_earliest);
      //Set the new status of the position
      m_State.time = msec_earliest;
      //start the video since we stopped the playback after this seek
      m_pMediaControl->Run();
      UpdateState();
      return 250;
    }

    //seek to where we should be
    SeekInMilliSec((double) stepInMsec);
    //stop when ready is currently make the graph wait for a frame to present before starting back again
    m_pMediaControl->StopWhenReady();

    UpdateTime();
    UpdateState(); // We need to know the new state
  }

  m_lAvgTimeToSeek = (__int64) DS_TIME_TO_MSEC((CDSClock::GetAbsoluteClock() - startTimer));
  if ( m_lAvgTimeToSeek <= 0 )
    m_lAvgTimeToSeek = 50;

  return (int) m_lAvgTimeToSeek; // Should never be over 250 / 300 ms, cast is fine
}

HRESULT CDSGraph::UnloadGraph()
{
  HRESULT hr = S_OK;

  BeginEnumFilters(g_dsGraph->pFilterGraph, pEM, pBF)
  {
    CStdString filterName;
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), filterName);

    try
    {
      hr = RemoveFilter(g_dsGraph->pFilterGraph, pBF);
    }
    catch (...)
    {
      hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully removed \"%s\" from the graph", __FUNCTION__, filterName.c_str());
    else 
      CLog::Log(LOGERROR, "%s Failed to remove \"%s\" from the graph", __FUNCTION__, filterName.c_str());

    pEM->Reset();
  }
  EndEnumFilters

  CFGLoader::Filters.DVD.Clear();
  m_pMediaControl.Release();
  m_pMediaEvent.Release();
  m_pMediaSeeking.Release();
  m_pVideoWindow.Release();
  m_pBasicAudio.Release();  

  /* delete filters */

  CLog::Log(LOGDEBUG, "%s Deleting filters ...", __FUNCTION__);

  /* Release config interfaces */
  g_dsconfig.ClearConfig();

  if (CFGLoader::Filters.Source.pBF)
    CFGLoader::Filters.Source.pBF.FullRelease();

  if (CFGLoader::Filters.Splitter.pBF)
    CFGLoader::Filters.Splitter.pBF.FullRelease();

  if (CFGLoader::Filters.AudioRenderer.pBF)
    CFGLoader::Filters.AudioRenderer.pBF.FullRelease();

  CFGLoader::Filters.VideoRenderer.pQualProp = NULL;

  if (CFGLoader::Filters.VideoRenderer.pBF)
    CFGLoader::Filters.VideoRenderer.pBF.FullRelease();

  if (CFGLoader::Filters.Audio.pBF)
    CFGLoader::Filters.Audio.pBF.FullRelease();
  
  if (CFGLoader::Filters.Video.pBF)
    CFGLoader::Filters.Video.pBF.FullRelease();
  
  while (! CFGLoader::Filters.Extras.empty())
  {
    if (CFGLoader::Filters.Extras.back().pBF)
      CFGLoader::Filters.Extras.back().pBF.FullRelease();

    CFGLoader::Filters.Extras.pop_back();
  }

  CLog::Log(LOGDEBUG, "%s ... done!", __FUNCTION__);

  return hr;
}

bool CDSGraph::IsPaused() const
{
  return CDSPlayer::PlayerState == DSPLAYER_PAUSED;
}
bool CDSGraph::IsInMenu() const
{
  return m_DvdState.isInMenu;
}
void CDSGraph::SeekInMilliSec(double sec)
{
  LONGLONG seekrequest = 0;

  if ( !m_pMediaSeeking )
    return;

  if( m_VideoInfo.time_format == TIME_FORMAT_MEDIA_TIME )
    seekrequest = ( LONGLONG )( MSEC_TO_DS_TIME(sec) );
  else
    seekrequest = (LONGLONG) sec;

  if ( seekrequest < 0 )
    seekrequest = 0;

  if (!m_VideoInfo.isDVD)
  {
    m_pMediaSeeking->SetPositions(&seekrequest, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
  }
  else
  {
    if (!CFGLoader::Filters.DVD.dvdControl)
      return;
    seekrequest = seekrequest * 10000;
    DVD_HMSF_TIMECODE tc = DShowUtil::RT2HMSF(seekrequest);
    CFGLoader::Filters.DVD.dvdControl->PlayAtTime(&tc, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
  }
}
void CDSGraph::Seek(bool bPlus, bool bLargeStep)
{
  // Chapter support
  if (bLargeStep && CChaptersManager::Get()->GetChapterCount() > 1)
  {
    int chapter = 0;
    if (bPlus)
      chapter = CChaptersManager::Get()->SeekChapter(
        CChaptersManager::Get()->GetChapter() + 1);
    else
      chapter = CChaptersManager::Get()->SeekChapter(
        CChaptersManager::Get()->GetChapter() - 1);

    if (chapter >= 0)
      m_callback.OnPlayBackSeekChapter(chapter);

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
    generalInfo = "Source Filter: " + CFGLoader::Filters.Source.osdname;

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
  CStreamsManager *c = CStreamsManager::Get();

  audioInfo.Format("Audio Decoder: %s (%s, %d Hz, %d Channels) | Renderer: %s",
    CFGLoader::Filters.Audio.osdname,
    c->GetAudioCodecDisplayName(),
    c->GetSampleRate(),
    c->GetChannels(),
    CFGLoader::Filters.AudioRenderer.osdname);
    
  return audioInfo;
}

CStdString CDSGraph::GetVideoInfo()
{
  CStdString videoInfo = "";
  CStreamsManager *c = CStreamsManager::Get();
  videoInfo.Format("Video Decoder: %s (%s, %dx%d)",
    CFGLoader::Filters.Video.osdname,
    c->GetVideoCodecDisplayName(),
    c->GetPictureWidth(),
    c->GetPictureHeight());

  if (!m_pStrCurrentFrameRate.empty())
    videoInfo += m_pStrCurrentFrameRate.c_str();

  if (!g_dsconfig.GetDXVAMode().empty())
    videoInfo += " | " + g_dsconfig.GetDXVAMode();

  return videoInfo;
}

bool CDSGraph::CanSeek()
{
  //Dvd are not using the interface IMediaSeeking for seeking
  //if the filter dont support seeking you would get VFW_E_DVD_OPERATION_INHIBITED on the PlayAtTime
  if (m_VideoInfo.isDVD)
  {
    return true;
  }
  DWORD seekcaps = AM_SEEKING_CanSeekForwards;
  seekcaps |= AM_SEEKING_CanSeekBackwards;
  seekcaps |= AM_SEEKING_CanSeekAbsolute;
  return SUCCEEDED(m_pMediaSeeking->CheckCapabilities(&seekcaps));  
}

void CDSGraph::ProcessMessage(WPARAM wParam, LPARAM lParam)
{
  
  if ( wParam == ID_DVD_MOUSE_MOVE)
  {
    //TODO make the xbmc gui stay hidden when moving mouse over menu
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    ULONG pButtonIndex;
    /**** Didnt found really where dvdplayer are doing it exactly so here it is *****/
    XBMC_Event newEvent;
    newEvent.type = XBMC_MOUSEMOTION;
    newEvent.motion.x = (uint16_t) pt.x;
    newEvent.motion.y = (uint16_t) pt.y;
    g_application.OnEvent(newEvent);
    /*CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
    g_windowManager.SendMessage(msg);*/
    /**** End of ugly hack ***/
    if (SUCCEEDED(CFGLoader::Filters.DVD.dvdInfo->GetButtonAtPosition(pt,&pButtonIndex)))
      CFGLoader::Filters.DVD.dvdControl->SelectButton(pButtonIndex);
    
  }
  else if ( wParam == ID_DVD_MOUSE_CLICK)
  {
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    ULONG pButtonIndex;
    if (SUCCEEDED(CFGLoader::Filters.DVD.dvdInfo->GetButtonAtPosition(pt, &pButtonIndex)))
      CFGLoader::Filters.DVD.dvdControl->SelectAndActivateButton(pButtonIndex);
  }
  else if ( wParam == ID_DS_SET_WINDOW_POS)
  {
    UpdateWindowPosition();
  }
  else if ( wParam == ID_PLAY_PLAY )
  {
    Play();
  }
  else if ( wParam == ID_SEEK_TO )
  {
    SeekInMilliSec((LPARAM)lParam);
  }
  else if ( wParam == ID_SEEK_FORWARDSMALL )
  {
    Seek(true, false);
    CLog::Log(LOGDEBUG,"%s ID_SEEK_FORWARDSMALL",__FUNCTION__);
  }
  else if ( wParam == ID_SEEK_FORWARDLARGE )
  {
    Seek(true, true);
    CLog::Log(LOGDEBUG,"%s ID_SEEK_FORWARDLARGE",__FUNCTION__);
  }
  else if ( wParam == ID_SEEK_BACKWARDSMALL )
  {
    Seek(false,false);
    CLog::Log(LOGDEBUG,"%s ID_SEEK_BACKWARDSMALL",__FUNCTION__);
  }
  else if ( wParam == ID_SEEK_BACKWARDLARGE )
  {
    Seek(false,true);
    CLog::Log(LOGDEBUG,"%s ID_SEEK_BACKWARDLARGE",__FUNCTION__);
  }
  else if ( wParam == ID_PLAY_PAUSE )
  {
    Pause();
    CLog::Log(LOGDEBUG,"%s ID_PLAY_PAUSE",__FUNCTION__);
  }
  else if ( wParam == ID_PLAY_STOP )
  {
    Stop(true);
    CLog::Log(LOGDEBUG,"%s ID_PLAY_STOP",__FUNCTION__);
  }
  else if ( wParam == ID_STOP_DSPLAYER )
  {
    Stop(true);
    CLog::Log(LOGDEBUG,"%s ID_STOP_DSPLAYER",__FUNCTION__);
  }

/*DVD COMMANDS*/
  if ( wParam == ID_DVD_NAV_UP )
  {
    CFGLoader::Filters.DVD.dvdControl->SelectRelativeButton(DVD_Relative_Upper);
  }
  else if ( wParam == ID_DVD_NAV_DOWN )
  {
    CFGLoader::Filters.DVD.dvdControl->SelectRelativeButton(DVD_Relative_Lower);
  }
  else if ( wParam == ID_DVD_NAV_LEFT )
  {
    CFGLoader::Filters.DVD.dvdControl->SelectRelativeButton(DVD_Relative_Left);
  }
  else if ( wParam == ID_DVD_NAV_RIGHT )
  {
    CFGLoader::Filters.DVD.dvdControl->SelectRelativeButton(DVD_Relative_Right);
  }
  else if ( wParam == ID_DVD_MENU_ROOT )
  {
    CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
    g_windowManager.SendMessage(msg);
    CFGLoader::Filters.DVD.dvdControl->ShowMenu(DVD_MENU_Root , DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
  }
  else if ( wParam == ID_DVD_MENU_EXIT )
  {
    CFGLoader::Filters.DVD.dvdControl->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
  }
  else if ( wParam == ID_DVD_MENU_BACK )
  {
    CFGLoader::Filters.DVD.dvdControl->ReturnFromSubmenu(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
  }
  else if ( wParam == ID_DVD_MENU_SELECT )
  {
    CFGLoader::Filters.DVD.dvdControl->ActivateButton();
  }
  else if ( wParam == ID_DVD_MENU_TITLE )
  {
    CFGLoader::Filters.DVD.dvdControl->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
  }
  else if ( wParam == ID_DVD_MENU_SUBTITLE )
  {
  }
  else if ( wParam == ID_DVD_MENU_AUDIO )
  {
  }
  else if ( wParam == ID_DVD_MENU_ANGLE )
  {
  }
}