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

#ifdef HAS_DS_PLAYER

#include "DSGraph.h"
#include "DSPlayer.h"
#include "Filters/RendererSettings.h"
#include "PixelShaderList.h"
#include "windowing/windows/winsystemwin32.h" //Important needed to get the right hwnd
#include "windowing/WindowingFactory.h" //important needed to get d3d object and device
#include "Util.h"
#include "Application.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include <iomanip>
#include "utils/Log.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "StreamsManager.h"
#include "GUIInfoManager.h"
#include <streams.h>


#include "FgManager.h"
#include "qnetwork.h"
#include "DSUtil/SmartPtr.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "DSUtil/MediaTypeEx.h"
#include "utils/timeutils.h"

enum 
{
  WM_GRAPHNOTIFY = WM_APP+1,
};


#include "utils/win32exception.h"
#include "Filters/EVRAllocatorPresenter.h"
#include "DSConfig.h"

using namespace std;

CDSGraph* g_dsGraph = NULL;
int32_t CDSGraph::m_threadID = 0;

CDSGraph::CDSGraph(CDVDClock* pClock, IPlayerCallback& callback)
    : m_pGraphBuilder(NULL), m_iCurrentFrameRefreshCycle(0),
    m_userId(0xACDCACDC), m_bReachedEnd(false), m_callback(callback),
    m_canSeek(-1), m_currentVolume(0)
{
  m_threadID = 0;
}

CDSGraph::~CDSGraph()
{
  m_threadID = 0;
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
  END_PERFORMANCE_COUNTER("Rendering file");

  if (FAILED(hr))
    return hr;
  /* Usually the call coming from IMediaSeeking is done on renderers and is passed upstream
     In every case i seen so far its much better to directly query the IMediaSeeking from the splitter
     directly which avoid confusion when the codecs dont pass the call correctly upstream
  */
  /* blinkseb: Unfortunatly, this does _not_ work. Haali always returns 0, and mkvsource's time is
    translated by about 30 seconds */
  /*BeginEnumFilters(pFilterGraph, pEF, pBF)
  {
    if (IsSplitter(pBF))
      hr = pBF->QueryInterface(__uuidof(m_pMediaSeeking), (void**)&m_pMediaSeeking);
  }
  EndEnumFilters
  if (!m_pMediaSeeking)
    hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaSeeking), (void**)&m_pMediaSeeking);*/

  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaSeeking), (void**)&m_pMediaSeeking);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaControl), (void**)&m_pMediaControl);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pMediaEvent), (void**)&m_pMediaEvent);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pBasicAudio), (void**)&m_pBasicAudio);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pBasicVideo), (void**)&m_pBasicVideo);
  hr = pFilterGraph->QueryInterface(__uuidof(m_pVideoWindow), (void**)&m_pVideoWindow);

  // Be sure we are using TIME_FORMAT_MEDIA_TIME
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

  m_VideoInfo.isDVD = CGraphFilters::Get()->IsDVD();
  m_threadID = GetCurrentThreadId();
  
  // Chapters
  CChaptersManager::Get()->InitManager();
  if (!CChaptersManager::Get()->LoadChapters())
    CLog::Log(LOGNOTICE, "%s No chapters found!", __FUNCTION__);

  SetVolume(g_settings.m_nVolumeLevel);
  
  CDSPlayer::PlayerState = DSPLAYER_LOADED;
  return hr;
}

