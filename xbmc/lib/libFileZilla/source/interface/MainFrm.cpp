// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// MainFrm.cpp : Implementierung der Klasse CMainFrame
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "misc\led.h"
#include "MainFrm.h"
#include "statusview.h"
#include "usersdlg.h"
#include "GroupsDlg.h"
#include "options.h"
#include "usersview.h"
#include "userslistctrl.h"
#include "misc/systemtray.h"
#include "offlineaskdlg.h"
#include "version.h"
#include "AdminSocket.h"
#include "OptionsDlg.h"
#include "ConnectDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_EDIT_SETTINGS, OnEditSettings)
	ON_COMMAND(ID_ACTIVE, OnActive)
	ON_UPDATE_COMMAND_UI(ID_ACTIVE, OnUpdateActive)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_TRAY_EXIT, OnTrayExit)
	ON_COMMAND(ID_TRAY_RESTORE, OnTrayRestore)
	ON_COMMAND(ID_LOCK, OnLock)
	ON_UPDATE_COMMAND_UI(ID_LOCK, OnUpdateLock)
	ON_WM_TIMER()
	ON_COMMAND(ID_USERS, OnMenuEditUsers)
	ON_COMMAND(ID_GROUPS, OnMenuEditGroups)
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_COMMAND(ID_FILE_DISCONNECT, OnFileDisconnect)
	ON_UPDATE_COMMAND_UI(ID_FILE_DISCONNECT, OnUpdateFileDisconnect)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SETTINGS, OnUpdateEditSettings)
	ON_UPDATE_COMMAND_UI(ID_MENU_EDIT_USERS, OnUpdateMenuEditUsers)
	ON_UPDATE_COMMAND_UI(ID_MENU_EDIT_GROUPS, OnUpdateMenuEditGroups)
	ON_UPDATE_COMMAND_UI(ID_USERS, OnUpdateUsers)
	ON_UPDATE_COMMAND_UI(ID_GROUPS, OnUpdateGroups)
	ON_COMMAND(ID_MENU_EDIT_USERS, OnMenuEditUsers)
	ON_COMMAND(ID_MENU_EDIT_GROUPS, OnMenuEditGroups)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // Statusleistenanzeige
	ID_INDICATOR_RECVCOUNT,
	ID_INDICATOR_RECVRATE,
	ID_INDICATOR_SENDCOUNT,
	ID_INDICATOR_SENDRATE,
	ID_INDICATOR_RECVLED,
	ID_INDICATOR_SENDLED
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame Konstruktion/Zerstörung

CMainFrame::CMainFrame(COptions *pOptions)
{
	ASSERT(pOptions);
	nTrayNotificationMsg_=RegisterWindowMessage("FileZilla Server Tray Notification Message");	
	m_bQuit=FALSE;
	m_nSendCount=0;
	m_nRecvCount=0;
	m_lastchecktime=GetTickCount();
	m_lastreaddiff=0;
	m_lastwritediff=0;
	m_nOldRecvCount=0;
	m_nOldSendCount=0;
	m_pOptions = pOptions;
	m_pAdminSocket = NULL;
	m_nServerState = 0;
	m_nEdit = 0;
}

