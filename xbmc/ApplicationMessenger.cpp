/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "ApplicationMessenger.h"
#include "Application.h"

#include "TextureManager.h"
#include "PlayListPlayer.h"
#include "Util.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "GUIWindowSlideShow.h"
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#endif
#include "utils/Network.h"
#include "GUIWindowManager.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "FileItem.h"
#ifdef HAS_HAL
#include "linux/HalManager.h"
#elif defined _WIN32PC
#include "WIN32Util.h"
#define CHalManager CWIN32Util
#elif defined __APPLE__
#include "CocoaUtils.h"
#endif

using namespace std;

extern HWND g_hWnd;

CApplicationMessenger::~CApplicationMessenger()
{
  Cleanup();
}

void CApplicationMessenger::Cleanup()
{
  while (m_vecMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecMessages.front();
    delete pMsg;
    m_vecMessages.pop();
  }

  while (m_vecWindowMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();
    delete pMsg;
    m_vecWindowMessages.pop();
  }
}

void CApplicationMessenger::SendMessage(ThreadMessage& message, bool wait)
{
  message.hWaitEvent = NULL;
  if (wait)
  { // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (GetCurrentThreadId() != g_application.GetThreadId())
      message.hWaitEvent = CreateEvent(NULL, true, false, NULL);
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return;
    }
  }

  ThreadMessage* msg = new ThreadMessage();
  msg->dwMessage = message.dwMessage;
  msg->dwParam1 = message.dwParam1;
  msg->dwParam2 = message.dwParam2;
  msg->hWaitEvent = message.hWaitEvent;
  msg->lpVoid = message.lpVoid;
  msg->strParam = message.strParam;

  CSingleLock lock (m_critSection);
  if (msg->dwMessage == TMSG_DIALOG_DOMODAL ||
      msg->dwMessage == TMSG_WRITE_SCRIPT_OUTPUT)
    m_vecWindowMessages.push(msg);
  else
    m_vecMessages.push(msg);
  lock.Leave();

  if (message.hWaitEvent)
  {
    WaitForSingleObject(message.hWaitEvent, INFINITE);
    CloseHandle(message.hWaitEvent);
    message.hWaitEvent = NULL;
  }
}

void CApplicationMessenger::ProcessMessages()
{
  // process threadmessages
  CSingleLock lock (m_critSection);
  while (m_vecMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecMessages.pop();

    //Leave here as the message might make another
    //thread call processmessages or sendmessage
    lock.Leave();

    ProcessMessage(pMsg);
    if (pMsg->hWaitEvent)
      SetEvent(pMsg->hWaitEvent);
    delete pMsg;

    lock.Enter();
  }
}

void CApplicationMessenger::ProcessMessage(ThreadMessage *pMsg)
{
  switch (pMsg->dwMessage)
  {
    case TMSG_SHUTDOWN:
      {
        switch (g_guiSettings.GetInt("system.shutdownstate"))
        {
          case POWERSTATE_SHUTDOWN:
            Powerdown();
            break;

          case POWERSTATE_SUSPEND:
            Suspend();
            break;

          case POWERSTATE_HIBERNATE:
            Hibernate();
            break;

          case POWERSTATE_QUIT:
            Quit();
            break;

          case POWERSTATE_MINIMIZE:
            Minimize();
            break;
        }
      }
      break;

case TMSG_POWERDOWN:
      {
#ifndef HAS_SDL
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
#ifdef HAS_HAL
        if (CHalManager::PowerManagement(POWERSTATE_SHUTDOWN))
#elif defined(_WIN32PC)
        if (CWIN32Util::PowerManagement(POWERSTATE_SHUTDOWN))
#endif
        {
          g_application.Stop();
          exit(64);
        }
      }
      break;

    case TMSG_QUIT:
      {
        g_application.Stop();
        exit(0);
      }
      break;

    case TMSG_HIBERNATE:
      {
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_HIBERNATE);
#elif defined(_WIN32PC)
        CWIN32Util::PowerManagement(POWERSTATE_HIBERNATE);
#elif defined __APPLE__
        Cocoa_SleepSystem();
#endif
      }
      break;

    case TMSG_SUSPEND:
      {
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_SUSPEND);
#elif defined(_WIN32PC)
        CWIN32Util::PowerManagement(POWERSTATE_SUSPEND);
#elif defined(__APPLE__)
        Cocoa_SleepSystem();
#endif
      }
      break;

    case TMSG_RESTART:
      {
        g_application.Stop();
        Sleep(200);
#if !defined(_LINUX)
#ifndef HAS_SDL
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
#ifdef _WIN32PC
        CWIN32Util::PowerManagement(POWERSTATE_REBOOT);
#endif
#else
        // exit the application
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_REBOOT);
#endif
        exit(66);
