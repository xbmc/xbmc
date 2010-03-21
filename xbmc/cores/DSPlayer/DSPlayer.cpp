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
#include "MouseStat.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "RegExp.h"
#include "URL.h"

#include "dshowutil/dshowutil.h" // unload loaded filters

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "GUIDialogBusy.h"

using namespace std;

DSPLAYER_STATE CDSPlayer::PlayerState = DSPLAYER_CLOSED;
CFileItem CDSPlayer::currentFileItem;

CDSPlayer::CDSPlayer(IPlayerCallback& callback)
    : IPlayer(callback), CThread(), m_pDsGraph(&m_pDsClock),
      m_hReadyEvent(true)
{
}

CDSPlayer::~CDSPlayer()
{
  if (PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  StopThread();

  DShowUtil::UnloadExternalObjects();
  CLog::Log(LOGDEBUG, "%s External objects unloaded", __FUNCTION__);
}

bool CDSPlayer::OpenFile(const CFileItem& file,const CPlayerOptions &options)
{
  if(PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  PlayerState = DSPLAYER_LOADING;

  currentFileItem = file;
  m_Filename = file.GetAsUrl();
  m_PlayerOptions = options;
  m_currentSpeed = 10000;
  m_currentRate = 1;
  
  m_hReadyEvent.Reset();
  Create();
  
  if(!m_hReadyEvent.WaitMSec(100))
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    dialog->Show();
    while(!m_hReadyEvent.WaitMSec(1))
      g_windowManager.Process(true);
    dialog->Close();
  }

  return true;
}
bool CDSPlayer::CloseFile()
{
  if (PlayerState == DSPLAYER_CLOSED)
    return true;

  PlayerState = DSPLAYER_CLOSING;

  m_pDsGraph.CloseFile();
  
  CLog::Log(LOGNOTICE, "%s DSPlayer is now closed", __FUNCTION__);
  
  PlayerState = DSPLAYER_CLOSED;

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
  CSingleLock lock(m_StateSection);
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
}

void CDSPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  CSingleLock lock(m_StateSection);
  strGeneralInfo = m_pDsGraph.GetGeneralInfo();
}

//CThread
void CDSPlayer::OnStartup()
{
  CThread::SetName("CDSPlayer");
}

void CDSPlayer::OnExit()
{
  /*try
  {
    m_pDsGraph.CloseFile();
   
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close the graph", __FUNCTION__);
  }*/

  if (PlayerState == DSPLAYER_CLOSING)
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();

  m_bStop = true;
}

void CDSPlayer::HandleStart()
{
  if (m_PlayerOptions.starttime>0)
    SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_TO ,((LPARAM)m_PlayerOptions.starttime * 1000 ));
  //SendMessage(g_hWnd,WM_COMMAND, ID_DS_HIDE_SUB ,0);
  //In case ffdshow has the subtitles filter enabled
  
  // That's done by XBMC when starting playing
  /*if ( CStreamsManager::getSingleton()->GetSubtitleCount() == 0 )
    CStreamsManager::getSingleton()->SetSubtitleVisible(false);
  else
  {
    //If there more than one we will load the first one in the list
    // CStreamsManager::getSingleton()->SetSubtitle(0); // No Need, already connected on graph setup
    CStreamsManager::getSingleton()->SetSubtitleVisible(true);
  }*/
}

