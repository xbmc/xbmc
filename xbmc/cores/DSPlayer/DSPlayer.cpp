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

#include "DSPlayer.h"
#include "winsystemwin32.h" //Important needed to get the right hwnd
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "RegExp.h"
#include "URL.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
using namespace std;

CDSPlayer::CDSPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
	    m_pDsGraph()
{
  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);
  m_bAbortRequest = false;
  
}

CDSPlayer::~CDSPlayer()
{
  m_pDsGraph.CloseFile();
  m_bAbortRequest = true;
  //g_renderManager.UnInit();
  CLog::Log(LOGNOTICE, "DSPlayer: waiting for threads to exit");
  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();
}

bool CDSPlayer::OpenFile(const CFileItem& file,const CPlayerOptions &options)
{
  HRESULT hr;
  if(ThreadHandle())
    CloseFile();
  //Creating the graph and querying every filter required for the playback
  ResetEvent(m_hReadyEvent);
  m_Filename = file.GetAsUrl();
  m_PlayerOptions = options;
  m_currentSpeed = 10000;
  m_currentRate = 1.0;
  hr = m_pDsGraph.SetFile(file,m_PlayerOptions);
  if ( FAILED(hr) )
  {
    CLog::Log(LOGERROR,"%s failed to start this file with dsplayer %s",__FUNCTION__,file.GetAsUrl().GetFileName().c_str());
    return false;
  }
  
  Create();
  WaitForSingleObject(m_hReadyEvent, INFINITE);
  return true;
}
bool CDSPlayer::CloseFile()
{
  // unpause the player
  //SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;
  m_pDsGraph.CloseFile();
  //g_renderManager.UnInit();
  CLog::Log(LOGNOTICE, "CDSPlayer: finished waiting");
  //StopThread();
  m_bAbortRequest = true;
  m_callback.OnPlayBackEnded();
  return true;
}

bool CDSPlayer::IsPlaying() const
{
  return !m_bStop;
}

bool CDSPlayer::HasVideo() const
{
  return true;
}
bool CDSPlayer::HasAudio() const
{
  return true;
}

//TO DO EVERY INFO FOR THE GUI
void CDSPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  
  strAudioInfo = m_pDsGraph.GetAudioInfo();
  //"CDSPlayer:GetAudioInfo";
  //this is the function from dvdplayeraudio
  //s << "aq:"     << setw(2) << min(99,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  //s << ", kB/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;
  //return s.str();
}

void CDSPlayer::GetVideoInfo(CStdString& strVideoInfo)
{	
  strVideoInfo = m_pDsGraph.GetVideoInfo();
  //strVideoInfo.Format("D(%s) P(%s)", m_pDsGraph.GetPlayerInfo().c_str());
}

void CDSPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = m_pDsGraph.GetGeneralInfo();
  //GetGeneralInfo.Format("CPU:( ThreadRelative:%2i%% FiltersThread:%2i%% )"                         
  //                       , (int)(CThread::GetRelativeUsage()*100)
  //  					   , (int) (m_pDsGraph.GetRelativeUsage()*100));
  //strGeneralInfo = "CDSPlayer:GetGeneralInfo";

}

//CThread
void CDSPlayer::OnStartup()
{
  CThread::SetName("CDSPlayer");
}