#endif
      }
      break;

    case TMSG_RESET:
      {
        g_application.Stop();
        Sleep(200);
#if !defined(_LINUX)
#ifndef HAS_SDL
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
#ifdef _WIN32PC
        CWIN32Util::PowerManagement(POWERSTATE_REBOOT);
#endif
#else
        // exit the application
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_REBOOT);
#endif
        exit(66);
#endif
      }
      break;

    case TMSG_RESTARTAPP:
      {
#ifdef _WIN32PC
        g_application.Stop();
        Sleep(200);
#endif
        exit(65);
        // TODO
        //char szXBEFileName[1024];

        //CIoSupport::GetXbePath(szXBEFileName);
        //CUtil::RunXBE(szXBEFileName);
      }
      break;

    case TMSG_MEDIA_PLAY:
      {
        // first check if we were called from the PlayFile() function
        if (pMsg->lpVoid)
        {
          CFileItem *item = (CFileItem *)pMsg->lpVoid;
          g_application.PlayFile(*item, pMsg->dwParam1 != 0);
          delete item;
          return;
        }
        // restore to previous window if needed
        if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          m_gWindowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        //g_application.StopPlaying();
        // play file
        CFileItem item(pMsg->strParam, false);
        if (item.IsAudio())
          item.SetMusicThumb();
        else
          item.SetVideoThumb();
        item.FillInDefaultIcon();
        g_application.PlayMedia(item, item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO); //Note: this will play playlists always in the temp music playlist (default 2nd parameter), maybe needs some tweaking.
      }
      break;

    case TMSG_MEDIA_RESTART:
      g_application.Restart(true);
      break;

    case TMSG_PICTURE_SHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        // stop playing file
        if (g_application.IsPlayingVideo()) g_application.StopPlaying();

        if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
          m_gWindowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        g_graphicsContext.Lock();
        pSlideShow->Reset();
        if (m_gWindowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
          m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
        if (CUtil::IsZIP(pMsg->strParam) || CUtil::IsRAR(pMsg->strParam)) // actually a cbz/cbr
        {
          CFileItemList items;
          CStdString strPath;
          if (CUtil::IsZIP(pMsg->strParam))
            CUtil::CreateArchivePath(strPath, "zip", pMsg->strParam.c_str(), "");
          else
            CUtil::CreateArchivePath(strPath, "rar", pMsg->strParam.c_str(), "");

          CUtil::GetRecursiveListing(strPath, items, g_stSettings.m_pictureExtensions);
          if (items.Size() > 0)
          {
            for (int i=0;i<items.Size();++i)
            {
              pSlideShow->Add(items[i].get());
            }
            pSlideShow->Select(items[0]->m_strPath);
          }
        }
        else
        {
          CFileItem item(pMsg->strParam, false);
          pSlideShow->Add(&item);
          pSlideShow->Select(pMsg->strParam);
        }
        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_SLIDESHOW_SCREENSAVER:
    case TMSG_PICTURE_SLIDESHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        g_graphicsContext.Lock();
        pSlideShow->Reset();

        CFileItemList items;
        CStdString strPath = pMsg->strParam;
        if (pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER &&
            g_guiSettings.GetString("screensaver.mode").Equals("Fanart Slideshow"))
        {
          CUtil::GetRecursiveListing(g_settings.GetVideoFanartFolder(), items, ".tbn");
          CUtil::GetRecursiveListing(g_settings.GetMusicFanartFolder(), items, ".tbn");
        }
        else
          CUtil::GetRecursiveListing(strPath, items, g_stSettings.m_pictureExtensions);

        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i].get());
          pSlideShow->StartSlideShow(pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER); //Start the slideshow!
        }
        if (pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER && g_guiSettings.GetBool("screensaver.slideshowshuffle"))
          pSlideShow->Shuffle();

        if (m_gWindowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
          m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);

        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_MEDIA_STOP:
      {
        // restore to previous window if needed
        if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          m_gWindowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        // stop playing file
        if (g_application.IsPlaying()) g_application.StopPlaying();
      }
      break;

    case TMSG_MEDIA_PAUSE:
      if (g_application.m_pPlayer)
      {
        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_SWITCHTOFULLSCREEN:
      if( m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
        g_application.SwitchToFullScreen();
      break;

    case TMSG_MINIMIZE:
      g_application.Minimize();
      break;

    case TMSG_HTTPAPI:
    {
#ifdef HAS_WEB_SERVER
      if (!m_pXbmcHttp)
      {
        CSectionLoader::Load("LIBHTTP");
        m_pXbmcHttp = new CXbmcHttp();
      }
      switch (m_pXbmcHttp->xbmcCommand(pMsg->strParam))
      {
        case 1:
          g_application.getApplicationMessenger().Restart();
          break;

        case 2:
          g_application.getApplicationMessenger().Shutdown();
          break;

        case 3:
          g_application.getApplicationMessenger().RebootToDashBoard();
          break;

        case 4:
          g_application.getApplicationMessenger().Reset();
          break;

        case 5:
          g_application.getApplicationMessenger().RestartApp();
          break;
      }
#endif
    }
    break;

    case TMSG_EXECUTE_SCRIPT:
#ifdef HAS_PYTHON
      g_pythonParser.evalFile(pMsg->strParam.c_str());
#endif
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CUtil::ExecBuiltIn(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.Play(pMsg->dwParam1);
      else
        g_playlistPlayer.Play();

      break;

    case TMSG_PLAYLISTPLAYER_NEXT:
      g_playlistPlayer.PlayNext();
      break;

    case TMSG_PLAYLISTPLAYER_PREV:
      g_playlistPlayer.PlayPrevious();
      break;

    // Window messages below here...
    case TMSG_DIALOG_DOMODAL:  //doModel of window
      {
        CGUIDialog* pDialog = (CGUIDialog*)m_gWindowManager.GetWindow(pMsg->dwParam1);
        if (!pDialog) return ;
        pDialog->DoModal();
      }
      break;

    case TMSG_WRITE_SCRIPT_OUTPUT:
      {
        //send message to window 2004 (CGUIWindowScriptsInfo)
        CGUIMessage msg(GUI_MSG_USER, 0, 0);
        msg.SetLabel(pMsg->strParam);
        CGUIWindow* pWindowScripts = m_gWindowManager.GetWindow(WINDOW_SCRIPTS_INFO);
        if (pWindowScripts) pWindowScripts->OnMessage(msg);
      }
      break;

    case TMSG_NETWORKMESSAGE:
      {
        g_application.getNetwork().NetworkMessage((CNetwork::EMESSAGE)pMsg->dwParam1, pMsg->dwParam2);
      }
      break;

    case TMSG_GUI_DO_MODAL:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->DoModal_Internal((int)pMsg->dwParam1, pMsg->strParam);
      }
      break;

    case TMSG_GUI_SHOW:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->Show_Internal();
      }
      break;

    case TMSG_GUI_ACTIVATE_WINDOW:
      {
        m_gWindowManager.ActivateWindow(pMsg->dwParam1, pMsg->strParam, pMsg->dwParam2 > 0);
      }
      break;

    case TMSG_GUI_WIN_MANAGER_PROCESS:
      m_gWindowManager.Process_Internal(0 != pMsg->dwParam1);
      break;

    case TMSG_GUI_WIN_MANAGER_RENDER:
      m_gWindowManager.Render_Internal();
      break;
  }
}