CMainFrame::~CMainFrame()
{
	delete m_pOptions;
	m_pOptions=0;
	if (m_pAdminSocket)
		CloseAdminSocket();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	SetupTrayIcon();

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Symbolleiste konnte nicht erstellt werden\n");
		return -1;      // Fehler bei Erstellung
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Statusleiste konnte nicht erstellt werden\n");
		return -1;      // Fehler bei Erstellung
	}

	CRect rect;
	m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVLED), rect);  

	//Create the first LED control
	m_RecvLed.Create("",WS_VISIBLE|WS_CHILD, rect, &m_wndStatusBar, m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVLED)); 
	m_RecvLed.SetLed( CLed::LED_COLOR_GREEN, CLed::LED_OFF, CLed::LED_ROUND);

	CRect rect2;
	m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVLED), rect2);
	// Reposition the first LED correctly!
	m_RecvLed.SetWindowPos(&wndTop, rect2.left, rect2.top, rect2.right - rect2.left,
		rect2.bottom - rect2.top, 0);

	//Create the second LED control
	m_SendLed.Create("",WS_VISIBLE|WS_CHILD, rect, &m_wndStatusBar, m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED)); 
	m_SendLed.SetLed( CLed::LED_COLOR_RED, CLed::LED_OFF, CLed::LED_ROUND);

	
	m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED), &rect2);
	// Reposition the second LED correctly!
	m_SendLed.SetWindowPos(&wndTop, rect2.left, rect2.top, rect2.right - rect2.left,
		rect2.bottom - rect2.top, 0);

	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVLED),ID_INDICATOR_RECVLED,SBPS_NOBORDERS,6);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED),ID_INDICATOR_SENDLED,SBPS_NOBORDERS,0);

	// ZU ERLEDIGEN: Löschen Sie diese drei Zeilen, wenn Sie nicht wollen, dass die Symbolleiste
	//  andockbar ist.
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	CStatusView *view=(CStatusView *)m_wndSplitter.GetPane(0,0);
	view->ShowStatus(GetVersionString(),0);
	view->ShowStatus("Copyright 2001 by Tim Kosse (Tim.Kosse@gmx.de)",0);

	m_nTimerID=SetTimer(7777, 10000, 0);
	m_nRateTimerID=SetTimer(7778, 1000, 0);

	SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDCOUNT), "0 bytes sent");
	SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVCOUNT), "0 bytes received");
	SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVRATE), "0 B/s");
	SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDRATE), "0 B/s");


	if (m_pOptions->GetOptionVal(IOPTION_ALWAYS))
	{
		m_pAdminSocket = new CAdminSocket(this);
		view->ShowStatus("Connecting to server...", 0);
		m_pAdminSocket->Create();
		m_pAdminSocket->m_Password = m_pOptions->GetOption(IOPTION_LASTSERVERPASS);
		if (!m_pAdminSocket->Connect(m_pOptions->GetOption(IOPTION_LASTSERVERADDRESS), m_pOptions->GetOptionVal(IOPTION_LASTSERVERPORT)) && WSAGetLastError()!=WSAEWOULDBLOCK)
		{
			ShowStatus(_T("Error, could not connect to server"), 1);
			CloseAdminSocket();
		}
	}
	else
	{
		CConnectDialog dlg(m_pOptions);
		if (dlg.DoModal() == IDOK)
		{
			m_pAdminSocket = new CAdminSocket(this);
			view->ShowStatus("Connecting to server...", 0);
			m_pAdminSocket->m_Password = m_pOptions->GetOption(IOPTION_LASTSERVERPASS);
			m_pAdminSocket->Create();
			if (!m_pAdminSocket->Connect(m_pOptions->GetOption(IOPTION_LASTSERVERADDRESS), m_pOptions->GetOptionVal(IOPTION_LASTSERVERPORT)) && WSAGetLastError()!=WSAEWOULDBLOCK)
			{
				ShowStatus(_T("Error, could not connect to server"), 1);
				CloseAdminSocket();
			}
		}
	}

	return 0;
}

//////////////////
// Helper function to register a new window class based on an already
// existing window class, but with a different name and icon. 
// Returns new name if successful; otherwise NULL.
//
static LPCSTR RegisterSimilarClass(LPCSTR lpszNewClassName,
	LPCSTR lpszOldClassName, UINT nIDResource)
{
	// Get class info for old class.
	//
	HINSTANCE hInst = AfxGetInstanceHandle();
	WNDCLASS wc;
	if (!::GetClassInfo(hInst, lpszOldClassName, &wc)) {
		TRACE("Can't find window class %s\n", lpszOldClassName);
		return NULL;
	}

	// Register new class with same info, but different name and icon.
	//
	wc.lpszClassName = lpszNewClassName;
	wc.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(nIDResource));
	if (!AfxRegisterClass(&wc)) {
		TRACE("Unable to register window class%s\n", lpszNewClassName);
		return NULL;
	}
	return lpszNewClassName;
}

