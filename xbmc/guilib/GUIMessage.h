/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMessage.h
\brief
*/

constexpr const int GUI_MSG_WINDOW_INIT    = 1;  // initialize window
constexpr const int GUI_MSG_WINDOW_DEINIT  = 2;  // deinit window
constexpr const int GUI_MSG_WINDOW_RESET   = 27; // reset window to initial state

constexpr const int GUI_MSG_SETFOCUS       = 3;  // set focus to control param1=up/down/left/right
constexpr const int GUI_MSG_LOSTFOCUS      = 4;  // control lost focus

constexpr const int GUI_MSG_CLICKED        = 5;  // control has been clicked

constexpr const int GUI_MSG_VISIBLE        = 6;  // set control visible
constexpr const int GUI_MSG_HIDDEN         = 7;   // set control hidden

constexpr const int GUI_MSG_ENABLED        = 8;   // enable control
constexpr const int GUI_MSG_DISABLED       = 9;   // disable control

constexpr const int GUI_MSG_SET_SELECTED   = 10;   // control = selected
constexpr const int GUI_MSG_SET_DESELECTED = 11;   // control = not selected

constexpr const int GUI_MSG_LABEL_ADD      = 12;   // add label control (for controls supporting more then 1 label)

constexpr const int GUI_MSG_LABEL_SET      = 13;  // set the label of a control

constexpr const int GUI_MSG_LABEL_RESET    = 14;  // clear all labels of a control // add label control (for controls supporting more then 1 label)

constexpr const int GUI_MSG_ITEM_SELECTED  = 15;  // ask control 2 return the selected item
constexpr const int GUI_MSG_ITEM_SELECT    = 16;  // ask control 2 select a specific item
constexpr const int GUI_MSG_LABEL2_SET     = 17;
constexpr const int GUI_MSG_SHOWRANGE      = 18;

constexpr const int GUI_MSG_FULLSCREEN     = 19;  // should go to fullscreen window (vis or video)
constexpr const int GUI_MSG_EXECUTE        = 20;  // user has clicked on a button with <execute> tag

constexpr const int GUI_MSG_NOTIFY_ALL     = 21;  // message will be send to all active and inactive(!) windows, all active modal and modeless dialogs
                                  // dwParam1 must contain an additional message the windows should react on

constexpr const int GUI_MSG_REFRESH_THUMBS = 22; // message is sent to all windows to refresh all thumbs

constexpr const int GUI_MSG_MOVE           = 23; // message is sent to the window from the base control class when it's
                                 // been asked to move.  dwParam1 contains direction.

constexpr const int GUI_MSG_LABEL_BIND     = 24;   // bind label control (for controls supporting more then 1 label)

constexpr const int GUI_MSG_FOCUSED        = 26;  // a control has become focused

constexpr const int GUI_MSG_PAGE_CHANGE    = 28;  // a page control has changed the page number

constexpr const int GUI_MSG_REFRESH_LIST   = 29; // message sent to all listing controls telling them to refresh their item layouts

constexpr const int GUI_MSG_PAGE_UP      = 30; // page up
constexpr const int GUI_MSG_PAGE_DOWN    = 31; // page down
constexpr const int GUI_MSG_MOVE_OFFSET  = 32; // Instruct the control to MoveUp or MoveDown by offset amount

constexpr const int GUI_MSG_SET_TYPE     = 33; ///< Instruct a control to set it's type appropriately

/*!
 \brief Message indicating the window has been resized
 Any controls that keep stored sizing information based on aspect ratio or window size should
 recalculate sizing information
 */
constexpr const int GUI_MSG_WINDOW_RESIZE  = 34;

/*!
 \brief Message indicating loss of renderer, prior to reset
 Any controls that keep shared resources should free them on receipt of this message, as the renderer
 is about to be reset.
 */
constexpr const int GUI_MSG_RENDERER_LOST = 35;

/*!
 \brief Message indicating regain of renderer, after reset
 Any controls that keep shared resources may reallocate them now that the renderer is back
 */
constexpr const int GUI_MSG_RENDERER_RESET = 36;

/*!
 \brief A control wishes to have (or release) exclusive access to mouse actions
 */
constexpr const int GUI_MSG_EXCLUSIVE_MOUSE = 37;

/*!
 \brief A request for supported gestures is made
 */
constexpr const int GUI_MSG_GESTURE_NOTIFY = 38;

/*!
 \brief A request to add a control
 */
constexpr const int GUI_MSG_ADD_CONTROL = 39;

/*!
 \brief A request to remove a control
 */
constexpr const int GUI_MSG_REMOVE_CONTROL = 40;

/*!
 \brief A request to unfocus all currently focused controls
 */
constexpr const int GUI_MSG_UNFOCUS_ALL = 41;

constexpr const int GUI_MSG_SET_TEXT    = 42;

