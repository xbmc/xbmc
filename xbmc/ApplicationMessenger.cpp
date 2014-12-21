/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "ApplicationMessenger.h"
#include "Application.h"

#include "LangInfo.h"
#include "PlayListPlayer.h"
#include "Util.h"
#include "pictures/GUIWindowSlideShow.h"
#include "interfaces/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "network/Network.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/GUIDialog.h"
#include "guilib/Key.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/Resolution.h"
#include "GUIInfoManager.h"
#include "utils/Splash.h"
#include "cores/IPlayer.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "cores/AudioEngine/AEFactory.h"
#include "music/tags/MusicInfoTag.h"

#include "peripherals/Peripherals.h"
#include "powermanagement/PowerManager.h"

#ifdef TARGET_WINDOWS
#include "WIN32Util.h"
#define CHalManager CWIN32Util
#elif defined(TARGET_DARWIN)
#include "osx/CocoaInterface.h"
#endif
#include "addons/AddonCallbacks.h"
#include "addons/AddonCallbacksGUI.h"
#include "storage/MediaManager.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "URL.h"

#include "playlists/PlayList.h"

#include "pvr/PVRManager.h"
#include "windows/GUIWindowLoginScreen.h"

#include "utils/GlobalsHandling.h"
#if defined(TARGET_ANDROID)
  #include "xbmc/android/activity/XBMCApp.h"
#endif

using namespace PVR;
using namespace std;
using namespace MUSIC_INFO;
using namespace PERIPHERALS;

CDelayedMessage::CDelayedMessage(ThreadMessage& msg, unsigned int delay) : CThread("DelayedMessage")
{
  m_msg.dwMessage  = msg.dwMessage;
  m_msg.param1     = msg.param1;
  m_msg.param2     = msg.param2;
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
    CApplicationMessenger::Get().SendMessage(m_msg, false);
}


CApplicationMessenger& CApplicationMessenger::Get()
{
  return s_messenger;
}

