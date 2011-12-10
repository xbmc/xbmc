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

#include "system.h"
#include "ApplicationMessenger.h"
#include "Application.h"

#include "guilib/TextureManager.h"
#include "PlayListPlayer.h"
#include "Util.h"
#include "SectionLoader.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "pictures/GUIWindowSlideShow.h"
#include "interfaces/Builtins.h"
#include "network/Network.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "guilib/GUIDialog.h"
#include "windowing/WindowingFactory.h"
#include "GUIInfoManager.h"
#include "utils/Splash.h"
#include "cores/VideoRenderers/RenderManager.h"

#include "powermanagement/PowerManager.h"

#ifdef _WIN32
#include "WIN32Util.h"
#define CHalManager CWIN32Util
#elif defined __APPLE__
#include "CocoaInterface.h"
#endif
#include "addons/AddonCallbacks.h"
#include "addons/AddonCallbacksGUI.h"
#include "storage/MediaManager.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#ifdef HAS_PYTHON
#include "interfaces/python/xbmcmodule/GUIPythonWindowDialog.h"
#include "interfaces/python/xbmcmodule/GUIPythonWindowXMLDialog.h"
#endif

#ifdef HAS_HTTPAPI
#include "interfaces/http-api/XBMChttp.h"
#endif

#include "playlists/PlayList.h"
#include "FileItem.h"

using namespace std;

CDelayedMessage::CDelayedMessage(ThreadMessage& msg, unsigned int delay)
{
  m_msg.dwMessage  = msg.dwMessage;
  m_msg.dwParam1   = msg.dwParam1;
  m_msg.dwParam2   = msg.dwParam2;
  m_msg.waitEvent  = msg.waitEvent;
  m_msg.lpVoid     = msg.lpVoid;
  m_msg.strParam   = msg.strParam;
  m_msg.params     = msg.params;

  m_delay = delay;
}

void CDelayedMessage::Process()
{
  Sleep(m_delay);

  if (!m_bStop)
    g_application.getApplicationMessenger().SendMessage(m_msg, false);
}

CApplicationMessenger::~CApplicationMessenger()
{
  Cleanup();
}

void CApplicationMessenger::Cleanup()
{
  CSingleLock lock (m_critSection);

  while (m_vecMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecMessages.pop();
  }

  while (m_vecWindowMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecWindowMessages.pop();
  }
}