void CApplicationMessenger::ProcessWindowMessages()
{
  CSingleLock lock (m_critSection);
  //message type is window, process window messages
  while (m_vecWindowMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecWindowMessages.pop();

    // leave here in case we make more thread messages from this one
    lock.Leave();

    ProcessMessage(pMsg);
    if (pMsg->hWaitEvent)
      SetEvent(pMsg->hWaitEvent);
    delete pMsg;

    lock.Enter();
  }
}

int CApplicationMessenger::SetResponse(CStdString response)
{
  CSingleLock lock (m_critBuffer);
  bufferResponse=response;
  lock.Leave();
  return 0;
}

CStdString CApplicationMessenger::GetResponse()
{
  CStdString tmp;
  CSingleLock lock (m_critBuffer);
  tmp=bufferResponse;
  lock.Leave();
  return tmp;
}

void CApplicationMessenger::HttpApi(string cmd, bool wait)
{
  SetResponse("");
  ThreadMessage tMsg = {TMSG_HTTPAPI};
  tMsg.strParam = cmd;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::ExecBuiltIn(const CStdString &command)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
  tMsg.strParam = command;
  SendMessage(tMsg);
}

void CApplicationMessenger::MediaPlay(string filename)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  tMsg.strParam = filename;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayFile(const CFileItem &item, bool bRestart /*= false*/)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItem *pItem = new CFileItem(item);
  tMsg.lpVoid = (void *)pItem;
  tMsg.dwParam1 = bRestart ? 1 : 0;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::MediaStop()
{
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaPause()
{
  ThreadMessage tMsg = {TMSG_MEDIA_PAUSE};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaRestart(bool bWait)
{
  ThreadMessage tMsg = {TMSG_MEDIA_RESTART};
  SendMessage(tMsg, bWait);
}

void CApplicationMessenger::PlayListPlayerPlay()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPlay(int iSong)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, iSong};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerNext()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_NEXT};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPrevious()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PREV};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PictureShow(string filename)
{
  ThreadMessage tMsg = {TMSG_PICTURE_SHOW};
  tMsg.strParam = filename;
  SendMessage(tMsg);
}