// Static class member holds window class name
// (NULL if not registered yet).
// 
LPCSTR CMainFrame::s_winClassName = NULL;

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// ZU ERLEDIGEN: Ändern Sie hier die Fensterklasse oder das Erscheinungsbild, indem Sie
	//  CREATESTRUCT cs modifizieren.

	cs.lpszClass = AfxRegisterWndClass(0);

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	//Change the window class name
	if (s_winClassName==NULL) 
	{
		// One-time initialization: register the class
		//
		s_winClassName = RegisterSimilarClass("FileZilla Server Main Window", 
						cs.lpszClass, IDR_MAINFRAME);
		if (!s_winClassName)
			return FALSE;
	}
	cs.lpszClass = s_winClassName;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame Diagnose

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame Nachrichten-Handler
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	// Fokus an das Ansichtfenster weitergeben
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// andernfalls die Standardbehandlung durchführen
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// Unterteiltes Fenster erstellen
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;
	CRect rect;
	GetWindowRect(rect);
	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CStatusView), CSize(rect.right-rect.left-150, 95), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}
	if (!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CUsersView), CSize(100, 95), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

CStatusView* CMainFrame::GetStatusPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 0);
	CStatusView* pView = DYNAMIC_DOWNCAST(CStatusView, pWnd);
	return pView;
}

CUsersView* CMainFrame::GetUsersPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CUsersView* pView = DYNAMIC_DOWNCAST(CUsersView, pWnd);
	return pView;
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	if (m_wndStatusBar.GetSafeHwnd())
	{
		if (nType!=SIZE_MAXIMIZED)
			m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED),ID_INDICATOR_SENDLED,SBPS_NOBORDERS,0);
		else
			m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED),ID_INDICATOR_SENDLED,SBPS_NOBORDERS,10);
	}

	if (m_wndSplitter.GetSafeHwnd())
	{
		//Hide the queue if visible
		m_wndSplitter.HideColumn(1, 0);
	}
	//Now only the main splitter gets resized
	CFrameWnd::OnSize(nType, cx, cy);
	if (m_wndSplitter.GetSafeHwnd())
	{
		//Restore the queue
		m_wndSplitter.ShowColumn(1);
	}
	
	if (m_wndStatusBar.GetSafeHwnd())
	{
		RECT rc;

		m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVLED), &rc);

		// Reposition the first LED correctly!
		m_RecvLed.SetWindowPos(&wndTop, rc.left, rc.top+1, rc.right - rc.left,
			rc.bottom - rc.top, 0); 

		m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDLED), &rc);
	
		// Reposition the second LED correctly!
		m_SendLed.SetWindowPos(&wndTop, rc.left, rc.top+1, rc.right - rc.left,
				rc.bottom - rc.top, 0); 
	}
}

void CMainFrame::OnClose() 
{
	CFrameWnd::OnClose();
}

void CMainFrame::OnEditSettings() 
{
	SendCommand(5, 0, 0);
	ShowStatus(_T("Retrieving settings, please wait..."), 0);
}

