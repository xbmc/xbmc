/*!
	\file GUIMessage.h
	\brief 
	*/

#ifndef GUILIB_MESSAGE_H
#define GUILIB_MESSAGE_H

#pragma once
#include "gui3d.h"

#define GUI_MSG_WINDOW_INIT     1   // initialize window
#define GUI_MSG_WINDOW_DEINIT   2   // deinit window

#define GUI_MSG_SETFOCUS        3   // set focus to control param1=up/down/left/right
#define GUI_MSG_LOSTFOCUS       4   // control lost focus

#define GUI_MSG_CLICKED         5   // control has been clicked
#define GUI_MSG_SELCHANGED		25  // selection within the control has changed

#define GUI_MSG_VISIBLE         6   // set control visible
#define GUI_MSG_HIDDEN          7   // set control hidden

#define GUI_MSG_ENABLED         8   // enable control
#define GUI_MSG_DISABLED        9   // disable control

#define GUI_MSG_SELECTED       10   // control = selected
#define GUI_MSG_DESELECTED     11   // control = not selected

#define GUI_MSG_LABEL_ADD      12   // add label control (for controls supporting more then 1 label)
#define GUI_MSG_LABEL_BIND     23   // bind label control (for controls supporting more then 1 label)

#define GUI_MSG_LABEL_SET      13		// set the label of a control

#define GUI_MSG_LABEL_RESET    14		// clear all labels of a control // add label control (for controls supporting more then 1 label)

#define GUI_MSG_ITEM_SELECTED  15		// ask control 2 return the selected item
#define GUI_MSG_ITEM_SELECT		 16		// ask control 2 select a specific item
#define	GUI_MSG_LABEL2_SET		 17
#define	GUI_MSG_SHOWRANGE      18

#define GUI_MSG_FULLSCREEN		19		// should go to fullscreen window (vis or video)
#define GUI_MSG_EXECUTE				20		// user has clicked on a button with <execute> tag

#define GUI_MSG_USER         1000
#include <string>
using namespace std;

/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_SELECT(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_SELECTED, GetID(), dwControlID); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_DESELECT(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), dwControlID); \
	OnMessage(msg); \
}


/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_ENABLE(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_ENABLED, GetID(), dwControlID); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_DISABLE(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_DISABLED, GetID(), dwControlID); \
	OnMessage(msg); \
}


/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_ENABLE_ON_CONDITION(dwControlID, bCondition) \
{ \
	CGUIMessage msg(bCondition ? GUI_MSG_ENABLED:GUI_MSG_DISABLED, GetID(), dwControlID); \
	OnMessage(msg); \
}


/*!
	\ingroup winmsg
	\brief 
	*/
#define CONTROL_SELECT_ITEM(dwControlID,iItem) \
{ \
	CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), dwControlID,iItem); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define SET_CONTROL_LABEL(dwControlID,label) \
{ \
	CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), dwControlID); \
	msg.SetLabel(label); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define SET_CONTROL_HIDDEN(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), dwControlID); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define SET_CONTROL_FOCUS(dwControlID, dwParam) \
{ \
	CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControlID, dwParam); \
	OnMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
#define SET_CONTROL_VISIBLE(dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), dwControlID); \
	OnMessage(msg); \
}

#define SET_CONTROL_SELECTED(dwSenderId, dwControlID, bSelect) \
{ \
	CGUIMessage msg(bSelect?GUI_MSG_SELECTED:GUI_MSG_DESELECTED, dwSenderId, dwControlID); \
	OnMessage(msg); \
}

#define BIND_CONTROL(i,c,pv) \
{ \
	pv = ((c*)GetControl(i));\
} \

/*!
	\ingroup winmsg
	\brief Click message sent from controls to windows.
	*/
#define SEND_CLICK_MESSAGE(dwID, dwParentID, dwAction) \
{ \
	CGUIMessage msg(GUI_MSG_CLICKED, dwID, dwParentID, dwAction); \
	g_graphicsContext.SendMessage(msg); \
}

/*!
	\ingroup winmsg
	\brief 
	*/
class CGUIMessage
{
public:
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1=0, DWORD dwParam2=0, void* lpVoid=NULL);
  CGUIMessage(const CGUIMessage& msg);
  virtual ~CGUIMessage(void);
  const CGUIMessage& operator = (const CGUIMessage& msg);

  DWORD 					GetControlId() const ;
  DWORD 					GetMessage()  const;
  void* 					GetLPVOID()   const;
  DWORD 					GetParam1()   const;
  DWORD 					GetParam2()   const;
  DWORD 					GetSenderId() const;
  void						SetParam1(DWORD dwParam1);
  void						SetParam2(DWORD dwParam2);
  void						SetLPVOID(void* lpVoid);
	void						SetLabel(const wstring& wstrLabel);
	void						SetLabel(const string& wstrLabel);
	void						SetLabel(int iString);
	const wstring&	GetLabel() const;
	void						SetStringParam(const string& strParam);
	const string&	GetStringParam() const;

private:
	wstring 				m_strLabel;
	string 				m_strParam;
  DWORD 					m_dwSenderID;
  DWORD 					m_dwControlID;
  DWORD 					m_dwMessage;
  void* 					m_lpVoid;
  DWORD 					m_dwParam1;
  DWORD 					m_dwParam2;
};
#endif
