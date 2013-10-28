// Win32++   Version 7.2
// Released: 5th AUgust 2011
//
//      David Nash
//      email: dnash@bigpond.net.au
//      url: https://sourceforge.net/projects/win32-framework
//
//
// Copyright (c) 2005-2011  David Nash
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////


///////////////////////////////////////////////////////
// wincore.h
//  Declaration of the following classes:
//  CWinApp, CWnd, CWinException, CCriticalSection,
//  CPoint, CRect, and CSize
//
// This file contains the declarations for the core set of classes required to
// create simple windows using Win32++.
//
// 1) CCriticalSection: This class is used internally to manage thread access
//            to shared resources. You can also use this class to lock and
//            release your own critical sections.
//
// 2) CWinException: This class is used internally by Win32++ to handle
//            exceptions. You can also use it to throw and catch exceptions.
//
// 3) WinApp: This class is used start Win32++ and run the message loop. You
//            should inherit from this class to start Win32++ in your own
//            application.
//
// 4) CWnd:   This class is used to represent a window. It provides a means
//            of creating the window, and handling its messages. Inherit
//            from this class to define and control windows.
//
//
// Note: This header file (or another Win32++ header file which includes it)
//       should be included before all other header files. It sets some
//       important macros which need to be set before including Windows.h
//       Including this file first also allows it to disable some pointless
//       warning messages (see below).



#ifndef _WIN32XX_WINCORE_H_
#define _WIN32XX_WINCORE_H_


// Remove pointless warning messages
#ifdef _MSC_VER
  #pragma warning (disable : 4996) // function or variable may be unsafe (deprecated)
  #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS // eliminate deprecation warnings for VS2005/VS2010
  #endif
  #if _MSC_VER < 1500
    #pragma warning (disable : 4511) // copy operator could not be generated
    #pragma warning (disable : 4512) // assignment operator could not be generated
    #pragma warning (disable : 4702) // unreachable code (bugs in Microsoft's STL)
    #pragma warning (disable : 4786) // identifier was truncated
  #endif
#endif

#ifdef __BORLANDC__
  #pragma option -w-8019			// code has no effect
  #pragma option -w-8026            // functions with exception specifiations are not expanded inline
  #pragma option -w-8027		    // function not expanded inline
  #define STRICT 1
#endif

#ifdef __GNUC__
  #pragma GCC diagnostic ignored "-Wmissing-braces"
  #pragma GCC diagnostic ignored "-Wunused-value"
#endif

#ifdef _WIN32_WCE
  #include "wcestddef.h"
#endif

#define _WINSOCKAPI_            // Prevent winsock.h #include's.

#include <assert.h>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <shlwapi.h>
#include "shared_ptr.h"
//#include "winutils.h"			// included later in this file
//#include "cstring.h"			// included later in this file
//#include "gdi.h"				// included later in this file
//#include "menu.h"				// included later in this file

// For compilers lacking Win64 support
#ifndef  GetWindowLongPtr
  #define GetWindowLongPtr   GetWindowLong
  #define SetWindowLongPtr   SetWindowLong
  #define GWLP_WNDPROC       GWL_WNDPROC
  #define GWLP_HINSTANCE     GWL_HINSTANCE
  #define GWLP_ID            GWL_ID
  #define GWLP_USERDATA      GWL_USERDATA
  #define DWLP_DLGPROC       DWL_DLGPROC
  #define DWLP_MSGRESULT     DWL_MSGRESULT
  #define DWLP_USER          DWL_USER
  #define DWORD_PTR          DWORD
  #define LONG_PTR           LONG
  #define ULONG_PTR          LONG
#endif
#ifndef GetClassLongPtr
  #define GetClassLongPtr    GetClassLong
  #define SetClassLongPtr    SetClassLong
  #define GCLP_HBRBACKGROUND GCL_HBRBACKGROUND
  #define GCLP_HCURSOR       GCL_HCURSOR
  #define GCLP_HICON         GCL_HICON
  #define GCLP_HICONSM       GCL_HICONSM
  #define GCLP_HMODULE       GCL_HMODULE
  #define GCLP_MENUNAME      GCL_MENUNAME
  #define GCLP_WNDPROC       GCL_WNDPROC
#endif


// Messages defined by Win32++
#define UWM_POPUPMENU		(WM_APP + 1)	// Message - creates the menubar popup menu
#define UWM_DOCK_START		(WM_APP + 2)	// Notification - about to start undocking
#define UWM_DOCK_MOVE		(WM_APP + 3)	// Notification - undocked docker is being moved
#define UWM_DOCK_END		(WM_APP + 4)	// Notification - docker has been docked
#define UWM_BAR_START		(WM_APP + 5)	// Notification - docker bar selected for move
#define UWM_BAR_MOVE		(WM_APP + 6)	// Notification - docker bar moved
#define UWM_BAR_END			(WM_APP + 7)	// Notification - end of docker bar move
#define UWM_UNDOCKED		(WM_APP + 8)	// Notification - sent by docker when undocked
#define UWM_FRAMELOSTFOCUS	(WM_APP + 9)    // Notification - sent by frame to view window when focus lost
#define UWM_FRAMEGOTFOCUS	(WM_APP + 10)   // Notification - sent by frame to view window when focus acquired
#define UWM_DOCK_DESTROYED	(WM_APP + 11)	// Message - posted when docker is destroyed
#define UWM_TAB_CHANGED     (WM_APP + 12)	// Notification - tab layout changed
#define UWM_TOOLBAR_RESIZE  (WM_APP + 13)   // Message - sent by toolbar to parent. Used by the rebar
#define UWM_UPDATE_COMMAND  (WM_APP + 14)   // Message - sent before a menu is displayed. Used by OnUpdate
#define UWM_DOCK_ACTIVATED  (WM_APP + 15)   // Message - sent to dock ancestor when a docker is activated or deactivated.
#define UWM_GETMENUTHEME    (WM_APP + 16)	// Message - returns a pointer to MenuTheme
#define UWM_GETREBARTHEME   (WM_APP + 17)	// Message - returns a pointer to CToolBar
#define UWM_GETTOOLBARTHEME (WM_APP + 18)   // Message - returns a pointer to ToolBarTheme
#define UWM_CLEANUPTEMPS	(WM_APP + 19)	// Message - posted to cleanup temporary CDCs


// Automatically include the Win32xx namespace
// define NO_USING_NAMESPACE to skip this step
namespace Win32xx {}
#ifndef NO_USING_NAMESPACE
  using namespace Win32xx;
#endif

// Required for WinCE
#ifndef TLS_OUT_OF_INDEXES
  #define TLS_OUT_OF_INDEXES ((DWORD_PTR) -1)
#endif
#ifndef WM_PARENTNOTIFY
  #define WM_PARENTNOTIFY 0x0210
#endif


namespace Win32xx
{

	////////////////////////////////////////////////
	// Forward declarations.
	//  These classes are defined later or elsewhere
	class CDC;
	class CGDIObject;
	class CMenu;
	class CWinApp;
	class CWnd;
	class CBitmap;
	class CBrush;
	class CFont;
	class CPalette;
	class CPen;
	class CRgn;

	// tString is a TCHAR std::string
	typedef std::basic_string<TCHAR> tString;

	// tStringStream is a TCHAR std::stringstream
	typedef std::basic_stringstream<TCHAR> tStringStream;

	// Some useful smart pointers
	typedef Shared_Ptr<CDC> DCPtr;
	typedef Shared_Ptr<CGDIObject> GDIPtr;
	typedef Shared_Ptr<CMenu> MenuPtr;
	typedef Shared_Ptr<CWnd> WndPtr;
	typedef Shared_Ptr<CBitmap> BitmapPtr;
	typedef Shared_Ptr<CBrush> BrushPtr;
	typedef Shared_Ptr<CFont> FontPtr;
	typedef Shared_Ptr<CPalette> PalettePtr;
	typedef Shared_Ptr<CPen> PenPtr;
	typedef Shared_Ptr<CRgn> RgnPtr;

	enum Constants			// Defines the maximum size for TCHAR strings
	{
		MAX_MENU_STRING = 80,
		MAX_STRING_SIZE = 255,
	};

	struct CompareHDC		// The comparison function object used by CWinApp::m_mapHDC
	{
		bool operator()(HDC const a, const HDC b) const
			{return ((DWORD_PTR)a < (DWORD_PTR)b);}
	};

	struct CompareGDI		// The comparison function object used by CWinApp::m_mapGDI
	{
		bool operator()(HGDIOBJ const a, const HGDIOBJ b) const
			{return ((DWORD_PTR)a < (DWORD_PTR)b);}
	};

	struct CompareHMENU		// The comparison function object used by CWinApp::m_mapHMENU
	{
		bool operator()(HMENU const a, const HMENU b) const
			{return ((DWORD_PTR)a < (DWORD_PTR)b);}
	};

	struct CompareHWND		// The comparison function object used by CWinApp::m_mapHWND
	{
		bool operator()(HWND const a, const HWND b) const
			{return ((DWORD_PTR)a < (DWORD_PTR)b);}
	};

	struct TLSData			// Used for Thread Local Storage (TLS)
	{
		CWnd* pCWnd;		// pointer to CWnd object for Window creation
		CWnd* pMenuBar;		// pointer to CMenuBar object used for the WH_MSGFILTER hook
		HHOOK hHook;		// WH_MSGFILTER hook for CMenuBar and Modeless Dialogs

		std::vector<DCPtr> vTmpDCs;		// A vector of temporary CDC pointers
		std::vector<GDIPtr> vTmpGDIs;	// A vector of temporary CGDIObject pointers
		std::vector<WndPtr> vTmpWnds;	// A vector of temporary CWnd pointers
		TLSData() : pCWnd(0), pMenuBar(0), hHook(0) {}

#ifndef _WIN32_WCE
		std::vector<MenuPtr> vTmpMenus;	// A vector of temporary CMenu pointers
#endif
	};


	/////////////////////////////////////////
	// Declarations for the CCriticalSection class
	// This class is used for thread synchronisation
	class CCriticalSection
	{
	public:
		CCriticalSection()	{ ::InitializeCriticalSection(&m_cs); }
		~CCriticalSection()	{ ::DeleteCriticalSection(&m_cs); }

		void Lock() 	{ ::EnterCriticalSection(&m_cs); }
		void Release()	{ ::LeaveCriticalSection(&m_cs); }

	private:
		CCriticalSection ( const CCriticalSection& );
		CCriticalSection& operator = ( const CCriticalSection& );

		CRITICAL_SECTION m_cs;
	};


	////////////////////////////////////////
	// Declaration of the CWinException class
	//
	// Note: Each function guarantees not to throw an exception

	class CWinException : public std::exception
	{
	public:
		CWinException(LPCTSTR pszText) throw ();
		~CWinException() throw() {}
		DWORD GetError() const throw ();
		LPCTSTR GetErrorString() const throw ();
		const char * what () const throw ();

	private:
		DWORD  m_Error;
		LPCTSTR m_pszText;
		TCHAR m_szErrorString[MAX_STRING_SIZE];
	};


	///////////////////////////////////
	// Declaration of the CWinApp class
	//
	class CWinApp
	{
		// Provide these access to CWinApp's private members: 
		friend class CDC;
		friend class CDialog;
		friend class CGDIObject;
		friend class CMenu;
		friend class CMenuBar;
		friend class CPropertyPage;
		friend class CPropertySheet;
		friend class CTaskDialog;
		friend class CWnd;
		friend CWinApp* GetApp();
		friend CGDIObject* FromHandle(HGDIOBJ hObject);
		friend CBitmap* FromHandle(HBITMAP hBitmap);
		friend CBrush* FromHandle(HBRUSH hBrush);
		friend CFont* FromHandle(HFONT hFont);
		friend CPalette* FromHandle(HPALETTE hPalette);
		friend CPen* FromHandle(HPEN hPen);
		friend CRgn* FromHandle(HRGN hRgn);
		friend CDC* FromHandle(HDC hDC);
		friend CWnd* FromHandle(HWND hWnd);
#ifndef _WIN32_WCE
		friend CMenu* FromHandle(HMENU hMenu);
#endif

		typedef Shared_Ptr<TLSData> TLSDataPtr;

	public:
		CWinApp();
		virtual ~CWinApp();

		HACCEL GetAccelerators() const { return m_hAccel; }
		HINSTANCE GetInstanceHandle() const { return m_hInstance; }
		HINSTANCE GetResourceHandle() const { return (m_hResource ? m_hResource : m_hInstance); }
		void SetAccelerators(HACCEL hAccel, CWnd* pWndAccel);
		void SetResourceHandle(HINSTANCE hResource);

		// These are the functions you might wish to override
		virtual BOOL InitInstance();
		virtual int  MessageLoop();
		virtual int Run();

	protected:
		virtual BOOL OnIdle(LONG lCount);
		virtual BOOL PreTranslateMessage(MSG Msg);

	private:
		CWinApp(const CWinApp&);				// Disable copy construction
		CWinApp& operator = (const CWinApp&);	// Disable assignment operator
		CDC* GetCDCFromMap(HDC hDC);
		CGDIObject* GetCGDIObjectFromMap(HGDIOBJ hObject);		
		CMenu* GetCMenuFromMap(HMENU hMenu);
		CWnd* GetCWndFromMap(HWND hWnd);