constexpr const int GUI_MSG_WINDOW_LOAD = 43;

constexpr const int GUI_MSG_VALIDITY_CHANGED = 44;

/*!
 \brief Check whether a button is selected
 */
constexpr const int GUI_MSG_IS_SELECTED  = 45;

/*!
 \brief Bind a set of labels to a spin (or similar) control
 */
constexpr const int GUI_MSG_SET_LABELS = 46;

/*!
 \brief Set the filename for an image control
 */
constexpr const int GUI_MSG_SET_FILENAME = 47;

/*!
 \brief Get the filename of an image control
 */

constexpr const int GUI_MSG_GET_FILENAME  = 48;

/*!
 \brief The user interface is ready for usage
 */
constexpr const int GUI_MSG_UI_READY = 49;

 /*!
 \brief Called every 500ms to allow time dependent updates
 */
constexpr const int GUI_MSG_REFRESH_TIMER = 50;

 /*!
 \brief Called if state has changed which could lead to GUI changes
 */
constexpr const int GUI_MSG_STATE_CHANGED = 51;

/*!
 \brief Called when a subtitle download has finished
 */
constexpr const int GUI_MSG_SUBTITLE_DOWNLOADED = 52;


constexpr const int GUI_MSG_USER = 1000;

/*!
\brief Complete to get codingtable page
*/
constexpr const int GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED = 65000;

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_SELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DESELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_DESELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ENABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DISABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE_ON_CONDITION(controlID, bCondition) \
  do \
  { \
    CGUIMessage _msg(bCondition ? GUI_MSG_ENABLED : GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT_ITEM(controlID, iItem) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ITEM_SELECT, GetID(), controlID, iItem); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL_THREAD_SAFE(controlID, label) \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    if (CServiceBroker::GetAppMessenger()->IsProcessThread()) \
      OnMessage(_msg); \
    else \
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(_msg, GetID()); \
  }

/*!
 \ingroup winmsg
 \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL2_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief Set a bunch of labels on the given control
 */
#define SET_CONTROL_LABELS(controlID, defaultValue, labels) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_LABELS, GetID(), controlID, defaultValue); \
    _msg.SetPointer(labels); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_FILENAME(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_FILENAME, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_HIDDEN(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_HIDDEN, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_FOCUS(controlID, dwParam) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SETFOCUS, GetID(), controlID, dwParam); \
    OnMessage(_msg); \
  } while (0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_VISIBLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_VISIBLE, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_SELECTED(dwSenderId, controlID, bSelect) \
  do \
  { \
    CGUIMessage _msg(bSelect ? GUI_MSG_SET_SELECTED : GUI_MSG_SET_DESELECTED, dwSenderId, \
                     controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
\ingroup winmsg
\brief Click message sent from controls to windows.
 */
#define SEND_CLICK_MESSAGE(id, parentID, action) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_CLICKED, id, parentID, action); \
    SendWindowMessage(_msg); \
  } while (0)

#include <string>
#include <vector>
#include <memory>

// forwards
class CGUIListItem;
class CFileItemList;

/*!
 \ingroup winmsg
 \brief
 */
class CGUIMessage final
{
public:
  CGUIMessage(int dwMsg, int senderID, int controlID, int64_t param1 = 0, int64_t param2 = 0);
  CGUIMessage(
      int msg, int senderID, int controlID, int64_t param1, int64_t param2, CFileItemList* item);
  CGUIMessage(int msg,
              int senderID,
              int controlID,
              int64_t param1,
              int64_t param2,
              const std::shared_ptr<CGUIListItem>& item);
  CGUIMessage(const CGUIMessage& msg);
  ~CGUIMessage(void);
  CGUIMessage& operator = (const CGUIMessage& msg);

  int GetControlId() const ;
  int GetMessage() const;
  void* GetPointer() const;
  std::shared_ptr<CGUIListItem> GetItem() const;
  int GetParam1() const;
  int64_t GetParam1AsI64() const;
  int GetParam2() const;
  int64_t GetParam2AsI64() const;
  int GetSenderId() const;
  void SetParam1(int64_t param1);
  void SetParam2(int64_t param2);
  void SetPointer(void* pointer);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString);               // for convenience - looks up in strings.po
  const std::string& GetLabel() const;
  void SetStringParam(const std::string &strParam);
  void SetStringParams(const std::vector<std::string> &params);
  const std::string& GetStringParam(size_t param = 0) const;
  size_t GetNumStringParams() const;
  void SetItem(std::shared_ptr<CGUIListItem> item);

private:
  std::string m_strLabel;
  std::vector<std::string> m_params;
  int m_senderID;
  int m_controlID;
  int m_message;
  void* m_pointer;
  int64_t m_param1;
  int64_t m_param2;
  std::shared_ptr<CGUIListItem> m_item;

  static std::string empty_string;
};

