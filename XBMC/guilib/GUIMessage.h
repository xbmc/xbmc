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

#define GUI_MSG_LABEL_ADD      12   // add label 2 spin control (lpvoid points to char* CStdString)

#define GUI_MSG_LABEL_SET      13

#define GUI_MSG_LABEL_RESET    14
#define GUI_MSG_ITEM_SELECTED  15

#define GUI_MSG_USER         1000

class CGUIMessage
{
public:
  CGUIMessage(DWORD dwMsg, DWORD dwSenderId, DWORD dwControlID, DWORD dwParam1=0, DWORD dwParam2=0, void* lpVoid=NULL);
  CGUIMessage(const CGUIMessage& msg);
  virtual ~CGUIMessage(void);
  const CGUIMessage& operator = (const CGUIMessage& msg);
  DWORD GetControlId() const ;
  DWORD GetMessage()  const;
  void* GetLPVOID()   const;
  DWORD GetParam1()   const;
  DWORD GetParam2()   const;
  DWORD GetSenderId() const;
  void SetParam1(DWORD dwParam1);
  void SetParam2(DWORD dwParam2);
  void SetLPVOID(void* lpVoid);
private:
  DWORD m_dwSenderID;
  DWORD m_dwControlID;
  DWORD m_dwMessage;
  void* m_lpVoid;
  DWORD m_dwParam1;
  DWORD m_dwParam2;
};
#endif