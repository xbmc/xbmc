/*!
	\file GUIWindowManager.h
	\brief 
	*/

#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

#pragma once

#include "guiwindow.h"
#include "IMsgSenderCallback.h"
#include "IWindowManagerCallback.h"
#include "IMsgTargetCallback.h"

/*!
	\ingroup winman
	\brief 
	*/
class CGUIWindowManager:public IMsgSenderCallback
{
public:
	CGUIWindowManager(void);
	virtual ~CGUIWindowManager(void);  
	virtual void    SendMessage(CGUIMessage& message);
	void			Initialize();
	void            Add(CGUIWindow* pWindow);
	void            AddCustomWindow(CGUIWindow* pWindow);
	void            AddModeless(CGUIWindow* pWindow);
	void			Remove(DWORD dwID);
	void			RemoveModeless(DWORD dwID);
	void            ActivateWindow(int iWindowID, const CStdString& strPath = "");
	void			PreviousWindow();
	void			RefreshWindow();
	void            OnAction(const CAction &action);
	void            Render();
	CGUIWindow*     GetWindow(DWORD dwID);
	void			Process();
	void			SetCallback(IWindowManagerCallback& callback);
	void			DeInitialize();
	void			RouteToWindow(CGUIWindow* pWindow);
	void            UnRoute(DWORD dwID);
	int             GetTopMostRoutedWindowID() const;
	void			SendThreadMessage(CGUIMessage& message);
	void			DispatchThreadMessages();
	void			AddMsgTarget( IMsgTargetCallback* pMsgTarget );
	int				GetActiveWindow() const;
	bool            IsRouted() const;
private:
	vector <CGUIWindow*>	m_vecWindows;
	vector <CGUIWindow*>	m_vecModelessWindows;
	vector <CGUIWindow*>	m_vecModalWindows;
	vector <CGUIWindow*>	m_vecCustomWindows;

	int								m_iActiveWindow;
	IWindowManagerCallback*			m_pCallback;
	vector <CGUIMessage*>			m_vecThreadMessages;
	CRITICAL_SECTION				m_critSection;
	vector <IMsgTargetCallback*>	m_vecMsgTargets;
};

/*!
	\ingroup winman
	\brief 
	*/
extern  CGUIWindowManager     m_gWindowManager;
#endif