void CApplicationMessenger::SendMessage(ThreadMessage& message, bool wait)
{
  message.waitEvent.reset();
  boost::shared_ptr<CEvent> waitEvent;
  if (wait)
  { // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (!g_application.IsCurrentThread())
    {
      message.waitEvent.reset(new CEvent(true));
      waitEvent = message.waitEvent;
    }
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return;
    }
  }

  CSingleLock lock (m_critSection);

  if (g_application.m_bStop)
  {
    if (message.waitEvent)
      message.waitEvent.reset();
    return;
  }

  ThreadMessage* msg = new ThreadMessage();
  msg->dwMessage = message.dwMessage;
  msg->dwParam1 = message.dwParam1;
  msg->dwParam2 = message.dwParam2;
  msg->waitEvent = message.waitEvent;
  msg->lpVoid = message.lpVoid;
  msg->strParam = message.strParam;
  msg->params = message.params;

  if (msg->dwMessage == TMSG_DIALOG_DOMODAL)
    m_vecWindowMessages.push(msg);
  else
    m_vecMessages.push(msg);
  lock.Leave();  // this releases the lock on the vec of messages and
                 //   allows the ProcessMessage to execute and therefore
                 //   delete the message itself. Therefore any accesss
                 //   of the message itself after this point consittutes
                 //   a race condition (yarc - "yet another race condition")
                 //
  if (waitEvent) // ... it just so happens we have a spare reference to the
                 //  waitEvent ... just for such contingencies :)
  { 
    // ensure the thread doesn't hold the graphics lock
    CSingleExit exit(g_graphicsContext);
    waitEvent->Wait();
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

    boost::shared_ptr<CEvent> waitEvent = pMsg->waitEvent; 
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
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
        switch (g_guiSettings.GetInt("powermanagement.shutdownstate"))
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

          case TMSG_RENDERER_FLUSH:
            g_renderManager.Flush();
            break;
        }
      }
      break;

    case TMSG_POWERDOWN:
      {
        g_application.Stop(EXITCODE_POWERDOWN);
        g_powerManager.Powerdown();
      }
      break;

    case TMSG_QUIT:
      {
        g_application.Stop(EXITCODE_QUIT);
      }
      break;

    case TMSG_HIBERNATE:
      {
        g_powerManager.Hibernate();
      }
      break;

    case TMSG_SUSPEND:
      {
        g_powerManager.Suspend();
      }
      break;

    case TMSG_RESTART:
    case TMSG_RESET:
      {
        g_application.Stop(EXITCODE_REBOOT);
        g_powerManager.Reboot();
      }
      break;

    case TMSG_RESTARTAPP:
      {
#ifdef _WIN32
        g_application.Stop(EXITCODE_RESTARTAPP);
#endif
        // TODO
      }
      break;

    case TMSG_INHIBITIDLESHUTDOWN:
      {
        g_application.InhibitIdleShutdown((bool)pMsg->dwParam1);
      }
      break;

    case TMSG_MEDIA_PLAY:
      {
        // first check if we were called from the PlayFile() function
        if (pMsg->lpVoid && pMsg->dwParam2 == 0)
        {
          CFileItem *item = (CFileItem *)pMsg->lpVoid;
          g_application.PlayFile(*item, pMsg->dwParam1 != 0);
          delete item;
          return;
        }
        // restore to previous window if needed
        if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();

        //g_application.StopPlaying();
        // play file
        if(pMsg->lpVoid)
        {
          CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

          if (list->Size() > 0)
          {
            int playlist = PLAYLIST_MUSIC;
            for (int i = 0; i < list->Size(); i++)
            {
              if ((*list)[i]->IsVideo())
              {
                playlist = PLAYLIST_VIDEO;
                break;
              }
            }

            g_playlistPlayer.ClearPlaylist(playlist);
            g_playlistPlayer.SetCurrentPlaylist(playlist);
            //For single item lists try PlayMedia. This covers some more cases where a playlist is not appropriate
            //It will fall through to PlayFile
            if (list->Size() == 1 && !(*list)[0]->IsPlayList())
              g_application.PlayMedia(*((*list)[0]), playlist);
            else
            {
              g_playlistPlayer.Add(playlist, (*list));
              g_playlistPlayer.Play(pMsg->dwParam1);
            }
          }

          delete list;
        }
        else if (pMsg->dwParam1 == PLAYLIST_MUSIC || pMsg->dwParam1 == PLAYLIST_VIDEO)
        {
          if (g_playlistPlayer.GetCurrentPlaylist() != (int)pMsg->dwParam1)
            g_playlistPlayer.SetCurrentPlaylist(pMsg->dwParam1);

          PlayListPlayerPlay(pMsg->dwParam2);
        }
      }
      break;

    case TMSG_MEDIA_RESTART:
      g_application.Restart(true);
      break;

    case TMSG_PICTURE_SHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        // stop playing file
        if (g_application.IsPlayingVideo()) g_application.StopPlaying();

        if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();

        g_graphicsContext.Lock();

        if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
          g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        if (URIUtils::IsZIP(pMsg->strParam) || URIUtils::IsRAR(pMsg->strParam)) // actually a cbz/cbr
        {
          CFileItemList items;
          CStdString strPath;
          if (URIUtils::IsZIP(pMsg->strParam))
            URIUtils::CreateArchivePath(strPath, "zip", pMsg->strParam.c_str(), "");
          else
            URIUtils::CreateArchivePath(strPath, "rar", pMsg->strParam.c_str(), "");

          CUtil::GetRecursiveListing(strPath, items, g_settings.m_pictureExtensions);
          if (items.Size() > 0)
          {
            pSlideShow->Reset();
            for (int i=0;i<items.Size();++i)
            {
              pSlideShow->Add(items[i].get());
            }
            pSlideShow->Select(items[0]->GetPath());
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
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        if (g_application.IsPlayingVideo())
          g_application.StopPlaying();

        g_graphicsContext.Lock();
        pSlideShow->Reset();

        CFileItemList items;
        CStdString strPath = pMsg->strParam;
        CStdString extensions = g_settings.m_pictureExtensions;
        if (pMsg->dwParam1)
          extensions += "|.tbn";
        CUtil::GetRecursiveListing(strPath, items, extensions);

        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i].get());
          pSlideShow->StartSlideShow(pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER); //Start the slideshow!
        }
        if (pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER)
          pSlideShow->Shuffle();

        if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
        {
          if(items.Size() == 0)
          {
            g_guiSettings.SetString("screensaver.mode", "screensaver.xbmc.builtin.dim");
            g_application.ActivateScreenSaver();
          }
          else
            g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        }

        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_SETLANGUAGE:
      g_guiSettings.SetLanguage(pMsg->strParam);
      break;
    case TMSG_MEDIA_STOP:
      {
        // restore to previous window if needed
        if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();

        // stop playing file
        if (g_application.IsPlaying()) g_application.StopPlaying();
      }
      break;

    case TMSG_MEDIA_PAUSE:
      if (g_application.m_pPlayer)
      {
        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_MEDIA_UNPAUSE:
      if (g_application.IsPaused())
      {
        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_SWITCHTOFULLSCREEN:
      if( g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
        g_application.SwitchToFullScreen();
      break;

    case TMSG_TOGGLEFULLSCREEN:
      g_graphicsContext.Lock();
      g_graphicsContext.ToggleFullScreenRoot();
      g_graphicsContext.Unlock();
      break;

    case TMSG_MINIMIZE:
      g_application.Minimize();
      break;

    case TMSG_EXECUTE_OS:
#if defined( _LINUX) && !defined(__APPLE__)
      CUtil::RunCommandLine(pMsg->strParam.c_str(), (pMsg->dwParam1 == 1));
#elif defined(_WIN32)
      CWIN32Util::XBMCShellExecute(pMsg->strParam.c_str(), (pMsg->dwParam1 == 1));
#endif
      break;

    case TMSG_HTTPAPI:
    {
#ifdef HAS_HTTPAPI
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
          g_application.getApplicationMessenger().Quit();
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
      g_pythonParser.evalFile(pMsg->strParam.c_str(),ADDON::AddonPtr());
#endif
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CBuiltins::Execute(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.Play(pMsg->dwParam1);
      else
        g_playlistPlayer.Play();
      break;

    case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
      if (pMsg->dwParam1 != (DWORD) -1)
      {
        bool *result = (bool*)pMsg->lpVoid;
        *result = g_playlistPlayer.PlaySongId(pMsg->dwParam1);
      }
      else
        g_playlistPlayer.Play();
      break;

    case TMSG_PLAYLISTPLAYER_NEXT:
      g_playlistPlayer.PlayNext();
      break;

    case TMSG_PLAYLISTPLAYER_PREV:
      g_playlistPlayer.PlayPrevious();
      break;

    case TMSG_PLAYLISTPLAYER_ADD:
      if(pMsg->lpVoid)
      {
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

        g_playlistPlayer.Add(pMsg->dwParam1, (*list));
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_INSERT:
      if (pMsg->lpVoid)
      {
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;
        g_playlistPlayer.Insert(pMsg->dwParam1, (*list), pMsg->dwParam2);
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_REMOVE:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.Remove(pMsg->dwParam1,pMsg->dwParam2);
      break;

    case TMSG_PLAYLISTPLAYER_CLEAR:
      g_playlistPlayer.ClearPlaylist(pMsg->dwParam1);
      break;

    case TMSG_PLAYLISTPLAYER_SHUFFLE:
      g_playlistPlayer.SetShuffle(pMsg->dwParam1, pMsg->dwParam2 > 0);
      break;

    case TMSG_PLAYLISTPLAYER_REPEAT:
      g_playlistPlayer.SetRepeat(pMsg->dwParam1, (PLAYLIST::REPEAT_STATE)pMsg->dwParam2);
      break;

    case TMSG_PLAYLISTPLAYER_GET_ITEMS:
      if (pMsg->lpVoid)
      {
        PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(pMsg->dwParam1);
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

        for (int i = 0; i < playlist.size(); i++)
          list->Add(CFileItemPtr(new CFileItem(*playlist[i])));
      }
      break;

    case TMSG_PLAYLISTPLAYER_SWAP:
      if (pMsg->lpVoid)
      {
        vector<int> *indexes = (vector<int> *)pMsg->lpVoid;
        if (indexes->size() == 2)
          g_playlistPlayer.Swap(pMsg->dwParam1, indexes->at(0), indexes->at(1));
        delete indexes;
      }
      break;

    // Window messages below here...
    case TMSG_DIALOG_DOMODAL:  //doModel of window
      {
        CGUIDialog* pDialog = (CGUIDialog*)g_windowManager.GetWindow(pMsg->dwParam1);
        if (!pDialog) return ;
        pDialog->DoModal();
      }
      break;

    case TMSG_NETWORKMESSAGE:
      {
        g_application.getNetwork().NetworkMessage((CNetwork::EMESSAGE)pMsg->dwParam1, (int)pMsg->dwParam2);
      }
      break;

    case TMSG_GUI_DO_MODAL:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->DoModal((int)pMsg->dwParam1, pMsg->strParam);
      }
      break;

    case TMSG_GUI_SHOW:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->Show();
      }
      break;

    case TMSG_GUI_WINDOW_CLOSE:
      {
        CGUIWindow *window = (CGUIWindow *)pMsg->lpVoid;
        if (window)
          window->Close(pMsg->dwParam2 & 0x1 ? true : false, pMsg->dwParam1, pMsg->dwParam2 & 0x2 ? true : false);
      }
      break;

    case TMSG_GUI_ACTIVATE_WINDOW:
      {
        g_windowManager.ActivateWindow(pMsg->dwParam1, pMsg->params, pMsg->dwParam2 > 0);
      }
      break;

    case TMSG_GUI_ADDON_DIALOG:
      {
        if (pMsg->lpVoid)
        { // TODO: This is ugly - really these python dialogs should just be normal XBMC dialogs
          ((ADDON::CGUIAddonWindowDialog *) pMsg->lpVoid)->Show_Internal(pMsg->dwParam2 > 0);
        }
      }
      break;

    case TMSG_GUI_PYTHON_DIALOG:
      {
        if (pMsg->lpVoid)
        { // TODO: This is ugly - really these python dialogs should just be normal XBMC dialogs
          if (pMsg->dwParam1)
            ((CGUIPythonWindowXMLDialog *)pMsg->lpVoid)->Show_Internal(pMsg->dwParam2 > 0);
          else
            ((CGUIPythonWindowDialog *)pMsg->lpVoid)->Show_Internal(pMsg->dwParam2 > 0);
        }
      }
      break;

    case TMSG_GUI_ACTION:
      {
        if (pMsg->lpVoid)
        {
          CAction *action = (CAction *)pMsg->lpVoid;
          if (pMsg->dwParam1 == WINDOW_INVALID)
            g_application.OnAction(*action);
          else
          {
            CGUIWindow *pWindow = g_windowManager.GetWindow(pMsg->dwParam1);  
            if (pWindow)
              pWindow->OnAction(*action);
            else
              CLog::Log(LOGWARNING, "Failed to get window with ID %i to send an action to", pMsg->dwParam1);
          }
          delete action;
        }
      }
      break;

    case TMSG_GUI_INFOLABEL:
      {
        if (pMsg->lpVoid)
        {
          vector<CStdString> *infoLabels = (vector<CStdString> *)pMsg->lpVoid;
          for (unsigned int i = 0; i < pMsg->params.size(); i++)
            infoLabels->push_back(g_infoManager.GetLabel(g_infoManager.TranslateString(pMsg->params[i])));
        }
      }
      break;
    case TMSG_GUI_INFOBOOL:
      {
        if (pMsg->lpVoid)
        {
          vector<bool> *infoLabels = (vector<bool> *)pMsg->lpVoid;
          for (unsigned int i = 0; i < pMsg->params.size(); i++)
            infoLabels->push_back(g_infoManager.EvaluateBool(pMsg->params[i]));
        }
      }
      break;

#ifdef HAS_DVD_DRIVE
    case TMSG_OPTICAL_MOUNT:
      {
        CMediaSource share;
        share.strPath = pMsg->strParam;
        share.strStatus = g_mediaManager.GetDiskLabel(share.strPath);
        share.strDiskUniqueId = g_mediaManager.GetDiskUniqueId(share.strPath);
        if(g_mediaManager.IsAudio(share.strPath))
          share.strStatus = "Audio-CD";
        else if(share.strStatus == "")
          share.strStatus = g_localizeStrings.Get(446);
        share.strName = share.strPath;
        share.m_ignore = true;
        share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
        g_mediaManager.AddAutoSource(share, pMsg->dwParam1 != 0);
      }
      break;

    case TMSG_OPTICAL_UNMOUNT:
      {
        CMediaSource share;
        share.strPath = pMsg->strParam;
        share.strName = share.strPath;
        g_mediaManager.RemoveAutoSource(share);
      }
      break;
    case TMSG_CALLBACK:
      {
        ThreadMessageCallback *callback = (ThreadMessageCallback*)pMsg->lpVoid;
        callback->callback(callback->userptr);
      }
#endif
    case TMSG_VOLUME_SHOW:
      {
        CAction action((int)pMsg->dwParam1);
        g_application.ShowVolumeBar(&action);
      }
    case TMSG_SPLASH_MESSAGE:
      {
        if (g_application.m_splash)
          g_application.m_splash->Show(pMsg->strParam);
      }
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

    boost::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
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

void CApplicationMessenger::ExecBuiltIn(const CStdString &command, bool wait)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
  tMsg.strParam = command;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::MediaPlay(string filename)
{
  CFileItem item(filename, false);
  if (item.IsAudio())
    item.SetMusicThumb();
  else
    item.SetVideoThumb();
  item.FillInDefaultIcon();

  MediaPlay(item);
}

void CApplicationMessenger::MediaPlay(const CFileItem &item)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));

  MediaPlay(list);
}

void CApplicationMessenger::MediaPlay(const CFileItemList &list, int song)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void*)listcopy;
  tMsg.dwParam1 = song;
  tMsg.dwParam2 = 1;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaPlay(int playlistid, int song /* = -1 */)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  tMsg.lpVoid = NULL;
  tMsg.dwParam1 = playlistid;
  tMsg.dwParam2 = song;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayFile(const CFileItem &item, bool bRestart /*= false*/)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItem *pItem = new CFileItem(item);
  tMsg.lpVoid = (void *)pItem;
  tMsg.dwParam1 = bRestart ? 1 : 0;
  tMsg.dwParam2 = 0;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::MediaStop(bool bWait /* = true */)
{
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  SendMessage(tMsg, bWait);
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

bool CApplicationMessenger::PlayListPlayerPlaySongId(int songId)
{
  bool returnState;
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY_SONG_ID, songId};
  tMsg.lpVoid = (void *)&returnState;
  SendMessage(tMsg, true);
  return returnState;
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

void CApplicationMessenger::PlayListPlayerAdd(int playlist, const CFileItem &item)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));

  PlayListPlayerAdd(playlist, list);
}