void CMainFrame::OnActive() 
{
	CStatusView *view=(CStatusView *)m_wndSplitter.GetPane(0,0);
	if (m_nServerState&1 && !(m_nServerState&14))
	{
		if (!GetUsersPane()->m_pListCtrl->GetItemCount())
		{
			if (AfxMessageBox("Do you really want to take the server offline?", MB_YESNO|MB_ICONQUESTION)!=IDYES)
				return;
			int nServerState=m_nServerState|2;
			unsigned char buffer[2];
			buffer[0]=nServerState/256;
			buffer[1]=nServerState%256;
			SendCommand(2, buffer, 2);
			view->ShowStatus("Server is going offline...", 0);
			return;
		}
		else
		{
			COfflineAskDlg dlg;
			if (dlg.DoModal()!=IDOK)
				return;
			if (dlg.m_nRadio==2)
			{
				int nServerState=m_nServerState|16;
				unsigned char buffer[2];
				buffer[0]=nServerState/256;
				buffer[1]=nServerState%256;
				SendCommand(2, buffer, 2);
				view->ShowStatus("Server is going offline...", 0);
				return;
			}
			if (dlg.m_nRadio==1)
			{
				int nServerState=m_nServerState|8;
				unsigned char buffer[2];
				buffer[0]=nServerState/256;
				buffer[1]=nServerState%256;
				SendCommand(2, buffer, 2);
				view->ShowStatus("Server is going offline...", 0);				
				return;
			}				
		}
		int nServerState=m_nServerState|2;
		unsigned char buffer[2];
		buffer[0]=nServerState/256;
		buffer[1]=nServerState%256;
		SendCommand(2, buffer, 2);
		view->ShowStatus("Server is going offline...", 0);
		return;
	}
	else
	{
		int nServerState=1 + (m_nServerState&16);
		unsigned char buffer[2];
		buffer[0]=nServerState/256;
		buffer[1]=nServerState%256;
		SendCommand(2, buffer, 2);
	}
}

void CMainFrame::OnUpdateActive(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
	pCmdUI->SetCheck(m_nServerState&1 && !(m_nServerState&14));
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	CFrameWnd::OnSysCommand(nID, lParam);
	if (nID == SC_MINIMIZE)
		ShowWindow(SW_HIDE);
	else if (nID == SC_RESTORE)
		ShowWindow(SW_SHOW);
}

//// SetupTrayIcon /////////////////////////////////////////////////////
// If we're minimized, create an icon in the systray.  Otherwise, remove
// the icon, if one is present.

void CMainFrame::SetupTrayIcon()
{
	m_TrayIcon.Create(0, nTrayNotificationMsg_, "FileZilla Server",
		0, IDR_SYSTRAY_MENU);
	m_TrayIcon.SetIcon(IDI_UNKNOWN);
}


//// SetupTaskBarButton ////////////////////////////////////////////////
// Show or hide the taskbar button for this app, depending on whether
// we're minimized right now or not.

void CMainFrame::OnTrayExit() 
{
	if (!m_bQuit)
		OnClose();	
}

void CMainFrame::OnTrayRestore() 
{
	ShowWindow(SW_RESTORE);
	ShowWindow(SW_SHOW);
}

void CMainFrame::SetIcon()
{
	if (!m_pAdminSocket || !m_pAdminSocket->IsConnected())
	{
		m_TrayIcon.StopAnimation();
		m_TrayIcon.SetIcon(IDI_UNKNOWN);
	}
	else if (!(m_nServerState&1) || m_nServerState&14)
	{
		if (!GetUsersPane()->m_pListCtrl->GetItemCount())
		{	
			m_TrayIcon.StopAnimation();
			m_TrayIcon.SetIcon(IDI_RED);
		}
		else
		{
			m_TrayIcon.SetIconList(IDI_GREEN, IDI_RED);
			m_TrayIcon.Animate(500);
		}
	}
	else if (m_nServerState&16)
	{
		if (GetUsersPane()->m_pListCtrl->GetItemCount())
		{	
			m_TrayIcon.SetIconList(IDI_GREEN, IDI_YELLOW);
			m_TrayIcon.Animate(300);
		}
		else
		{
			m_TrayIcon.SetIconList(IDI_YELLOW, IDI_RED);
			m_TrayIcon.Animate(500);
		}

	}
	else
	{
		m_TrayIcon.StopAnimation();
		m_TrayIcon.SetIcon(GetUsersPane()->m_pListCtrl->GetItemCount()?IDI_GREEN:IDI_YELLOW);
	}

}