void CApplicationMessenger::PictureSlideShow(string pathname, bool bScreensaver /* = false */)
{
  DWORD dwMessage = TMSG_PICTURE_SLIDESHOW;
  if (bScreensaver)
    dwMessage = TMSG_SLIDESHOW_SCREENSAVER;
  ThreadMessage tMsg = {dwMessage};
  tMsg.strParam = pathname;
  SendMessage(tMsg);
}

void CApplicationMessenger::Shutdown()
{
  ThreadMessage tMsg = {TMSG_SHUTDOWN};
  SendMessage(tMsg);
}

void CApplicationMessenger::Powerdown()
{
  ThreadMessage tMsg = {TMSG_POWERDOWN};
  SendMessage(tMsg);
}

void CApplicationMessenger::Quit()
{
  ThreadMessage tMsg = {TMSG_QUIT};
  SendMessage(tMsg);
}

void CApplicationMessenger::Hibernate()
{
  ThreadMessage tMsg = {TMSG_HIBERNATE};
  SendMessage(tMsg);
}

void CApplicationMessenger::Suspend()
{
  ThreadMessage tMsg = {TMSG_SUSPEND};
  SendMessage(tMsg);
}

void CApplicationMessenger::Restart()
{
  ThreadMessage tMsg = {TMSG_RESTART};
  SendMessage(tMsg);
}

void CApplicationMessenger::Reset()
{
  ThreadMessage tMsg = {TMSG_RESET};
  SendMessage(tMsg);
}

void CApplicationMessenger::RestartApp()
{
  ThreadMessage tMsg = {TMSG_RESTARTAPP};
  SendMessage(tMsg);
}

void CApplicationMessenger::RebootToDashBoard()
{
  ThreadMessage tMsg = {TMSG_DASHBOARD};
  SendMessage(tMsg);
}

void CApplicationMessenger::NetworkMessage(DWORD dwMessage, DWORD dwParam)
{
  ThreadMessage tMsg = {TMSG_NETWORKMESSAGE, dwMessage, dwParam};
  SendMessage(tMsg);
}

void CApplicationMessenger::SwitchToFullscreen()
{
  /* FIXME: ideally this call should return upon a successfull switch but currently
     is causing deadlocks between the dvdplayer destructor and the rendermanager
  */
  ThreadMessage tMsg = {TMSG_SWITCHTOFULLSCREEN};
  SendMessage(tMsg, false);
}

void CApplicationMessenger::Minimize()
{
  ThreadMessage tMsg = {TMSG_MINIMIZE};
  SendMessage(tMsg, false);
}

void CApplicationMessenger::DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param)
{
  ThreadMessage tMsg = {TMSG_GUI_DO_MODAL};
  tMsg.lpVoid = pDialog;
  tMsg.dwParam1 = (DWORD)iWindowID;
  tMsg.strParam = param;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::Show(CGUIDialog *pDialog)
{
  ThreadMessage tMsg = {TMSG_GUI_SHOW};
  tMsg.lpVoid = pDialog;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::ActivateWindow(int windowID, const CStdString &path, bool swappingWindows)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTIVATE_WINDOW, windowID, swappingWindows ? 1 : 0};
  tMsg.strParam = path;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::WindowManagerProcess(bool renderOnly)
{
  ThreadMessage tMsg = {TMSG_GUI_WIN_MANAGER_PROCESS};
  tMsg.dwParam1 = (DWORD)renderOnly;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::Render()
{
  ThreadMessage tMsg = {TMSG_GUI_WIN_MANAGER_RENDER};
  SendMessage(tMsg, true);
}