		void	AddTmpDC(CDC* pDC);
		void	AddTmpGDI(CGDIObject* pObject);
		CMenu*	AddTmpMenu(HMENU hMenu);
		CWnd*	AddTmpWnd(HWND hWnd);
		void	CleanupTemps();
		DWORD	GetTlsIndex() const {return m_dwTlsIndex;}
		void	SetCallback();
		TLSData* SetTlsIndex();
		static CWinApp* SetnGetThis(CWinApp* pThis = 0);

		std::map<HDC, CDC*, CompareHDC> m_mapHDC;			// maps device context handles to CDC objects
		std::map<HGDIOBJ, CGDIObject*, CompareGDI> m_mapGDI;	// maps GDI handles to CGDIObjects.
		std::map<HMENU, CMenu*, CompareHMENU> m_mapHMENU;	// maps menu handles to CMenu objects
		std::map<HWND, CWnd*, CompareHWND> m_mapHWND;		// maps window handles to CWnd objects
		std::vector<TLSDataPtr> m_vTLSData;		// vector of TLSData smart pointers, one for each thread
		CCriticalSection m_csMapLock;	// thread synchronisation for m_mapHWND
		CCriticalSection m_csTLSLock;	// thread synchronisation for m_vTLSData
		CCriticalSection m_csAppStart;	// thread synchronisation for application startup
		HINSTANCE m_hInstance;			// handle to the applications instance
		HINSTANCE m_hResource;			// handle to the applications resources
		DWORD m_dwTlsIndex;				// Thread Local Storage index
		WNDPROC m_Callback;				// callback address of CWnd::StaticWndowProc
		HACCEL m_hAccel;				// handle to the accelerator table
		CWnd* m_pWndAccel;				// handle to the window for accelerator keys

	};

}

#include "winutils.h"
#include "cstring.h"


namespace Win32xx
{
	////////////////////////////////
	// Declaration of the CWnd class
	//
	class CWnd
	{
	friend class CMDIChild;
	friend class CDialog;
	friend class CPropertyPage;
	friend class CTaskDialog;
	friend class CWinApp;

	public:
		CWnd();				// Constructor
		virtual ~CWnd();	// Destructor

		// These virtual functions can be overridden
		virtual BOOL Attach(HWND hWnd);
		virtual BOOL AttachDlgItem(UINT nID, CWnd* pParent);
		virtual void CenterWindow() const;
		virtual HWND Create(CWnd* pParent = NULL);
		virtual HWND CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, CWnd* pParent, CMenu* pMenu, LPVOID lpParam = NULL);
		virtual HWND CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rc, CWnd* pParent, CMenu* pMenu, LPVOID lpParam = NULL);
		virtual void Destroy();
		virtual HWND Detach();
		virtual HICON SetIconLarge(int nIcon);
		virtual HICON SetIconSmall(int nIcon);

		// Attributes
		HWND GetHwnd() const				{ return m_hWnd; }
		WNDPROC GetPrevWindowProc() const	{ return m_PrevWindowProc; }

		// Wrappers for Win32 API functions
		// These functions aren't virtual, and shouldn't be overridden
		CDC*  BeginPaint(PAINTSTRUCT& ps) const;
		BOOL  BringWindowToTop() const;
		LRESULT CallWindowProc(WNDPROC lpPrevWndFunc, UINT Msg, WPARAM wParam, LPARAM lParam) const;
		BOOL  CheckDlgButton(int nIDButton, UINT uCheck) const;
		BOOL  CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) const;
		CWnd* ChildWindowFromPoint(POINT pt) const;
		BOOL  ClientToScreen(POINT& pt) const;
		BOOL  ClientToScreen(RECT& rc) const;
		LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) const;
		HDWP  DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) const;
		HDWP  DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, const RECT& rc, UINT uFlags) const;
		BOOL  DrawMenuBar() const;
		BOOL  EnableWindow(BOOL bEnable = TRUE) const;
		BOOL  EndPaint(PAINTSTRUCT& ps) const;
		CWnd* GetActiveWindow() const;
		CWnd* GetAncestor(UINT gaFlag = 3 /*= GA_ROOTOWNER*/) const;
		CWnd* GetCapture() const;
		ULONG_PTR GetClassLongPtr(int nIndex) const;
		CString GetClassName() const;
		CRect GetClientRect() const;
		CDC*  GetDC() const;
		CDC*  GetDCEx(HRGN hrgnClip, DWORD flags) const;
		CWnd* GetDesktopWindow() const;
		CWnd* GetDlgItem(int nIDDlgItem) const;
		UINT  GetDlgItemInt(int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned) const;
		CString GetDlgItemText(int nIDDlgItem) const;
		CWnd* GetFocus() const;
		CFont* GetFont() const;
		HICON GetIcon(BOOL bBigIcon) const;
		CWnd* GetNextDlgGroupItem(CWnd* pCtl, BOOL bPrevious) const;
		CWnd* GetNextDlgTabItem(CWnd* pCtl, BOOL bPrevious) const;
		CWnd* GetParent() const;
		BOOL  GetScrollInfo(int fnBar, SCROLLINFO& si) const;
		CRect GetUpdateRect(BOOL bErase) const;
		int GetUpdateRgn(CRgn* pRgn, BOOL bErase) const;
		CWnd* GetWindow(UINT uCmd) const;
		CDC*  GetWindowDC() const;
		LONG_PTR GetWindowLongPtr(int nIndex) const;
		CRect GetWindowRect() const;
		CString GetWindowText() const;
		int   GetWindowTextLength() const;
		void  Invalidate(BOOL bErase = TRUE) const;
		BOOL  InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE) const;
		BOOL  InvalidateRgn(CRgn* pRgn, BOOL bErase = TRUE) const;
		BOOL  IsChild(CWnd* pChild) const;
		BOOL  IsDialogMessage(LPMSG lpMsg) const;
		UINT  IsDlgButtonChecked(int nIDButton) const;
		BOOL  IsWindow() const;
		BOOL  IsWindowEnabled() const;
		BOOL  IsWindowVisible() const;
		BOOL  KillTimer(UINT_PTR uIDEvent) const;
		int   MessageBox(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) const;
		void  MapWindowPoints(CWnd* pWndTo, POINT& pt) const;
		void  MapWindowPoints(CWnd* pWndTo, RECT& rc) const;
		void  MapWindowPoints(CWnd* pWndTo, LPPOINT ptArray, UINT nCount) const;
		BOOL  MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE) const;
		BOOL  MoveWindow(const RECT& rc, BOOL bRepaint = TRUE) const;
		BOOL  PostMessage(UINT uMsg, WPARAM wParam = 0L, LPARAM lParam = 0L) const;
		BOOL  PostMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const;
		BOOL  RedrawWindow(LPCRECT lpRectUpdate = NULL, CRgn* pRgn = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN) const;
		int   ReleaseDC(CDC* pDC) const;
		BOOL  ScreenToClient(POINT& Point) const;
		BOOL  ScreenToClient(RECT& rc) const;
		LRESULT SendDlgItemMessage(int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) const;
		LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0L, LPARAM lParam = 0L) const;
		LRESULT SendMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const;
		BOOL  SendNotifyMessage(UINT Msg, WPARAM wParam, LPARAM lParam) const;
		CWnd* SetActiveWindow() const;
		CWnd* SetCapture() const;
		ULONG_PTR SetClassLongPtr(int nIndex, LONG_PTR dwNewLong) const;
		BOOL  SetDlgItemInt(int nIDDlgItem, UINT uValue, BOOL bSigned) const;
		BOOL  SetDlgItemText(int nIDDlgItem, LPCTSTR lpString) const;
		CWnd* SetFocus() const;
		void  SetFont(CFont* pFont, BOOL bRedraw = TRUE) const;
		BOOL  SetForegroundWindow() const;
		HICON SetIcon(HICON hIcon, BOOL bBigIcon) const;
		CWnd* SetParent(CWnd* pWndParent) const;
		BOOL  SetRedraw(BOOL bRedraw = TRUE) const;
		int   SetScrollInfo(int fnBar, const SCROLLINFO& si, BOOL fRedraw) const;
		UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) const;
		LONG_PTR SetWindowLongPtr(int nIndex, LONG_PTR dwNewLong) const;
		BOOL  SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) const;
		BOOL  SetWindowPos(HWND hWndInsertAfter, const RECT& rc, UINT uFlags) const;
		int   SetWindowRgn(CRgn* pRgn, BOOL bRedraw = TRUE) const;
		BOOL  SetWindowText(LPCTSTR lpString) const;
		HRESULT SetWindowTheme(LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) const;
		BOOL  ShowWindow(int nCmdShow = SW_SHOWNORMAL) const;
		BOOL  UpdateWindow() const;
		BOOL  ValidateRect(LPCRECT prc) const;
		BOOL  ValidateRgn(CRgn* pRgn) const;
		static CWnd* WindowFromPoint(POINT pt);

  #ifndef _WIN32_WCE
		BOOL  CloseWindow() const;
		int   DlgDirList(LPTSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT uFileType) const;
		int   DlgDirListComboBox(LPTSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT uFiletype) const;
		BOOL  DlgDirSelectEx(LPTSTR lpString, int nCount, int nIDListBox) const;
		BOOL  DlgDirSelectComboBoxEx(LPTSTR lpString, int nCount, int nIDComboBox) const;
		BOOL  DrawAnimatedRects(int idAni, RECT& rcFrom, RECT& rcTo) const;
		BOOL  DrawCaption(CDC* pDC, RECT& rc, UINT uFlags) const;
		BOOL  EnableScrollBar(UINT uSBflags, UINT uArrows) const;
		CWnd* GetLastActivePopup() const;
		CMenu* GetMenu() const;
		int   GetScrollPos(int nBar) const;
		BOOL  GetScrollRange(int nBar, int& MinPos, int& MaxPos) const;
		CMenu* GetSystemMenu(BOOL bRevert) const;
		CWnd* GetTopWindow() const;
		BOOL  GetWindowPlacement(WINDOWPLACEMENT& pWndpl) const;
		BOOL  HiliteMenuItem(CMenu* pMenu, UINT uItemHilite, UINT uHilite) const;
		BOOL  IsIconic() const;
		BOOL  IsZoomed() const;
		BOOL  LockWindowUpdate() const;
		BOOL  OpenIcon() const;
		void  Print(CDC* pDC, DWORD dwFlags) const;
		BOOL  SetMenu(CMenu* pMenu) const;
		BOOL  ScrollWindow(int XAmount, int YAmount, LPCRECT lprcScroll, LPCRECT lprcClip) const;
		int   ScrollWindowEx(int dx, int dy, LPCRECT lprcScroll, LPCRECT lprcClip, CRgn* prgnUpdate, LPRECT lprcUpdate, UINT flags) const;
		int   SetScrollPos(int nBar, int nPos, BOOL bRedraw) const;
		BOOL  SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw) const;
		BOOL  SetWindowPlacement(const WINDOWPLACEMENT& wndpl) const;
		BOOL  ShowOwnedPopups(BOOL fShow) const;
		BOOL  ShowScrollBar(int nBar, BOOL bShow) const;
		BOOL  ShowWindowAsync(int nCmdShow) const;
		BOOL  UnLockWindowUpdate() const;
		CWnd* WindowFromDC(CDC* pDC) const;

    #ifndef WIN32_LEAN_AND_MEAN
		void  DragAcceptFiles(BOOL fAccept) const;
    #endif
  #endif

		static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		operator HWND() const { return m_hWnd; }

	protected:
		// Override these functions as required
		virtual LRESULT FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
		virtual void OnCreate();
		virtual void OnDraw(CDC* pDC);
		virtual BOOL OnEraseBkgnd(CDC* pDC);
		virtual void OnInitialUpdate();
		virtual void OnMenuUpdate(UINT nID);
		virtual LRESULT OnMessageReflect(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotifyReflect(WPARAM wParam, LPARAM lParam);
		virtual void PreCreate(CREATESTRUCT& cs);
		virtual void PreRegisterClass(WNDCLASS& wc);
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

		HWND m_hWnd;					// handle to this object's window

	private:
		CWnd(const CWnd&);				// Disable copy construction
		CWnd& operator = (const CWnd&); // Disable assignment operator
		void AddToMap();
		void Cleanup();
		LRESULT MessageReflect(HWND hwndParent, UINT uMsg, WPARAM wParam, LPARAM lParam);
		BOOL RegisterClass(WNDCLASS& wc);
		BOOL RemoveFromMap();
		void Subclass(HWND hWnd);

		Shared_Ptr<WNDCLASS> m_pwc;		// defines initialisation parameters for PreRegisterClass
		Shared_Ptr<CREATESTRUCT> m_pcs;	// defines initialisation parameters for PreCreate and Create
		WNDPROC m_PrevWindowProc;		// pre-subclassed Window Procedure
		BOOL m_IsTmpWnd;				// True if this CWnd is a TmpWnd

	}; // class CWnd

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "gdi.h"	
#include "menu.h"

namespace Win32xx
{

