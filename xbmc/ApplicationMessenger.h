#pragma once

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

#include "utils/StdString.h"
#include "guilib/WindowIDs.h"
#include "threads/Thread.h"
#include <memory>

#include <queue>
#include "utils/GlobalsHandling.h"

class CFileItem;
class CFileItemList;
class CGUIDialog;
class CGUIWindow;
class CGUIMessage;
class CVideoInfoTag;
class CAction;

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103
#define TMSG_EXECUTE_OS           104

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
// the PAUSE is indeed a PLAYPAUSE
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203
#define TMSG_MEDIA_UNPAUSE        204
#define TMSG_MEDIA_PAUSE_IF_PLAYING   205

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
#define TMSG_UPDATE_CURRENT_ITEM 225

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221

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
#define TMSG_LOADPROFILE          314
#define TMSG_ACTIVATESCREENSAVER  315
#define TMSG_CECTOGGLESTATE       316
#define TMSG_CECACTIVATESOURCE    317
#define TMSG_CECSTANDBY           318
#define TMSG_SETVIDEORESOLUTION   319

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
#define TMSG_GUI_MESSAGE              611
#define TMSG_START_ANDROID_ACTIVITY   612

#define TMSG_CALLBACK             800

#define TMSG_VOLUME_SHOW          900
#define TMSG_SPLASH_MESSAGE       901

#define TMSG_DISPLAY_SETUP      1000
#define TMSG_DISPLAY_DESTROY    1001

typedef struct
{
  unsigned int dwMessage;
  int param1;
  int param2;
  CStdString strParam;
  std::vector<std::string> params;
  std::shared_ptr<CEvent> waitEvent;
  void* lpVoid;
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

class CApplicationMessenger;
namespace xbmcutil
{
   template<class T> class GlobalsSingleton;
}

class CApplicationMessenger
{
public:
  /*!
   \brief The only way through which the global instance of the CApplicationMessenger should be accessed.
   \return the global instance.
   */
  static CApplicationMessenger& Get();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();


  void MediaPlay(std::string filename);
  void MediaPlay(const CFileItem &item, bool wait = true);
  void MediaPlay(const CFileItemList &item, int song = 0, bool wait = true);
  void MediaPlay(int playlistid, int song = -1);
  void MediaStop(bool bWait = true, int playlistid = -1);
  void MediaPause();
  void MediaUnPause();
  void MediaPauseIfPlaying();
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
  void PictureSlideShow(std::string pathname, bool addTBN = false);
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
  void ActivateScreensaver();
  void SwitchToFullscreen(); //
  void Minimize(bool wait = false);
  void ExecOS(const CStdString &command, bool waitExit = false);
  void UserEvent(int code);
  //! \brief Set the tag for the currently playing song
  void SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag& tag);
  //! \brief Set the tag for the currently playing video
  void SetCurrentVideoTag(const CVideoInfoTag& tag);
  //! \brief Set the currently currently item
  void SetCurrentItem(const CFileItem& item);

  void LoadProfile(unsigned int idx);
  bool CECToggleState();
  void CECActivateSource();
  void CECStandby();

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void ExecBuiltIn(const CStdString &command, bool wait = false);

  void NetworkMessage(int dwMessage, int dwParam = 0);

  void DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param = "");
  void Show(CGUIDialog *pDialog);
  void Close(CGUIWindow *window, bool forceClose, bool waitResult = true, int nextWindowID = 0, bool enableSound = true);
  void ActivateWindow(int windowID, const std::vector<std::string> &params, bool swappingWindows);
  void SendAction(const CAction &action, int windowID = WINDOW_INVALID, bool waitResult=true);

  //! \brief Send text to currently focused window / keyboard.
  void SendText(const std::string &aTextString, bool closeKeyboard = false);

  /*! \brief Send a GUIMessage, optionally waiting before it's processed to return.
   Should be used to send messages to the GUI from other threads.
   \param msg the GUIMessage to send.
   \param windowID optional window to send the message to (defaults to no specified window).
   \param waitResult whether to wait for the result (defaults to false).
   */
  void SendGUIMessage(const CGUIMessage &msg, int windowID = WINDOW_INVALID, bool waitResult=false);

  std::vector<std::string> GetInfoLabels(const std::vector<std::string> &properties);
  std::vector<bool> GetInfoBooleans(const std::vector<std::string> &properties);

  void ShowVolumeBar(bool up);

  void SetSplashMessage(const CStdString& message);
  void SetSplashMessage(int stringID);
  
  bool SetupDisplay();
  bool DestroyDisplay();
  void StartAndroidActivity(const std::vector<std::string> &params);

  virtual ~CApplicationMessenger();
private:
  // private construction, and no assignements; use the provided singleton methods
   friend class xbmcutil::GlobalsSingleton<CApplicationMessenger>;
  CApplicationMessenger();
  CApplicationMessenger(const CApplicationMessenger&);
  CApplicationMessenger const& operator=(CApplicationMessenger const&);
  void ProcessMessage(ThreadMessage *pMsg);

  std::queue<ThreadMessage*> m_vecMessages;
  std::queue<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;
};

XBMC_GLOBAL_REF(CApplicationMessenger,s_messenger);
#define s_messenger XBMC_GLOBAL_USE(CApplicationMessenger)