void CMainFrame::OnLock() 
{
	if (!(m_nServerState&1) || m_nServerState&14)
		return;
	if (m_nServerState&16)
	{
		int nServerState=m_nServerState&15;
		unsigned char buffer[2];
		buffer[0]=nServerState/256;
		buffer[1]=nServerState%256;
		SendCommand(2, buffer, 2);
	}
	else
	{
		if (AfxMessageBox("Do you really want to lock the server? No new connenctions will be accepted while locked.", MB_YESNO|MB_ICONQUESTION)!=IDYES)
			return;
		int nServerState=(m_nServerState&15) + 16;
		unsigned char buffer[2];
		buffer[0]=nServerState/256;
		buffer[1]=nServerState%256;
		SendCommand(2, buffer, 2);
		ShowStatus("Server locked", 0);	
	}	
}

void CMainFrame::OnUpdateLock(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected() && m_nServerState&1 && !(m_nServerState&14));
	pCmdUI->SetCheck((m_nServerState&16)?1:0);
	
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent==m_nTimerID)
	{
		SendCommand(8);
		m_TrayIcon.RefreshIcon();
	}
	else if (nIDEvent==m_nRateTimerID)
	{
		__int64 diff=m_nSendCount-m_nOldSendCount;
		if (m_lastwritediff && diff)
		{
			__int64 tmp=diff;
			diff=(diff+m_lastwritediff)/2;
			m_lastwritediff=tmp;
		}
		else
			m_lastwritediff=diff;

		int span=GetTickCount()-m_lastchecktime;
		diff=(__int64)((double)diff*1000/span);

		CString str="";
		CString digit;
		while (diff)
		{
			digit = (TCHAR)('0' + static_cast<TCHAR>(diff%10));
			str=digit+str;
			diff/=10;
		}
		if (str=="")
			str="0";

		int shift=0;
		while (str.GetLength()>4)
		{
			shift++;
			str=str.Left(str.GetLength()-1);
		}
		if (shift%3)
			str=str.Left((shift%3)+1)+","+str.Right(str.GetLength()-(shift%3)-1);
		shift+=2;
		shift/=3;
		if (!shift)
			str+=" B/s";
		else if (shift==1)
			str+=" KB/s";
		else if (shift==2)
			str+=" MB/s";
		else if (shift==3)
			str+=" GB/s";
		else if (shift==4)
			str+=" TB/s";
		else
			str="n/a"; //If this happens, you really have a fast connection
		SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDRATE),str);
		m_nOldSendCount=m_nSendCount;
		
		diff=m_nRecvCount-m_nOldRecvCount;
		if (diff)
		{
			int x=0;
		}
		if (m_lastreaddiff && diff)
		{
			__int64 tmp=diff;
			diff=(diff+m_lastreaddiff)/2;
			m_lastreaddiff=tmp;
		}
		else
			m_lastreaddiff=diff;
		
		span=GetTickCount()-m_lastchecktime;
		diff=(__int64)((double)diff*1000/span);

		str=""; //diff in String umwandeln
		while (diff)
		{
			digit = (TCHAR)('0' + static_cast<TCHAR>(diff%10));
			str=digit+str;
			diff/=10;
		}
		if (str=="")
			str="0";

		shift=0;
		while (str.GetLength()>4)
		{
			shift++;
			str=str.Left(str.GetLength()-1);
		}
		if (shift%3)
			str=str.Left((shift%3)+1)+","+str.Right(str.GetLength()-(shift%3)-1);
		shift+=2;
		shift/=3;
		if (!shift)
			str+=" B/s";
		else if (shift==1)
			str+=" KB/s";
		else if (shift==2)
			str+=" MB/s";
		else if (shift==3)
			str+=" GB/s";
		else if (shift==4)
			str+=" TB/s";
		else
			str="n/a"; //If this happens, you really have a fast connection
		SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVRATE),str);
		m_lastchecktime=GetTickCount();
		m_nOldRecvCount=m_nRecvCount;

	}

	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::SetStatusbarText(int nIndex,CString str)
{
	m_wndStatusBar.GetStatusBarCtrl().SetText(str,nIndex,0);
	HFONT hFont = (HFONT)m_wndStatusBar.SendMessage(WM_GETFONT);
	if (str=="")
	{
		str.LoadString(m_wndStatusBar.GetItemID(nIndex));		
	}
	CClientDC dcScreen(NULL);
	HGDIOBJ hOldFont = NULL;
	if (hFont != NULL)
	hOldFont = dcScreen.SelectObject(hFont);
	int cx=dcScreen.GetTextExtent(str).cx;
	int cxold;
	unsigned int nID,nStyle;
	m_wndStatusBar.GetPaneInfo(nIndex,nID,nStyle,cxold);
	if (cx!=cxold)
	{
		if (cx<cxold)
			cx=cxold;
		m_wndStatusBar.SetPaneInfo(nIndex,nID,nStyle,cx);
	
	}
	if (hOldFont != NULL)
		dcScreen.SelectObject(hOldFont);

}

