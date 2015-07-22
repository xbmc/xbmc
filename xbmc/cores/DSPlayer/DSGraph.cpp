/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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
#include "ApplicationMessenger.h"
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
#include "utils/ipinhook.h"
#include "DSInputStreamPVRManager.h"
#include "MadvrCallback.h"

enum
{
  WM_GRAPHNOTIFY = WM_APP + 1,
};


/*#include "utils/win32exception.h"*/
#include "Filters/EVRAllocatorPresenter.h"
#include "FGManager2.h"

using namespace std;

CDSGraph* g_dsGraph = NULL;

CDSGraph::CDSGraph(CDVDClock* pClock, IPlayerCallback& callback)
  : m_pGraphBuilder(NULL),
  m_iCurrentFrameRefreshCycle(0),
  m_callback(callback),
  m_canSeek(-1),
  m_currentVolume(0.0f)
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

  m_pGraphBuilder = new CFGManager2();
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

  m_pMediaSeeking = pFilterGraph;
  m_pMediaControl = pFilterGraph;
  m_pMediaEvent = pFilterGraph;
  m_pBasicAudio = pFilterGraph;
  m_pBasicVideo = pFilterGraph;
  m_pVideoWindow = pFilterGraph;
  m_pAMOpenProgress = pFilterGraph;

  // Be sure we are using TIME_FORMAT_MEDIA_TIME
  hr = m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
  m_VideoInfo.time_format = TIME_FORMAT_MEDIA_TIME;

  if (m_pVideoWindow)
  {
    //HRESULT hr;
    //m_pVideoWindow->put_Owner((OAHWND)g_hWnd);
    m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    m_pVideoWindow->put_Visible(OATRUE);
    m_pVideoWindow->put_AutoShow(OATRUE);
    m_pVideoWindow->put_WindowState(SW_SHOW);
    m_pVideoWindow->SetWindowForeground(OATRUE);
    m_pVideoWindow->put_MessageDrain((OAHWND)CDSPlayer::GetDShWnd());
  }

  // if needed set resolution to match fps then set pixelshader & settings for madVR
  if (CMadvrCallback::Get()->UsingMadvr())
  {
    CMadvrCallback::Get()->GetCallback()->SetResolution();
    CMadvrCallback::Get()->GetCallback()->SetMadvrPixelShader();
    CMadvrCallback::Get()->GetCallback()->RestoreMadvrSettings();
  }

  //TODO Ti-Ben
  //with the vmr9 we need to add AM_DVD_SWDEC_PREFER  AM_DVD_VMR9_ONLY on the ivmr9config prefs

  m_VideoInfo.isDVD = CGraphFilters::Get()->IsDVD();

  CStdString filterName;
  BeginEnumFilters(pFilterGraph, pEF, pBF)
  {
    if (IsVideoRenderer(pBF)) {
      CGraphFilters::Get()->VideoRenderer.SetFilterInfo(pBF);
    }
    else if (IsAudioWaveRenderer(pBF)) {
      if (!CGraphFilters::Get()->AudioRenderer.pBF)
        CGraphFilters::Get()->AudioRenderer.pBF = pBF;
      CGraphFilters::Get()->AudioRenderer.SetFilterInfo(pBF);
    }

    if (!m_pAMOpenProgress)
    {
      m_pAMOpenProgress = pBF;
    }

    if (!CGraphFilters::Get()->AudioRenderer.osdname.IsEmpty() && !CGraphFilters::Get()->VideoRenderer.osdname.IsEmpty() && m_pAMOpenProgress)
      break;
  }
  EndEnumFilters

    SetVolume(g_application.GetVolume(false));

  CDSPlayer::PlayerState = DSPLAYER_LOADED;
  return hr;
}