void CApplicationMessenger::PlayListPlayerAdd(int playlist, const CFileItemList &list)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_ADD};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void*)listcopy;
  tMsg.dwParam1 = playlist;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerInsert(int playlist, const CFileItem &item, int index)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));
  PlayListPlayerInsert(playlist, list, index);
}

void CApplicationMessenger::PlayListPlayerInsert(int playlist, const CFileItemList &list, int index)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_INSERT};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void *)listcopy;
  tMsg.dwParam1 = playlist;
  tMsg.dwParam2 = index;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerRemove(int playlist, int position)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_REMOVE, playlist, position};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerClear(int playlist)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_CLEAR};
  tMsg.dwParam1 = playlist;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerShuffle(int playlist, bool shuffle)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_SHUFFLE};
  tMsg.dwParam1 = playlist;
  tMsg.dwParam2 = shuffle ? 1 : 0;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerGetItems(int playlist, CFileItemList &list)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_GET_ITEMS};
  tMsg.dwParam1 = playlist;
  tMsg.lpVoid = (void *)&list;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerSwap(int playlist, int indexItem1, int indexItem2)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_SWAP};
  tMsg.dwParam1 = playlist;
  vector<int> *indexes = new vector<int>();
  indexes->push_back(indexItem1);
  indexes->push_back(indexItem2);
  tMsg.lpVoid = (void *)indexes;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerRepeat(int playlist, int repeatState)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_REPEAT};
  tMsg.dwParam1 = playlist;
  tMsg.dwParam2 = repeatState;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PictureShow(string filename)
{
  ThreadMessage tMsg = {TMSG_PICTURE_SHOW};
  tMsg.strParam = filename;
  SendMessage(tMsg);
}