void CDSPlayer::Process()
{

#define CHECK_PLAYER_STATE if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED) break;

  /* INIT: Load file */
  HRESULT hr = S_OK;
  START_PERFORMANCE_COUNTER
  hr = m_pDsGraph.SetFile(currentFileItem, m_PlayerOptions);
  END_PERFORMANCE_COUNTER

  if ( FAILED(hr) )
  {
    CLog::Log(LOGERROR,"%s failed to start this file with dsplayer %s", __FUNCTION__, currentFileItem.GetAsUrl().GetFileName().c_str());
    CloseFile();
    return;
  }

  m_callback.OnPlayBackStarted();
  bool pStartPosDone = false;

  m_hReadyEvent.Set(); // We're ready to go!
  m_pDsClock.SetSpeed(1000);

  while (PlayerState != DSPLAYER_CLOSING && PlayerState != DSPLAYER_CLOSED)
  {
    CHECK_PLAYER_STATE

    //The graph need to be started to handle those stuff
    if (!pStartPosDone)
    {
      HandleStart();
      pStartPosDone = true;
    }

    CHECK_PLAYER_STATE

    m_pDsGraph.HandleGraphEvent();

    CHECK_PLAYER_STATE
    
    //Handle fastforward stuff
    if (m_currentSpeed == 0)
    {
      m_pDsClock.SetSpeed(0);
      Sleep(250);
    } 
    else if (m_currentSpeed != 10000)
    {
      //m_pDsClock.SetSpeed(m_currentRate * 1000);
      m_pDsGraph.DoFFRW(m_currentRate);
      Sleep(250);
    } 
    else
    {
      Sleep(250);
      m_pDsGraph.UpdateTime();
      CChaptersManager::getSingleton()->UpdateChapters();
    }

    CHECK_PLAYER_STATE

    if (m_pDsGraph.FileReachedEnd())
    { 
      CLog::Log(LOGDEBUG,"%s Graph detected end of video file",__FUNCTION__);
      CloseFile();
      break;
    }
  }  
}

void CDSPlayer::Stop()
{
  SendMessage(g_hWnd,WM_COMMAND, ID_STOP_DSPLAYER,0);
}

void CDSPlayer::Pause()
{
  
  if ( PlayerState == DSPLAYER_PAUSED )
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
      m_currentSpeed = -10000;
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
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDSMALL,0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDLARGE,0);
  }
  else
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDSMALL,0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDLARGE,0);
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

  if ( m_pDsGraph.IsDvd() )
  {
    if ( action.GetID() == ACTION_SHOW_VIDEOMENU )
    {
      SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_ROOT,0);
      return true;
    }
    if ( m_pDsGraph.IsInMenu() )
    {
      switch (action.GetID())
      {
        case ACTION_PREVIOUS_MENU:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_BACK,0);
        break;
        case ACTION_MOVE_LEFT:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_LEFT,0);
        break;
        case ACTION_MOVE_RIGHT:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_RIGHT,0);
        break;
        case ACTION_MOVE_UP:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_UP,0);
        break;
        case ACTION_MOVE_DOWN:
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_NAV_DOWN,0);
        break;

      /*case ACTION_MOUSE:
        {
          // check the action
          CAction action2 = action;
          action2.buttonCode = g_Mouse.bClick[MOUSE_LEFT_BUTTON] ? 1 : 0;
          action2.amount1 = g_Mouse.GetLocation().x;
          action2.amount2 = g_Mouse.GetLocation().y;

          CRect rs, rd;
          GetVideoRect(rs, rd);
          if (action2.amount1 < rd.x1 || action2.amount1 > rd.x2 ||
              action2.amount2 < rd.y1 || action2.amount2 > rd.y2)
            return false; // out of bounds
          //THREAD_ACTION(action2);
          // convert to video coords...
          CPoint pt(action2.amount1, action2.amount2);
          pt -= CPoint(rd.x1, rd.y1);
          pt.x *= rs.Width() / rd.Width();
          pt.y *= rs.Height() / rd.Height();
          pt += CPoint(rs.x1, rs.y1);
          LPARAM ptparam;
          ptparam = g_geometryHelper.ConvertPointToLParam(pt.x,pt.y);
          if (action2.buttonCode)
            SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_CLICK,ptparam);
          //return m_pDsGraph.OnMouseClick(pt);
            
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_MOVE,ptparam);
          return true;
        }
        break;*/
      case ACTION_SELECT_ITEM:
        {
          // show button pushed overlay
          SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MENU_SELECT,0);
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
      {
        // Offset from key codes back to button number
        // int button = action.actionId - REMOTE_0;
        //CLog::Log(LOGDEBUG, " - button pressed %d", button);
        //pStream->SelectButton(button);
      }
      break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }

  switch(action.GetID())
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PAGE_UP:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() + 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
    case ACTION_PAGE_DOWN:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() - 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }
  
  // return false to inform the caller we didn't handle the message
  return false;
}