void CDSGraph::CloseFile()
{
  HRESULT hr;

  if (m_pGraphBuilder)
  {
    if (CStreamsManager::Get())
      CStreamsManager::Get()->WaitUntilReady();

    Stop(true);

    CSingleLock lock(m_ObjectLock);
    hr = m_pGraphBuilder->RemoveFromROT();

    CStreamsManager::Destroy();
    CChaptersManager::Destroy();
    g_dsSettings.pixelShaderList->DisableAll();

    UnloadGraph();

    m_VideoInfo.Clear();
    m_State.Clear();
    
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
  // Not sure if it's needed
  /*if (m_threadID != GetCurrentThreadId())
  {
    PostMessage( new CDSMsg(CDSMsg::PLAYER_UPDATE_TIME));
    return;
  }*/

  CSingleLock lock(m_ObjectLock);

  if (!m_pMediaSeeking)
    return;
  LONGLONG Position;

  if(SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
    m_State.time = Position;

  if (m_State.time_total == 0)
    //we dont have the duration of the video yet so try to request it
    UpdateTotalTime();

  if ((CGraphFilters::Get()->VideoRenderer.pQualProp) && m_iCurrentFrameRefreshCycle <= 0)
  {
    //this is too slow if we are doing it on every UpdateTime
    int avgRate;
    CGraphFilters::Get()->VideoRenderer.pQualProp->get_AvgFrameRate(&avgRate);
    m_pStrCurrentFrameRate.Format(" | Real FPS: %4.2f", (float) avgRate / 100);
    m_iCurrentFrameRefreshCycle = 5;
  }
  m_iCurrentFrameRefreshCycle--;

  //On dvd playback the current time is received in the handlegraphevent
  if ( m_VideoInfo.isDVD )
  {
    CStreamsManager::Get()->UpdateDVDStream();
    return;
  }

  if (( m_State.time_total != 0 && m_State.time >= m_State.time_total ))
    m_bReachedEnd = true;

  CChaptersManager::Get()->UpdateChapters(m_State.time);
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
    if (!CGraphFilters::Get()->DVD.dvdInfo)
      return;

    REFERENCE_TIME rtDur = 0;
    DVD_HMSF_TIMECODE tcDur;
    ULONG ulFlags;
    if(SUCCEEDED(CGraphFilters::Get()->DVD.dvdInfo->GetTotalTitleTime(&tcDur, &ulFlags)))
    {
      rtDur = HMSF2RT(tcDur);
      m_State.time_total = rtDur;
    }
  }
  else
  {
    if (! m_pMediaSeeking)
      return;

    if(SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
      m_State.time_total = Duration;
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
      case EC_ERRORABORTEX:
        if (evParam2)
        {
          CStdString error = (CStdString) ((BSTR) evParam2);
          CLog::Log(LOGDEBUG,"%s EC_ERRORABORT. Error code: 0x%X; Error message: %s", __FUNCTION__, evParam1, error.c_str());
        } else
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

        m_State.time = HMSF2RT(*((DVD_HMSF_TIMECODE*)&evParam1), fps);;

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
            
            if (CGraphFilters::Get()->DVD.dvdInfo && SUCCEEDED (CGraphFilters::Get()->DVD.dvdInfo->GetDiscID (NULL, &m_pDvdStatus.DvdGuid)))
            {
              if (m_pDvdStatus.DvdTitleId != 0)
              {
                //s.NewDvd (llDVDGuid);
                // Set command line position
                CGraphFilters::Get()->DVD.dvdControl->PlayTitle(m_pDvdStatus.DvdTitleId, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                if (m_pDvdStatus.DvdChapterId > 1)
                  CGraphFilters::Get()->DVD.dvdControl->PlayChapterInTitle(m_pDvdStatus.DvdTitleId, m_pDvdStatus.DvdChapterId, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                else
                {
                  // Trick : skip trailers with somes DVDs
                  CGraphFilters::Get()->DVD.dvdControl->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
                  CGraphFilters::Get()->DVD.dvdControl->PlayAtTime(&m_pDvdStatus.DvdTimecode, DVD_CMD_FLAG_Flush, NULL);
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
  CSingleLock lock(m_ObjectLock);

  if (m_pBasicAudio && (nVolume != m_currentVolume))
  {
    m_pBasicAudio->put_Volume((nVolume == VOLUME_MINIMUM) ? -10000 : nVolume);
    m_currentVolume = nVolume;
  }
}

void CDSGraph::Stop(bool rewind)
{
  if (m_threadID != GetCurrentThreadId())
  {
    CDSGraph::PostMessage( new CDSMsgBool(CDSMsg::PLAYER_STOP, rewind) );
    return;
  }

  CSingleLock lock(m_ObjectLock);

  LONGLONG pos = 0;
  
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

  if (rewind && m_pMediaSeeking)
    m_pMediaSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

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
  CSingleLock lock(m_ObjectLock);

  HRESULT hr;
  hr = CGraphFilters::Get()->DVD.dvdControl->SelectAtPosition(pt);
  if (SUCCEEDED(hr))
    return true;
  return true;
}

void CDSGraph::Play(bool force/* = false*/)
{
  if (m_threadID != GetCurrentThreadId())
  {
    PostMessage( new CDSMsgBool(CDSMsg::PLAYER_PLAY, force) );
    return;
  }

  CSingleLock lock(m_ObjectLock);
  if (m_pMediaControl && (force || m_State.current_filter_state != State_Running))
    m_pMediaControl->Run();

  UpdateState();
}

void CDSGraph::Pause()
{
  if (m_threadID != GetCurrentThreadId())
  {
    PostMessage( new CDSMsg(CDSMsg::PLAYER_PAUSE));
    return;
  }

  CSingleLock lock(m_ObjectLock);
  if (CDSPlayer::PlayerState == DSPLAYER_PAUSED)
  {
    if (m_State.current_filter_state != State_Running)
      m_pMediaControl->Run();
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
      }
  }

  UpdateState();
}

HRESULT CDSGraph::UnloadGraph()
{
  HRESULT hr = S_OK;

  BeginEnumFilters(pFilterGraph, pEM, pBF)
  {
    CStdString filterName;
    g_charsetConverter.wToUTF8(GetFilterName(pBF), filterName);

    try
    {
      hr = RemoveFilter(pFilterGraph, pBF);
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

  CGraphFilters::Get()->DVD.Clear();
  m_pMediaControl.Release();
  m_pMediaEvent.Release();
  m_pMediaSeeking.Release();
  m_pVideoWindow.Release();
  m_pBasicAudio.Release();

  /* delete filters */

  CLog::Log(LOGDEBUG, "%s Deleting filters ...", __FUNCTION__);

  /* Release config interfaces */
  g_dsconfig.ClearConfig();

  CGraphFilters::Destroy();

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
void CDSGraph::SeekInMilliSec(double position)
{
  Seek(MSEC_TO_DS_TIME(position));
}

void CDSGraph::Seek(uint64_t position, uint32_t flags /*= AM_SEEKING_AbsolutePositioning*/, bool showPopup /*= true*/)
{
  if (m_threadID != GetCurrentThreadId())
  {
    PostMessage( new CDSMsgPlayerSeekTime(position, flags) );
    return;
  }

  CSingleLock lock(m_ObjectLock);

  if (showPopup)
    g_infoManager.SetDisplayAfterSeek(100000);

  if ( !m_pMediaSeeking )
    return;

  if (!m_VideoInfo.isDVD)
  {
    m_pMediaSeeking->SetPositions((LONGLONG *) &position, flags, NULL, AM_SEEKING_NoPositioning);
  }
  else
  {
    if (!CGraphFilters::Get()->DVD.dvdControl)
      return;

    DVD_HMSF_TIMECODE tc = RT2HMSF(position);
    CGraphFilters::Get()->DVD.dvdControl->PlayAtTime(&tc, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
  }
  int iTime = DS_TIME_TO_MSEC(position);
  int seekOffset = (int)(iTime - DS_TIME_TO_MSEC(GetTime()));
  m_callback.OnPlayBackSeek(iTime,seekOffset);
  // set flag to indicate we have finished a seeking request
  g_infoManager.m_performingSeek = false;
  if (showPopup)
    g_infoManager.SetDisplayAfterSeek();
}

void CDSGraph::Seek(bool bPlus, bool bLargeStep)
{
  // Chapter support
  if ((CChaptersManager::Get()->HasChapters())
  && (((bPlus && CChaptersManager::Get()->GetChapter() < CChaptersManager::Get()->GetChapterCount())
  || (!bPlus && CChaptersManager::Get()->GetChapter() > 1)) && bLargeStep))
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

  int64_t seek = 0;
  if (g_advancedSettings.m_videoUseTimeSeeking && DS_TIME_TO_SEC(GetTotalTime()) > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    seek = GetTime() + SEC_TO_DS_TIME(seek);
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = (float) (bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig);
    else
      percent = (float) (bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward);
    seek = GetTotalTime() * (float) ((GetPercentage() + percent) / 100);
  }

  UpdateTime();
  Seek(seek);
  UpdateTime();
}

void CDSGraph::SeekPercentage(float iPercent)
{
  uint64_t seek = GetTotalTime() * (int64_t) ((GetPercentage() + iPercent) / 100);
  Seek(seek);
}

// return time in DS_TIME_BASE unit
uint64_t CDSGraph::GetTime()
{
  return m_State.time;
}

// return length in DS_TIME_BASE unit
uint64_t CDSGraph::GetTotalTime()
{
  return m_State.time_total;
}

float CDSGraph::GetPercentage()
{
  uint64_t iTotalTime = GetTotalTime();

  if (!iTotalTime)
    return 0.0f;

  return (GetTime() * 100.0f / (float) iTotalTime);
}

CStdString CDSGraph::GetGeneralInfo()
{
  CStdString generalInfo = "";

  if (! CGraphFilters::Get()->Source.osdname.empty() )
    generalInfo = "Source Filter: " + CGraphFilters::Get()->Source.osdname;

  if (! CGraphFilters::Get()->Splitter.osdname.empty())
  {
    if (generalInfo.empty())
      generalInfo = "Splitter: " + CGraphFilters::Get()->Splitter.osdname;
    else
      generalInfo += " | Splitter: " + CGraphFilters::Get()->Splitter.osdname;
  }

  if (generalInfo.empty())
    generalInfo = "Video renderer: " + CGraphFilters::Get()->VideoRenderer.osdname;
  else
    generalInfo += " | Video renderer: " + CGraphFilters::Get()->VideoRenderer.osdname;

  return generalInfo;
}

CStdString CDSGraph::GetAudioInfo()
{
  CStdString audioInfo;
  CStreamsManager *c = CStreamsManager::Get();

  if (! c)
    return "File closed";

  audioInfo.Format("Audio Decoder: %s (%s, %d Hz, %d Channels) | Renderer: %s",
    CGraphFilters::Get()->Audio.osdname,
    c->GetAudioCodecDisplayName(),
    c->GetSampleRate(),
    c->GetChannels(),
    CGraphFilters::Get()->AudioRenderer.osdname);
    
  return audioInfo;
}

CStdString CDSGraph::GetVideoInfo()
{
  CStdString videoInfo = "";
  CStreamsManager *c = CStreamsManager::Get();

  if (! c)
    return "File closed";

  videoInfo.Format("Video Decoder: %s (%s, %dx%d)",
    CGraphFilters::Get()->Video.osdname,
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
  if (CDSPlayer::PlayerState == DSPLAYER_CLOSING || CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    return false;

  //Dvd are not using the interface IMediaSeeking for seeking
  //if the filter dont support seeking you would get VFW_E_DVD_OPERATION_INHIBITED on the PlayAtTime
  if (m_VideoInfo.isDVD)
    return true;

  if (m_canSeek > -1)
    return (m_canSeek == 1);
  
  if (! m_pMediaSeeking)
    return false;

  DWORD seekcaps = AM_SEEKING_CanSeekForwards
    | AM_SEEKING_CanSeekBackwards
    | AM_SEEKING_CanSeekAbsolute;

  // Cache seeking capabilities. It seems to hang if it's called too quickly
  if SUCCEEDED(m_pMediaSeeking->CheckCapabilities(&seekcaps))
    m_canSeek = 1;
  else
    m_canSeek = 0;

  return (m_canSeek == 1);
}

void CDSGraph::ProcessThreadMessages()
{
  MSG _msg;
  BOOL bRet;
  while ((bRet = GetMessage(&_msg, (HWND) -1, 0, 0) != 0) &&
    _msg.message == WM_GRAPHMESSAGE)
  {
    CDSMsg* msg = reinterpret_cast<CDSMsg *>( _msg.lParam );
    CLog::Log(LOGDEBUG, "%s Message received : %d on thread 0x%X", __FUNCTION__, msg->GetMessageType(), m_threadID);

    if (CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    {
      msg->Set();
      msg->Release();
      break;
    }

    if ( msg->IsType(CDSMsg::GENERAL_SET_WINDOW_POS) )
    {
      UpdateWindowPosition();
    }
    else if ( msg->IsType(CDSMsg::PLAYER_SEEK_TIME) )
    {
      CDSMsgPlayerSeekTime* speMsg = reinterpret_cast<CDSMsgPlayerSeekTime *>( msg );
      Seek(speMsg->GetTime(), speMsg->GetFlags(), speMsg->ShowPopup());
    }
    else if ( msg->IsType(CDSMsg::PLAYER_SEEK) )
    {
      CDSMsgPlayerSeek* speMsg = reinterpret_cast<CDSMsgPlayerSeek*>( msg );
      Seek( speMsg->Forward(), speMsg->LargeStep() );
    }
    else if ( msg->IsType(CDSMsg::PLAYER_SEEK_PERCENT) )
    {
      CDSMsgDouble * speMsg = reinterpret_cast<CDSMsgDouble *>( msg );
      SeekPercentage((float) speMsg->m_value );
    }
    else if ( msg->IsType(CDSMsg::PLAYER_PAUSE) )
    {
      Pause();
    }
    else if ( msg->IsType(CDSMsg::PLAYER_STOP) )
    {
      CDSMsgBool* speMsg = reinterpret_cast<CDSMsgBool *>( msg );
      Stop(speMsg->m_value);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_PLAY) )
    {
      CDSMsgBool* speMsg = reinterpret_cast<CDSMsgBool *>( msg );
      Play(speMsg->m_value);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_UPDATE_TIME) )
    {
      UpdateTime();
    }

    /*DVD COMMANDS*/
    if ( msg->IsType(CDSMsg::PLAYER_DVD_MOUSE_MOVE) )
    {
      CDSMsgInt* speMsg = reinterpret_cast<CDSMsgInt *>( msg );
      //TODO make the xbmc gui stay hidden when moving mouse over menu
      POINT pt;
      pt.x = GET_X_LPARAM(speMsg->m_value);
      pt.y = GET_Y_LPARAM(speMsg->m_value);
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
      if (SUCCEEDED(CGraphFilters::Get()->DVD.dvdInfo->GetButtonAtPosition(pt, &pButtonIndex)))
        CGraphFilters::Get()->DVD.dvdControl->SelectButton(pButtonIndex);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MOUSE_CLICK) )
    {
      CDSMsgInt* speMsg = reinterpret_cast<CDSMsgInt *>( msg );
      POINT pt;
      pt.x = GET_X_LPARAM(speMsg->m_value);
      pt.y = GET_Y_LPARAM(speMsg->m_value);
      ULONG pButtonIndex;
      if (SUCCEEDED(CGraphFilters::Get()->DVD.dvdInfo->GetButtonAtPosition(pt, &pButtonIndex)))
        CGraphFilters::Get()->DVD.dvdControl->SelectAndActivateButton(pButtonIndex);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_NAV_UP) )
    {
      CGraphFilters::Get()->DVD.dvdControl->SelectRelativeButton(DVD_Relative_Upper);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_NAV_DOWN) )
    {
      CGraphFilters::Get()->DVD.dvdControl->SelectRelativeButton(DVD_Relative_Lower);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_NAV_LEFT) )
    {
      CGraphFilters::Get()->DVD.dvdControl->SelectRelativeButton(DVD_Relative_Left);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_NAV_RIGHT) )
    {
      CGraphFilters::Get()->DVD.dvdControl->SelectRelativeButton(DVD_Relative_Right);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_ROOT) )
    {
      CGUIMessage _msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
      g_windowManager.SendMessage(_msg);
      CGraphFilters::Get()->DVD.dvdControl->ShowMenu(DVD_MENU_Root , DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_EXIT) )
    {
      CGraphFilters::Get()->DVD.dvdControl->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_BACK) )
    {
      CGraphFilters::Get()->DVD.dvdControl->ReturnFromSubmenu(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_SELECT) )
    {
      CGraphFilters::Get()->DVD.dvdControl->ActivateButton();
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_TITLE) )
    {
      CGraphFilters::Get()->DVD.dvdControl->ShowMenu(DVD_MENU_Title, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_SUBTITLE) )
    {
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_AUDIO) )
    {
    }
    else if ( msg->IsType(CDSMsg::PLAYER_DVD_MENU_ANGLE) )
    {
    }

    msg->Set();
    msg->Release();
  }
}

#endif