/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "ApplicationMessenger.h"
#include "application.h"
#ifdef _XBOX
#include "xbox/xkutils.h"
#include "xbox/XKHDD.h"
#endif

#include "texturemanager.h"
#include "playlistplayer.h"
#include "util.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowSlideShow.h"
#include "lib/libGoAhead/xbmchttp.h"
#include "xbox/network.h"

extern HWND g_hWnd;

CApplicationMessenger g_applicationMessenger;

void CApplicationMessenger::Cleanup()
{
  vector<ThreadMessage*>::iterator it = m_vecMessages.begin();
  while (it != m_vecMessages.end())
  {
    ThreadMessage* pMsg = *it;
    delete pMsg;
    it = m_vecMessages.erase(it);
  }

  it = m_vecWindowMessages.begin();
  while (it != m_vecWindowMessages.end())
  {
    ThreadMessage* pMsg = *it;
    delete pMsg;
    it = m_vecWindowMessages.erase(it);
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
  {
    m_vecWindowMessages.push_back(msg);
  }
  else m_vecMessages.push_back(msg);
  lock.Leave();

  if (message.hWaitEvent)
  {
    WaitForSingleObject(message.hWaitEvent, INFINITE);
    CloseHandle(message.hWaitEvent);
  }
}

void CApplicationMessenger::ProcessMessages()
{
  // process threadmessages
  CSingleLock lock (m_critSection);
  while (m_vecMessages.size() > 0)
  {
    vector<ThreadMessage*>::iterator it = m_vecMessages.begin();

    ThreadMessage* pMsg = *it;
    //first remove the message from the queue, else the message could be processed more then once
    it = m_vecMessages.erase(it);

    //Leave here as the message might make another
    //thread call processmessages or sendmessage
    lock.Leave();

    ProcessMessage(pMsg);

    if (pMsg->hWaitEvent)
      SetEvent(pMsg->hWaitEvent);

    delete pMsg;

    //Reenter here again, to not ruin message vector
    lock.Enter();
  }
}

void CApplicationMessenger::ProcessMessage(ThreadMessage *pMsg)
{
  switch (pMsg->dwMessage)
  {
    case TMSG_SHUTDOWN:
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKHDD::SpindownHarddisk(); // Spindown the Harddisk
        XKUtils::XBOXPowerOff();
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

    case TMSG_DASHBOARD:
      {
        CUtil::ExecBuiltIn("XBMC.Dashboard()");
      }
      break;

    case TMSG_RESTART:
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKUtils::XBOXPowerCycle();
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

    case TMSG_RESET:
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKUtils::XBOXReset();
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

    case TMSG_RESTARTAPP:
      {
        char szXBEFileName[1024];

        CIoSupport::GetXbePath(szXBEFileName);
        CUtil::RunXBE(szXBEFileName);
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
            CUtil::CreateZipPath(strPath, pMsg->strParam.c_str(), "", 2, "", "Z:\\");
          else
            CUtil::CreateRarPath(strPath, pMsg->strParam.c_str(), "", 2, "", "Z:\\");

          CUtil::GetRecursiveListing(strPath, items, g_stSettings.m_pictureExtensions);
          if (items.Size() > 0)
          {
            for (int i=0;i<items.Size();++i)
              pSlideShow->Add(items[i]);
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
        CUtil::GetRecursiveListing(strPath, items, g_stSettings.m_pictureExtensions);
        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i]);
          pSlideShow->StartSlideShow(); //Start the slideshow!
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

    case TMSG_HTTPAPI:
	{
      if (!m_pXbmcHttp)
      {
	    CSectionLoader::Load("LIBHTTP");
        m_pXbmcHttp = new CXbmcHttp();
      }
	  int ret=m_pXbmcHttp->xbmcCommand(pMsg->strParam);
      switch(ret)
      {
      case 1:
        g_applicationMessenger.Restart();
        break;
      case 2:
        g_applicationMessenger.Shutdown();
        break;
      case 3:
        g_applicationMessenger.RebootToDashBoard();
        break;
      case 4:
        g_applicationMessenger.Reset();
        break;
      case 5:
        g_applicationMessenger.RestartApp();
        break;
      }
      break;
	}
    case TMSG_EXECUTE_SCRIPT:
      g_pythonParser.evalFile(pMsg->strParam.c_str());
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CUtil::ExecBuiltIn(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      if (pMsg->dwParam1 != -1)
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
        g_network.NetworkMessage((CNetwork::EMESSAGE)pMsg->dwParam1, pMsg->dwParam2);
      }
      break;
  }
}

void CApplicationMessenger::ProcessWindowMessages()
{
  CSingleLock lock (m_critSection);
  //message type is window, process window messages
  if (m_vecWindowMessages.size() > 0)
  {
    vector<ThreadMessage*>::iterator it = m_vecWindowMessages.begin();
    while (it != m_vecWindowMessages.end())
    {
      ThreadMessage* pMsg = *it;
      //first remove the message from the queue, else the message could be processed more then once
      it = m_vecWindowMessages.erase(it);

      // leave here in case we make more thread messages from this one
      lock.Leave();
      ProcessMessage(pMsg);
      if (pMsg->hWaitEvent)
        SetEvent(pMsg->hWaitEvent);

      delete pMsg;
      // reenter
      lock.Enter();
    }
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

void CApplicationMessenger::HttpApi(string cmd)
{
  SetResponse("");
  ThreadMessage tMsg = {TMSG_HTTPAPI};
  tMsg.strParam = cmd;
  SendMessage(tMsg, true);
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
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, -1};
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
  ThreadMessage tMsg = {TMSG_SWITCHTOFULLSCREEN};
  SendMessage(tMsg, true);
}