	//////////////////////////////////////////
	// Definitions for the CWinException class
	//
	inline CWinException::CWinException(LPCTSTR pszText) throw () : m_Error(::GetLastError()), m_pszText(pszText)
	{
		memset(m_szErrorString, 0, MAX_STRING_SIZE * sizeof(TCHAR));

		if (m_Error != 0)
		{
			DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
			::FormatMessage(dwFlags, NULL, m_Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), m_szErrorString, MAX_STRING_SIZE-1, NULL);
		}
	}

	inline DWORD CWinException::GetError() const throw ()
	{
		return m_Error;
	}

	inline LPCTSTR CWinException::GetErrorString() const throw ()
	{
		return m_szErrorString;
	}

	inline const char * CWinException::what() const throw ()
	{
		// Sends the last error string to the debugger (typically displayed in the IDE's output window).
		::OutputDebugString(m_szErrorString);
		return "CWinException thrown";
	}


	////////////////////////////////////
	// Definitions for the CWinApp class
	//

	// To begin Win32++, inherit your application class from this one.
	// You must run only one instance of the class inherited from this.
	inline CWinApp::CWinApp() : m_Callback(NULL), m_hAccel(0), m_pWndAccel(0)
	{
		try
		{
			m_csAppStart.Lock();
			assert( 0 == SetnGetThis() );	// Test if this is the first instance of CWinApp

			m_dwTlsIndex = ::TlsAlloc();
			if (m_dwTlsIndex == TLS_OUT_OF_INDEXES)
			{
				// We only get here in the unlikely event that all TLS indexes are already allocated by this app
				// At least 64 TLS indexes per process are allowed. Win32++ requires only one TLS index.
				m_csAppStart.Release();
				throw CWinException(_T("CWinApp::CWinApp  Failed to allocate TLS Index"));
			}

			SetnGetThis(this);
			m_csAppStart.Release();

			// Set the instance handle
	#ifdef _WIN32_WCE
			m_hInstance = (HINSTANCE)GetModuleHandle(0);
	#else
			MEMORY_BASIC_INFORMATION mbi = {0};
			VirtualQuery( (LPCVOID)SetnGetThis, &mbi, sizeof(mbi) );
			assert(mbi.AllocationBase);
			m_hInstance = (HINSTANCE)mbi.AllocationBase;
	#endif

			m_hResource = m_hInstance;
			SetCallback();
		}

		catch (const CWinException &e)
		{
			e.what();
			throw;
		}
	}

	inline CWinApp::~CWinApp()
	{
		std::vector<TLSDataPtr>::iterator iter;
		for (iter = m_vTLSData.begin(); iter < m_vTLSData.end(); ++iter)
		{
			(*iter)->vTmpDCs.clear();
#ifndef _WIN32_WCE
			(*iter)->vTmpMenus.clear();
#endif
			(*iter)->vTmpWnds.clear();
		}

		// Check that all CWnd windows are destroyed
		std::map<HWND, CWnd*, CompareHWND>::iterator m;
		for (m = m_mapHWND.begin(); m != m_mapHWND.end(); ++m)
		{
			HWND hWnd = (*m).first;
			if (::IsWindow(hWnd))
				::DestroyWindow(hWnd);
		}
		m_mapHWND.clear();
		m_mapGDI.clear();
		m_mapHDC.clear();
		m_mapHMENU.clear();

		// Do remaining tidy up
		if (m_dwTlsIndex != TLS_OUT_OF_INDEXES)
		{
			::TlsSetValue(GetTlsIndex(), NULL);
			::TlsFree(m_dwTlsIndex);
		}

		SetnGetThis((CWinApp*)-1);
	}

	inline void CWinApp::AddTmpDC(CDC* pDC)
	{
		// The TmpMenus are created by GetSybMenu.
		// They are removed by CleanupTemps
		assert(pDC);

		// Ensure this thread has the TLS index set
		TLSData* pTLSData = GetApp()->SetTlsIndex();
		pTLSData->vTmpDCs.push_back(pDC); // save pDC as a smart pointer
	}

	inline void CWinApp::AddTmpGDI(CGDIObject* pObject)
	{
		// The temporary CGDIObjects are removed by CleanupTemps
		assert(pObject);
	
		// Ensure this thread has the TLS index set
		TLSData* pTLSData = GetApp()->SetTlsIndex();
		pTLSData->vTmpGDIs.push_back(pObject); // save pObject as a smart pointer
	}

#ifndef _WIN32_WCE
	inline CMenu* CWinApp::AddTmpMenu(HMENU hMenu)
	{
		// The TmpMenus are created by GetSybMenu.
		// They are removed by CleanupTemps
		assert(::IsMenu(hMenu));
		assert(!GetCMenuFromMap(hMenu));

		CMenu* pMenu = new CMenu;
		pMenu->m_hMenu = hMenu;
		m_csMapLock.Lock();
		m_mapHMENU.insert(std::make_pair(hMenu, pMenu));
		m_csMapLock.Release();
		pMenu->m_IsTmpMenu = TRUE;

		// Ensure this thread has the TLS index set
		TLSData* pTLSData = GetApp()->SetTlsIndex();
		pTLSData->vTmpMenus.push_back(pMenu); // save pMenu as a smart pointer
		return pMenu;
	}
