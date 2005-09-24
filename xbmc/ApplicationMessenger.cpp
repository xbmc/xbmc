
#include "stdafx.h"
#include "ApplicationMessenger.h"
#include "application.h"
#include "xbox/xkutils.h"
#include "texturemanager.h"
#include "playlistplayer.h"
#include "util.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowSlideShow.h"
#include "lib/libGoAhead/xbmchttp.h"

CApplicationMessenger g_applicationMessenger;

void CApplicationMessenger::Cleanup()
{
  vector<ThreadMessage*>::iterator it = m_vecMessages.begin();
  while (it != m_vecMessages.end())
  {
    ThreadMessage* pMsg = *it;
    if (pMsg->hWaitEvent) CloseHandle(pMsg->hWaitEvent);
    delete pMsg;
    it = m_vecMessages.erase(it);
  }

  it = m_vecWindowMessages.begin();
  while (it != m_vecWindowMessages.end())
  {
    ThreadMessage* pMsg = *it;
    if (pMsg->hWaitEvent) CloseHandle(pMsg->hWaitEvent);
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
      message.hWaitEvent = CreateEvent(NULL, false, false, "threadWaitEvent");
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
    {
      PulseEvent(pMsg->hWaitEvent);
      CloseHandle(pMsg->hWaitEvent);
    }
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
      g_application.Stop();
      Sleep(200);
      XKUtils::XBOXPowerOff();
      break;

    case TMSG_DASHBOARD:
      CUtil::ExecBuiltIn("XBMC.Dashboard()");
      break;

    case TMSG_RESTART:
      g_application.Stop();
      Sleep(200);
      XKUtils::XBOXPowerCycle();
      break;

    case TMSG_RESET:
      g_application.Stop();
      Sleep(200);
      XKUtils::XBOXReset();
      break;

    case TMSG_RESTARTAPP:
      {
        char szXBEFileName[1024];
        CIoSupport helper;
        helper.GetXbePath(szXBEFileName);
        CUtil::RunXBE(szXBEFileName);
      }
      break;

    case TMSG_MEDIA_PLAY:
      {
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
          item.SetThumb();
        item.FillInDefaultIcon();
        g_application.PlayFile(item);
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
            strPath.Format("zip://Z:\\,2,,%s,\\",pMsg->strParam.c_str());
          else
            strPath.Format("rar://Z:\\,2,,%s,\\",pMsg->strParam.c_str());
          CUtil::GetRecursiveListing(strPath,items,g_stSettings.m_szMyPicturesExtensions);
          if (items.Size() > 0)
          {
            for (int i=0;i<items.Size();++i)
              pSlideShow->Add(items[i]->m_strPath);
            pSlideShow->Select(items[0]->m_strPath);
          }
        }
        else
        {
          pSlideShow->Add(pMsg->strParam);
          pSlideShow->Select(pMsg->strParam);
        }
        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_PICTURE_SLIDESHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        g_graphicsContext.Lock();
        pSlideShow->Reset();
        if (m_gWindowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
          m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
        
        CFileItemList items;
        CStdString strPath = pMsg->strParam;
        CUtil::GetRecursiveListing(strPath,items,g_stSettings.m_szMyPicturesExtensions);
        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i]->m_strPath);
          pSlideShow->StartSlideShow(); //Start the slideshow!
        }
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

    case TMSG_HTTPAPI:
      if (!pXbmcHttp)
      {
        pXbmcHttp = new CXbmcHttp();
      }
      pXbmcHttp->xbmcCommand(pMsg->strParam);
      break;

    case TMSG_EXECUTE_SCRIPT:
      g_pythonParser.evalFile(pMsg->strParam.c_str());
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CUtil::ExecBuiltIn(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      g_playlistPlayer.Play(pMsg->dwParam1);
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
        pDialog->DoModal(pMsg->dwParam2);
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

      ProcessMessage(pMsg);
      if (pMsg->hWaitEvent)
      {
        PulseEvent(pMsg->hWaitEvent);
        CloseHandle(pMsg->hWaitEvent);
      }
      delete pMsg;
    }
  }
  lock.Leave();
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

void CApplicationMessenger::MediaPlay(string filename)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  tMsg.strParam = filename;
  SendMessage(tMsg, true);
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
void CApplicationMessenger::PictureSlideShow(string pathname)
{
  ThreadMessage tMsg = {TMSG_PICTURE_SLIDESHOW};
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
