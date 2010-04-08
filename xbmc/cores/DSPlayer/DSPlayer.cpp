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
#include "GUIWindowManager.h"
#include "GUIDialogBusy.h"
#include "DShowUtil/smartptr.h"
#include "WindowingFactory.h"

using namespace std;

DSPLAYER_STATE CDSPlayer::PlayerState = DSPLAYER_CLOSED;
CFileItem CDSPlayer::currentFileItem;

CDSPlayer::CDSPlayer(IPlayerCallback& callback)
    : IPlayer(callback), CThread(), m_pDsGraph(&m_pDsClock, callback),
      m_hReadyEvent(true), m_bSpeedChanged(false), m_callSetFileFromThread(true)
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
  m_currentRate = 1;
  
  m_hReadyEvent.Reset();

  if ( g_Windowing.IsFullScreen() && 
       !g_guiSettings.GetBool("videoscreen.fakefullscreen") &&
       !g_sysinfo.IsVistaOrHigher() )
  {
    // Using VMR in true fullscreen. Calling SetFile() in Process makes XBMC freeze
    m_callSetFileFromThread = false;
    START_PERFORMANCE_COUNTER
      if (FAILED(m_pDsGraph.SetFile(currentFileItem, m_PlayerOptions)))
        PlayerState = DSPLAYER_ERROR;
    END_PERFORMANCE_COUNTER
  }
  Create();

  /* Show busy dialog while SetFile() not returned */
  if(!m_hReadyEvent.WaitMSec(100))
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    dialog->Show();
    while(!m_hReadyEvent.WaitMSec(1))
      g_windowManager.Process(true);
    dialog->Close();
  }

  // Starts playback
  if (PlayerState != DSPLAYER_ERROR)
  {
    m_pDsGraph.Play();
    if (CFGLoader::Filters.isDVD)
      CStreamsManager::getSingleton()->LoadDVDStreams();
  }

  return (PlayerState != DSPLAYER_ERROR);
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
}

void CDSPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  CSingleLock lock(m_StateSection);
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
}

void CDSPlayer::Process()
{

#define CHECK_PLAYER_STATE if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED) break;

  if (m_callSetFileFromThread)
  {
    START_PERFORMANCE_COUNTER
    if (FAILED(m_pDsGraph.SetFile(currentFileItem, m_PlayerOptions)))
      PlayerState = DSPLAYER_ERROR;
    END_PERFORMANCE_COUNTER
  }

  m_hReadyEvent.Set(); // Start playback

  if (PlayerState == DSPLAYER_ERROR)
    return;

  HRESULT hr = S_OK;
  bool pStartPosDone = false;
  int sleepTime;

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
    
    if (m_bSpeedChanged)
    {
      m_pDsClock.SetSpeed(m_currentRate * 1000);
      
    }
    m_pDsGraph.UpdateTime();

    CHECK_PLAYER_STATE

    sleepTime = m_pDsGraph.DoFFRW(m_currentRate);
    if ((m_currentRate == 0 ) || ( m_currentRate == 1 ))
      sleepTime = 250;
    else
    {
      //This way the time it wasted to seek we remove it from the sleep time
      sleepTime = 250 - sleepTime;
    }

    CHECK_PLAYER_STATE

    if (m_currentRate == 1)
    {
      CChaptersManager::getSingleton()->UpdateChapters();
      CStreamsManager::getSingleton()->UpdateDVDStream();
    }
    //Handle fastforward stuff
   
    Sleep(sleepTime);
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
  m_bSpeedChanged = true;
  if ( PlayerState == DSPLAYER_PAUSED )
  {
    m_currentRate = 1;
    m_callback.OnPlayBackResumed();    
  } 
  else
  {
    m_currentRate = 0;
    m_callback.OnPlayBackPaused();
  }

  SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PAUSE,0);
}
void CDSPlayer::ToFFRW(int iSpeed)
{
  m_bSpeedChanged = true;
  if (iSpeed != 1)
    g_infoManager.SetDisplayAfterSeek();
  m_currentRate = iSpeed;
  if (iSpeed == 1)
    SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PLAY,0);

}

void CDSPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bPlus)
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDSMALL, 0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDLARGE, 0);
  }
  else
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDSMALL, 0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDLARGE, 0);
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
        /*case ACTION_MOUSE_MOVE:
        case ACTION_MOUSE_LEFT_CLICK:
        {
          CRect rs, rd;
          GetVideoRect(rs, rd);
          CPoint pt(action.GetAmount(), action.GetAmount(1));
          if (!rd.PtInRect(pt))
            return false;
          pt -= CPoint(rd.x1, rd.y1);
          pt.x *= rs.Width() / rd.Width();
          pt.y *= rs.Height() / rd.Height();
          pt += CPoint(rs.x1, rs.y1);
          if (action.GetID() == ACTION_MOUSE_LEFT_CLICK)
            SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_CLICK,MAKELPARAM(pt.x,pt.y));
          else
            SendMessage(g_hWnd, WM_COMMAND, ID_DVD_MOUSE_MOVE,MAKELPARAM(pt.x,pt.y));
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