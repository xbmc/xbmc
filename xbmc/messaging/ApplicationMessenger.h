/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/WindowIDs.h"
#include "messaging/ThreadMessage.h"
#include "threads/Thread.h"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

// clang-format off
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
#define TMSG_PLAYLISTPLAYER_PLAY_ITEM_ID  TMSG_MASK_PLAYLISTPLAYER + 7
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
#define TMSG_MEDIA_SEEK_TIME              TMSG_MASK_PLAYLISTPLAYER + 18

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
#define TMSG_RESETSCREENSAVER TMSG_MASK_APPLICATION + 10
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
#define TMSG_INHIBITSCREENSAVER           TMSG_MASK_APPLICATION + 29

#define TMSG_SYSTEM_POWERDOWN             TMSG_MASK_APPLICATION + 30
#define TMSG_RENDERER_PREINIT             TMSG_MASK_APPLICATION + 31
#define TMSG_RENDERER_UNINIT              TMSG_MASK_APPLICATION + 32
#define TMSG_EVENT                        TMSG_MASK_APPLICATION + 33
#define TMSG_MOVETOSCREEN                 TMSG_MASK_APPLICATION + 34

/// @brief Called from the player when its current item is updated
#define TMSG_UPDATE_PLAYER_ITEM TMSG_MASK_APPLICATION + 35

#define TMSG_SET_VOLUME                   TMSG_MASK_APPLICATION + 36
#define TMSG_SET_MUTE                     TMSG_MASK_APPLICATION + 37

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
#define TMSG_GUI_DIALOG_OK                TMSG_MASK_WINDOWMANAGER + 9

/*!
  \def TMSG_GUI_PREVIOUS_WINDOW
  \brief Message sent through CApplicationMessenger to go back to the previous window

  This is an alternative to TMSG_GUI_ACTIVATE_WINDOW, but it keeps
  all configured parameters, like startup directory.
*/
#define TMSG_GUI_PREVIOUS_WINDOW          TMSG_MASK_WINDOWMANAGER + 10


#define TMSG_CALLBACK                     800
// clang-format on

class CGUIMessage;

namespace KODI
{
namespace MESSAGING
{
class IMessageTarget;

struct ThreadMessageCallback
{
  void (*callback)(void *userptr);
  void *userptr;
};

/*!
 * \class CApplicationMessenger ApplicationMessenger.h "messaging/ApplicationMessenger.h"
 * \brief This implements a simple message dispatcher/router for Kodi
 *
 * For most users that wants to send message go to the documentation for these
 * \sa CApplicationMessenger::SendMsg
 * \sa CApplicationMessenger::PostMsg
 *
 * For anyone wanting to implement a message receiver, go to the documentation for
 * \sa IMessageTarget
 *
 * IMPLEMENTATION SPECIFIC NOTES - DOCUMENTED HERE FOR THE SOLE PURPOSE OF IMPLEMENTERS OF THIS CLASS
 * On a high level this implements two methods for dispatching messages, SendMsg and PostMsg.
 * These are roughly modeled on the implementation of SendMessage and PostMessage in Windows.
 *
 * PostMsg is the preferred method to use as it's non-blocking and does not wait for any response before
 * returning to the caller. Messages will be stored in a queue and processed in order.
 *
 * SendMsg is a blocking version and has a bit more subtleties to it regarding how inter-process
 * dispatching is handled.
 *
 * Calling SendMsg with a message type that doesn't require marshalling will bypass the message queue
 * and call the receiver directly
 *
 * Calling SendMsg with a message type that require marshalling to a specific thread when not on that thread
 * will add a message to the queue with a an event, it will then block the calling thread waiting on this event
 * to be signaled.
 * The message will be processed by the correct thread in it's message pump and the event will be signaled, unblocking
 * the calling thread
 *
 * Calling SendMsg with a message type that require marshalling to a specific thread when already on that thread
 * will behave as scenario one, it will bypass the queue and call the receiver directly.
 *
 * Currently there is a hack implemented in the message dispatcher that releases the graphicslock before dispatching
 * a message. This was here before the redesign and removing it will require careful inspection of every call site.
 * TODO: add logging if the graphicslock is held during message dispatch
 *
 * Current design has three different message types
 * 1. Normal messages that can be processed on any thread
 * 2. GUI messages that require marshalling to the UI thread
 * 3. A thread message that will spin up a background thread and wait a specified amount of time before posting the message
 *    This should probably be removed, it's left for compatibility
 *
 * Heavy emphasis on current design, the idea is that we can easily add more message types to route messages
 * to more threads or other scenarios.
 *
 * \sa CApplicationMessenger::ProcessMessages()
 * handles regular messages that require no marshalling, this can be called from any thread to drive the message
 * pump
 *
 * \sa CApplicationMessenger::ProcessWindowMessages()
 * handles GUI messages and currently should only be called on the UI thread
 *
 * If/When this is expanded upon ProcessMessage() and ProcessWindowMessages() should be combined into a single method
 * taking an enum or similar to indicate which message it's interested in.
 *
 * The above methods are backed by two messages queues, one for each type of message. If more types are added
 * this might need to be redesigned to simplify the lookup of the correct message queue but currently they're implemented
 * as two member variables
 *
 * The design is meant to be very encapsulated and easy to extend without altering the public interface.
 * e.g. If GUI messages should be handled on another thread, call \sa CApplicationMessenger::ProcessWindowMessage() on that
 * thread and nothing else has to change. The callers have no knowledge of how this is implemented.
 *
 * The design is also meant to be very dependency free to work as a bridge between lower layer functionality without
 * having to have knowledge of the GUI or having a dependency on the GUI in any way. This is not the reality currently as
 * this depends on \sa CApplication and the graphicslock but should be fixed soon enough.
 *
 * To keep things simple the current implementation routes messages based on a mask that the receiver provides.
 * Any message fitting that mask will be routed to that specific receiver.
 * This will likely need to change if many different receivers are added but it should be possible to do it without
 * any of the callers being changed.
 */
class CApplicationMessenger
{
public:
  CApplicationMessenger();
  ~CApplicationMessenger();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  /*!
   * \brief Send a blocking message and wait for a response
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * Under no circumestances shall the caller hold a lock when calling SendMsg as there's
   * no guarantee what the receiver will do to answer the request.
   *
   * \param [in] messageId defined further up in this file
   * \return meaning of the return varies based on the message
   */
  int SendMsg(uint32_t messageId);