#endif

	inline CWnd* CWinApp::AddTmpWnd(HWND hWnd)
	{
		// TmpWnds are created if required to support functions like CWnd::GetParent.
		// They are removed by CleanupTemps
		assert(::IsWindow(hWnd));
		assert(!GetCWndFromMap(hWnd));

		CWnd* pWnd = new CWnd;
		pWnd->m_hWnd = hWnd;
		pWnd->AddToMap();
		pWnd->m_IsTmpWnd = TRUE;

		// Ensure this thread has the TLS index set
		TLSData* pTLSData = GetApp()->SetTlsIndex();
		pTLSData->vTmpWnds.push_back(pWnd); // save pWnd as a smart pointer
		return pWnd;
	}

	inline void CWinApp::CleanupTemps()
	// Removes all Temporary CWnds and CMenus belonging to this thread
	{
		// Retrieve the pointer to the TLS Data
		TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
		assert(pTLSData);

		pTLSData->vTmpDCs.clear();
		pTLSData->vTmpGDIs.clear();
		pTLSData->vTmpWnds.clear();


	#ifndef _WIN32_WCE
		pTLSData->vTmpMenus.clear();
	#endif
	}

	inline CDC* CWinApp::GetCDCFromMap(HDC hDC)
	{
		// Allocate an iterator for our HWND map
		std::map<HDC, CDC*, CompareHDC>::iterator m;

		// Find the CDC pointer mapped to this HDC
		CDC* pDC = 0;
		m_csMapLock.Lock();
		m = m_mapHDC.find(hDC);

		if (m != m_mapHDC.end())
			pDC = m->second;

		m_csMapLock.Release();
		return pDC;
	}

	inline CGDIObject* CWinApp::GetCGDIObjectFromMap(HGDIOBJ hObject)
	{
		// Allocate an iterator for our HWND map
		std::map<HGDIOBJ, CGDIObject*, CompareGDI>::iterator m;

		// Find the CGDIObject pointer mapped to this HGDIOBJ
		CGDIObject* pObject = 0;
		m_csMapLock.Lock();
		m = m_mapGDI.find(hObject);

		if (m != m_mapGDI.end())
			pObject = m->second;

		m_csMapLock.Release();
		return pObject;
	}

	inline CMenu* CWinApp::GetCMenuFromMap(HMENU hMenu)
	{
		std::map<HMENU, CMenu*, CompareHMENU>::iterator m;

		// Find the CMenu pointer mapped to this HMENU
		CMenu* pMenu = 0;
		m_csMapLock.Lock();
		m = m_mapHMENU.find(hMenu);

		if (m != m_mapHMENU.end())
			pMenu = m->second;

		m_csMapLock.Release();
		return pMenu;
	}

	inline CWnd* CWinApp::GetCWndFromMap(HWND hWnd)
	{
		// Allocate an iterator for our HWND map
		std::map<HWND, CWnd*, CompareHWND>::iterator m;

		// Find the CWnd pointer mapped to this HWND
		CWnd* pWnd = 0;
		m_csMapLock.Lock();
		m = m_mapHWND.find(hWnd);

		if (m != m_mapHWND.end())
			pWnd = m->second;

		m_csMapLock.Release();
		return pWnd;
	}

	inline BOOL CWinApp::InitInstance()
	{
		// InitInstance contains the initialization code for your application
		// You should override this function with the code to run when the application starts.

		// return TRUE to indicate success. FALSE will end the application
		return TRUE;
	}

	inline int CWinApp::MessageLoop()
	{
		// This gets any messages queued for the application, and dispatches them.
		MSG Msg = {0};
		int status = 1;
		LONG lCount = 0;

		while (status != 0)
		{		
			// While idle, perform idle processing until OnIdle returns FALSE
			while (!::PeekMessage(&Msg, 0, 0, 0, PM_NOREMOVE) && OnIdle(lCount) == TRUE)
			{
				++lCount;
			}
			
			lCount = 0;

			// Now wait until we get a message
			if ((status = ::GetMessage(&Msg, NULL, 0, 0)) == -1)
				return -1;

			if (!PreTranslateMessage(Msg))
			{
				::TranslateMessage(&Msg);
				::DispatchMessage(&Msg);
			}
		}
		
		return LOWORD(Msg.wParam);
	}

	inline BOOL CWinApp::OnIdle(LONG lCount)
	{
		if (lCount == 0)
			CleanupTemps();

		return FALSE;
	}

	inline BOOL CWinApp::PreTranslateMessage(MSG Msg)
	{
		// This functions is called by the MessageLoop. It processes the
		// keyboard accelerator keys and calls CWnd::PreTranslateMessage for
		// keyboard and mouse events.

		BOOL Processed = FALSE;

		// only pre-translate mouse and keyboard input events
		if ((Msg.message >= WM_KEYFIRST && Msg.message <= WM_KEYLAST) ||
			(Msg.message >= WM_MOUSEFIRST && Msg.message <= WM_MOUSELAST))
		{
			// Process keyboard accelerators
			if (m_pWndAccel && ::TranslateAccelerator(*m_pWndAccel, m_hAccel, &Msg))
				Processed = TRUE;
			else
			{
				// Search the chain of parents for pretranslated messages.
				for (HWND hWnd = Msg.hwnd; hWnd != NULL; hWnd = ::GetParent(hWnd))
				{
					CWnd* pWnd = GetCWndFromMap(hWnd);
					if (pWnd)
					{
						Processed = pWnd->PreTranslateMessage(&Msg);
						if(Processed)
							break;
					}
				}
			}
		}

		return Processed;
	}

	inline int CWinApp::Run()
	{
		// InitInstance runs the App's initialization code
		if (InitInstance())
		{
			// Dispatch the window messages
			return MessageLoop();
		}
		else
		{
			TRACE(_T("InitInstance failed!  Terminating program\n"));
			::PostQuitMessage(-1);
			return -1;
		}
	}

	inline void CWinApp::SetAccelerators(HACCEL hAccel, CWnd* pWndAccel)
	// nID is the resource ID of the accelerator table
	// pWndAccel is the window pointer for translated messages
	{
		assert (hAccel);
		assert (pWndAccel);

		m_pWndAccel = pWndAccel;
		m_hAccel = hAccel;
	}

	inline void CWinApp::SetCallback()
	{
		// Registers a temporary window class so we can get the callback
		// address of CWnd::StaticWindowProc.
		// This technique works for all Window versions, including WinCE.

		WNDCLASS wcDefault = {0};

		LPCTSTR szClassName		= _T("Win32++ Temporary Window Class");
		wcDefault.hInstance		= GetInstanceHandle();
		wcDefault.lpfnWndProc	= CWnd::StaticWindowProc;
		wcDefault.lpszClassName = szClassName;

		::RegisterClass(&wcDefault);

		// Retrieve the class information
		ZeroMemory(&wcDefault, sizeof(wcDefault));
		::GetClassInfo(GetInstanceHandle(), szClassName, &wcDefault);

		// Save the callback address of CWnd::StaticWindowProc
		assert(wcDefault.lpfnWndProc);	// Assert fails when running UNICODE build on ANSI OS.
		m_Callback = wcDefault.lpfnWndProc;
		::UnregisterClass(szClassName, GetInstanceHandle());
	}

	inline CWinApp* CWinApp::SetnGetThis(CWinApp* pThis /*= 0*/)
	{
		// This function stores the 'this' pointer in a static variable.
		// Once stored, it can be used later to return the 'this' pointer.
		// CWinApp's Destructor calls this function with a value of -1.

		static CWinApp* pWinApp = 0;

		if ((CWinApp*)-1 == pThis)
			pWinApp = 0;
		else if (0 == pWinApp)
			pWinApp = pThis;

		return pWinApp;
	}

	inline void CWinApp::SetResourceHandle(HINSTANCE hResource)
	{
		// This function can be used to load a resource dll.
		// A resource dll can be used to define resources in different languages.
		// To use this function, place code like this in InitInstance
		//
		// HINSTANCE hResource = LoadLibrary(_T("MyResourceDLL.dll"));
		// SetResourceHandle(hResource);

		m_hResource = hResource;
	}

	inline TLSData* CWinApp::SetTlsIndex()
	{
		TLSData* pTLSData = (TLSData*)::TlsGetValue(GetTlsIndex());
		if (NULL == pTLSData)
		{
			pTLSData = new TLSData;

			m_csTLSLock.Lock();
			m_vTLSData.push_back(pTLSData);	// store as a Shared_Ptr
			m_csTLSLock.Release();

			::TlsSetValue(GetTlsIndex(), pTLSData);
		}

		return pTLSData;
	}


	////////////////////////////////////////
	// Definitions for the CWnd class
	//
	inline CWnd::CWnd() : m_hWnd(NULL), m_PrevWindowProc(NULL), m_IsTmpWnd(FALSE)
	{
		// Note: m_hWnd is set in CWnd::CreateEx(...)
		m_pcs = new CREATESTRUCT;	// store the CREATESTRICT in a smart pointer
		m_pwc = new WNDCLASS;		// store the WNDCLASS in a smart pointer
		::ZeroMemory(m_pcs.get(), sizeof(CREATESTRUCT));
		::ZeroMemory(m_pwc.get(), sizeof(WNDCLASS));
	}

	inline CWnd::~CWnd()
	{
		// Destroys the window for this object and cleans up resources.
		Destroy();
	}

	inline void CWnd::AddToMap()
	// Store the window handle and CWnd pointer in the HWND map
	{
		assert( GetApp() );
		GetApp()->m_csMapLock.Lock();

		assert(::IsWindow(m_hWnd));
		assert(!GetApp()->GetCWndFromMap(m_hWnd));

		GetApp()->m_mapHWND.insert(std::make_pair(m_hWnd, this));
		GetApp()->m_csMapLock.Release();
	}

	inline BOOL CWnd::Attach(HWND hWnd)
	// Subclass an existing window and attach it to a CWnd
	{
		assert( GetApp() );
		assert(::IsWindow(hWnd));

		// Ensure this thread has the TLS index set
		// Note: Perform the attach from the same thread as the window's message loop
		GetApp()->SetTlsIndex();

		if (m_PrevWindowProc)
			Detach();

		Subclass(hWnd);

		// Store the CWnd pointer in the HWND map
		AddToMap();
		OnCreate();
		OnInitialUpdate();

		return TRUE;
	}

	inline BOOL CWnd::AttachDlgItem(UINT nID, CWnd* pParent)
	// Converts a dialog item to a CWnd object
	{
		assert(pParent->IsWindow());

		HWND hWnd = ::GetDlgItem(pParent->GetHwnd(), nID);
		return Attach(hWnd);
	}

	inline void CWnd::CenterWindow() const
	// Centers this window over it's parent
	{

	// required for multi-monitor support with Dev-C++ and VC6
	#ifndef _WIN32_WCE
	#ifndef MONITOR_DEFAULTTONEAREST
		#define MONITOR_DEFAULTTONEAREST    0x00000002
	#endif
	#ifndef HMONITOR
		DECLARE_HANDLE(HMONITOR);
	#endif
	#ifndef MONITORINFO
		typedef struct tagMONITORINFO
		{
			DWORD   cbSize;
			RECT    rcMonitor;
			RECT    rcWork;
			DWORD   dwFlags;
		} MONITORINFO, *LPMONITORINFO;
	#endif	// MONITOR_DEFAULTTONEAREST
	#endif	// _WIN32_WCE

		assert(::IsWindow(m_hWnd));

		CRect rc = GetWindowRect();
		CRect rcParent;
		CRect rcDesktop;

		// Get screen dimensions excluding task bar
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);

		// Get the parent window dimensions (parent could be the desktop)
		if (GetParent() != NULL) rcParent = GetParent()->GetWindowRect();
		else rcParent = rcDesktop;

	#ifndef _WIN32_WCE
		// Import the GetMonitorInfo and MonitorFromWindow functions
		HMODULE hUser32 = LoadLibrary(_T("USER32.DLL"));
		typedef BOOL (WINAPI* LPGMI)(HMONITOR hMonitor, LPMONITORINFO lpmi);
		typedef HMONITOR (WINAPI* LPMFW)(HWND hwnd, DWORD dwFlags);
		LPMFW pfnMonitorFromWindow = (LPMFW)::GetProcAddress(hUser32, "MonitorFromWindow");
	#ifdef _UNICODE
		LPGMI pfnGetMonitorInfo = (LPGMI)::GetProcAddress(hUser32, "GetMonitorInfoW");
	#else
		LPGMI pfnGetMonitorInfo = (LPGMI)::GetProcAddress(hUser32, "GetMonitorInfoA");
	#endif

		// Take multi-monitor systems into account
		if (pfnGetMonitorInfo && pfnMonitorFromWindow)
		{
			HMONITOR hActiveMonitor = pfnMonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi), 0};

			if(pfnGetMonitorInfo(hActiveMonitor, &mi))
			{
				rcDesktop = mi.rcWork;
				if (GetParent() == NULL) rcParent = mi.rcWork;
			}
		}
		FreeLibrary(hUser32);
  #endif

		// Calculate point to center the dialog over the portion of parent window on this monitor
		rcParent.IntersectRect(rcParent, rcDesktop);
		int x = rcParent.left + (rcParent.Width() - rc.Width())/2;
		int y = rcParent.top + (rcParent.Height() - rc.Height())/2;

		// Keep the dialog wholly on the monitor display
		x = (x < rcDesktop.left)? rcDesktop.left : x;
		x = (x > rcDesktop.right - rc.Width())? rcDesktop.right - rc.Width() : x;
		y = (y < rcDesktop.top) ? rcDesktop.top: y;
		y = (y > rcDesktop.bottom - rc.Height())? rcDesktop.bottom - rc.Height() : y;

		SetWindowPos(HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
	}

	inline void CWnd::Cleanup()
	// Returns the CWnd to its default state
	{
		if ( GetApp() ) RemoveFromMap();
		m_hWnd = NULL;
		m_PrevWindowProc = NULL;
		m_IsTmpWnd = FALSE;
	}

	inline HWND CWnd::Create(CWnd* pParent /* = NULL */)
	// Creates the window. This is the default method of window creation.
	{

		// Test if Win32++ has been started
		assert( GetApp() );

		// Set the WNDCLASS parameters
		PreRegisterClass(*m_pwc);
		if (m_pwc->lpszClassName)
		{
			RegisterClass(*m_pwc);
			m_pcs->lpszClass = m_pwc->lpszClassName;
		}

		// Set the CREATESTRUCT parameters
		PreCreate(*m_pcs);

		// Set the Window Class Name
		if (!m_pcs->lpszClass)
			m_pcs->lpszClass = _T("Win32++ Window");

		// Set Parent
		HWND hWndParent = pParent? pParent->GetHwnd() : 0;
		if (!hWndParent && m_pcs->hwndParent)
			hWndParent = m_pcs->hwndParent;

		// Set the window style
		DWORD dwStyle;
		DWORD dwOverlappedStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		if (m_pcs->style)
			dwStyle = m_pcs->style;
		else
			dwStyle = WS_VISIBLE | ((hWndParent)? WS_CHILD : dwOverlappedStyle);

		// Set window size and position
		int x  = (m_pcs->cx || m_pcs->cy)? m_pcs->x  : CW_USEDEFAULT;
		int cx = (m_pcs->cx || m_pcs->cy)? m_pcs->cx : CW_USEDEFAULT;
		int y  = (m_pcs->cx || m_pcs->cy)? m_pcs->y  : CW_USEDEFAULT;
		int cy = (m_pcs->cx || m_pcs->cy)? m_pcs->cy : CW_USEDEFAULT;

		// Create the window
#ifndef _WIN32_WCE
		CreateEx(m_pcs->dwExStyle, m_pcs->lpszClass, m_pcs->lpszName, dwStyle, x, y,
				cx, cy, pParent, FromHandle(m_pcs->hMenu), m_pcs->lpCreateParams);
#else
		CreateEx(m_pcs->dwExStyle, m_pcs->lpszClass, m_pcs->lpszName, dwStyle, x, y,
				cx, cy, pParent, 0, m_pcs->lpCreateParams);
#endif

		return m_hWnd;
	}

	inline HWND CWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rc, CWnd* pParent, CMenu* pMenu, LPVOID lpParam /*= NULL*/)
	// Creates the window by specifying all the window creation parameters
	{
		int x = rc.left;
		int y = rc.top;
		int cx = rc.right - rc.left;
		int cy = rc.bottom - rc.top;
		return CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, x, y, cx, cy, pParent, pMenu, lpParam);
	}

	inline HWND CWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, CWnd* pParent, CMenu* pMenu, LPVOID lpParam /*= NULL*/)
	// Creates the window by specifying all the window creation parameters
	{

		assert( GetApp() );		// Test if Win32++ has been started
		assert(!::IsWindow(m_hWnd));	// Only one window per CWnd instance allowed

		try
		{
			// Prepare the CWnd if it has been reused
			Destroy();

			// Ensure a window class is registered
			std::vector<TCHAR> vTChar( MAX_STRING_SIZE+1, _T('\0') );
			TCHAR* ClassName = &vTChar[0];
			if (0 == lpszClassName || 0 == lstrlen(lpszClassName) )
				lstrcpyn (ClassName, _T("Win32++ Window"), MAX_STRING_SIZE);
			else
				// Create our own local copy of szClassName.
				lstrcpyn(ClassName, lpszClassName, MAX_STRING_SIZE);

			WNDCLASS wc = {0};
			wc.lpszClassName = ClassName;
			wc.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
			wc.hCursor		 = ::LoadCursor(NULL, IDC_ARROW);

			// Register the window class (if not already registered)
			if (!RegisterClass(wc))
				throw CWinException(_T("Failed to register window class"));

			HWND hWndParent = pParent? pParent->GetHwnd() : 0;

			// Ensure this thread has the TLS index set
			TLSData* pTLSData = GetApp()->SetTlsIndex();

			// Store the CWnd pointer in thread local storage
			pTLSData->pCWnd = this;

			// Create window
#ifdef _WIN32_WCE
			m_hWnd = ::CreateWindowEx(dwExStyle, ClassName, lpszWindowName, dwStyle, x, y, nWidth, nHeight,
									hWndParent, 0, GetApp()->GetInstanceHandle(), lpParam);
#else
			HMENU hMenu = pMenu? pMenu->GetHandle() : NULL;
			m_hWnd = ::CreateWindowEx(dwExStyle, ClassName, lpszWindowName, dwStyle, x, y, nWidth, nHeight,
									hWndParent, hMenu, GetApp()->GetInstanceHandle(), lpParam);
#endif

			// Now handle window creation failure
			if (!m_hWnd)
				throw CWinException(_T("Failed to Create Window"));

			// Automatically subclass predefined window class types
			::GetClassInfo(GetApp()->GetInstanceHandle(), lpszClassName, &wc);
			if (wc.lpfnWndProc != GetApp()->m_Callback)
			{
				Subclass(m_hWnd);

				// Send a message to force the HWND to be added to the map
				SendMessage(WM_NULL, 0L, 0L);

				OnCreate(); // We missed the WM_CREATE message, so call OnCreate now
			}

			// Clear the CWnd pointer from TLS
			pTLSData->pCWnd = NULL;
		}

		catch (const CWinException &e)
		{
			TRACE(_T("\n*** Failed to create window ***\n"));
			e.what();	// Display the last error message.

			// eat the exception (don't rethrow)
		}

		// Window creation is complete. Now call OnInitialUpdate
		OnInitialUpdate();

		return m_hWnd;
	}

	inline void CWnd::Destroy()
	// Destroys the window and returns the CWnd back to its default state, ready for reuse.
	{
		if (m_IsTmpWnd)
			m_hWnd = NULL;

		if (IsWindow())
			::DestroyWindow(m_hWnd);

		// Return the CWnd to its default state
		Cleanup();
	}

	inline HWND CWnd::Detach()
	// Reverse an Attach
	{
		assert(::IsWindow(m_hWnd));
		assert(0 != m_PrevWindowProc);	// Only a subclassed window can be detached

		SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)m_PrevWindowProc);
		HWND hWnd = m_hWnd;
		Cleanup();

		return hWnd;
	}

	inline LRESULT CWnd::FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// Pass messages on to the appropriate default window procedure
	// CMDIChild and CMDIFrame override this function
	{
		return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

	inline CWnd* CWnd::GetAncestor(UINT gaFlags /*= GA_ROOTOWNER*/) const
	// The GetAncestor function retrieves a pointer to the ancestor (root parent)
	// of the window. Supports Win95.
	{
		assert(::IsWindow(m_hWnd));
		HWND hWnd;

#if (WINVER < 0x0500)	// Win2000 and above
		UNREFERENCED_PARAMETER(gaFlags);
		hWnd = m_hWnd;
		HWND hWndParent = ::GetParent(hWnd);
		while (::IsChild(hWndParent, hWnd))
		{
			hWnd = hWndParent;
			hWndParent = ::GetParent(hWnd);
		}
#else
		hWnd = ::GetAncestor(m_hWnd, gaFlags);
#endif

		return FromHandle(hWnd);

	}

	inline CString CWnd::GetClassName() const
	// Retrieves the name of the class to which the specified window belongs.
	{
		assert(::IsWindow(m_hWnd));

		CString str;
		LPTSTR szStr = str.GetBuffer(MAX_STRING_SIZE+1);
		::GetClassName(m_hWnd, szStr, MAX_STRING_SIZE+1);
		str.ReleaseBuffer();
		return str;
	}

	inline CString CWnd::GetDlgItemText(int nIDDlgItem) const
	// Retrieves the title or text associated with a control in a dialog box.
	{
		assert(::IsWindow(m_hWnd));

		int nLength = ::GetWindowTextLength(::GetDlgItem(m_hWnd, nIDDlgItem));
		CString str;
		LPTSTR szStr = str.GetBuffer(nLength+1);
		::GetDlgItemText(m_hWnd, nIDDlgItem, szStr, nLength+1);
		str.ReleaseBuffer();
		return str;
	}

	inline CString CWnd::GetWindowText() const
	// Retrieves the text of the window's title bar.
	{
		assert(::IsWindow(m_hWnd));

		int nLength = ::GetWindowTextLength(m_hWnd);
		CString str;
		LPTSTR szStr = str.GetBuffer(nLength+1);
		::GetWindowText(m_hWnd, szStr, nLength+1);
		str.ReleaseBuffer();
		return str;
	}

	inline BOOL CWnd::OnCommand(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);

		// Override this to handle WM_COMMAND messages, for example

		//	switch (LOWORD(wParam))
		//	{
		//	case IDM_FILE_NEW:
		//		OnFileNew();
		//		TRUE;	// return TRUE for handled commands
		//	}

		// return FALSE for unhandled commands
		return FALSE;
	}

	inline void CWnd::OnCreate()
	{
		// This function is called when a WM_CREATE message is recieved
		// Override it in your derived class to automatically perform tasks
		//  during window creation.
	}

	inline void CWnd::OnDraw(CDC* pDC)
	// Called when part of the client area of the window needs to be drawn
	{
		UNREFERENCED_PARAMETER(pDC);

	    // Override this function in your derived class to perform drawing tasks.
	}

	inline BOOL CWnd::OnEraseBkgnd(CDC* pDC)
	// Called when the background of the window's client area needs to be erased.
	{
		UNREFERENCED_PARAMETER(pDC);

	    // Override this function in your derived class to perform drawing tasks.

		// Return Value: Return FALSE to also permit default erasure of the background
		//				 Return TRUE to prevent default erasure of the background

		return FALSE;
	}


	inline void CWnd::OnInitialUpdate()
	{
		// This function is called automatically once the window is created
		// Override it in your derived class to automatically perform tasks
		// after window creation.
	}

	inline LRESULT CWnd::MessageReflect(HWND hWndParent, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// A function used to call OnMessageReflect. You shouldn't need to call or
		//  override this function.

		HWND hWnd = NULL;
		switch (uMsg)
		{
		case WM_COMMAND:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
		case WM_CHARTOITEM:
		case WM_VKEYTOITEM:
		case WM_HSCROLL:
		case WM_VSCROLL:
			hWnd = (HWND)lParam;
			break;

		case WM_DRAWITEM:
		case WM_MEASUREITEM:
		case WM_DELETEITEM:
		case WM_COMPAREITEM:
			hWnd = ::GetDlgItem(hWndParent, (int)wParam);
			break;

		case WM_PARENTNOTIFY:
			switch(LOWORD(wParam))
			{
			case WM_CREATE:
			case WM_DESTROY:
				hWnd = (HWND)lParam;
				break;
			}
		}

		CWnd* Wnd = GetApp()->GetCWndFromMap(hWnd);

		if (Wnd != NULL)
			return Wnd->OnMessageReflect(uMsg, wParam, lParam);

		return 0L;
	}

	inline LRESULT CWnd::OnMessageReflect(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(uMsg);
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);
		// This function processes those special messages (see above) sent
		// by some older controls, and reflects them back to the originating CWnd object.
		// Override this function in your derrived class to handle these special messages.

		// Your overriding function should look like this ...

		// switch (uMsg)
		// {
		//		Handle your reflected messages here
		// }

		// return 0L for unhandled messages
		return 0L;
	}

	inline LRESULT CWnd::OnNotify(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);

		// You can use either OnNotifyReflect or OnNotify to handle notifications
		// Override OnNotifyReflect to handle notifications in the CWnd class that
		//   generated the notification.   OR
		// Override OnNotify to handle notifications in the PARENT of the CWnd class
		//   that generated the notification.

		// Your overriding function should look like this ...

		// switch (((LPNMHDR)lParam)->code)
		// {
		//		Handle your notifications from the CHILD window here
		//      Return the value recommended by the Windows API documentation.
		//      For many notifications, the return value doesn't matter, but for some it does.
		// }

		// return 0L for unhandled notifications
		return 0L;
	}

	inline LRESULT CWnd::OnNotifyReflect(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);

		// Override OnNotifyReflect to handle notifications in the CWnd class that
		//   generated the notification.

		// Your overriding function should look like this ...

		// switch (((LPNMHDR)lParam)->code)
		// {
		//		Handle your notifications from this window here
		//      Return the value recommended by the Windows API documentation.
		// }

		// return 0L for unhandled notifications
		return 0L;
	}

	inline void CWnd::OnMenuUpdate(UINT nID)
	// Called when menu items are about to be displayed
	{
		UNREFERENCED_PARAMETER(nID);

		// Override this function to modify the behaviour of menu items,
		// such as adding or removing checkmarks
	}

	inline void CWnd::PreCreate(CREATESTRUCT& cs)
	// Called by CWnd::Create to set some window parameters
	{
		// Test if Win32++ has been started
		assert(GetApp());	// Test if Win32++ has been started

		m_pcs->cx             = cs.cx;
		m_pcs->cy             = cs.cy;
		m_pcs->dwExStyle      = cs.dwExStyle;
		m_pcs->hInstance      = GetApp()->GetInstanceHandle();
		m_pcs->hMenu          = cs.hMenu;
		m_pcs->hwndParent     = cs.hwndParent;
		m_pcs->lpCreateParams = cs.lpCreateParams;
		m_pcs->lpszClass      = cs.lpszClass;
		m_pcs->lpszName       = cs.lpszName;
		m_pcs->style          = cs.style;
		m_pcs->x              = cs.x;
		m_pcs->y              = cs.y;

		// Overide this function in your derived class to set the
		// CREATESTRUCT values prior to window creation.
		// The cs.lpszClass parameter should NOT be specified if the
		// PreRegisterClass function is used to create a window class.
	}

	inline void CWnd::PreRegisterClass(WNDCLASS& wc)
	// Called by CWnd::Create to set some window parameters
	//  Useful for setting the background brush and cursor
	{
		// Test if Win32++ has been started
		assert( GetApp() );

		m_pwc->style			= wc.style;
		m_pwc->lpfnWndProc		= CWnd::StaticWindowProc;
		m_pwc->cbClsExtra		= wc.cbClsExtra;
		m_pwc->cbWndExtra		= wc.cbWndExtra;
		m_pwc->hInstance		= GetApp()->GetInstanceHandle();
		m_pwc->hIcon			= wc.hIcon;
		m_pwc->hCursor			= wc.hCursor;
		m_pwc->hbrBackground	= wc.hbrBackground;
		m_pwc->lpszMenuName		= wc.lpszMenuName;
		m_pwc->lpszClassName	= wc.lpszClassName;

		// Overide this function in your derived class to set the
		// WNDCLASS values prior to window creation.

		// ADDITIONAL NOTES:
		// 1) The lpszClassName must be set for this function to take effect.
		// 2) The lpfnWndProc is always CWnd::StaticWindowProc.
		// 3) No other defaults are set, so the following settings might prove useful
		//     wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		//     wc.hbrBackground = (HBRUSH)::GetStockObject(WHITE_BRUSH);
		//     wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		// 4) The styles that can be set here are WNDCLASS styles. These are a different
		//     set of styles to those set by CREATESTRUCT (used in PreCreate).
		// 5) RegisterClassEx is not used because its not supported on WinCE.
		//     To set a small icon for the window, use SetIconSmall.
	}

	inline BOOL CWnd::PreTranslateMessage(MSG* pMsg)
	{
		UNREFERENCED_PARAMETER(pMsg);

		// Override this function if your class requires input messages to be
		// translated before normal processing. Function which translate messages
		// include TranslateAccelerator, TranslateMDISysAccel and IsDialogMessage.
		// Return TRUE if the message is translated.

		return FALSE;
	}

	inline BOOL CWnd::RegisterClass(WNDCLASS& wc)
	// A private function used by the PreRegisterClass function to register a
	//  window class prior to window creation
	{
		assert( GetApp() );
		assert( (0 != lstrlen(wc.lpszClassName) && ( lstrlen(wc.lpszClassName) <=  MAX_STRING_SIZE) ) );

		// Check to see if this classname is already registered
		WNDCLASS wcTest = {0};
		BOOL Done = FALSE;

		if (::GetClassInfo(GetApp()->GetInstanceHandle(), wc.lpszClassName, &wcTest))
		{
			wc = wcTest;
			Done = TRUE;
		}

		if (!Done)
		{
			// Set defaults
			wc.hInstance	= GetApp()->GetInstanceHandle();
			wc.lpfnWndProc	= CWnd::StaticWindowProc;

			// Register the WNDCLASS structure
			if ( !::RegisterClass(&wc) )
				throw CWinException(_T("Failed to register window class"));

			Done = TRUE;
		}

		return Done;
	}

	inline BOOL CWnd::RemoveFromMap()
	{
		BOOL Success = FALSE;

		if (GetApp())
		{

			// Allocate an iterator for our HWND map
			std::map<HWND, CWnd*, CompareHWND>::iterator m;

			CWinApp* pApp = GetApp();
			if (pApp)
			{
				// Erase the CWnd pointer entry from the map
				pApp->m_csMapLock.Lock();
				for (m = pApp->m_mapHWND.begin(); m != pApp->m_mapHWND.end(); ++m)
				{
					if (this == m->second)
					{
						pApp->m_mapHWND.erase(m);
						Success = TRUE;
						break;
					}
				}

				pApp->m_csMapLock.Release();
			}
		}

		return Success;
	}

	inline HICON CWnd::SetIconLarge(int nIcon)
	// Sets the large icon associated with the window
	{
		assert( GetApp() );
		assert(::IsWindow(m_hWnd));

		HICON hIconLarge = (HICON) (::LoadImage (GetApp()->GetResourceHandle(), MAKEINTRESOURCE (nIcon), IMAGE_ICON,
		::GetSystemMetrics (SM_CXICON), ::GetSystemMetrics (SM_CYICON), 0));

		if (hIconLarge)
			SendMessage (WM_SETICON, WPARAM (ICON_BIG), LPARAM (hIconLarge));
		else
			TRACE(_T("**WARNING** SetIconLarge Failed\n"));

		return hIconLarge;
	}

	inline HICON CWnd::SetIconSmall(int nIcon)
	// Sets the small icon associated with the window
	{
		assert( GetApp() );
		assert(::IsWindow(m_hWnd));

		HICON hIconSmall = (HICON) (::LoadImage (GetApp()->GetResourceHandle(), MAKEINTRESOURCE (nIcon), IMAGE_ICON,
		::GetSystemMetrics (SM_CXSMICON), ::GetSystemMetrics (SM_CYSMICON), 0));

		if (hIconSmall)
			SendMessage (WM_SETICON, WPARAM (ICON_SMALL), LPARAM (hIconSmall));
		else
			TRACE(_T("**WARNING** SetIconSmall Failed\n"));

		return hIconSmall;
	}

	inline LRESULT CALLBACK CWnd::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	// All CWnd windows direct their messages here. This function redirects the message
	// to the CWnd's WndProc function.
	{
		assert( GetApp() );

		CWnd* w = GetApp()->GetCWndFromMap(hWnd);
		if (0 == w)
		{
			// The CWnd pointer wasn't found in the map, so add it now

			// Retrieve the pointer to the TLS Data
			TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
			assert(pTLSData);

			// Retrieve pointer to CWnd object from Thread Local Storage TLS
			w = pTLSData->pCWnd;
			assert(w);				// pTLSData->pCWnd is assigned in CreateEx
			pTLSData->pCWnd = NULL;

			// Store the CWnd pointer in the HWND map
			w->m_hWnd = hWnd;
			w->AddToMap();
		}

		return w->WndProc(uMsg, wParam, lParam);

	} // LRESULT CALLBACK StaticWindowProc(...)

	inline void CWnd::Subclass(HWND hWnd)
	// A private function used by CreateEx, Attach and AttachDlgItem
	{
		assert(::IsWindow(hWnd));

		m_PrevWindowProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)CWnd::StaticWindowProc);
		m_hWnd = hWnd;
	}

	inline LRESULT CWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Override this function in your class derrived from CWnd to handle
		//  window messages. A typical function might look like this:

		//	switch (uMsg)
		//	{
		//	case MESSAGE1:		// Some Windows API message
		//		OnMessage1();	// A user defined function
		//		break;			// Also do default processing
		//	case MESSAGE2:
		//		OnMessage2();
		//		return x;		// Don't do default processing, but instead return
		//						//  a value recommended by the Windows API documentation
		//	}

		// Always pass unhandled messages on to WndProcDefault
		return WndProcDefault(uMsg, wParam, lParam);
	}

	inline LRESULT CWnd::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// All WndProc functions should pass unhandled window messages to this function
	{
		LRESULT lr = 0L;

    	switch (uMsg)
		{
		case UWM_CLEANUPTEMPS:
			{
				TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
				pTLSData->vTmpWnds.clear();
			}
			break;
		case WM_COMMAND:
			{
				// Refelect this message if it's from a control
				CWnd* pWnd = GetApp()->GetCWndFromMap((HWND)lParam);
				if (pWnd != NULL)
					lr = pWnd->OnCommand(wParam, lParam);

				// Handle user commands
				if (!lr)
					lr =  OnCommand(wParam, lParam);

				if (lr) return 0L;
			}
			break;  // Note: Some MDI commands require default processing
		case WM_CREATE:
			OnCreate();
			break;
	// An example of how to end the application when the window closes
	//  If needed, put this in the class you inherit from CWnd
	//	case WM_DESTROY:
	//		::PostQuitMessage(0);
	//		return 0L;
		case WM_NOTIFY:
			{
				// Do Notification reflection if it came from a CWnd object
				HWND hwndFrom = ((LPNMHDR)lParam)->hwndFrom;
				CWnd* pWndFrom = GetApp()->GetCWndFromMap(hwndFrom);

				if (lstrcmp(GetClassName(), _T("ReBarWindow32")) != 0)	// Skip notification reflection for rebars to avoid double handling
				{
					if (pWndFrom != NULL)
						lr = pWndFrom->OnNotifyReflect(wParam, lParam);
					else
					{
						// Some controls (eg ListView) have child windows.
						// Reflect those notifications too.
						CWnd* pWndFromParent = GetApp()->GetCWndFromMap(::GetParent(hwndFrom));
						if (pWndFromParent != NULL)
							lr = pWndFromParent->OnNotifyReflect(wParam, lParam);
					}
				}

				// Handle user notifications
				if (!lr) lr = OnNotify(wParam, lParam);
				if (lr) return lr;
			}
			break;

		case WM_PAINT:
			{
				// Subclassed controls expect to do their own painting.
				// CustomDraw or OwnerDraw are normally used to modify the drawing of controls.
				if (m_PrevWindowProc) break;

				if (::GetUpdateRect(m_hWnd, NULL, FALSE))
				{
					CPaintDC dc(this);
					OnDraw(&dc);
				}
				else
				// RedrawWindow can require repainting without an update rect
				{
					CClientDC dc(this);
					OnDraw(&dc);
				}
			}
			return 0L;

		case WM_ERASEBKGND:
			{
				CDC dc((HDC)wParam);
				BOOL bResult = OnEraseBkgnd(&dc);
				dc.Detach();
				if (bResult) return TRUE;
			}
			break;

		// A set of messages to be reflected back to the control that generated them
		case WM_CTLCOLORBTN:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
		case WM_DELETEITEM:
		case WM_COMPAREITEM:
		case WM_CHARTOITEM:
		case WM_VKEYTOITEM:
		case WM_HSCROLL:
		case WM_VSCROLL:
		case WM_PARENTNOTIFY:
			{
			//	if (m_PrevWindowProc) break; // Suppress for subclassed windows

				LRESULT lr = MessageReflect(m_hWnd, uMsg, wParam, lParam);
				if (lr) return lr;	// Message processed so return
			}
			break;				// Do default processing when message not already processed

		case UWM_UPDATE_COMMAND:
			OnMenuUpdate((UINT)wParam); // Perform menu updates
		break;

		} // switch (uMsg)

		// Now hand all messages to the default procedure
		if (m_PrevWindowProc)
			return ::CallWindowProc(m_PrevWindowProc, m_hWnd, uMsg, wParam, lParam);
		else
			return FinalWindowProc(uMsg, wParam, lParam);

	} // LRESULT CWnd::WindowProc(...)


	//
	// Wrappers for Win32 API functions
	//

	inline CDC* CWnd::BeginPaint(PAINTSTRUCT& ps) const
	// The BeginPaint function prepares the specified window for painting and fills a PAINTSTRUCT structure with
	// information about the painting.
	{
        assert(::IsWindow(m_hWnd));
		return FromHandle(::BeginPaint(m_hWnd, &ps));
	}

	inline BOOL CWnd::BringWindowToTop() const
	// The BringWindowToTop function brings the specified window to the top
	// of the Z order. If the window is a top-level window, it is activated.
	{
        assert(::IsWindow(m_hWnd));
		return ::BringWindowToTop(m_hWnd);
	}

	inline LRESULT CWnd::CallWindowProc(WNDPROC lpPrevWndFunc, UINT Msg, WPARAM wParam, LPARAM lParam) const
	{
        assert(::IsWindow(m_hWnd));
		return ::CallWindowProc(lpPrevWndFunc, m_hWnd, Msg, wParam, lParam);
	}

	inline BOOL CWnd::CheckDlgButton(int nIDButton, UINT uCheck) const
	// The CheckDlgButton function changes the check state of a button control.
	{
        assert(::IsWindow(m_hWnd));
		return ::CheckDlgButton(m_hWnd, nIDButton, uCheck);
	}

	inline BOOL CWnd::CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) const
	// The CheckRadioButton function adds a check mark to (checks) a specified radio button in a group
	// and removes a check mark from (clears) all other radio buttons in the group.
	{
		assert(::IsWindow(m_hWnd));
		return ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton);
	}

	inline CWnd* CWnd::ChildWindowFromPoint(POINT pt) const
	// determines which, if any, of the child windows belonging to a parent window contains
	// the specified point. The search is restricted to immediate child windows. 
	// Grandchildren, and deeper descendant windows are not searched.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle(::ChildWindowFromPoint(m_hWnd, pt));
	}

	inline BOOL CWnd::ClientToScreen(POINT& pt) const
	// The ClientToScreen function converts the client-area coordinates of a specified point to screen coordinates.
	{
		assert(::IsWindow(m_hWnd));
		return ::ClientToScreen(m_hWnd, &pt);
	}

	inline BOOL CWnd::ClientToScreen(RECT& rc) const
	// The ClientToScreen function converts the client-area coordinates of a specified RECT to screen coordinates.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)::MapWindowPoints(m_hWnd, NULL, (LPPOINT)&rc, 2);
	}

	inline HDWP CWnd::DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) const
	// The DeferWindowPos function updates the specified multiple-window – position structure for the window.
	{
        assert(::IsWindow(m_hWnd));
		return ::DeferWindowPos(hWinPosInfo, m_hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
	}

	inline HDWP CWnd::DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, const RECT& rc, UINT uFlags) const
	// The DeferWindowPos function updates the specified multiple-window – position structure for the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::DeferWindowPos(hWinPosInfo, m_hWnd, hWndInsertAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, uFlags);
	}

	inline LRESULT CWnd::DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) const
	// This function provides default processing for any window messages that an application does not process.
	{
		assert(::IsWindow(m_hWnd));
		return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

	inline BOOL CWnd::DrawMenuBar() const
	// The DrawMenuBar function redraws the menu bar of the specified window. If the menu bar changes after
	// the system has created the window, this function must be called to draw the changed menu bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::DrawMenuBar(m_hWnd);
	}

	inline BOOL CWnd::EnableWindow(BOOL bEnable /*= TRUE*/) const
	// The EnableWindow function enables or disables mouse and
	// keyboard input to the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::EnableWindow(m_hWnd, bEnable);
	}

	inline BOOL CWnd::EndPaint(PAINTSTRUCT& ps) const
	// The EndPaint function marks the end of painting in the specified window. This function is required for
	// each call to the BeginPaint function, but only after painting is complete.
	{
		assert(::IsWindow(m_hWnd));
		return ::EndPaint(m_hWnd, &ps);
	}

	inline CWnd* CWnd::GetActiveWindow() const
	// The GetActiveWindow function retrieves a pointer to the active window attached to the calling
	// thread's message queue.
	{
		return FromHandle( ::GetActiveWindow() );
	}

	inline CWnd* CWnd::GetCapture() const
	// The GetCapture function retrieves a pointer to the window (if any) that has captured the mouse.
	{
		return FromHandle( ::GetCapture() );
	}

	inline ULONG_PTR CWnd::GetClassLongPtr(int nIndex) const
	// The GetClassLongPtr function retrieves the specified value from the
	// WNDCLASSEX structure associated with the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetClassLongPtr(m_hWnd, nIndex);
	}

	inline CRect CWnd::GetClientRect() const
	// The GetClientRect function retrieves the coordinates of a window's client area.
	// The client coordinates specify the upper-left and lower-right corners of the
	// client area. Because client coordinates are relative to the upper-left corner
	// of a window's client area, the coordinates of the upper-left corner are (0,0).
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		::GetClientRect(m_hWnd, &rc);
		return rc;
	}

	inline CDC* CWnd::GetDC() const
	// The GetDC function retrieves a handle to a display device context (DC) for the
	// client area of the window.
	{
		assert(::IsWindow(m_hWnd));
		return CDC::AddTempHDC(::GetDC(m_hWnd), m_hWnd);
	}

	inline CDC* CWnd::GetDCEx(HRGN hrgnClip, DWORD flags) const
	// The GetDCEx function retrieves a handle to a display device context (DC) for the
	// client area or entire area of a window
	{
		assert(::IsWindow(m_hWnd));
		return CDC::AddTempHDC(::GetDCEx(m_hWnd, hrgnClip, flags), m_hWnd);
	}

	inline CWnd* CWnd::GetDesktopWindow() const
	// The GetDesktopWindow function retrieves a pointer to the desktop window.
	{
		return FromHandle( ::GetDesktopWindow() );
	}

	inline CWnd* CWnd::GetDlgItem(int nIDDlgItem) const
	// The GetDlgItem function retrieves a handle to a control in the dialog box.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::GetDlgItem(m_hWnd, nIDDlgItem) );
	}

	inline UINT CWnd::GetDlgItemInt(int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned) const
	// The GetDlgItemInt function translates the text of a specified control in a dialog box into an integer value.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetDlgItemInt(m_hWnd, nIDDlgItem, lpTranslated, bSigned);
	}

	inline CWnd* CWnd::GetFocus() const
	// The GetFocus function retrieves a pointer to the window that has the keyboard focus, if the window
	// is attached to the calling thread's message queue.
	{
		return FromHandle( ::GetFocus() );
	}

	inline CFont* CWnd::GetFont() const
	// Retrieves the font with which the window is currently drawing its text.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle((HFONT)SendMessage(WM_GETFONT, 0, 0));
	}

	inline HICON CWnd::GetIcon(BOOL bBigIcon) const
	// Retrieves a handle to the large or small icon associated with a window.
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(WM_GETICON, (WPARAM)bBigIcon, 0);
	}

	inline CWnd* CWnd::GetNextDlgGroupItem(CWnd* pCtl, BOOL bPrevious) const
	// The GetNextDlgGroupItem function retrieves a pointer to the first control in a group of controls that
	// precedes (or follows) the specified control in a dialog box.
	{
		assert(::IsWindow(m_hWnd));
		assert(pCtl);
		return FromHandle(::GetNextDlgGroupItem(m_hWnd, pCtl->GetHwnd(), bPrevious));
	}

	inline CWnd* CWnd::GetNextDlgTabItem(CWnd* pCtl, BOOL bPrevious) const
	// The GetNextDlgTabItem function retrieves a pointer to the first control that has the WS_TABSTOP style
	// that precedes (or follows) the specified control.
	{
		assert(::IsWindow(m_hWnd));
		assert(pCtl);
		return FromHandle(::GetNextDlgTabItem(m_hWnd, pCtl->GetHwnd(), bPrevious));
	}

	inline CWnd* CWnd::GetParent() const
	// The GetParent function retrieves a pointer to the specified window's parent or owner.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::GetParent(m_hWnd) );
	}

	inline LONG_PTR CWnd::GetWindowLongPtr(int nIndex) const
	// The GetWindowLongPtr function retrieves information about the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetWindowLongPtr(m_hWnd, nIndex);
	}

	inline BOOL CWnd::GetScrollInfo(int fnBar, SCROLLINFO& si) const
	// The GetScrollInfo function retrieves the parameters of a scroll bar, including
	// the minimum and maximum scrolling positions, the page size, and the position
	// of the scroll box (thumb).
	{
		assert(::IsWindow(m_hWnd));
		return ::GetScrollInfo(m_hWnd, fnBar, &si);
	}

	inline CRect CWnd::GetUpdateRect(BOOL bErase) const
	// The GetUpdateRect function retrieves the coordinates of the smallest rectangle that completely
	// encloses the update region of the specified window.
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		::GetUpdateRect(m_hWnd, &rc, bErase);
		return rc;
	}

	inline int CWnd::GetUpdateRgn(CRgn* pRgn, BOOL bErase) const
	// The GetUpdateRgn function retrieves the update region of a window by copying it into the specified region.
	{
		assert(::IsWindow(m_hWnd));
		assert(pRgn);
		HRGN hRgn = (HRGN)pRgn->GetHandle();
		return ::GetUpdateRgn(m_hWnd, hRgn, bErase);
	}

	inline CWnd* CWnd::GetWindow(UINT uCmd) const
	// The GetWindow function retrieves a pointer to a window that has the specified
	// relationship (Z-Order or owner) to the specified window.
	// Possible uCmd values: GW_CHILD, GW_ENABLEDPOPUP, GW_HWNDFIRST, GW_HWNDLAST,
	// GW_HWNDNEXT, GW_HWNDPREV, GW_OWNER
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::GetWindow(m_hWnd, uCmd) );
	}

	inline CDC* CWnd::GetWindowDC() const
	// The GetWindowDC function retrieves the device context (DC) for the entire
	// window, including title bar, menus, and scroll bars.
	{
		assert(::IsWindow(m_hWnd));
		return CDC::AddTempHDC(::GetWindowDC(m_hWnd), m_hWnd);
	}

	inline CRect CWnd::GetWindowRect() const
	// retrieves the dimensions of the bounding rectangle of the window.
	// The dimensions are given in screen coordinates that are relative to the
	// upper-left corner of the screen.
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		::GetWindowRect(m_hWnd, &rc);
		return rc;
	}

	inline int CWnd::GetWindowTextLength() const
	// The GetWindowTextLength function retrieves the length, in characters, of the specified window's
	// title bar text (if the window has a title bar).
	{
		assert(::IsWindow(m_hWnd));
		return ::GetWindowTextLength(m_hWnd);
	}

	inline void CWnd::Invalidate(BOOL bErase /*= TRUE*/) const
	// The Invalidate function adds the entire client area the window's update region.
	// The update region represents the portion of the window's client area that must be redrawn.
	{
		assert(::IsWindow(m_hWnd));
		::InvalidateRect(m_hWnd, NULL, bErase);
	}

	inline BOOL CWnd::InvalidateRect(LPCRECT lpRect, BOOL bErase /*= TRUE*/) const
	// The InvalidateRect function adds a rectangle to the window's update region.
	// The update region represents the portion of the window's client area that must be redrawn.
	{
		assert(::IsWindow(m_hWnd));
		return ::InvalidateRect(m_hWnd, lpRect, bErase);
	}

	inline BOOL CWnd::InvalidateRgn(CRgn* pRgn, BOOL bErase /*= TRUE*/) const
	// The InvalidateRgn function invalidates the client area within the specified region
	// by adding it to the current update region of a window. The invalidated region,
	// along with all other areas in the update region, is marked for painting when the
	// next WM_PAINT message occurs.
	{
		assert(::IsWindow(m_hWnd));
		HRGN hRgn = pRgn? (HRGN)pRgn->GetHandle() : NULL;
		return ::InvalidateRgn(m_hWnd, hRgn, bErase);
	}

	inline BOOL CWnd::IsChild(CWnd* pChild) const
	// The IsChild function tests whether a window is a child window or descendant window
	// of a parent window's CWnd.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsChild(m_hWnd, pChild->GetHwnd());
	}

	inline BOOL CWnd::IsDialogMessage(LPMSG lpMsg) const
	// The IsDialogMessage function determines whether a message is intended for the specified dialog box and,
	// if it is, processes the message.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsDialogMessage(m_hWnd, lpMsg);
	}

	inline UINT CWnd::IsDlgButtonChecked(int nIDButton) const
	// The IsDlgButtonChecked function determines whether a button control has a check mark next to it
	// or whether a three-state button control is grayed, checked, or neither.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsDlgButtonChecked(m_hWnd, nIDButton);
	}

	inline BOOL CWnd::IsWindowEnabled() const
	// The IsWindowEnabled function determines whether the window is enabled
	// for mouse and keyboard input.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsWindowEnabled(m_hWnd);
	}

	inline BOOL CWnd::IsWindowVisible() const
	// The IsWindowVisible function retrieves the visibility state of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsWindowVisible(m_hWnd);
	}

	inline BOOL CWnd::IsWindow() const
	// The IsWindow function determines whether the window exists.
	{
		return ::IsWindow(m_hWnd);
	}

	inline void  CWnd::MapWindowPoints(CWnd* pWndTo, POINT& pt) const
	// The MapWindowPoints function converts (maps) a set of points from a coordinate space relative to one
	// window to a coordinate space relative to another window.
	{
		assert (m_hWnd);
		if(pWndTo)
		{
			assert (pWndTo->GetHwnd());
			::MapWindowPoints(m_hWnd, pWndTo->GetHwnd(), &pt, 1);
		}
		else
			::MapWindowPoints(m_hWnd, NULL, &pt, 1);
	}

	inline void CWnd::MapWindowPoints(CWnd* pWndTo, RECT& rc) const
	// The MapWindowPoints function converts (maps) a set of points from a coordinate space relative to one
	// window to a coordinate space relative to another window.
	{
		assert (m_hWnd);
		if(pWndTo)
		{
			assert (pWndTo->GetHwnd());
			::MapWindowPoints(m_hWnd, pWndTo->GetHwnd(), (LPPOINT)&rc, 2);
		}
		else
			::MapWindowPoints(m_hWnd, NULL, (LPPOINT)&rc, 2);
	}

	inline void CWnd::MapWindowPoints(CWnd* pWndTo, LPPOINT ptArray, UINT nCount) const
	// The MapWindowPoints function converts (maps) a set of points from a coordinate space relative to one
	// window to a coordinate space relative to another window.
	{
		assert (m_hWnd);
		if (pWndTo)
		{
			assert (pWndTo->GetHwnd());
			::MapWindowPoints(m_hWnd, pWndTo->GetHwnd(), (LPPOINT)ptArray, nCount);
		}
		else
			::MapWindowPoints(m_hWnd, NULL, (LPPOINT)ptArray, nCount);
	}

	inline int CWnd::MessageBox(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) const
	// The MessageBox function creates, displays, and operates a message box.
	// Possible combinations of uType values include: MB_OK, MB_HELP, MB_OKCANCEL, MB_RETRYCANCEL,
	// MB_YESNO, MB_YESNOCANCEL, MB_ICONEXCLAMATION, MB_ICONWARNING, MB_ICONERROR (+ many others).
	{
		assert(::IsWindow(m_hWnd));
		return ::MessageBox(m_hWnd, lpText, lpCaption, uType);
	}

	inline BOOL CWnd::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint /* = TRUE*/) const
	// The MoveWindow function changes the position and dimensions of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint = TRUE);
	}

	inline BOOL CWnd::MoveWindow(const RECT& rc, BOOL bRepaint /* = TRUE*/) const
	// The MoveWindow function changes the position and dimensions of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::MoveWindow(m_hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, bRepaint);
	}

	inline BOOL CWnd::PostMessage(UINT uMsg, WPARAM wParam /*= 0L*/, LPARAM lParam /*= 0L*/) const
	// The PostMessage function places (posts) a message in the message queue
	// associated with the thread that created the window and returns without
	// waiting for the thread to process the message.
	{
		assert(::IsWindow(m_hWnd));
		return ::PostMessage(m_hWnd, uMsg, wParam, lParam);
	}

	inline BOOL CWnd::PostMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const
	// Required by by some macros
	{
		assert(::IsWindow(m_hWnd));
		return ::PostMessage(hWnd, uMsg, wParam, lParam);
	}

	inline BOOL CWnd::RedrawWindow(LPCRECT lpRectUpdate, CRgn* pRgn, UINT flags) const
	// The RedrawWindow function updates the specified rectangle or region in a window's client area.
	{
		assert(::IsWindow(m_hWnd));
		HRGN hRgn = pRgn? (HRGN)pRgn->GetHandle() : NULL;
		return ::RedrawWindow(m_hWnd, lpRectUpdate, hRgn, flags);
	}

	inline int CWnd::ReleaseDC(CDC* pDC) const
	// The ReleaseDC function releases a device context (DC), freeing it for use
	// by other applications.
	{
		assert(::IsWindow(m_hWnd));
		assert(pDC);
		return ::ReleaseDC(m_hWnd, pDC->GetHDC());
	}

	inline BOOL CWnd::ScreenToClient(POINT& Point) const
	// The ScreenToClient function converts the screen coordinates of a specified point on the screen to client-area coordinates.
	{
		assert(::IsWindow(m_hWnd));
		return ::ScreenToClient(m_hWnd, &Point);
	}

	inline BOOL CWnd::ScreenToClient(RECT& rc) const
	// The ScreenToClient function converts the screen coordinates of a specified RECT on the screen to client-area coordinates.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rc, 2);
	}

	inline LRESULT CWnd::SendDlgItemMessage(int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam) const
	// The SendDlgItemMessage function sends a message to the specified control in a dialog box.
	{
		assert(::IsWindow(m_hWnd));
		return ::SendDlgItemMessage(m_hWnd, nIDDlgItem, Msg, wParam, lParam);
	}

	inline LRESULT CWnd::SendMessage(UINT uMsg, WPARAM wParam /*= 0L*/, LPARAM lParam /*= 0L*/) const
	// The SendMessage function sends the specified message to a window or windows.
	// It calls the window procedure for the window and does not return until the
	// window procedure has processed the message.
	{
		assert(::IsWindow(m_hWnd));
		return ::SendMessage(m_hWnd, uMsg, wParam, lParam);
	}

	inline LRESULT CWnd::SendMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const
	// Required by by some macros
	{
		assert(::IsWindow(m_hWnd));
		return ::SendMessage(hWnd, uMsg, wParam, lParam);
	}

	inline BOOL CWnd::SendNotifyMessage(UINT Msg, WPARAM wParam, LPARAM lParam) const
	// The SendNotifyMessage function sends the specified message to a window or windows. If the window was created by the
	// calling thread, SendNotifyMessage calls the window procedure for the window and does not return until the window procedure
	// has processed the message. If the window was created by a different thread, SendNotifyMessage passes the message to the
	// window procedure and returns immediately; it does not wait for the window procedure to finish processing the message.
	{
		assert(::IsWindow(m_hWnd));
		return ::SendNotifyMessage(m_hWnd, Msg, wParam, lParam);
	}

	inline CWnd* CWnd::SetActiveWindow() const
	// The SetActiveWindow function activates the window, but
	// not if the application is in the background.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::SetActiveWindow(m_hWnd) );
	}

	inline CWnd* CWnd::SetCapture() const
	// The SetCapture function sets the mouse capture to the window.
	// SetCapture captures mouse input either when the mouse is over the capturing
	// window, or when the mouse button was pressed while the mouse was over the
	// capturing window and the button is still down.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::SetCapture(m_hWnd) );
	}

	inline ULONG_PTR CWnd::SetClassLongPtr(int nIndex, LONG_PTR dwNewLong) const
	// The SetClassLongPtr function replaces the specified value at the specified offset in the
	// extra class memory or the WNDCLASSEX structure for the class to which the window belongs.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetClassLongPtr(m_hWnd, nIndex, dwNewLong);
	}

	inline CWnd* CWnd::SetFocus() const
	// The SetFocus function sets the keyboard focus to the window.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::SetFocus(m_hWnd) );
	}

	inline void CWnd::SetFont(CFont* pFont, BOOL bRedraw /* = TRUE*/) const
	// Specifies the font that the window will use when drawing text.
	{
		assert(::IsWindow(m_hWnd));
		assert(pFont);
		SendMessage(WM_SETFONT, (WPARAM)pFont->GetHandle(), (LPARAM)bRedraw);
	}

	inline HICON CWnd::SetIcon(HICON hIcon, BOOL bBigIcon) const
	// Associates a new large or small icon with a window.
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(WM_SETICON, (WPARAM)bBigIcon, (LPARAM)hIcon);
	}

	inline BOOL CWnd::SetForegroundWindow() const
	// The SetForegroundWindow function puts the thread that created the window into the
	// foreground and activates the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetForegroundWindow(m_hWnd);
	}

	inline CWnd* CWnd::SetParent(CWnd* pWndParent) const
	// The SetParent function changes the parent window of the child window.
	{
		assert(::IsWindow(m_hWnd));
		if (pWndParent)
		{
			HWND hParent = pWndParent->GetHwnd();
			return FromHandle(::SetParent(m_hWnd, hParent));
		}
		else
			return FromHandle(::SetParent(m_hWnd, 0));
	}

	inline BOOL CWnd::SetRedraw(BOOL bRedraw /*= TRUE*/) const
	// This function allows changes in that window to be redrawn or prevents changes
	// in that window from being redrawn.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0L);
	}

	inline LONG_PTR CWnd::SetWindowLongPtr(int nIndex, LONG_PTR dwNewLong) const
	// The SetWindowLongPtr function changes an attribute of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetWindowLongPtr(m_hWnd, nIndex, dwNewLong);
	}

	inline BOOL CWnd::SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) const
	// The SetWindowPos function changes the size, position, and Z order of a child, pop-up,
	// or top-level window. The hWndInsertAfter can be a HWND or one of:
	// HWND_BOTTOM, HWND_NOTOPMOST, HWND_TOP, HWND_TOPMOST
	{
		assert(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
	}

	inline BOOL CWnd::SetWindowPos(HWND hWndInsertAfter, const RECT& rc, UINT uFlags) const
	// The SetWindowPos function changes the size, position, and Z order of a child, pop-up,
	// or top-level window. The hWndInsertAfter can be a HWND or one of:
	// HWND_BOTTOM, HWND_NOTOPMOST, HWND_TOP, HWND_TOPMOST
	{
		assert(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, uFlags);
	}

	inline int CWnd::SetWindowRgn(CRgn* pRgn, BOOL bRedraw /*= TRUE*/) const
	// The SetWindowRgn function sets the window region of the window.
	// The window region determines the area within the window where the system permits drawing.
	{
		assert(::IsWindow(m_hWnd));
		HRGN hRgn = pRgn? (HRGN)pRgn->GetHandle() : NULL;
		int iResult = ::SetWindowRgn(m_hWnd, hRgn, bRedraw);
		if (iResult && pRgn)
			pRgn->Detach();	// The system owns the region now
		return iResult;
	}

	inline BOOL CWnd::SetDlgItemInt(int nIDDlgItem, UINT uValue, BOOL bSigned) const
	// The SetDlgItemInt function sets the text of a control in a dialog box to the string representation of a specified integer value.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetDlgItemInt(m_hWnd, nIDDlgItem, uValue, bSigned);
	}

	inline BOOL CWnd::SetDlgItemText(int nIDDlgItem, LPCTSTR lpString) const
	// The SetDlgItemText function sets the title or text of a control in a dialog box.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetDlgItemText(m_hWnd, nIDDlgItem, lpString);
	}

	inline UINT_PTR CWnd::SetTimer(UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) const
	// Creates a timer with the specified time-out value.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetTimer(m_hWnd, nIDEvent, uElapse, lpTimerFunc);
	}

	inline BOOL CWnd::SetWindowText(LPCTSTR lpString) const
	// The SetWindowText function changes the text of the window's title bar (if it has one).
	{
		assert(::IsWindow(m_hWnd));
		return ::SetWindowText(m_hWnd, lpString);
	}

	inline HRESULT CWnd::SetWindowTheme(LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) const
	// Set the XP Theme for a window.
	// Exampes:
	//  SetWindowTheme(NULL, NULL);		// Reverts the window's XP theme back to default
	//  SetWindowTheme(L" ", L" ");		// Disables XP theme for the window
	{
		HRESULT hr = E_NOTIMPL;

#ifndef	_WIN32_WCE

		HMODULE hMod = ::LoadLibrary(_T("uxtheme.dll"));
		if(hMod)
		{
			typedef HRESULT (__stdcall *PFNSETWINDOWTHEME)(HWND hWnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
			PFNSETWINDOWTHEME pfn = (PFNSETWINDOWTHEME)GetProcAddress(hMod, "SetWindowTheme");

			hr = (*pfn)(m_hWnd, pszSubAppName, pszSubIdList);

			::FreeLibrary(hMod);
		}

#endif

		return hr;
	}

	inline BOOL CWnd::ShowWindow(int nCmdShow /*= SW_SHOWNORMAL*/) const
	// The ShowWindow function sets the window's show state.
	{
		assert(::IsWindow(m_hWnd));
		return ::ShowWindow(m_hWnd, nCmdShow);
	}

	inline BOOL CWnd::UpdateWindow() const
	// The UpdateWindow function updates the client area of the window by sending a
	// WM_PAINT message to the window if the window's update region is not empty.
	// If the update region is empty, no message is sent.
	{
		assert(::IsWindow(m_hWnd));
		return ::UpdateWindow(m_hWnd);
	}

	inline BOOL CWnd::ValidateRect(LPCRECT prc) const
	// The ValidateRect function validates the client area within a rectangle by
	// removing the rectangle from the update region of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::ValidateRect(m_hWnd, prc);
	}

	inline BOOL CWnd::ValidateRgn(CRgn* pRgn) const
	// The ValidateRgn function validates the client area within a region by
	// removing the region from the current update region of the window.
	{
		assert(::IsWindow(m_hWnd));
		HRGN hRgn = pRgn? (HRGN)pRgn->GetHandle() : NULL;
		return ::ValidateRgn(m_hWnd, hRgn);
	}

	inline CWnd* CWnd::WindowFromPoint(POINT pt)
	// Retrieves the window that contains the specified point (in screen coodinates).
	{
		return FromHandle(::WindowFromPoint(pt));
	}

	//
	// These functions aren't supported on WinCE
	//
  #ifndef _WIN32_WCE
	inline BOOL CWnd::CloseWindow() const
	// The CloseWindow function minimizes (but does not destroy) the window.
	// To destroy a window, an application can use the Destroy function.
	{
		assert(::IsWindow(m_hWnd));
		return ::CloseWindow(m_hWnd);
	}

	inline int CWnd::DlgDirList(LPTSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT uFileType) const
	// The DlgDirList function replaces the contents of a list box with the names of the subdirectories and files
	// in a specified directory. You can filter the list of names by specifying a set of file attributes.
	{
		assert(::IsWindow(m_hWnd));
		return ::DlgDirList(m_hWnd, lpPathSpec, nIDListBox, nIDStaticPath, uFileType);
	}

	inline int CWnd::DlgDirListComboBox(LPTSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT uFiletype) const
	// The DlgDirListComboBox function replaces the contents of a combo box with the names of the subdirectories
	// and files in a specified directory. You can filter the list of names by specifying a set of file attributes.
	{
		assert(::IsWindow(m_hWnd));
		return ::DlgDirListComboBox(m_hWnd, lpPathSpec, nIDComboBox, nIDStaticPath, uFiletype);
	}

	inline BOOL CWnd::DlgDirSelectEx(LPTSTR lpString, int nCount, int nIDListBox) const
	// The DlgDirSelectEx function retrieves the current selection from a single-selection list box. It assumes that the list box
	// has been filled by the DlgDirList function and that the selection is a drive letter, filename, or directory name.
	{
		assert(::IsWindow(m_hWnd));
		return ::DlgDirSelectEx(m_hWnd, lpString, nCount, nIDListBox);
	}

	inline BOOL CWnd::DlgDirSelectComboBoxEx(LPTSTR lpString, int nCount, int nIDComboBox) const
	// The DlgDirSelectComboBoxEx function retrieves the current selection from a combo box filled by using the
	// DlgDirListComboBox function. The selection is interpreted as a drive letter, a file, or a directory name.
	{
		assert(::IsWindow(m_hWnd));
		return ::DlgDirSelectComboBoxEx(m_hWnd, lpString, nCount, nIDComboBox);
	}

    #ifndef WIN32_LEAN_AND_MEAN
    inline void CWnd::DragAcceptFiles(BOOL fAccept) const
	// Registers whether a window accepts dropped files.
	{
		assert(::IsWindow(m_hWnd));
		::DragAcceptFiles(m_hWnd, fAccept);
	}
    #endif

	inline BOOL CWnd::DrawAnimatedRects(int idAni, RECT& rcFrom, RECT& rcTo) const
	// The DrawAnimatedRects function draws a wire-frame rectangle and animates it to indicate the opening of
	// an icon or the minimizing or maximizing of a window.
	{
		assert(::IsWindow(m_hWnd));
		return ::DrawAnimatedRects(m_hWnd, idAni, &rcFrom, &rcTo);
	}

	inline BOOL CWnd::DrawCaption(CDC* pDC, RECT& rc, UINT uFlags) const
	// The DrawCaption function draws a window caption.
	{
		assert(::IsWindow(m_hWnd));
		assert(pDC);
		return ::DrawCaption(m_hWnd, pDC->GetHDC(), &rc, uFlags);
	}

	inline BOOL CWnd::EnableScrollBar(UINT uSBflags, UINT uArrows) const
	// The EnableScrollBar function enables or disables one or both scroll bar arrows.
	{
		assert(::IsWindow(m_hWnd));
		return ::EnableScrollBar(m_hWnd, uSBflags, uArrows);
	}

	inline CWnd* CWnd::GetLastActivePopup() const
	// The GetLastActivePopup function determines which pop-up window owned by the specified window was most recently active.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::GetLastActivePopup(m_hWnd) );
	}

	inline CMenu* CWnd::GetMenu() const
	// The GetMenu function retrieves a handle to the menu assigned to the window.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle(::GetMenu(m_hWnd));
	}

	inline int CWnd::GetScrollPos(int nBar) const
	// The GetScrollPos function retrieves the current position of the scroll box
	// (thumb) in the specified scroll bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetScrollPos(m_hWnd, nBar);
	}

	inline BOOL CWnd::GetScrollRange(int nBar, int& MinPos, int& MaxPos) const
	// The GetScrollRange function retrieves the current minimum and maximum scroll box
	// (thumb) positions for the specified scroll bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetScrollRange(m_hWnd, nBar, &MinPos, &MaxPos );
	}

	inline CMenu* CWnd::GetSystemMenu(BOOL bRevert) const
	// The GetSystemMenu function allows the application to access the window menu (also known as the system menu
	// or the control menu) for copying and modifying.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle(::GetSystemMenu(m_hWnd, bRevert));
	}

	inline CWnd* CWnd::GetTopWindow() const
	// The GetTopWindow function examines the Z order of the child windows associated with the parent window and
	// retrieves a handle to the child window at the top of the Z order.
	{
		assert(::IsWindow(m_hWnd));
		return FromHandle( ::GetTopWindow(m_hWnd) );
	}

	inline BOOL CWnd::GetWindowPlacement(WINDOWPLACEMENT& wndpl) const
	// The GetWindowPlacement function retrieves the show state and the restored,
	// minimized, and maximized positions of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::GetWindowPlacement(m_hWnd, &wndpl);
	}

	inline BOOL CWnd::HiliteMenuItem(CMenu* pMenu, UINT uItemHilite, UINT uHilite) const
	// The HiliteMenuItem function highlights or removes the highlighting from an item in a menu bar.
	{
		assert(::IsWindow(m_hWnd));
		assert(pMenu);
		return ::HiliteMenuItem(m_hWnd, pMenu->GetHandle(), uItemHilite, uHilite);
	}

	inline BOOL CWnd::IsIconic() const
	// The IsIconic function determines whether the window is minimized (iconic).
	{
		assert(::IsWindow(m_hWnd));
		return ::IsIconic(m_hWnd);
	}

	inline BOOL CWnd::IsZoomed() const
	// The IsZoomed function determines whether the window is maximized.
	{
		assert(::IsWindow(m_hWnd));
		return ::IsZoomed(m_hWnd);
	}

	inline BOOL CWnd::KillTimer(UINT_PTR uIDEvent) const
	// Destroys the specified timer.
	{
		assert(::IsWindow(m_hWnd));
		return ::KillTimer(m_hWnd, uIDEvent);
	}

	inline BOOL CWnd::LockWindowUpdate() const
	// Disables drawing in the window. Only one window can be locked at a time.
	// Use UnLockWindowUpdate to re-enable drawing in the window
	{
		assert(::IsWindow(m_hWnd));
		return ::LockWindowUpdate(m_hWnd);
	}

	inline BOOL CWnd::OpenIcon() const
	// The OpenIcon function restores a minimized (iconic) window to its previous size and position.
	{
		assert(::IsWindow(m_hWnd));
		return ::OpenIcon(m_hWnd);
	}

	inline void CWnd::Print(CDC* pDC, DWORD dwFlags) const
	// Requests that the window draw itself in the specified device context, most commonly in a printer device context.
	{
		assert(::IsWindow(m_hWnd));
		assert(pDC);
		SendMessage(m_hWnd, WM_PRINT, (WPARAM)pDC, (LPARAM)dwFlags);
	}

	inline BOOL CWnd::ScrollWindow(int XAmount, int YAmount, LPCRECT lprcScroll, LPCRECT lprcClip) const
	// The ScrollWindow function scrolls the contents of the specified window's client area.
	{
		assert(::IsWindow(m_hWnd));
		return ::ScrollWindow(m_hWnd, XAmount, YAmount, lprcScroll, lprcClip);
	}

	inline int CWnd::ScrollWindowEx(int dx, int dy, LPCRECT lprcScroll, LPCRECT lprcClip, CRgn* prgnUpdate, LPRECT lprcUpdate, UINT flags) const
	// The ScrollWindow function scrolls the contents of the window's client area.
	{
		assert(::IsWindow(m_hWnd));
		HRGN hrgnUpdate = prgnUpdate? (HRGN)prgnUpdate->GetHandle() : NULL;
		return ::ScrollWindowEx(m_hWnd, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate, flags);
	}

	inline BOOL CWnd::SetMenu(CMenu* pMenu) const
	// The SetMenu function assigns a menu to the specified window.
	// A hMenu of NULL removes the menu.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetMenu(m_hWnd, pMenu? pMenu->GetHandle() : NULL);
	}

	inline int CWnd::SetScrollInfo(int fnBar, const SCROLLINFO& si, BOOL fRedraw) const
	// The SetScrollInfo function sets the parameters of a scroll bar, including
	// the minimum and maximum scrolling positions, the page size, and the
	// position of the scroll box (thumb).
	{
		assert(::IsWindow(m_hWnd));
		return ::SetScrollInfo(m_hWnd, fnBar, &si, fRedraw);
	}

	inline int CWnd::SetScrollPos(int nBar, int nPos, BOOL bRedraw) const
	// The SetScrollPos function sets the position of the scroll box (thumb) in
	// the specified scroll bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetScrollPos(m_hWnd, nBar, nPos, bRedraw);
	}

	inline BOOL CWnd::SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw) const
	// The SetScrollRange function sets the minimum and maximum scroll box positions for the scroll bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetScrollRange(m_hWnd, nBar, nMinPos, nMaxPos, bRedraw);
	}

	inline BOOL CWnd::SetWindowPlacement(const WINDOWPLACEMENT& wndpl) const
	// The SetWindowPlacement function sets the show state and the restored, minimized,
	// and maximized positions of the window.
	{
		assert(::IsWindow(m_hWnd));
		return ::SetWindowPlacement(m_hWnd, &wndpl);
	}

	inline BOOL CWnd::ShowOwnedPopups(BOOL fShow) const
	// The ShowOwnedPopups function shows or hides all pop-up windows owned by the specified window.
	{
		assert(::IsWindow(m_hWnd));
		return ::ShowOwnedPopups(m_hWnd, fShow);
	}

	inline BOOL CWnd::ShowScrollBar(int nBar, BOOL bShow) const
	// The ShowScrollBar function shows or hides the specified scroll bar.
	{
		assert(::IsWindow(m_hWnd));
		return ::ShowScrollBar(m_hWnd, nBar, bShow);
	}

	inline BOOL CWnd::ShowWindowAsync(int nCmdShow) const
	// The ShowWindowAsync function sets the show state of a window created by a different thread.
	{
		assert(::IsWindow(m_hWnd));
		return ::ShowWindowAsync(m_hWnd, nCmdShow);
	}

	inline BOOL CWnd::UnLockWindowUpdate() const
	// Enables drawing in the window. Only one window can be locked at a time.
	// Use LockWindowUpdate to disable drawing in the window
	{
		assert(::IsWindow(m_hWnd));
		return ::LockWindowUpdate(0);
	}

	inline CWnd* CWnd::WindowFromDC(CDC* pDC) const
	// The WindowFromDC function returns a handle to the window associated with the specified display device context (DC).
	{
		assert(pDC);
		return FromHandle( ::WindowFromDC(pDC->GetHDC()) );
	}

  #endif

}; // namespace Win32xx


#endif // _WIN32XX_WINCORE_H_