void CMainFrame::OnMenuEditUsers() 
{
	if (m_nEdit)
	{
		MessageBeep(MB_OK);
		return;
	}
	SendCommand(6, 0, 0);
	m_nEdit |= 1;
	ShowStatus(_T("Retrieving account settings, please wait..."), 0);
}

void CMainFrame::OnMenuEditGroups() 
{
	if (m_nEdit)
	{
		MessageBeep(MB_OK);
		return;
	}
	SendCommand(6, 0, 0);
	m_nEdit |= 2;
	ShowStatus(_T("Retrieving account settings, please wait..."), 0);
}

void CMainFrame::ShowStatus(LPCTSTR status, int nType)
{
	CStatusView *view=(CStatusView *)m_wndSplitter.GetPane(0,0);
	view->ShowStatus(status, nType);
}

void CMainFrame::ParseReply(int nReplyID, unsigned char *pData, int nDataLength)
{
	switch(nReplyID)
	{
	case 0:
		{
			ShowStatus(_T("Logged on"), 0);
			SendCommand(2);
			unsigned char buffer = USERCONTROL_GETLIST;
			SendCommand(3, &buffer, 1);
		}
		break;
	case 1:
		{
			char *pBuffer = new char[nDataLength];
			memcpy(pBuffer, pData+1, nDataLength-1);
			pBuffer[nDataLength-1] = 0;
			ShowStatus(pBuffer, *pData);
			delete [] pBuffer;
		}
		break;
	case 2:
		m_nServerState = *pData*256 + pData[1];
		SetIcon();
		break;
	case 3:
		{
			if (nDataLength<2)
			{
				ShowStatus(_T("Protocol error: Unexpected data length"), 1);
				return;
			}
			else if (!GetUsersPane()->m_pListCtrl->ParseUserControlCommand(pData, nDataLength))
				ShowStatus(_T("Protocol error: Invalid data"), 1);
			else
				SetIcon();
		}
		break;
	case 5:
		if (nDataLength == 1)
		{
			if (*pData == 0)
				ShowStatus(_T("Done sending settings."), 0);
			else if (*pData == 1)
				ShowStatus(_T("Could not change settings"), 1);
			break;
		}
		ShowStatus(_T("Done retrieving settings"), 0);
		if (nDataLength<2)
			ShowStatus(_T("Protocol error: Unexpected data length"), 1);
		else
		{
			COptionsDlg dlg(m_pOptions);
			if (!dlg.Init(pData, nDataLength))
				ShowStatus(_T("Protocol error: Invalid data"), 1);
			else if (dlg.Show())
			{
				char *pBuffer;
				DWORD dwBufferLength;
				if (dlg.GetAsCommand(&pBuffer, &dwBufferLength))
				{
					SendCommand(5, pBuffer, dwBufferLength);
					ShowStatus(_T("Sending settings, please wait..."), 0);
					delete [] pBuffer;
				}
			}
		}
		break;
	case 6:
		if (nDataLength == 1)
		{
			if (*pData == 0)
				ShowStatus(_T("Done sending account settings."), 0);
			else if (*pData == 1)
				ShowStatus(_T("Could not change account settings"), 1);
			break;
		}
		ShowStatus(_T("Done retrieving account settings"), 0);
		if (nDataLength<2)
			ShowStatus(_T("Protocol error: Unexpected data length"), 1);
		else
		{
			if (m_nEdit & 0x01)
			{
				CUsersDlg dlg;
				if (!dlg.Init(pData, nDataLength))
				{
					ShowStatus(_T("Protocol error: Invalid data"), 1);
					break;
				}
				else if (dlg.DoModal() == IDOK)
				{
					char *pBuffer;
					DWORD dwBufferLength;
					if (dlg.GetAsCommand(&pBuffer, &dwBufferLength))
					{
						SendCommand(6, pBuffer, dwBufferLength);
						ShowStatus(_T("Sending account settings, please wait..."), 0);
						delete [] pBuffer;
					}
				}
				m_nEdit ^= 0x01;
			}
			if (m_nEdit & 0x02)
			{
				CGroupsDlg dlg;
				if (!dlg.Init(pData, nDataLength))
				{
					ShowStatus(_T("Protocol error: Invalid data"), 1);
					break;
				}
				else if (dlg.DoModal() == IDOK)
				{
					char *pBuffer;
					DWORD dwBufferLength;
					if (dlg.GetAsCommand(&pBuffer, &dwBufferLength))
					{
						SendCommand(6, pBuffer, dwBufferLength);
						ShowStatus(_T("Sending account settings, please wait..."), 0);
						delete [] pBuffer;
					}
				}
				m_nEdit ^= 0x02;
			}
		}
		break;
	case 8:
		break;
	default:
		{
			CString str;
			str.Format(_T("Protocol error: Unexpected reply id (%d)."), nReplyID);
			ShowStatus(str, 1);
			break;
		}
	}
}