  /*!
   * \brief Send a blocking message and wait for a response
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * Under no circumestances shall the caller hold a lock when calling SendMsg as there's
   * no guarantee what the receiver will do to answer the request.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent, defaults to -1
   * \param [in] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   * \return meaning of the return varies based on the message
   */
  int SendMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);

  /*!
   * \brief Send a blocking message and wait for a response
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * Under no circumestances shall the caller hold a lock when calling SendMsg as there's
   * no guarantee what the receiver will do to answer the request.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent
   * \param [in,out] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   * \param [in] strParam value depends on the message being sent, remains for backward compat
   * \return meaning of the return varies based on the message
   */
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);

  /*!
   * \brief Send a blocking message and wait for a response
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * Under no circumestances shall the caller hold a lock when calling SendMsg as there's
   * no guarantee what the receiver will do to answer the request.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent
   * \param [in,out] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   * \param [in] strParam value depends on the message being sent, remains for backward compat
   * \param [in] params value depends on the message being sent, kept for backward compatibility
   * \return meaning of the return varies based on the message
   */
  int SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  /*!
   * \brief Send a non-blocking message and return immediately
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * \param [in] messageId defined further up in this file
   */
  void PostMsg(uint32_t messageId);

  /*!
   * \brief Send a non-blocking message and return immediately
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param3 value depends on the message being sent
   */
  void PostMsg(uint32_t messageId, int64_t param3);

  /*!
   * \brief Send a non-blocking message and return immediately
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent
   * \param [in,out] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   */
  void PostMsg(uint32_t messageId, int param1, int param2 = -1, void* payload = nullptr);

  /*!
   * \brief Send a non-blocking message and return immediately
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent
   * \param [in,out] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   * \param [in] strParam value depends on the message being sent, remains for backward compat
   */
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam);
  /*!
   * \brief Send a non-blocking message and return immediately
   *
   * If and what the response is depends entirely on the message being sent and
   * should be documented on the message.
   *
   * \param [in] messageId defined further up in this file
   * \param [in] param1 value depends on the message being sent
   * \param [in] param2 value depends on the message being sent
   * \param [in,out] payload this is a void pointer that is meant to send larger objects to the receiver
   *             what to send depends on the message
   * \param [in] strParam value depends on the message being sent, remains for backward compat
   * \param [in] params value depends on the message being sent, kept for backward compatibility
   */
  void PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params);

  /*!
   * \brief Called from any thread to dispatch messages
   */
  void ProcessMessages();

  /*!
   * \brief Called from the UI thread to dispatch UI messages
   * This is only of value to implementers of the message pump, do not rely on a specific thread
   * being used other than that it's appropriate for UI messages
   */
  void ProcessWindowMessages();

  /*! \brief Send a GUIMessage, optionally waiting before it's processed to return.
   * This is kept for backward compat and is just a convenience wrapper for for SendMsg and PostMsg
   * specifically for UI messages
   * \param msg the GUIMessage to send.
   * \param windowID optional window to send the message to (defaults to no specified window).
   * \param waitResult whether to wait for the result (defaults to false).
   */
  void SendGUIMessage(const CGUIMessage &msg, int windowID = WINDOW_INVALID, bool waitResult=false);

  /*!
   * \brief This should be called any class implementing \sa IMessageTarget before it
   * can receive any messages
   */
  void RegisterReceiver(IMessageTarget* target);

  /*!
   * \brief Set the UI thread id to avoid messenger being dependent on
   * CApplication to determine if marshaling is required
   * \param thread The UI thread ID
   */
  void SetGUIThread(const std::thread::id thread) { m_guiThreadId = thread; }

  /*!
   * \brief Set the processing thread id to avoid messenger being dependent on
   * CApplication to determine if marshaling is required
   * \param thread The processing thread ID
   */
  void SetProcessThread(const std::thread::id thread) { m_processThreadId = thread; }

  /*
   * \brief Signals the shutdown of the application and message processing
   */
  void Stop() { m_bStop = true; }

  //! \brief Returns true if this is the process / app loop thread.
  bool IsProcessThread() const;

private:
  CApplicationMessenger(const CApplicationMessenger&) = delete;
  CApplicationMessenger const& operator=(CApplicationMessenger const&) = delete;

  int SendMsg(ThreadMessage&& msg, bool wait);
  void ProcessMessage(ThreadMessage *pMsg);

  std::queue<ThreadMessage*> m_vecMessages; /*!< queue for regular messages */
  std::queue<ThreadMessage*> m_vecWindowMessages; /*!< queue for UI messages */
  std::map<int, IMessageTarget*> m_mapTargets; /*!< a map of registered receivers indexed on the message mask*/
  CCriticalSection m_critSection;
  std::thread::id m_guiThreadId;
  std::thread::id m_processThreadId;
  bool m_bStop{ false };
};
}
}