CApplicationMessenger::CApplicationMessenger()
{
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
  std::shared_ptr<CEvent> waitEvent;
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
  msg->param1   = message.param1;
  msg->param2   = message.param2;
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

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
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
        switch (CSettings::Get().GetInt("powermanagement.shutdownstate"))
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
        g_PVRManager.SetWakeupCommand();
        g_powerManager.Hibernate();
      }
      break;

    case TMSG_SUSPEND:
      {
        g_PVRManager.SetWakeupCommand();
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
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
        g_application.Stop(EXITCODE_RESTARTAPP);
#endif
      }
      break;

    case TMSG_INHIBITIDLESHUTDOWN:
      {
        g_application.InhibitIdleShutdown(pMsg->param1 != 0);
      }
      break;

    case TMSG_ACTIVATESCREENSAVER:
      {
        g_application.ActivateScreenSaver();
      }
      break;

    case TMSG_MEDIA_PLAY:
      {
        // first check if we were called from the PlayFile() function
        if (pMsg->lpVoid && pMsg->param2 == 0)
        {
          CFileItem *item = (CFileItem *)pMsg->lpVoid;
          g_application.PlayFile(*item, pMsg->param1 != 0);
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
              // Handle "shuffled" option if present
              if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
                g_playlistPlayer.SetShuffle(playlist, list->GetProperty("shuffled").asBoolean(), false);
              // Handle "repeat" option if present
              if (list->HasProperty("repeat") && list->GetProperty("repeat").isInteger())
                g_playlistPlayer.SetRepeat(playlist, (PLAYLIST::REPEAT_STATE)list->GetProperty("repeat").asInteger(), false);

              g_playlistPlayer.Add(playlist, (*list));
              g_playlistPlayer.Play(pMsg->param1);
            }
          }

          delete list;
        }
        else if (pMsg->param1 == PLAYLIST_MUSIC || pMsg->param1 == PLAYLIST_VIDEO)
        {
          if (g_playlistPlayer.GetCurrentPlaylist() != pMsg->param1)
            g_playlistPlayer.SetCurrentPlaylist(pMsg->param1);

          PlayListPlayerPlay(pMsg->param2);
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
        if (g_application.m_pPlayer->IsPlayingVideo()) g_application.StopPlaying();

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
          CURL pathToUrl;
          if (URIUtils::IsZIP(pMsg->strParam))
            pathToUrl = URIUtils::CreateArchivePath("zip", CURL(pMsg->strParam), "");
          else
            pathToUrl = URIUtils::CreateArchivePath("rar", CURL(pMsg->strParam), "");

          CUtil::GetRecursiveListing(pathToUrl.Get(), items, g_advancedSettings.m_pictureExtensions, XFILE::DIR_FLAG_NO_FILE_DIRS);
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
          pSlideShow->Reset();
          pSlideShow->Add(&item);
          pSlideShow->Select(pMsg->strParam);
        }
        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_PICTURE_SLIDESHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        if (g_application.m_pPlayer->IsPlayingVideo())
          g_application.StopPlaying();

        g_graphicsContext.Lock();
        pSlideShow->Reset();

        CFileItemList items;
        CStdString strPath = pMsg->strParam;
        CStdString extensions = g_advancedSettings.m_pictureExtensions;
        if (pMsg->param1)
          extensions += "|.tbn";
        CUtil::GetRecursiveListing(strPath, items, extensions);

        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i].get());
          pSlideShow->StartSlideShow(); //Start the slideshow!
        }

        if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
        {
          if(items.Size() == 0)
          {
            CSettings::Get().SetString("screensaver.mode", "screensaver.xbmc.builtin.dim");
            g_application.ActivateScreenSaver();
          }
          else
            g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        }

        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_SETLANGUAGE:
      g_application.SetLanguage(pMsg->strParam);
      break;
    case TMSG_MEDIA_STOP:
      {
        // restore to previous window if needed
        bool stopSlideshow = true;
        bool stopVideo = true;
        bool stopMusic = true;
        if (pMsg->param1 >= PLAYLIST_MUSIC && pMsg->param1 <= PLAYLIST_PICTURE)
        {
          stopSlideshow = (pMsg->param1 == PLAYLIST_PICTURE);
          stopVideo = (pMsg->param1 == PLAYLIST_VIDEO);
          stopMusic = (pMsg->param1 == PLAYLIST_MUSIC);
        }

        if ((stopSlideshow && g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW) ||
            (stopVideo && g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO) ||
            (stopMusic && g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION))
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();

        // stop playing file
        if (g_application.m_pPlayer->IsPlaying()) g_application.StopPlaying();
      }
      break;

    case TMSG_MEDIA_PAUSE:
      if (g_application.m_pPlayer->HasPlayer())
      {
        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_MEDIA_UNPAUSE:
      if (g_application.m_pPlayer->IsPausedPlayback())
      {
        g_application.ResetScreenSaver();
        g_application.WakeUpScreenSaverAndDPMS();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_MEDIA_PAUSE_IF_PLAYING:
      if (g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
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

    case TMSG_SETVIDEORESOLUTION:
      {
        RESOLUTION res = (RESOLUTION)pMsg->param1;
        bool forceUpdate = pMsg->param2 == 1 ? true : false;
        g_graphicsContext.SetVideoResolution(res, forceUpdate);
      }
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
      /* Suspend AE temporarily so exclusive or hog-mode sinks */
      /* don't block external player's access to audio device  */
      if (!CAEFactory::Suspend())
      {
        CLog::Log(LOGNOTICE, "%s: Failed to suspend AudioEngine before launching external program",__FUNCTION__);
      }
#if defined( TARGET_POSIX) && !defined(TARGET_DARWIN)
      CUtil::RunCommandLine(pMsg->strParam.c_str(), (pMsg->param1 == 1));
#elif defined(TARGET_WINDOWS)
      CWIN32Util::XBMCShellExecute(pMsg->strParam.c_str(), (pMsg->param1 == 1));
#endif
      /* Resume AE processing of XBMC native audio */
      if (!CAEFactory::Resume())
      {
        CLog::Log(LOGFATAL, "%s: Failed to restart AudioEngine after return from external player",__FUNCTION__);
      }
      break;

    case TMSG_EXECUTE_SCRIPT:
      CScriptInvocationManager::Get().Execute(pMsg->strParam);
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CBuiltins::Execute(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      if (pMsg->param1 != -1)
        g_playlistPlayer.Play(pMsg->param1);
      else
        g_playlistPlayer.Play();
      break;

    case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
      if (pMsg->param1 != -1)
      {
        bool *result = (bool*)pMsg->lpVoid;
        *result = g_playlistPlayer.PlaySongId(pMsg->param1);
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

        g_playlistPlayer.Add(pMsg->param1, (*list));
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_INSERT:
      if (pMsg->lpVoid)
      {
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;
        g_playlistPlayer.Insert(pMsg->param1, (*list), pMsg->param2);
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_REMOVE:
      if (pMsg->param1 != -1)
        g_playlistPlayer.Remove(pMsg->param1,pMsg->param2);
      break;

    case TMSG_PLAYLISTPLAYER_CLEAR:
      g_playlistPlayer.ClearPlaylist(pMsg->param1);
      break;

    case TMSG_PLAYLISTPLAYER_SHUFFLE:
      g_playlistPlayer.SetShuffle(pMsg->param1, pMsg->param2 > 0);
      break;

    case TMSG_PLAYLISTPLAYER_REPEAT:
      g_playlistPlayer.SetRepeat(pMsg->param1, (PLAYLIST::REPEAT_STATE)pMsg->param2);
      break;

    case TMSG_PLAYLISTPLAYER_GET_ITEMS:
      if (pMsg->lpVoid)
      {
        PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(pMsg->param1);
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
          g_playlistPlayer.Swap(pMsg->param1, indexes->at(0), indexes->at(1));
        delete indexes;
      }
      break;

    // Window messages below here...
    case TMSG_DIALOG_DOMODAL:  //doModel of window
      {
        CGUIDialog* pDialog = (CGUIDialog*)g_windowManager.GetWindow(pMsg->param1);
        if (!pDialog) return ;
        pDialog->DoModal();
      }
      break;

    case TMSG_NETWORKMESSAGE:
      {
        g_application.getNetwork().NetworkMessage((CNetwork::EMESSAGE)pMsg->param1, pMsg->param2);
      }
      break;

    case TMSG_GUI_DO_MODAL:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->DoModal(pMsg->param1, pMsg->strParam);
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
          window->Close(pMsg->param1 & 0x1 ? true : false, pMsg->param1, pMsg->param1 & 0x2 ? true : false);
      }
      break;

    case TMSG_GUI_ACTIVATE_WINDOW:
      {
        g_windowManager.ActivateWindow(pMsg->param1, pMsg->params, pMsg->param2 > 0);
      }
      break;

    case TMSG_GUI_ADDON_DIALOG:
      {
        if (pMsg->lpVoid)
        { // TODO: This is ugly - really these python dialogs should just be normal XBMC dialogs
          ((ADDON::CGUIAddonWindowDialog *) pMsg->lpVoid)->Show_Internal(pMsg->param2 > 0);
        }
      }
      break;

#ifdef HAS_PYTHON
    case TMSG_GUI_PYTHON_DIALOG:
      {
        // This hack is not much better but at least I don't need to make ApplicationMessenger
        //  know about Addon (Python) specific classes.
        CAction caction(pMsg->param1);
        ((CGUIWindow*)pMsg->lpVoid)->OnAction(caction);
      }
      break;
#endif

    case TMSG_GUI_ACTION:
      {
        if (pMsg->lpVoid)
        {
          CAction *action = (CAction *)pMsg->lpVoid;
          if (pMsg->param1 == WINDOW_INVALID)
            g_application.OnAction(*action);
          else
          {
            CGUIWindow *pWindow = g_windowManager.GetWindow(pMsg->param1);
            if (pWindow)
              pWindow->OnAction(*action);
            else
              CLog::Log(LOGWARNING, "Failed to get window with ID %i to send an action to", pMsg->param1);
          }
          delete action;
        }
      }
      break;

    case TMSG_GUI_MESSAGE:
      {
        if (pMsg->lpVoid)
        {
          CGUIMessage *message = (CGUIMessage *)pMsg->lpVoid;
          g_windowManager.SendMessage(*message, pMsg->param1);
          delete message;
        }
      }
      break;

    case TMSG_GUI_INFOLABEL:
      {
        if (pMsg->lpVoid)
        {
          vector<string> *infoLabels = (vector<string> *)pMsg->lpVoid;
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

    case TMSG_CALLBACK:
      {
        ThreadMessageCallback *callback = (ThreadMessageCallback*)pMsg->lpVoid;
        callback->callback(callback->userptr);
      }
      break;

    case TMSG_VOLUME_SHOW:
      {
        CAction action(pMsg->param1);
        g_application.ShowVolumeBar(&action);
      }
      break;

    case TMSG_SPLASH_MESSAGE:
      {
        if (g_application.GetSplash())
          g_application.GetSplash()->Show(pMsg->strParam);
      }
      break;
      
    case TMSG_DISPLAY_SETUP:
    {
      *((bool*)pMsg->lpVoid) = g_application.InitWindow();
      g_application.SetRenderGUI(true);
    }
    break;
    
    case TMSG_DISPLAY_DESTROY:
    {
      *((bool*)pMsg->lpVoid) = g_application.DestroyWindow();
      g_application.SetRenderGUI(false);
    }
    break;

    case TMSG_UPDATE_CURRENT_ITEM:
    {
      CFileItem* item = (CFileItem*)pMsg->lpVoid;
      if (!item)
        return;
      if (pMsg->param1 == 1 && item->HasMusicInfoTag()) // only grab music tag
        g_infoManager.SetCurrentSongTag(*item->GetMusicInfoTag());
      else if (pMsg->param1 == 2 && item->HasVideoInfoTag()) // only grab video tag
        g_infoManager.SetCurrentVideoTag(*item->GetVideoInfoTag());
      else
        g_infoManager.SetCurrentItem(*item);
      delete item;
      break;
    }

    case TMSG_LOADPROFILE:
    {
      CGUIWindowLoginScreen::LoadProfile(pMsg->param1);
      break;
    }
    case TMSG_CECTOGGLESTATE:
    {
      *((bool*)pMsg->lpVoid) = g_peripherals.ToggleDeviceState(STATE_SWITCH_TOGGLE);
      break;
    }
    case TMSG_CECACTIVATESOURCE:
    {
      g_peripherals.ToggleDeviceState(STATE_ACTIVATE_SOURCE);
      break;
    }
    case TMSG_CECSTANDBY:
    {
      g_peripherals.ToggleDeviceState(STATE_STANDBY);
      break;
    }
    case TMSG_START_ANDROID_ACTIVITY:
    {
#if defined(TARGET_ANDROID)
      if (pMsg->params.size())
      {
        CXBMCApp::StartActivity(pMsg->params[0],
                                pMsg->params.size() > 1 ? pMsg->params[1] : "",
                                pMsg->params.size() > 2 ? pMsg->params[2] : "",
                                pMsg->params.size() > 3 ? pMsg->params[3] : "");
      }
#endif
      break;
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

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
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

void CApplicationMessenger::ExecBuiltIn(const CStdString &command, bool wait)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
  tMsg.strParam = command;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::MediaPlay(string filename)
{
  CFileItem item(filename, false);
  MediaPlay(item);
}

void CApplicationMessenger::MediaPlay(const CFileItem &item, bool wait)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));

  MediaPlay(list, 0, wait);
}

void CApplicationMessenger::MediaPlay(const CFileItemList &list, int song, bool wait)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void*)listcopy;
  tMsg.param1 = song;
  tMsg.param2 = 1;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::MediaPlay(int playlistid, int song /* = -1 */)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  tMsg.lpVoid = NULL;
  tMsg.param1 = playlistid;
  tMsg.param2 = song;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayFile(const CFileItem &item, bool bRestart /*= false*/)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItem *pItem = new CFileItem(item);
  tMsg.lpVoid = (void *)pItem;
  tMsg.param1 = bRestart ? 1 : 0;
  tMsg.param2 = 0;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::MediaStop(bool bWait /* = true */, int playlistid /* = -1 */)
{
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  tMsg.param1 = playlistid;
  SendMessage(tMsg, bWait);
}

void CApplicationMessenger::MediaPause()
{
  ThreadMessage tMsg = {TMSG_MEDIA_PAUSE};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaUnPause()
{
  ThreadMessage tMsg = {TMSG_MEDIA_UNPAUSE};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaPauseIfPlaying()
{
  ThreadMessage tMsg = {TMSG_MEDIA_PAUSE_IF_PLAYING};
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
  tMsg.param1 = playlist;
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
  tMsg.param1 = playlist;
  tMsg.param2 = index;
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
  tMsg.param1 = playlist;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerShuffle(int playlist, bool shuffle)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_SHUFFLE};
  tMsg.param1 = playlist;
  tMsg.param2 = shuffle ? 1 : 0;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerGetItems(int playlist, CFileItemList &list)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_GET_ITEMS};
  tMsg.param1 = playlist;
  tMsg.lpVoid = (void *)&list;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerSwap(int playlist, int indexItem1, int indexItem2)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_SWAP};
  tMsg.param1 = playlist;
  vector<int> *indexes = new vector<int>();
  indexes->push_back(indexItem1);
  indexes->push_back(indexItem2);
  tMsg.lpVoid = (void *)indexes;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerRepeat(int playlist, int repeatState)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_REPEAT};
  tMsg.param1 = playlist;
  tMsg.param2 = repeatState;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PictureShow(string filename)
{
  ThreadMessage tMsg = {TMSG_PICTURE_SHOW};
  tMsg.strParam = filename;
  SendMessage(tMsg);
}

void CApplicationMessenger::PictureSlideShow(string pathname, bool addTBN /* = false */)
{
  unsigned int dwMessage = TMSG_PICTURE_SLIDESHOW;
  ThreadMessage tMsg = {dwMessage};
  tMsg.strParam = pathname;
  tMsg.param1 = addTBN ? 1 : 0;
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
  ThreadMessage tMsg = {TMSG_INHIBITIDLESHUTDOWN, inhibit};
  SendMessage(tMsg);
}

void CApplicationMessenger::ActivateScreensaver()
{
  ThreadMessage tMsg = {TMSG_ACTIVATESCREENSAVER};
  SendMessage(tMsg);
}

void CApplicationMessenger::NetworkMessage(int dwMessage, int dwParam)
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
  tMsg.param1 = iWindowID;
  tMsg.strParam = param;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::ExecOS(const CStdString &command, bool waitExit)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_OS};
  tMsg.strParam = command;
  tMsg.param1 = waitExit ? 1 : 0;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::UserEvent(int code)
{
  ThreadMessage tMsg = {(unsigned int)code};
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
  tMsg.param2 = (forceClose ? 0x01 : 0) | (enableSound ? 0x02 : 0);
  tMsg.lpVoid = window;
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::ActivateWindow(int windowID, const vector<string> &params, bool swappingWindows)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTIVATE_WINDOW, windowID, swappingWindows ? 1 : 0};
  tMsg.params = params;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::SendAction(const CAction &action, int windowID, bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTION};
  tMsg.param1 = windowID;
  tMsg.lpVoid = new CAction(action);
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::SendGUIMessage(const CGUIMessage &message, int windowID, bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_MESSAGE};
  tMsg.param1 = windowID == WINDOW_INVALID ? 0 : windowID;
  tMsg.lpVoid = new CGUIMessage(message);
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::SendText(const std::string &aTextString, bool closeKeyboard /* = false */)
{
  if (CGUIKeyboardFactory::SendTextToActiveKeyboard(aTextString, closeKeyboard))
    return;

  CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
  if (!window)
    return;

  CGUIMessage msg(GUI_MSG_SET_TEXT, 0, window->GetFocusedControlID());
  msg.SetLabel(aTextString);
  msg.SetParam1(closeKeyboard ? 1 : 0);
  SendGUIMessage(msg, window->GetID());
}

vector<string> CApplicationMessenger::GetInfoLabels(const vector<string> &properties)
{
  vector<string> infoLabels;

  ThreadMessage tMsg = {TMSG_GUI_INFOLABEL};
  tMsg.params = properties;
  tMsg.lpVoid = (void*)&infoLabels;
  SendMessage(tMsg, true);
  return infoLabels;
}

vector<bool> CApplicationMessenger::GetInfoBooleans(const vector<string> &properties)
{
  vector<bool> infoLabels;

  ThreadMessage tMsg = {TMSG_GUI_INFOBOOL};
  tMsg.params = properties;
  tMsg.lpVoid = (void*)&infoLabels;
  SendMessage(tMsg, true);
  return infoLabels;
}

void CApplicationMessenger::ShowVolumeBar(bool up)
{
  ThreadMessage tMsg = {TMSG_VOLUME_SHOW};
  tMsg.param1 = up ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN;
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

bool CApplicationMessenger::SetupDisplay()
{
  bool result;
  
  ThreadMessage tMsg = {TMSG_DISPLAY_SETUP};
  tMsg.lpVoid = (void*)&result;
  SendMessage(tMsg, true);
  
  return result;
}

bool CApplicationMessenger::DestroyDisplay()
{
  bool result;
  
  ThreadMessage tMsg = {TMSG_DISPLAY_DESTROY};
  tMsg.lpVoid = (void*)&result;
  SendMessage(tMsg, true);
  
  return result;
}

void CApplicationMessenger::SetCurrentSongTag(const CMusicInfoTag& tag)
{
  CFileItem* item = new CFileItem(tag);
  ThreadMessage tMsg = {TMSG_UPDATE_CURRENT_ITEM};
  tMsg.param1 = 1;
  tMsg.lpVoid = (void*)item;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::SetCurrentVideoTag(const CVideoInfoTag& tag)
{
  CFileItem* item = new CFileItem(tag);
  ThreadMessage tMsg = {TMSG_UPDATE_CURRENT_ITEM};
  tMsg.param1 = 2;
  tMsg.lpVoid = (void*)item;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::SetCurrentItem(const CFileItem& item)
{
  CFileItem* item2 = new CFileItem(item);
  ThreadMessage tMsg = {TMSG_UPDATE_CURRENT_ITEM};
  tMsg.lpVoid = (void*)item2;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::LoadProfile(unsigned int idx)
{
  ThreadMessage tMsg = {TMSG_LOADPROFILE};
  tMsg.param1 = idx;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::StartAndroidActivity(const vector<string> &params)
{
  ThreadMessage tMsg = {TMSG_START_ANDROID_ACTIVITY};
  tMsg.params = params;
  SendMessage(tMsg, false);
}

bool CApplicationMessenger::CECToggleState()
{
  bool result;

  ThreadMessage tMsg = {TMSG_CECTOGGLESTATE};
  tMsg.lpVoid = (void*)&result;
  SendMessage(tMsg, true);

  return result;
}

void CApplicationMessenger::CECActivateSource()
{
  ThreadMessage tMsg = {TMSG_CECACTIVATESOURCE};
  SendMessage(tMsg, false);
}

void CApplicationMessenger::CECStandby()
{
  ThreadMessage tMsg = {TMSG_CECSTANDBY};
  SendMessage(tMsg, false);
}
