#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

#pragma once

#include "guiwindow.h"
#include "IMsgSenderCallback.h"
#include "IWindowManagerCallback.h"

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
private:
  vector <CGUIWindow*>		m_vecWindows;
  int											m_iActiveWindow;
	IWindowManagerCallback* m_pCallback;
  CGUIWindow*             m_pRouteWindow;

};

extern  CGUIWindowManager     m_gWindowManager;
#endif