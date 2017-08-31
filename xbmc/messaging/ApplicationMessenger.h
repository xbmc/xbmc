#pragma once

/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/WindowIDs.h"
#include "threads/Thread.h"
#include "messaging/ThreadMessage.h"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#define TMSG_MASK_MESSAGE                 0xFFFF0000 // only keep the high bits to route messages
#define TMSG_MASK_APPLICATION             (1<<30) //Don't use bit 31 as it'll fail to build, using unsigned variable to hold the message.
#define TMSG_MASK_PLAYLISTPLAYER          (1<<29)
#define TMSG_MASK_GUIINFOMANAGER          (1<<28)
#define TMSG_MASK_WINDOWMANAGER           (1<<27)
#define TMSG_MASK_PERIPHERALS             (1<<26)

// defines here
#define TMSG_PLAYLISTPLAYER_PLAY          TMSG_MASK_PLAYLISTPLAYER + 0
#define TMSG_PLAYLISTPLAYER_NEXT          TMSG_MASK_PLAYLISTPLAYER + 1
#define TMSG_PLAYLISTPLAYER_PREV          TMSG_MASK_PLAYLISTPLAYER + 2
#define TMSG_PLAYLISTPLAYER_ADD           TMSG_MASK_PLAYLISTPLAYER + 3
#define TMSG_PLAYLISTPLAYER_CLEAR         TMSG_MASK_PLAYLISTPLAYER + 4
#define TMSG_PLAYLISTPLAYER_SHUFFLE       TMSG_MASK_PLAYLISTPLAYER + 5
#define TMSG_PLAYLISTPLAYER_GET_ITEMS     TMSG_MASK_PLAYLISTPLAYER + 6
#define TMSG_PLAYLISTPLAYER_PLAY_SONG_ID  TMSG_MASK_PLAYLISTPLAYER + 7
#define TMSG_PLAYLISTPLAYER_INSERT        TMSG_MASK_PLAYLISTPLAYER + 8
#define TMSG_PLAYLISTPLAYER_REMOVE        TMSG_MASK_PLAYLISTPLAYER + 9
#define TMSG_PLAYLISTPLAYER_SWAP          TMSG_MASK_PLAYLISTPLAYER + 10
#define TMSG_PLAYLISTPLAYER_REPEAT        TMSG_MASK_PLAYLISTPLAYER + 11
#define TMSG_MEDIA_PLAY                   TMSG_MASK_PLAYLISTPLAYER + 12
#define TMSG_MEDIA_STOP                   TMSG_MASK_PLAYLISTPLAYER + 13
// the PAUSE is indeed a PLAYPAUSE        
#define TMSG_MEDIA_PAUSE                  TMSG_MASK_PLAYLISTPLAYER + 14
#define TMSG_MEDIA_RESTART                TMSG_MASK_PLAYLISTPLAYER + 15
#define TMSG_MEDIA_UNPAUSE                TMSG_MASK_PLAYLISTPLAYER + 16
#define TMSG_MEDIA_PAUSE_IF_PLAYING       TMSG_MASK_PLAYLISTPLAYER + 17



#define TMSG_SHUTDOWN                     TMSG_MASK_APPLICATION + 0
#define TMSG_POWERDOWN                    TMSG_MASK_APPLICATION + 1
#define TMSG_QUIT                         TMSG_MASK_APPLICATION + 2
#define TMSG_HIBERNATE                    TMSG_MASK_APPLICATION + 3
#define TMSG_SUSPEND                      TMSG_MASK_APPLICATION + 4
#define TMSG_RESTART                      TMSG_MASK_APPLICATION + 5
#define TMSG_RESET                        TMSG_MASK_APPLICATION + 6
#define TMSG_RESTARTAPP                   TMSG_MASK_APPLICATION + 7
#define TMSG_ACTIVATESCREENSAVER          TMSG_MASK_APPLICATION + 8
#define TMSG_NETWORKMESSAGE               TMSG_MASK_APPLICATION + 9
#define TMSG_SETPVRMANAGERSTATE           TMSG_MASK_APPLICATION + 10
#define TMSG_VOLUME_SHOW                  TMSG_MASK_APPLICATION + 11
#define TMSG_DISPLAY_SETUP                TMSG_MASK_APPLICATION + 12
#define TMSG_DISPLAY_DESTROY              TMSG_MASK_APPLICATION + 13
#define TMSG_SETVIDEORESOLUTION           TMSG_MASK_APPLICATION + 14
#define TMSG_SWITCHTOFULLSCREEN           TMSG_MASK_APPLICATION + 15
#define TMSG_MINIMIZE                     TMSG_MASK_APPLICATION + 16
#define TMSG_TOGGLEFULLSCREEN             TMSG_MASK_APPLICATION + 17
#define TMSG_SETLANGUAGE                  TMSG_MASK_APPLICATION + 18
#define TMSG_RENDERER_FLUSH               TMSG_MASK_APPLICATION + 19
#define TMSG_INHIBITIDLESHUTDOWN          TMSG_MASK_APPLICATION + 20
#define TMSG_START_ANDROID_ACTIVITY       TMSG_MASK_APPLICATION + 21
#define TMSG_EXECUTE_SCRIPT               TMSG_MASK_APPLICATION + 22
#define TMSG_EXECUTE_BUILT_IN             TMSG_MASK_APPLICATION + 23
#define TMSG_EXECUTE_OS                   TMSG_MASK_APPLICATION + 24
#define TMSG_PICTURE_SHOW                 TMSG_MASK_APPLICATION + 25
#define TMSG_PICTURE_SLIDESHOW            TMSG_MASK_APPLICATION + 26
#define TMSG_LOADPROFILE                  TMSG_MASK_APPLICATION + 27
#define TMSG_VIDEORESIZE                  TMSG_MASK_APPLICATION + 28
#define TMSG_SETAUDIODSPSTATE             TMSG_MASK_APPLICATION + 29
#define TMSG_SYSTEM_POWERDOWN             TMSG_MASK_APPLICATION + 30

