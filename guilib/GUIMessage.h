#ifndef GUILIB_MESSAGE_H
#define GUILIB_MESSAGE_H

#pragma once
#include "gui3d.h"

#define GUI_MSG_WINDOW_INIT     1   // initialize window
#define GUI_MSG_WINDOW_DEINIT   2   // deinit window

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

#define GUI_MSG_LABEL_SET      13		// set the label of a control

#define GUI_MSG_LABEL_RESET    14		// clear all labels of a control // add label control (for controls supporting more then 1 label)

#define GUI_MSG_ITEM_SELECTED  15		// ask control 2 return the selected item
#define GUI_MSG_ITEM_SELECT		 16		// ask control 2 select a specific item
#define	GUI_MSG_LABEL2_SET		 17

#define GUI_MSG_USER         1000
#include <string>
using namespace std;

#define CONTROL_ENABLE(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_ENABLED, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define CONTROL_DISABLE(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_DISABLED, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define CONTROL_SELECT_ITEM(dwSenderId, dwControlID,iItem) \
{ \
	CGUIMessage msg(GUI_MSG_ITEM_SELECT, dwSenderId, dwControlID,iItem); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_LABEL(dwSenderId, dwControlID,label) \
{ \
	CGUIMessage msg(GUI_MSG_LABEL_SET, dwSenderId, dwControlID); \
	msg.SetLabel(label); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_HIDDEN(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_HIDDEN, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_FOCUS(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_SETFOCUS, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_VISIBLE(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_VISIBLE, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

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
private:
	wstring 				m_strLabel;
  DWORD 					m_dwSenderID;
  DWORD 					m_dwControlID;
  DWORD 					m_dwMessage;
  void* 					m_lpVoid;
  DWORD 					m_dwParam1;
  DWORD 					m_dwParam2;
};
#endif