/*!
\file GUIMessage.h
\brief
*/

#ifndef GUILIB_MESSAGE_H
#define GUILIB_MESSAGE_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define GUI_MSG_WINDOW_INIT     1   // initialize window
#define GUI_MSG_WINDOW_DEINIT   2   // deinit window
#define GUI_MSG_WINDOW_RESET    27  // reset window to initial state

#define GUI_MSG_SETFOCUS        3   // set focus to control param1=up/down/left/right
#define GUI_MSG_LOSTFOCUS       4   // control lost focus

#define GUI_MSG_CLICKED         5   // control has been clicked

#define GUI_MSG_VISIBLE         6   // set control visible
#define GUI_MSG_HIDDEN          7   // set control hidden

#define GUI_MSG_ENABLED         8   // enable control
#define GUI_MSG_DISABLED        9   // disable control

#define GUI_MSG_SELECTED       10   // control = selected
#define GUI_MSG_DESELECTED     11   // control = not selected

#define GUI_MSG_LABEL_ADD      12   // add label control (for controls supporting more then 1 label)

#define GUI_MSG_LABEL_SET      13  // set the label of a control

#define GUI_MSG_LABEL_RESET    14  // clear all labels of a control // add label control (for controls supporting more then 1 label)

#define GUI_MSG_ITEM_SELECTED  15  // ask control 2 return the selected item
#define GUI_MSG_ITEM_SELECT   16  // ask control 2 select a specific item
#define GUI_MSG_LABEL2_SET   17
#define GUI_MSG_SHOWRANGE      18

#define GUI_MSG_FULLSCREEN  19  // should go to fullscreen window (vis or video)
#define GUI_MSG_EXECUTE    20  // user has clicked on a button with <execute> tag

#define GUI_MSG_NOTIFY_ALL    21  // message will be send to all active and inactive(!) windows, all active modal and modeless dialogs
                                  // dwParam1 must contain an additional message the windows should react on

#define GUI_MSG_REFRESH_THUMBS 22 // message is sent to all windows to refresh all thumbs

#define GUI_MSG_MOVE          23 // message is sent to the window from the base control class when it's
                                 // been asked to move.  dwParam1 contains direction.

#define GUI_MSG_LABEL_BIND     24   // bind label control (for controls supporting more then 1 label)

#define GUI_MSG_SELCHANGED  25  // selection within the control has changed

#define GUI_MSG_FOCUSED     26  // a control has become focused

#define GUI_MSG_PAGE_CHANGE 28  // a page control has changed the page number

#define GUI_MSG_REFRESH_LIST 29 // message sent to all listing controls telling them to refresh their item layouts

#define GUI_MSG_PAGE_UP      30 // page up
#define GUI_MSG_PAGE_DOWN    31 // page down
#define GUI_MSG_MOVE_OFFSET  32 // Instruct the contorl to MoveUp or MoveDown by offset amount

#define GUI_MSG_SET_TYPE     33 ///< Instruct a control to set it's type appropriately

/*!
 \brief Message indicating the window has been resized
 Any controls that keep stored sizing information based on aspect ratio or window size should
 recalculate sizing information
 */
#define GUI_MSG_WINDOW_RESIZE  34

/*!
 \brief Message indicating loss of renderer, prior to reset
 Any controls that keep shared resources should free them on receipt of this message, as the renderer
 is about to be reset.
 */
#define GUI_MSG_RENDERER_LOST  35

/*!
 \brief Message indicating regain of renderer, after reset
 Any controls that keep shared resources may reallocate them now that the renderer is back
 */
#define GUI_MSG_RENDERER_RESET 36

/*!
 \brief A control wishes to have (or release) exclusive access to mouse actions
 */
#define GUI_MSG_EXCLUSIVE_MOUSE 37

/*!
 \brief A request for supported gestures is made
 */
#define GUI_MSG_GESTURE_NOTIFY  38

/*!
 \brief A request to add a control
 */
#define GUI_MSG_ADD_CONTROL     39

/*!
 \brief A request to remove a control
 */
#define GUI_MSG_REMOVE_CONTROL  40

/*!
 \brief A request to unfocus all currently focused controls
 */
#define GUI_MSG_UNFOCUS_ALL 41

#define GUI_MSG_SET_TEXT        42

#define GUI_MSG_WINDOW_LOAD 43

#define GUI_MSG_USER         1000

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_SELECTED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DESELECT(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_ENABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DISABLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_DISABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE_ON_CONDITION(controlID, bCondition) \
do { \
 CGUIMessage msg(bCondition ? GUI_MSG_ENABLED:GUI_MSG_DISABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT_ITEM(controlID,iItem) \
do { \
 CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), controlID,iItem); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL(controlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL_THREAD_SAFE(controlID,label) \
{ \
 CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 if(g_application.IsCurrentThread()) \
   OnMessage(msg); \
 else \
   g_windowManager.SendThreadMessage(msg, GetID()); \
}

/*!
 \ingroup winmsg
 \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(controlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL2_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_HIDDEN(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_FOCUS(controlID, dwParam) \
do { \
 CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), controlID, dwParam); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_VISIBLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

#define SET_CONTROL_SELECTED(dwSenderId, controlID, bSelect) \
do { \
 CGUIMessage msg(bSelect?GUI_MSG_SELECTED:GUI_MSG_DESELECTED, dwSenderId, controlID); \
 OnMessage(msg); \
} while(0)

#define BIND_CONTROL(i,c,pv) \
do { \
 pv = ((c*)GetControl(i));\
} while(0)

/*!
\ingroup winmsg
\brief Click message sent from controls to windows.
 */
#define SEND_CLICK_MESSAGE(id, parentID, action) \
do { \
 CGUIMessage msg(GUI_MSG_CLICKED, id, parentID, action); \
 SendWindowMessage(msg); \
} while(0)

#include <vector>
#include <boost/shared_ptr.hpp>
#include "utils/StdString.h"

// forwards
class CGUIListItem; typedef boost::shared_ptr<CGUIListItem> CGUIListItemPtr;
class CFileItemList;

/*!
 \ingroup winmsg
 \brief
 */
class CGUIMessage
{
public:
  CGUIMessage(int dwMsg, int senderID, int controlID, int param1 = 0, int param2 = 0);
  CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, CFileItemList* item);
  CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, const CGUIListItemPtr &item);
  CGUIMessage(const CGUIMessage& msg);
  virtual ~CGUIMessage(void);
  const CGUIMessage& operator = (const CGUIMessage& msg);

  int GetControlId() const ;
  int GetMessage() const;
  void* GetPointer() const;
  CGUIListItemPtr GetItem() const;
  int GetParam1() const;
  int GetParam2() const;
  int GetSenderId() const;
  void SetParam1(int param1);
  void SetParam2(int param2);
  void SetPointer(void* pointer);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString);               // for convience - looks up in strings.xml
  const std::string& GetLabel() const;
  void SetStringParam(const CStdString &strParam);
  void SetStringParams(const std::vector<CStdString> &params);
  const CStdString& GetStringParam(size_t param = 0) const;
  size_t GetNumStringParams() const;

private:
  std::string m_strLabel;
  std::vector<CStdString> m_params;
  int m_senderID;
  int m_controlID;
  int m_message;
  void* m_pointer;
  int m_param1;
  int m_param2;
  CGUIListItemPtr m_item;

  static CStdString empty_string;
};
#endif