void CDSPlayer::OnExit()
{
  try
  {
    m_pDsGraph.CloseFile();
   
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close the graph", __FUNCTION__);
  }
	if (m_bAbortRequest)
      m_callback.OnPlayBackStopped();
    else
      m_callback.OnPlayBackEnded();
  m_bStop = true;
}
void CDSPlayer::Process()
{
  m_callback.OnPlayBackStarted();
  bool pStartPosDone = true;
  // allow renderer to switch to fullscreen if requested
  //m_pDsGraph.EnableFullscreen(true);
  // make sure application know our info
  //UpdateApplication(0);
  //UpdatePlayState(0);
  if (m_PlayerOptions.starttime>0)
    pStartPosDone=false;
  
  SetEvent(m_hReadyEvent);
  while (!m_bAbortRequest)
  {
    if (m_bAbortRequest)
      break;
    if (!pStartPosDone)
	  {
      SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_TO ,((LPARAM)m_PlayerOptions.starttime * 1000 ));
      pStartPosDone = true;
	  }
	  m_pDsGraph.HandleGraphEvent();
    //Handle fastforward stuff
	  if (m_currentSpeed != 10000)
	  {
	    m_pDsGraph.DoFFRW(m_currentSpeed);
      Sleep(100);
	  }
	  else if (m_currentSpeed == 0)
      Sleep(250);
    else
    {
	    Sleep(250);
	    m_pDsGraph.UpdateTime();
	  }
  }

  m_callback.OnPlayBackEnded();
  //g_renderManager.UnInit();
  
}

void CDSPlayer::Stop()
{
  SendMessage(g_hWnd,WM_COMMAND, ID_STOP_DSPLAYER,0);
}

void CDSPlayer::Pause()
{
  
  if ( m_pDsGraph.IsPaused() )
  {
    m_currentSpeed = 10000;
    m_callback.OnPlayBackResumed();
    
  } 
  else
  {
    m_currentSpeed = 0;
    m_callback.OnPlayBackPaused();
  }
  SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PAUSE,0);
}
void CDSPlayer::ToFFRW(int iSpeed)
{
  if (iSpeed != 1)
    g_infoManager.SetDisplayAfterSeek();
  switch(iSpeed)
  {
    case -1:
      m_currentRate = -1;
      m_currentSpeed=-10000;
      break;
    case -2:
      m_currentRate = -2;
      m_currentSpeed = -15000;
      break;
    case -4:
      m_currentRate = -4;
      m_currentSpeed = -30000;
      break;
    case -8:
      m_currentRate = -8;
      m_currentSpeed = -45000;
      break;
    case -16:
      m_currentRate = -16;
      m_currentSpeed = -60000;
      break;
    case -32:
      m_currentRate = -32;
      m_currentSpeed = -75000;
      break;
    case 1:
      m_currentRate = 1;
      m_currentSpeed = 10000;
      SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PLAY,0);
	  //mediaCtrl.Run();
      break;
    case 2:
      m_currentRate = 2;
      m_currentSpeed = 15000;
      break;
    case 4:
      m_currentRate = 4;
      m_currentSpeed = 30000;
      break;
    case 8:
      m_currentRate = 8;
      m_currentSpeed = 45000;
      break;
    case 16:
      m_currentRate = 16;
      m_currentSpeed = 60000;
      break;
    default:
      m_currentRate = 32;
      m_currentSpeed = 75000;
      break;
  }
  //SendMessage(g_hWnd,WM_COMMAND, ID_SET_SPEEDRATE,iSpeed);
}

void CDSPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bPlus)
  {
    if (!bLargeStep)
      SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_FORWARDSMALL,0);
	else
	  SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_FORWARDLARGE,0);
  }
  else
  {
    if (!bLargeStep)
      SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_BACKWARDSMALL,0);
	else
	  SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_BACKWARDLARGE,0);
  }
}

void CDSPlayer::SeekPercentage(float iPercent)
{
  SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_PERCENT,(LPARAM)iPercent);
}

bool CDSPlayer::OnAction(const CAction &action)
{
#define THREAD_ACTION(action) \
  do { \
    if(GetCurrentThreadId() != CThread::ThreadId()) { \
      m_messenger.Put(new CDVDMsgType<CAction>(CDVDMsg::GENERAL_GUI_ACTION, action)); \
      return true; \
    } \
  } while(false)

  switch(action.id)
  {
    case ACTION_NEXT_ITEM:
      Stop();
	  break;
  }
	
  // return false to inform the caller we didn't handle the message
  return false;
	
}