void CMainFrame::ParseStatus(int nStatusID, unsigned char *pData, int nDataLength)
{
	switch(nStatusID)
	{
	case 1:
		{
			char *pBuffer = new char[nDataLength];
			memcpy(pBuffer, pData+1, nDataLength-1);
			pBuffer[nDataLength-1] = 0;
			ShowStatus(pBuffer, *pData);
			delete [] pBuffer;
		}
		break;
	case 2:
		m_nServerState = *pData*256 + pData[1];
		SetIcon();
		break;
	case 3:
		{
			if (nDataLength<2)
			{
				ShowStatus(_T("Protocol error: Unexpected data length"), 1);
				return;
			}
			else if (!GetUsersPane()->m_pListCtrl->ParseUserControlCommand(pData, nDataLength))
				ShowStatus(_T("Protocol error: Invalid data"), 1);
			else
				SetIcon();
		}
		break;
	case 4:
		{
			_int64 time;
			FILETIME fFileTime;
			SYSTEMTIME sFileTime;
			memcpy(&time, pData+1, 8);
			fFileTime.dwHighDateTime = (DWORD)(time>>32);
			fFileTime.dwLowDateTime = (DWORD)(time %0xFFFFFFFF);
			FileTimeToSystemTime(&fFileTime, &sFileTime);
			char text[80];
			if (!GetDateFormat(
				LOCALE_USER_DEFAULT,               // locale for which date is to be formatted
				DATE_SHORTDATE,             // flags specifying function options
				&sFileTime,  // date to be formatted
				0,          // date format string
				text,          // buffer for storing formatted string
				80                // size of buffer
				))
				break;
			CString text2=" ";
			text2+=text;
			
			if (!GetTimeFormat(
				LOCALE_USER_DEFAULT,               // locale for which date is to be formatted
				TIME_FORCE24HOURFORMAT,             // flags specifying function options
				&sFileTime,  // date to be formatted
				0,          // date format string
				text,          // buffer for storing formatted string
				80                // size of buffer
				))
				break;
			text2+=" ";
			text2+=text;
			CString str;
			memcpy(str.GetBuffer(nDataLength-9), pData+9, nDataLength-9);
			str.ReleaseBuffer(nDataLength-9);
			int pos=str.Find("-");
			if (pos!=-1)
			{
				str.Insert(pos, text2 + " ");
			}
			ShowStatus(str, *pData);
		}
		break;
	case 7:
		if (nDataLength!=5)
			ShowStatus(_T("Protocol error: Invalid data"), 1);
		else
		{
			int nType = *pData;
			int size;
			memcpy(&size, pData+1, 4);

			if (!nType)
			{
				m_nRecvCount += size;
				m_RecvLed.Ping(100);
				CString str;
				str.Format("%I64d bytes received",m_nRecvCount);
				SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_RECVCOUNT),str);
			}
			else
			{
				m_nSendCount += size;
				m_SendLed.Ping(100);
				CString str;
				str.Format("%I64d bytes sent",m_nSendCount);
				SetStatusbarText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SENDCOUNT),str);
			}

		}
		break;
	default:
		{
			CString str;
			str.Format(_T("Protocol error: Unexpected status id (%d)."), nStatusID);
			ShowStatus(str, 1);
		}
		break;

	}
}