void CDSGraph::CloseFile()
{
  HRESULT hr;

  if (m_pGraphBuilder)
  {
    CMadvrCallback::Get()->SetRenderOnMadvr(false);

    if (m_pAMOpenProgress)
      m_pAMOpenProgress->AbortOperation();

    if (CStreamsManager::Get())
      CStreamsManager::Get()->WaitUntilReady();

    Stop(true);

    CSingleLock lock(m_ObjectLock);

    CStreamsManager::Destroy();
    CChaptersManager::Destroy();
    g_dsSettings.pixelShaderList->DisableAll();

    m_VideoInfo.Clear();
    m_State.Clear();

    // Stop sending event messages
    if (m_pMediaEvent)
    {
      m_pMediaEvent->SetNotifyWindow((OAHWND)NULL, NULL, NULL);
    }

    /* delete filters */
    CLog::Log(LOGDEBUG, "%s Deleting filters ...", __FUNCTION__);
    CGraphFilters::Destroy();
    CLog::Log(LOGDEBUG, "%s ... done!", __FUNCTION__);
    CGraphFilters::Get()->DVD.Clear();
    pFilterGraph.Release();

    m_pVideoWindow.Release();
    m_pMediaControl.Release();
    m_pMediaEvent.Release();
    m_pMediaSeeking.Release();
    m_pBasicAudio.Release();
    m_pBasicVideo.Release();
    m_pDvdState.Release();
    m_pAMOpenProgress.Release();

    hr = m_pGraphBuilder->RemoveFromROT();
    SAFE_DELETE(m_pGraphBuilder);
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

  if (g_pPVRStream)
  {
	  if (SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
		  m_State.time = Position;

	  if (SUCCEEDED(m_pMediaSeeking->GetDuration(&Position)))
		  m_State.time_total = Position;
	  return;
  }

  if (SUCCEEDED(m_pMediaSeeking->GetPositions(&Position, NULL)))
    m_State.time = Position;

  // Update total time of video.
  // Duration time may increase during playback of in-progress recordings
  UpdateTotalTime();

  if ((CGraphFilters::Get()->VideoRenderer.pQualProp) && m_iCurrentFrameRefreshCycle <= 0 && !CMadvrCallback::Get()->UsingMadvr())
  {
    //this is too slow if we are doing it on every UpdateTime
    int avgRate;
    CGraphFilters::Get()->VideoRenderer.pQualProp->get_AvgFrameRate(&avgRate);
    m_pStrCurrentFrameRate.Format(" | Real FPS: %4.2f", (float)avgRate / 100);
    m_iCurrentFrameRefreshCycle = 5;
  }
  m_iCurrentFrameRefreshCycle--;

  //On dvd playback the current time is received in the handlegraphevent
  if (m_VideoInfo.isDVD)
  {
    CStreamsManager::Get()->UpdateDVDStream();
    return;
  }

  if (m_pAMOpenProgress) {
    int64_t t = 0, c = 0;
    if (SUCCEEDED(m_pAMOpenProgress->QueryProgress(&t, &c)) && t > 0 && c < t) {
      m_State.cache_offset = (double)c / t;
    }
  }

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
    if (SUCCEEDED(CGraphFilters::Get()->DVD.dvdInfo->GetTotalTitleTime(&tcDur, &ulFlags)))
    {
      rtDur = HMSF2RT(tcDur);
      m_State.time_total = rtDur;
    }
  }
  else
  {
    if (!m_pMediaSeeking)
      return;

    if (SUCCEEDED(m_pMediaSeeking->GetDuration(&Duration)))
      m_State.time_total = Duration;
  }
}

void CDSGraph::UpdateMadvrWindowPosition()
{
  CRect srcRect, destRect, viewRect;
  g_renderManager.GetVideoRect(srcRect, destRect, viewRect);
  CMadvrCallback::Get()->GetCallback()->SetMadvrPosition(viewRect, destRect);
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
  if (hr == VFW_S_CANT_CUE)
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
  while ((CDSPlayer::PlayerState != DSPLAYER_CLOSING && CDSPlayer::PlayerState != DSPLAYER_CLOSED)
    && SUCCEEDED(m_pMediaEvent->GetEvent(&evCode, &evParam1, &evParam2, 0)))
  {
    switch (evCode)
    {
    case EC_STEP_COMPLETE:
      CLog::Log(LOGDEBUG, "%s EC_STEP_COMPLETE", __FUNCTION__);
      CApplicationMessenger::Get().MediaStop();
      break;
    case EC_COMPLETE:
      CLog::Log(LOGDEBUG, "%s EC_COMPLETE", __FUNCTION__);
      m_State.eof = true;
      CApplicationMessenger::Get().MediaStop();
      break;
    case EC_BUFFERING_DATA:
      CLog::Log(LOGDEBUG, "%s EC_BUFFERING_DATA", __FUNCTION__);
      break;
    case EC_USERABORT:
      CLog::Log(LOGDEBUG, "%s EC_USERABORT", __FUNCTION__);
      CApplicationMessenger::Get().MediaStop();
      break;
    case EC_ERRORABORT:
    case EC_ERRORABORTEX:
      if (evParam2)
      {
        CStdString error = (CStdString)((BSTR)evParam2);
        CLog::Log(LOGDEBUG, "%s EC_ERRORABORT. Error code: 0x%X; Error message: %s", __FUNCTION__, evParam1, error.c_str());
      }
      else
        CLog::Log(LOGDEBUG, "%s EC_ERRORABORT. Error code: 0x%X", __FUNCTION__, evParam1);
      CApplicationMessenger::Get().MediaStop();
      break;
    case EC_STATE_CHANGE:
      CLog::Log(LOGDEBUG, "%s EC_STATE_CHANGE", __FUNCTION__);
      break;
    case EC_DEVICE_LOST:
      CLog::Log(LOGDEBUG, "%s EC_DEVICE_LOST", __FUNCTION__);
      break;
    case EC_VMR_RECONNECTION_FAILED:
      CLog::Log(LOGDEBUG, "%s EC_VMR_RECONNECTION_FAILED", __FUNCTION__);
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

      switch (m_pDvdStatus.DvdDomain)
      {
      case DVD_DOMAIN_FirstPlay:

        if (CGraphFilters::Get()->DVD.dvdInfo && SUCCEEDED(CGraphFilters::Get()->DVD.dvdInfo->GetDiscID(NULL, &m_pDvdStatus.DvdGuid)))
        {
          if (m_pDvdStatus.DvdTitleId != 0)
          {
            //s.NewDvd (llDVDGuid);
            // Set command line position
            CGraphFilters::Get()->DVD.dvdControl->PlayTitle(m_pDvdStatus.DvdTitleId, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
            if (m_pDvdStatus.DvdChapterId > 1)
              CGraphFilters::Get()->DVD.dvdControl->PlayChapterInTitle(m_pDvdStatus.DvdTitleId, m_pDvdStatus.DvdChapterId, DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
            else
            {
              // Trick : skip trailers with somes DVDs
              CGraphFilters::Get()->DVD.dvdControl->Resume(DVD_CMD_FLAG_Block | DVD_CMD_FLAG_Flush, NULL);
              CGraphFilters::Get()->DVD.dvdControl->PlayAtTime(&m_pDvdStatus.DvdTimecode, DVD_CMD_FLAG_Flush, NULL);
            }

            //m_iDVDTitle	  = s.lDVDTitle;
            m_pDvdStatus.DvdTitleId = 0;
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
void CDSGraph::SetVolume(float nVolume)
{
  CSingleLock lock(m_ObjectLock);

  if (m_pBasicAudio && (nVolume != m_currentVolume))
  {
    m_pBasicAudio->put_Volume((nVolume == VOLUME_MINIMUM) ? -10000 : (nVolume - 1) * 6000);
    m_currentVolume = nVolume;
  }
}

void CDSGraph::Stop(bool rewind)
{
  if (!CDSPlayer::IsCurrentThread())
  {
    CDSPlayer::PostMessage(new CDSMsgBool(CDSMsg::PLAYER_STOP, rewind));
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

  if (!m_pGraphBuilder)
    return;

  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    Com::SmartQIPtr<IFileSourceFilter> pFSF;
    pFSF = Com::SmartQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus>(pBF);
    if (pFSF)
    {
      WCHAR* pFN = NULL;
      AM_MEDIA_TYPE mt;
      if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN)
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
  if (!CDSPlayer::IsCurrentThread())
  {
    CDSPlayer::PostMessage(new CDSMsgBool(CDSMsg::PLAYER_PLAY, force));
    return;
  }
  CSingleLock lock(m_ObjectLock);
  if (m_pMediaControl && (force || m_State.current_filter_state != State_Running))
    m_pMediaControl->Run();

  UpdateState();
}

void CDSGraph::Pause()
{
  if (!CDSPlayer::IsCurrentThread())
  {
    CDSPlayer::PostMessage(new CDSMsg(CDSMsg::PLAYER_PAUSE));
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
      if (m_pMediaControl->Pause() == S_FALSE)
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
  if (!CDSPlayer::IsCurrentThread())
  {
    CDSPlayer::PostMessage(new CDSMsgPlayerSeekTime(position, flags));
    return;
  }
  CSingleLock lock(m_ObjectLock);

  if (g_pPVRStream || CGraphFilters::Get()->UsingMediaPortalTsReader())
  {
    // For live tv and in-progress recordings.
    // When seek position is close to the end of the file.
    uint64_t endOfTimeShiftFile = (GetTotalTime() >= (uint64_t)MSEC_TO_DS_TIME(2000)) ? (GetTotalTime() - (uint64_t)MSEC_TO_DS_TIME(2000)) : 0;
    if (position > endOfTimeShiftFile)
      position = endOfTimeShiftFile;
  }

  if (showPopup)
    g_infoManager.SetDisplayAfterSeek(100000);

  if (!m_pMediaSeeking)
    return;

  if (!m_VideoInfo.isDVD)
  {
    m_pMediaSeeking->SetPositions((LONGLONG *)&position, flags, NULL, AM_SEEKING_NoPositioning);
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
  m_callback.OnPlayBackSeek(iTime, seekOffset);
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
  if (g_advancedSettings.m_videoUseTimeSeeking && DS_TIME_TO_SEC(GetTotalTime()) > 2 * g_advancedSettings.m_videoTimeSeekForwardBig)
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
      percent = (float)(bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig);
    else
      percent = (float)(bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward);
    seek = GetTotalTime() * (float)((GetPercentage() + percent) / 100);
  }

  UpdateTime();
  Seek((seek < 0) ? 0 : seek);
  UpdateTime();
}

void CDSGraph::SeekPercentage(float iPercent)
{
  uint64_t iTotalTime = GetTotalTime();

  if (iTotalTime > 0)
  {
    Seek((uint64_t)iTotalTime * iPercent / 100);
  }
}

// return time in DS_TIME_BASE unit
uint64_t CDSGraph::GetTime(bool forcePlaybackTime)
{
  CSingleLock lock(m_ObjectLock);

  if (!g_pPVRStream || CDSPlayer::IsCurrentThread() || forcePlaybackTime) // Used by seek or none PVR video
    return m_State.time;
  else                                                                    // Used by GUI on PVR video
  {
    if (g_PVRManager.IsPlayingTV() && g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD))
      return MSEC_TO_DS_TIME(g_pPVRStream->GetTime());  // LiveTV EPG time
    else
      return m_State.time;                              // Playback Time
  }
}

// return length in DS_TIME_BASE unit
uint64_t CDSGraph::GetTotalTime(bool forcePlaybackTime)
{
  CSingleLock lock(m_ObjectLock);

  if (!g_pPVRStream || CDSPlayer::IsCurrentThread() || forcePlaybackTime) // Used by seek or none PVR video
    return m_State.time_total;
  else 												                                            // Used by GUI on PVR video
  {
    if (g_PVRManager.IsPlayingTV() && g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD))
      return MSEC_TO_DS_TIME(g_pPVRStream->GetTotalTime()); // LiveTV EPG time
    else
      return m_State.time_total;                            // Playback Time
  }
}

float CDSGraph::GetPercentage()
{
  uint64_t iTotalTime = GetTotalTime();

  if (iTotalTime)
  {
    return (GetTime() * 100.0f / (float)iTotalTime);
  }
  return 0.0f;
}

float CDSGraph::GetCachePercentage()
{
  return (m_State.cache_offset * 100) - GetPercentage();
}

CStdString CDSGraph::GetGeneralInfo()
{
  CStdString generalInfo, info;

  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    if (pBF == CGraphFilters::Get()->AudioRenderer.pBF || pBF == CGraphFilters::Get()->VideoRenderer.pBF)
      continue;

    // force osdname for XySubFilter
    if ((pBF == CGraphFilters::Get()->Subs.pBF) && CGraphFilters::Get()->Subs.osdname != "")
      info = CGraphFilters::Get()->Subs.osdname;
    else
      g_charsetConverter.wToUTF8(GetFilterName(pBF), info);
    if (!info.empty())
      generalInfo.empty() ? generalInfo += "Filters: " + info : generalInfo += " | " + info;
  }
  EndEnumFilters

    return generalInfo;
}

CStdString CDSGraph::GetAudioInfo()
{
  CStdString audioInfo;
  CStreamsManager *c = CStreamsManager::Get();

  if (!c)
    return "File closed";

  if (!CSettings::Get().GetBool("dsplayer.showsplitterdetail") ||
      CGraphFilters::Get()->UsingMediaPortalTsReader())
  {
    audioInfo.Format("Audio: (%s, %d Hz, %d Channels) | Renderer: %s",
      c->GetAudioCodecDisplayName(g_application.m_pPlayer->GetAudioStream()),
      c->GetSampleRate(g_application.m_pPlayer->GetAudioStream()),
      c->GetChannels(g_application.m_pPlayer->GetAudioStream()),
      CGraphFilters::Get()->AudioRenderer.osdname);
  }
  else
  {
    CStdString strStreamName;
    c->GetAudioStreamName(g_application.m_pPlayer->GetAudioStream(),strStreamName);
    audioInfo.Format("Audio: (%s ) | Renderer: %s",
      strStreamName,
      CGraphFilters::Get()->AudioRenderer.osdname);
  }
  return audioInfo;
}

CStdString CDSGraph::GetVideoInfo()
{
  CStdString videoInfo = "";
  CStreamsManager *c = CStreamsManager::Get();

  if (!c)
    return "File closed";
  
  if (!CSettings::Get().GetBool("dsplayer.showsplitterdetail") ||
      CGraphFilters::Get()->UsingMediaPortalTsReader())
  {
    videoInfo.Format("Video: (%s, %dx%d) | Renderer: %s",
      c->GetVideoCodecDisplayName(),
      c->GetPictureWidth(),
      c->GetPictureHeight(),
      CGraphFilters::Get()->VideoRenderer.osdname);
  } 
  else
  {
    CStdString strStreamName;
    c->GetVideoStreamName(strStreamName);
    videoInfo.Format("Video: (%s) | Renderer: %s", 
      strStreamName, 
      CGraphFilters::Get()->VideoRenderer.osdname);
  }

  if (!m_pStrCurrentFrameRate.empty())
    videoInfo += m_pStrCurrentFrameRate.c_str();

  CStdString strDXVA;
  if (!CMadvrCallback::Get()->UsingMadvr())
    strDXVA = GetDXVADecoderDescription();
  /*
  // I Don't know if this work properly
  else
    strDXVA = CMadvrCallback::Get()->GetCallback()->GetDXVADecoderDescription();
  */
  if (!strDXVA.empty())
    videoInfo += " | " + strDXVA;

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

  if (!m_pMediaSeeking)
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

#endif