void CApplicationMessenger::PictureSlideShow(string pathname, bool bScreensaver /* = false */, bool addTBN /* = false */)
{
  DWORD dwMessage = TMSG_PICTURE_SLIDESHOW;
  if (bScreensaver)
    dwMessage = TMSG_SLIDESHOW_SCREENSAVER;
  ThreadMessage tMsg = {dwMessage};
  tMsg.strParam = pathname;
  tMsg.dwParam1 = addTBN ? 1 : 0;
  SendMessage(tMsg);
}

void CApplicationMessenger::SetGUILanguage(const std::string &strLanguage)
{
  ThreadMessage tMsg = {TMSG_SETLANGUAGE};
  tMsg.strParam = strLanguage;
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

void CApplicationMessenger::InhibitIdleShutdown(bool inhibit)
{
  ThreadMessage tMsg = {TMSG_INHIBITIDLESHUTDOWN, (DWORD)inhibit};
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

void CApplicationMessenger::Minimize(bool wait)
{
  ThreadMessage tMsg = {TMSG_MINIMIZE};
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param)
{
  ThreadMessage tMsg = {TMSG_GUI_DO_MODAL};
  tMsg.lpVoid = pDialog;
  tMsg.dwParam1 = (DWORD)iWindowID;
  tMsg.strParam = param;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::ExecOS(const CStdString command, bool waitExit)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_OS};
  tMsg.strParam = command;
  tMsg.dwParam1 = (DWORD)waitExit;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::UserEvent(int code)
{
  ThreadMessage tMsg = {code};
  SendMessage(tMsg, false);
}

void CApplicationMessenger::Show(CGUIDialog *pDialog)
{
  ThreadMessage tMsg = {TMSG_GUI_SHOW};
  tMsg.lpVoid = pDialog;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::Close(CGUIWindow *window, bool forceClose, bool waitResult /*= true*/, int nextWindowID /*= 0*/, bool enableSound /*= true*/)
{
  ThreadMessage tMsg = {TMSG_GUI_WINDOW_CLOSE, nextWindowID};
  tMsg.dwParam2 = (DWORD)(forceClose ? 0x01 : 0 | enableSound ? 0x02 : 0);
  tMsg.lpVoid = window;
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::ActivateWindow(int windowID, const vector<CStdString> &params, bool swappingWindows)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTIVATE_WINDOW, windowID, swappingWindows ? 1 : 0};
  tMsg.params = params;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::SendAction(const CAction &action, int windowID, bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTION};
  tMsg.dwParam1 = windowID;
  tMsg.lpVoid = new CAction(action);
  SendMessage(tMsg, waitResult);
}

vector<CStdString> CApplicationMessenger::GetInfoLabels(const vector<CStdString> &properties)
{
  vector<CStdString> infoLabels;

  ThreadMessage tMsg = {TMSG_GUI_INFOLABEL};
  tMsg.params = properties;
  tMsg.lpVoid = (void*)&infoLabels;
  SendMessage(tMsg, true);
  return infoLabels;
}

vector<bool> CApplicationMessenger::GetInfoBooleans(const vector<CStdString> &properties)
{
  vector<bool> infoLabels;

  ThreadMessage tMsg = {TMSG_GUI_INFOBOOL};
  tMsg.params = properties;
  tMsg.lpVoid = (void*)&infoLabels;
  SendMessage(tMsg, true);
  return infoLabels;
}

void CApplicationMessenger::OpticalMount(CStdString device, bool bautorun)
{
  ThreadMessage tMsg = {TMSG_OPTICAL_MOUNT};
  tMsg.strParam = device;
  tMsg.dwParam1 = (DWORD)bautorun;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::OpticalUnMount(CStdString device)
{
  ThreadMessage tMsg = {TMSG_OPTICAL_UNMOUNT};
  tMsg.strParam = device;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::ShowVolumeBar(bool up)
{
  ThreadMessage tMsg = {TMSG_VOLUME_SHOW};
  tMsg.dwParam1 = up ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::SetSplashMessage(const CStdString& message)
{
  ThreadMessage tMsg = {TMSG_SPLASH_MESSAGE};
  tMsg.strParam = message;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::SetSplashMessage(int stringID)
{
  SetSplashMessage(g_localizeStrings.Get(stringID));
}