BOOL CMainFrame::SendCommand(int nType)
{
	if (!m_pAdminSocket)
		return FALSE;
	if (!m_pAdminSocket->SendCommand(nType))
	{
		CloseAdminSocket();
		ShowStatus("Error: Connection to server lost...", 1);
		return FALSE;
	}
	return TRUE;
}

BOOL CMainFrame::SendCommand(int nType, void *pData, int nDataLength)
{
	if (!m_pAdminSocket)
		return FALSE;
	if (!m_pAdminSocket->SendCommand(nType, pData, nDataLength))
	{
		CloseAdminSocket();
		ShowStatus("Error: Connection to server lost...", 1);
		return FALSE;
	}
	return TRUE;
}

void CMainFrame::CloseAdminSocket()
{
	if (m_pAdminSocket)
	{
		m_pAdminSocket->Close();
		delete m_pAdminSocket;
		m_pAdminSocket = NULL;
		SetIcon();
	}
	m_nEdit = 0;
}

void CMainFrame::OnFileConnect() 
{
	if (m_pAdminSocket)
		if (AfxMessageBox(_T("Do you really want to close the current connection?"), MB_ICONQUESTION|MB_YESNO) != IDYES)
			return;
	CConnectDialog dlg(m_pOptions);
	if (dlg.DoModal() == IDOK)
	{
		if (m_pAdminSocket)
			CloseAdminSocket();
		m_pAdminSocket = new CAdminSocket(this);
		CStatusView *view=(CStatusView *)m_wndSplitter.GetPane(0,0);
		view->ShowStatus("Connecting to server...", 0);
		m_pAdminSocket->m_Password = m_pOptions->GetOption(IOPTION_LASTSERVERPASS);
		m_pAdminSocket->Create();
		if (!m_pAdminSocket->Connect(m_pOptions->GetOption(IOPTION_LASTSERVERADDRESS), m_pOptions->GetOptionVal(IOPTION_LASTSERVERPORT)) && WSAGetLastError()!=WSAEWOULDBLOCK)
		{
			ShowStatus(_T("Error, could not connect to server"), 1);
			CloseAdminSocket();
		}
	}
}

void CMainFrame::OnFileDisconnect() 
{
	CloseAdminSocket();
}

void CMainFrame::OnUpdateFileDisconnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket?TRUE:FALSE);	
}

void CMainFrame::OnUpdateEditSettings(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
}

void CMainFrame::OnUpdateMenuEditUsers(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
}

void CMainFrame::OnUpdateMenuEditGroups(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
}

void CMainFrame::OnUpdateUsers(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
}

void CMainFrame::OnUpdateGroups(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pAdminSocket && m_pAdminSocket->IsConnected());
}

void CMainFrame::OnDestroy() 
{
	CloseAdminSocket();
	CFrameWnd::OnDestroy();	
}
