#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

#pragma once

#include "guiwindow.h"
#include "IMsgSenderCallback.h"
#include "IWindowManagerCallback.h"
#include "IMsgTargetCallback.h"

class CGUIWindowManager:public IMsgSenderCallback
{
public:
  CGUIWindowManager(void);
  virtual ~CGUIWindowManager(void);  
  virtual void    SendMessage(CGUIMessage& message);
  void            Add(CGUIWindow* pWindow);
  void            ActivateWindow(int iWindowID);
  void            OnKey(const CKey& key);
  void            Render();
	CGUIWindow*     GetWindow(DWORD dwID);
	void						Process();
	void						SetCallback(IWindowManagerCallback& callback);
	void						DeInitialize();
  void            RouteToWindow(DWORD dwID);
  void            UnRoute();
	void						SendThreadMessage(CGUIMessage& message);
	void						DispatchThreadMessages();
	void						AddMsgTarget( IMsgTargetCallback* pMsgTarget );
	int							GetActiveWindow() const;
private:
  vector <CGUIWindow*>					m_vecWindows;
  int														m_iActiveWindow;
	IWindowManagerCallback*				m_pCallback;
  CGUIWindow*										m_pRouteWindow;
	vector <CGUIMessage*>					m_vecThreadMessages;
	CRITICAL_SECTION							m_critSection;
	vector <IMsgTargetCallback*>	m_vecMsgTargets;
};

extern  CGUIWindowManager     m_gWindowManager;
#endif