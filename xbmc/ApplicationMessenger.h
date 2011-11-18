#pragma once

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

#include "threads/CriticalSection.h"
#include "utils/StdString.h"
#include "guilib/Key.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include <boost/shared_ptr.hpp>

#include <queue>

class CFileItem;
class CFileItemList;
class CGUIDialog;
class CGUIWindow;

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103
#define TMSG_EXECUTE_OS           104

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203
#define TMSG_MEDIA_UNPAUSE        204

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212
#define TMSG_PLAYLISTPLAYER_ADD   213
#define TMSG_PLAYLISTPLAYER_CLEAR 214
#define TMSG_PLAYLISTPLAYER_SHUFFLE   215
#define TMSG_PLAYLISTPLAYER_GET_ITEMS 216
#define TMSG_PLAYLISTPLAYER_PLAY_SONG_ID 217
#define TMSG_PLAYLISTPLAYER_INSERT 218
#define TMSG_PLAYLISTPLAYER_REMOVE 219
#define TMSG_PLAYLISTPLAYER_SWAP 223
#define TMSG_PLAYLISTPLAYER_REPEAT 224

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221
#define TMSG_SLIDESHOW_SCREENSAVER  222

#define TMSG_SHUTDOWN             300
#define TMSG_POWERDOWN            301
#define TMSG_QUIT                 302
#define TMSG_HIBERNATE            303
#define TMSG_SUSPEND              304
#define TMSG_RESTART              305
#define TMSG_RESET                306
#define TMSG_RESTARTAPP           307
#define TMSG_SWITCHTOFULLSCREEN   308
#define TMSG_MINIMIZE             309
#define TMSG_TOGGLEFULLSCREEN     310
#define TMSG_SETLANGUAGE          311
#define TMSG_RENDERER_FLUSH       312
#define TMSG_INHIBITIDLESHUTDOWN  313

#define TMSG_HTTPAPI              400

#define TMSG_NETWORKMESSAGE         500

#define TMSG_GUI_DO_MODAL             600
#define TMSG_GUI_SHOW                 601
#define TMSG_GUI_ACTIVATE_WINDOW      604
#define TMSG_GUI_PYTHON_DIALOG        605
#define TMSG_GUI_WINDOW_CLOSE         606
#define TMSG_GUI_ACTION               607
#define TMSG_GUI_INFOLABEL            608
#define TMSG_GUI_INFOBOOL             609
#define TMSG_GUI_ADDON_DIALOG         610

#define TMSG_OPTICAL_MOUNT        700
#define TMSG_OPTICAL_UNMOUNT      701

#define TMSG_CALLBACK             800

#define TMSG_VOLUME_SHOW          900
#define TMSG_SPLASH_MESSAGE       901

typedef struct
{
  DWORD dwMessage;
  DWORD dwParam1;
  DWORD dwParam2;
  CStdString strParam;
  std::vector<CStdString> params;
  boost::shared_ptr<CEvent> waitEvent;
  LPVOID lpVoid;
}
ThreadMessage;

class CDelayedMessage : public CThread
{
  public:
    CDelayedMessage(ThreadMessage& msg, unsigned int delay);
    virtual void Process();

  private:
    unsigned int   m_delay;
    ThreadMessage  m_msg;
};

struct ThreadMessageCallback
{
  void (*callback)(void *userptr);
  void *userptr;
};

class CApplicationMessenger
{

public:
  ~CApplicationMessenger();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();


  void MediaPlay(std::string filename);
  void MediaPlay(const CFileItem &item);
  void MediaPlay(const CFileItemList &item, int song = 0);
  void MediaPlay(int playlistid, int song = -1);
  void MediaStop(bool bWait = true);
  void MediaPause();
  void MediaRestart(bool bWait);

  void PlayListPlayerPlay();
  void PlayListPlayerPlay(int iSong);
  bool PlayListPlayerPlaySongId(int songId);
  void PlayListPlayerNext();
  void PlayListPlayerPrevious();
  void PlayListPlayerAdd(int playlist, const CFileItem &item);
  void PlayListPlayerAdd(int playlist, const CFileItemList &list);
  void PlayListPlayerClear(int playlist);
  void PlayListPlayerShuffle(int playlist, bool shuffle);
  void PlayListPlayerGetItems(int playlist, CFileItemList &list);
  void PlayListPlayerInsert(int playlist, const CFileItem &item, int position); 
  void PlayListPlayerInsert(int playlist, const CFileItemList &list, int position);
  void PlayListPlayerRemove(int playlist, int position);
  void PlayListPlayerSwap(int playlist, int indexItem1, int indexItem2);
  void PlayListPlayerRepeat(int playlist, int repeatState);

  void PlayFile(const CFileItem &item, bool bRestart = false); // thread safe version of g_application.PlayFile()
  void PictureShow(std::string filename);
  void PictureSlideShow(std::string pathname, bool bScreensaver = false, bool addTBN = false);
  void SetGUILanguage(const std::string &strLanguage);
  void Shutdown();
  void Powerdown();
  void Quit();
  void Hibernate();
  void Suspend();
  void Restart();
  void RestartApp();
  void Reset();
  void InhibitIdleShutdown(bool inhibit);
  void SwitchToFullscreen(); //
  void Minimize(bool wait = false);
  void ExecOS(const CStdString command, bool waitExit = false);
  void UserEvent(int code);

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void HttpApi(std::string cmd, bool wait = false);
  void ExecBuiltIn(const CStdString &command, bool wait = false);

  void NetworkMessage(DWORD dwMessage, DWORD dwParam = 0);

  void DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param = "");
  void Show(CGUIDialog *pDialog);
  void Close(CGUIWindow *window, bool forceClose, bool waitResult = true, int nextWindowID = 0, bool enableSound = true);
  void ActivateWindow(int windowID, const std::vector<CStdString> &params, bool swappingWindows);
  void SendAction(const CAction &action, int windowID = WINDOW_INVALID, bool waitResult=true);
  std::vector<CStdString> GetInfoLabels(const std::vector<CStdString> &properties);
  std::vector<bool> GetInfoBooleans(const std::vector<CStdString> &properties);

  void OpticalMount(CStdString device, bool bautorun=false);
  void OpticalUnMount(CStdString device);

  void ShowVolumeBar(bool up);

  void SetSplashMessage(const CStdString& message);
  void SetSplashMessage(int stringID);

private:
  void ProcessMessage(ThreadMessage *pMsg);


  std::queue<ThreadMessage*> m_vecMessages;
  std::queue<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;

};
