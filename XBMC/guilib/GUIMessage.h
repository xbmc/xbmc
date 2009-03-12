/*!
\file GUIMessage.h
\brief
*/

#ifndef GUILIB_MESSAGE_H
#define GUILIB_MESSAGE_H

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

#define GUI_MSG_USER         1000

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_SELECTED, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DESELECT(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_ENABLED, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DISABLE(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_DISABLED, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE_ON_CONDITION(dwControlID, bCondition) \
do { \
 CGUIMessage msg(bCondition ? GUI_MSG_ENABLED:GUI_MSG_DISABLED, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT_ITEM(dwControlID,iItem) \
do { \
 CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), dwControlID,iItem); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL(dwControlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), dwControlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(dwControlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL2_SET, GetID(), dwControlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_HIDDEN(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_FOCUS(dwControlID, dwParam) \
do { \
 CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControlID, dwParam); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_VISIBLE(dwControlID) \
do { \
 CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), dwControlID); \
 OnMessage(msg); \
} while(0)

#define SET_CONTROL_SELECTED(dwSenderId, dwControlID, bSelect) \
do { \
 CGUIMessage msg(bSelect?GUI_MSG_SELECTED:GUI_MSG_DESELECTED, dwSenderId, dwControlID); \
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
#define SEND_CLICK_MESSAGE(dwID, dwParentID, dwAction) \
do { \
 CGUIMessage msg(GUI_MSG_CLICKED, dwID, dwParentID, dwAction); \
 SendWindowMessage(msg); \
} while(0)

#include "boost/shared_ptr.hpp"

// forwards
class CGUIListItem; typedef boost::shared_ptr<CGUIListItem> CGUIListItemPtr;
class CFileItemList;
class CVisualisation;

/*!
 \ingroup winmsg
 \brief
 */
class CGUIMessage
{
public:
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1 = 0, DWORD dwParam2 = 0);
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, CFileItemList* item);
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, const CGUIListItemPtr &item);
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, CVisualisation* vis);
  CGUIMessage(const CGUIMessage& msg);
  virtual ~CGUIMessage(void);
  const CGUIMessage& operator = (const CGUIMessage& msg);

  DWORD GetControlId() const ;
  DWORD GetMessage() const;
  void* GetLPVOID() const;
  CGUIListItemPtr GetItem() const;
  DWORD GetParam1() const;
  DWORD GetParam2() const;
  DWORD GetSenderId() const;
  void SetParam1(DWORD dwParam1);
  void SetParam2(DWORD dwParam2);
  void SetLPVOID(void* lpVoid);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString);               // for convience - looks up in strings.xml
  const std::string& GetLabel() const;
  void SetStringParam(const std::string& strParam);
  const std::string& GetStringParam() const;

private:
  std::string m_strLabel;
  std::string m_strParam;
  DWORD m_dwSenderID;
  DWORD m_dwControlID;
  DWORD m_dwMessage;
  void* m_lpVoid;
  DWORD m_dwParam1;
  DWORD m_dwParam2;
  CGUIListItemPtr m_item;
};
#endif