#define TMSG_GUI_INFOLABEL                TMSG_MASK_GUIINFOMANAGER + 0
#define TMSG_GUI_INFOBOOL                 TMSG_MASK_GUIINFOMANAGER + 1
#define TMSG_UPDATE_CURRENT_ITEM          TMSG_MASK_GUIINFOMANAGER + 2


#define TMSG_CECTOGGLESTATE               TMSG_MASK_PERIPHERALS + 1
#define TMSG_CECACTIVATESOURCE            TMSG_MASK_PERIPHERALS + 2
#define TMSG_CECSTANDBY                   TMSG_MASK_PERIPHERALS + 3




#define TMSG_GUI_DIALOG_OPEN              TMSG_MASK_WINDOWMANAGER + 1
#define TMSG_GUI_ACTIVATE_WINDOW          TMSG_MASK_WINDOWMANAGER + 2
#define TMSG_GUI_PYTHON_DIALOG            TMSG_MASK_WINDOWMANAGER + 3
#define TMSG_GUI_WINDOW_CLOSE             TMSG_MASK_WINDOWMANAGER + 4
#define TMSG_GUI_ACTION                   TMSG_MASK_WINDOWMANAGER + 5
#define TMSG_GUI_ADDON_DIALOG             TMSG_MASK_WINDOWMANAGER + 6
#define TMSG_GUI_MESSAGE                  TMSG_MASK_WINDOWMANAGER + 7

/*!
  \def TMSG_GUI_DIALOG_YESNO
  \brief Message sent through CApplicationMessenger to open a yes/no dialog box

  There's two ways to send this message, a short and concise way and a more
  flexible way allowing more customization.

  Option 1:
  CApplicationMessenger::Get().SendMsg(TMSG_GUI_DIALOG_YESNO, 123, 456);
  123: This is the string id for the heading
  456: This is the string id for the text

  Option 2:
  \a HELPERS::DialogYesNoMessage options.
  Fill in options
  CApplicationMessenger::Get().SendMsg(TMSG_GUI_DIALOG_YESNO, -1, -1, static_cast<void*>(&options));

  \returns -1 for cancelled, 0 for No and 1 for Yes
  \sa HELPERS::DialogYesNoMessage
*/
#define TMSG_GUI_DIALOG_YESNO             TMSG_MASK_WINDOWMANAGER + 8


#define TMSG_CALLBACK                     800



class CGUIMessage;

namespace KODI
{
namespace MESSAGING
{

class CDelayedMessage : public CThread
{
  public:
    CDelayedMessage(ThreadMessage& msg, unsigned int delay);
    virtual void Process() override;

  private:
    unsigned int   m_delay;
    ThreadMessage  m_msg;
};

struct ThreadMessageCallback
{
  void (*callback)(void *userptr);
  void *userptr;
};

class IMessageTarget;

class CApplicationMessenger
{
public:
  /*!
   \brief The only way through which the global instance of the CApplicationMessenger should be accessed.
   \return the global instance.
   */
  static CApplicationMessenger& GetInstance();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  int SendMsg(uint32_t messageId);
  int SendMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  void PostMsg(uint32_t messageId);
  void PostMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();

  /*! \brief Send a GUIMessage, optionally waiting before it's processed to return.
   Should be used to send messages to the GUI from other threads.
   \param msg the GUIMessage to send.
   \param windowID optional window to send the message to (defaults to no specified window).
   \param waitResult whether to wait for the result (defaults to false).
   */
  void SendGUIMessage(const CGUIMessage &msg, int windowID = WINDOW_INVALID, bool waitResult=false);

  void RegisterReceiver(IMessageTarget* target);

private:
  // private construction, and no assignments; use the provided singleton methods
  CApplicationMessenger();
  CApplicationMessenger(const CApplicationMessenger&) = delete;
  CApplicationMessenger const& operator=(CApplicationMessenger const&) = delete;
  ~CApplicationMessenger();

  int SendMsg(ThreadMessage&& msg, bool wait);
  void ProcessMessage(ThreadMessage *pMsg);

  std::queue<ThreadMessage*> m_vecMessages;
  std::queue<ThreadMessage*> m_vecWindowMessages;
  std::map<int, IMessageTarget*> m_mapTargets;
  CCriticalSection m_critSection;
};
}
}


