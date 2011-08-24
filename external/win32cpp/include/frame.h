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


////////////////////////////////////////////////////////
// frame.h
//  Declaration of the CFrame and CMenuBar classes

// The classes declared in this file support SDI (Single Document Interface)
// frames on Win32/Win64 operating systems (not Windows CE). For Windows CE,
// use wceframe.h instead. SDI frames are a simple frame which supports a
// single view window. Refer to mdi.h for frames that support several
// child windows.

// CFrame also includes each of the following classes as members:
// * CReBar for managing the frame's rebar control.
// * CMenuBar for managing the menu inside the rebar.
// * CToolBar for managing the frame's toolbar.
// * CStatusBar for managing the frame's status bar.
// In each case these members are exposed by a GetXXX function, allowing
// them to be accessed or sent messages.

// CFrame is responsible for creating a "frame" window. This window has a
// menu and and several child windows, including a toolbar (usualy hosted
// within a rebar), a status bar, and a view positioned over the frame
// window's non-client area. The "view" window is a seperate CWnd object
// assigned to the frame with the SetView function.

// When compiling an application with these classes, it will need to be linked
// with Comctl32.lib.

// To create a SDI frame application, inherit a CMainFrame class from CFrame.
// Use the Frame sample application as the starting point for your own frame
// applications.
// Refer to the Notepad and Scribble samples for examples on how to use these
// classes to create a frame application.


#ifndef _WIN32XX_FRAME_H_
#define _WIN32XX_FRAME_H_

#include "wincore.h"
#include "dialog.h"
#include "gdi.h"
#include "statusbar.h"
#include "toolbar.h"
#include "rebar.h"
#include "default_resource.h"

#ifndef RBN_MINMAX
  #define RBN_MINMAX (RBN_FIRST - 21)
#endif


namespace Win32xx
{

	////////////////////////////////////////////////
	// Declarations for structures for themes
	//
	struct MenuTheme
	{
		BOOL UseThemes;			// TRUE if themes are used
		COLORREF clrHot1;		// Colour 1 for top menu. Color of selected menu item
		COLORREF clrHot2;		// Colour 2 for top menu. Color of checkbox
		COLORREF clrPressed1;	// Colour 1 for pressed top menu and side bar
		COLORREF clrPressed2;	// Colour 2 for pressed top menu and side bar
		COLORREF clrOutline;	// Colour for border outline
	};


	// Forward declaration of CFrame. Its defined later.
	class CFrame;


	////////////////////////////////////
	// Declaration of the CMenuBar class
	//
	class CMenuBar : public CToolBar
	{
		friend class CFrame;

	public:
		CMenuBar();
		virtual ~CMenuBar();
		virtual void SetMenu(HMENU hMenu);
		virtual void SetMenuBarTheme(MenuTheme& Theme);

		HMENU GetMenu() const {return m_hTopMenu;}
		MenuTheme& GetMenuBarTheme() {return m_ThemeMenu;}

	protected:
	//Overridables
		virtual void OnCreate();
		virtual LRESULT OnCustomDraw(NMHDR* pNMHDR);
		virtual void OnKeyDown(WPARAM wParam, LPARAM lParam);
		virtual void OnLButtonDown(WPARAM wParam, LPARAM lParam);
		virtual void OnLButtonUp(WPARAM wParam, LPARAM lParam);
		virtual void OnMenuChar(WPARAM wParam, LPARAM lParam);
		virtual BOOL OnMenuInput(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual void OnMouseLeave();
		virtual void OnMouseMove(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotifyReflect(WPARAM wParam, LPARAM lParam);
		virtual void OnSysCommand(WPARAM wParam, LPARAM lParam);
		virtual void OnWindowPosChanged();
		virtual void PreCreate(CREATESTRUCT &cs);
		virtual void PreRegisterClass(WNDCLASS &wc);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CMenuBar(const CMenuBar&);				// Disable copy construction
		CMenuBar& operator = (const CMenuBar&); // Disable assignment operator
		void DoAltKey(WORD KeyCode);
		void DoPopupMenu();
		void DrawAllMDIButtons(CDC& DrawDC);
		void DrawMDIButton(CDC& DrawDC, int iButton, UINT uState);
		void ExitMenu();
		HWND GetActiveMDIChild();
		void GrabFocus();
		BOOL IsMDIChildMaxed() const;
		BOOL IsMDIFrame() const;
		void ReleaseFocus();
		void SetHotItem(int nHot);
		static LRESULT CALLBACK StaticMsgHook(int nCode, WPARAM wParam, LPARAM lParam);

		enum MDIButtonType
		{
			MDI_MIN = 0,
			MDI_RESTORE = 1,
			MDI_CLOSE = 2,
		};

		BOOL  m_bExitAfter;		// Exit after Popup menu ends
		BOOL  m_bKeyMode;		// keyboard navigation mode
		BOOL  m_bMenuActive;	// popup menu active
		BOOL  m_bSelPopup;		// a popup (cascade) menu is selected
		HMENU m_hPopupMenu;		// handle to the popup menu
		HMENU m_hSelMenu;		// handle to the casceded popup menu
		HMENU m_hTopMenu;		// handle to the top level menu
		HWND  m_hPrevFocus;		// handle to window which had focus
		CRect m_MDIRect[3];		// array of CRect for MDI buttons
		int   m_nHotItem;		// hot item
		int   m_nMDIButton;		// the MDI button (MDIButtonType) pressed
		CPoint m_OldMousePos;	// old Mouse position
		MenuTheme m_ThemeMenu;	// Theme structure
		CFrame* m_pFrame;       // Pointer to the frame

	};  // class CMenuBar



	//////////////////////////////////
	// Declaration of the CFrame class
	//
	class CFrame : public CWnd
	{
		friend class CMenuBar;

		struct ItemData
		// Each Dropdown menu item has this data
		{
			HMENU hMenu;
			UINT  nPos;
			UINT  fType;
			std::vector<TCHAR> vItemText;
			HMENU hSubMenu;

			ItemData() : hMenu(0), nPos(0), fType(0), hSubMenu(0) { vItemText.assign(MAX_MENU_STRING, _T('\0')); }
			LPTSTR GetItemText() {return &vItemText[0];}
		};

		typedef Shared_Ptr<ItemData> ItemDataPtr;

	public:
		CFrame();
		virtual ~CFrame();

		// Override these functions as required
		virtual void AdjustFrameRect(RECT rcView) const;
		virtual CRect GetViewRect() const;
		virtual BOOL IsMDIFrame() const { return FALSE; }
		virtual void SetStatusIndicators();
		virtual void SetStatusText();
		virtual void RecalcLayout();
		virtual MenuTheme& GetMenuTheme() const			{ return (MenuTheme&) m_ThemeMenu; }
		virtual ReBarTheme& GetReBarTheme()	const		{ return (ReBarTheme&)GetReBar().GetReBarTheme(); }
		virtual ToolBarTheme& GetToolBarTheme() const	{ return (ToolBarTheme&)GetToolBar().GetToolBarTheme(); }

		// Virtual Attributes
		// If you need to modify the default behaviour of the menubar, rebar,
		// statusbar or toolbar, inherit from those classes, and override
		// the following attribute functions.
		virtual CMenuBar& GetMenuBar() const		{ return (CMenuBar&)m_MenuBar; }
		virtual CReBar& GetReBar() const			{ return (CReBar&)m_ReBar; }
		virtual CStatusBar& GetStatusBar() const	{ return (CStatusBar&)m_StatusBar; }
		virtual CToolBar& GetToolBar() const		{ return (CToolBar&)m_ToolBar; }

		// These functions aren't virtual, and shouldn't be overridden
		HACCEL GetFrameAccel() const				{ return m_hAccel; }
		CMenu& GetFrameMenu() const					{ return (CMenu&)m_Menu; }
		std::vector<tString> GetMRUEntries() const	{ return m_vMRUEntries; }
		tString GetRegistryKeyName() const			{ return m_tsKeyName; }
		CWnd* GetView() const						{ return m_pView; }
		tString GetMRUEntry(UINT nIndex);
		void SetFrameMenu(INT ID_MENU);
		void SetFrameMenu(HMENU hMenu);
		void SetMenuTheme(MenuTheme& Theme);
		void SetView(CWnd& wndView);
		BOOL IsMenuBarUsed() const		{ return (GetMenuBar() != 0); }
		BOOL IsReBarSupported() const	{ return (GetComCtlVersion() >= 470); }
		BOOL IsReBarUsed() const		{ return (GetReBar() != 0); }

	protected:
		// Override these functions as required
		virtual BOOL AddMenuIcon(int nID_MenuItem, HICON hIcon, int cx = 16, int cy = 16);
		virtual UINT AddMenuIcons(const std::vector<UINT>& MenuData, COLORREF crMask, UINT ToolBarID, UINT ToolBarDisabledID);
		virtual void AddMenuBarBand();
		virtual void AddMRUEntry(LPCTSTR szMRUEntry);
		virtual void AddToolBarBand(CToolBar& TB, DWORD dwStyle, UINT nID);
		virtual void AddToolBarButton(UINT nID, BOOL bEnabled = TRUE, LPCTSTR szText = 0);
		virtual void CreateToolBar();
		virtual void DrawCheckmark(LPDRAWITEMSTRUCT pdis, CDC& DrawDC);
		virtual void DrawMenuIcon(LPDRAWITEMSTRUCT pdis, CDC& DrawDC, BOOL bDisabled);
		virtual void DrawMenuText(CDC& DrawDC, LPCTSTR ItemText, CRect& rc, COLORREF colorText);
		virtual int  GetMenuItemPos(HMENU hMenu, LPCTSTR szItem);
		virtual BOOL LoadRegistrySettings(LPCTSTR szKeyName);
		virtual BOOL LoadRegistryMRUSettings(UINT nMaxMRU = 0);
		virtual void OnActivate(WPARAM wParam, LPARAM lParam);
		virtual void OnClose();
		virtual void OnCreate();
		virtual void OnDestroy();
		virtual LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam);
		virtual void OnExitMenuLoop();
		virtual void OnHelp();
		virtual void OnInitMenuPopup(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnMeasureItem(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnMenuChar(WPARAM wParam, LPARAM lParam);
		virtual void OnMenuSelect(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
		virtual void OnSetFocus();
		virtual void OnSysColorChange();
		virtual LRESULT OnSysCommand(WPARAM wParam, LPARAM lParam);
		virtual	void OnTimer(WPARAM wParam);
		virtual void OnViewStatusBar();
		virtual void OnViewToolBar();
		virtual void PreCreate(CREATESTRUCT& cs);
		virtual void PreRegisterClass(WNDCLASS &wc);
		virtual void RemoveMRUEntry(LPCTSTR szMRUEntry);
		virtual BOOL SaveRegistrySettings();
		virtual void SetMenuBarBandSize();
		virtual UINT SetMenuIcons(const std::vector<UINT>& MenuData, COLORREF crMask, UINT ToolBarID, UINT ToolBarDisabledID);
		virtual void SetupToolBar();
		virtual void SetTheme();
		virtual void SetToolBarImages(COLORREF crMask, UINT ToolBarID, UINT ToolBarHotID, UINT ToolBarDisabledID);
		virtual void ShowMenu(BOOL bShow);
		virtual void ShowStatusBar(BOOL bShow);
		virtual void ShowToolBar(BOOL bShow);
		virtual void UpdateMRUMenu();
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

		enum Constants
		{
			ID_STATUS_TIMER = 1,
			POST_TEXT_GAP   = 16,			// for owner draw menu item
		};

		tString m_tsStatusText;				// TCHAR std::string for status text
        BOOL m_bShowIndicatorStatus;		// set to TRUE to see indicators in status bar
		BOOL m_bShowMenuStatus;				// set to TRUE to see menu and toolbar updates in status bar
		BOOL m_bUseReBar;					// set to TRUE if ReBars are to be used
		BOOL m_bUseThemes;					// set to TRUE if themes are to be used
		BOOL m_bUpdateTheme;				// set to TRUE to run SetThemes when theme changes
		BOOL m_bUseToolBar;					// set to TRUE if the toolbar is used
		BOOL m_bUseCustomDraw;				// set to TRUE to perform custom drawing on menu items
		BOOL m_bShowStatusBar;				// A flag to indicate if the StatusBar should be displayed
		BOOL m_bShowToolBar;				// A flag to indicate if the ToolBar should be displayed
		MenuTheme m_ThemeMenu;				// Theme structure for popup menus
		HIMAGELIST m_himlMenu;				// Imagelist of menu icons
		HIMAGELIST m_himlMenuDis;			// Imagelist of disabled menu icons

	private:
		CFrame(const CFrame&);				// Disable copy construction
		CFrame& operator = (const CFrame&); // Disable assignment operator
		void LoadCommonControls();

		std::vector<ItemDataPtr> m_vMenuItemData;// vector of ItemData pointers
		std::vector<UINT> m_vMenuIcons;		// vector of menu icon resource IDs
		std::vector<tString> m_vMRUEntries;	// Vector of tStrings for MRU entires
		CDialog m_AboutDialog;				// Help about dialog
		CMenuBar m_MenuBar;					// CMenuBar object
		CReBar m_ReBar;						// CReBar object
		CStatusBar m_StatusBar;				// CStatusBar object
		CToolBar m_ToolBar;					// CToolBar object
		CMenu m_Menu;						// handle to the frame menu
		HACCEL m_hAccel;					// handle to the frame's accelerator table
		CWnd* m_pView;						// pointer to the View CWnd object
		LPCTSTR m_OldStatus[3];				// Array of TCHAR pointers;
		tString m_tsKeyName;				// TCHAR std::string for Registry key name
		tString m_tsTooltip;				// TCHAR std::string for tool tips
		UINT m_nMaxMRU;						// maximum number of MRU entries
		CRect m_rcPosition;					// CRect of the starting window position
		HWND m_hOldFocus;					// The window which had focus prior to the app'a deactivation
		int m_nOldID;						// The previous ToolBar ID displayed in the statusbar

	};  // class CFrame

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	/////////////////////////////////////
	// Definitions for the CMenuBar class
	//
	inline CMenuBar::CMenuBar()
	{
		m_bExitAfter	= FALSE;
		m_hTopMenu		= NULL;
		m_nHotItem		= -1;
		m_bSelPopup		= FALSE;
		m_hSelMenu		= NULL;
		m_bMenuActive	= FALSE;
		m_bKeyMode		= FALSE;
		m_hPrevFocus	= NULL;
		m_nMDIButton    = 0;
		m_hPopupMenu	= 0;

		ZeroMemory(&m_ThemeMenu, sizeof(MenuTheme));
	}

	inline CMenuBar::~CMenuBar()
	{
	}

	inline void CMenuBar::DoAltKey(WORD KeyCode)
	{
		//Handle key pressed with Alt held down
		UINT ID;
		if (SendMessage(TB_MAPACCELERATOR, KeyCode, (LPARAM) &ID))
		{
			GrabFocus();
			m_bKeyMode = TRUE;
			SetHotItem(ID);
			m_bMenuActive = TRUE;
			PostMessage(UWM_POPUPMENU, 0L, 0L);
		}
		else
			::MessageBeep(MB_OK);
	}

	inline void CMenuBar::DoPopupMenu()
	{
		if (m_bKeyMode)
			// Simulate a down arrow key press
			PostMessage(WM_KEYDOWN, VK_DOWN, 0L);

		m_bKeyMode = FALSE;
		m_bExitAfter = FALSE;
		m_OldMousePos = GetCursorPos();

		HWND hMaxMDIChild = NULL;
		if (IsMDIChildMaxed())
			hMaxMDIChild = GetActiveMDIChild();

		// Load the submenu
		int nMaxedOffset = IsMDIChildMaxed()? 1:0;
		m_hPopupMenu = ::GetSubMenu(m_hTopMenu, m_nHotItem - nMaxedOffset);
		if (IsMDIChildMaxed() && (0 == m_nHotItem) )
			m_hPopupMenu = ::GetSystemMenu(hMaxMDIChild, FALSE);

        // Retrieve the bounding rectangle for the toolbar button
		CRect rc = GetItemRect(m_nHotItem);

		// convert rectangle to desktop coordinates
		ClientToScreen(rc);

		// Position popup above toolbar if it won't fit below
		TPMPARAMS tpm;
		tpm.cbSize = sizeof(TPMPARAMS);
		tpm.rcExclude = rc;

		// Set the hot button
		SendMessage(TB_SETHOTITEM, m_nHotItem, 0L);
		SendMessage(TB_PRESSBUTTON, m_nHotItem, MAKELONG(TRUE, 0));

		m_bSelPopup = FALSE;
		m_hSelMenu = NULL;
		m_bMenuActive = TRUE;

		// We hook mouse input to process mouse and keyboard input during
		//  the popup menu. Messages are sent to StaticMsgHook.

		// Remove any remaining hook first
		TLSData* pTLSData = (TLSData*)::TlsGetValue(GetApp()->GetTlsIndex());
		pTLSData->pMenuBar = this;
		if (pTLSData->hHook != NULL)
			::UnhookWindowsHookEx(pTLSData->hHook);

		// Hook messages about to be processed by the shortcut menu
		pTLSData->hHook = ::SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC)StaticMsgHook, NULL, ::GetCurrentThreadId());

		// Display the shortcut menu
		BOOL bRightToLeft = FALSE;

#if defined(WINVER) && defined (WS_EX_LAYOUTRTL) && (WINVER >= 0x0500)
		bRightToLeft = ((GetAncestor()->GetWindowLongPtr(GWL_EXSTYLE)) & WS_EX_LAYOUTRTL);
#endif

		int xPos = bRightToLeft? rc.right : rc.left;
		UINT nID = ::TrackPopupMenuEx(m_hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
			xPos, rc.bottom, m_hWnd, &tpm);

		// We get here once the TrackPopupMenuEx has ended
		m_bMenuActive = FALSE;

		// Remove the message hook
		::UnhookWindowsHookEx(pTLSData->hHook);
		pTLSData->hHook = NULL;

		// Process MDI Child system menu
		if (IsMDIChildMaxed())
		{
			if (::GetSystemMenu(hMaxMDIChild, FALSE) == m_hPopupMenu )
			{
				if (nID)
					::SendMessage(hMaxMDIChild, WM_SYSCOMMAND, nID, 0L);
			}
		}

		// Resestablish Focus
		if (m_bKeyMode)
			GrabFocus();
	}

	inline void CMenuBar::DrawAllMDIButtons(CDC& DrawDC)
	{
		if (!IsMDIFrame())
			return;

		if (IsMDIChildMaxed())
		{
			int cx = GetSystemMetrics(SM_CXSMICON);
			int cy = GetSystemMetrics(SM_CYSMICON);
			CRect rc = GetClientRect();
			int gap = 4;
			rc.right -= gap;

			// Assign values to each element of the CRect array
			for (int i = 0 ; i < 3 ; ++i)
			{
				int left = rc.right - (i+1)*cx - gap*(i+1);
				int top = rc.bottom/2 - cy/2;
				int right = rc.right - i*cx - gap*(i+1);
				int bottom = rc.bottom/2 + cy/2;
				::SetRect(&m_MDIRect[2 - i], left, top, right, bottom);
			}

			// Hide the MDI button if it won't fit
			for (int k = 0 ; k <= 2 ; ++k)
			{

				if (m_MDIRect[k].left < GetMaxSize().cx)
				{
					::SetRectEmpty(&m_MDIRect[k]);
				}
			}

			DrawMDIButton(DrawDC, MDI_MIN, 0);
			DrawMDIButton(DrawDC, MDI_RESTORE, 0);
			DrawMDIButton(DrawDC, MDI_CLOSE, 0);
		}
	}

	inline void CMenuBar::DrawMDIButton(CDC& DrawDC, int iButton, UINT uState)
	{
		if (!IsRectEmpty(&m_MDIRect[iButton]))
		{
			switch (uState)
			{
			case 0:
				{
					// Draw a grey outline
					DrawDC.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
					DrawDC.MoveTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].top);
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].top);
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
				}
				break;
			case 1:
				{
					// Draw outline, white at top, black on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.MoveTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].top);
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
				}

				break;
			case 2:
				{
					// Draw outline, black on top, white on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.MoveTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].bottom);
					DrawDC.LineTo(m_MDIRect[iButton].right, m_MDIRect[iButton].top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].top);
					DrawDC.LineTo(m_MDIRect[iButton].left, m_MDIRect[iButton].bottom);
				}
				break;
			}

			DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

			switch (iButton)
			{
			case MDI_MIN:
				// Manually Draw Minimise button
				DrawDC.MoveTo(m_MDIRect[0].left + 4, m_MDIRect[0].bottom -4);
				DrawDC.LineTo(m_MDIRect[0].right - 4, m_MDIRect[0].bottom - 4);

				DrawDC.MoveTo(m_MDIRect[0].left + 4, m_MDIRect[0].bottom -5);
				DrawDC.LineTo(m_MDIRect[0].right - 4, m_MDIRect[0].bottom - 5);
				break;
			case MDI_RESTORE:
				// Manually Draw Restore Button
				DrawDC.MoveTo(m_MDIRect[1].left + 3, m_MDIRect[1].top + 7);
				DrawDC.LineTo(m_MDIRect[1].left + 3, m_MDIRect[1].bottom -4);
				DrawDC.LineTo(m_MDIRect[1].right - 6, m_MDIRect[1].bottom -4);
				DrawDC.LineTo(m_MDIRect[1].right - 6, m_MDIRect[1].top + 7);
				DrawDC.LineTo(m_MDIRect[1].left + 3, m_MDIRect[1].top + 7);

				DrawDC.MoveTo(m_MDIRect[1].left + 3, m_MDIRect[1].top + 8);
				DrawDC.LineTo(m_MDIRect[1].right - 6, m_MDIRect[1].top + 8);

				DrawDC.MoveTo(m_MDIRect[1].left + 5, m_MDIRect[1].top + 7);
				DrawDC.LineTo(m_MDIRect[1].left + 5, m_MDIRect[1].top + 4);
				DrawDC.LineTo(m_MDIRect[1].right - 4, m_MDIRect[1].top + 4);
				DrawDC.LineTo(m_MDIRect[1].right - 4, m_MDIRect[1].bottom -6);
				DrawDC.LineTo(m_MDIRect[1].right - 6, m_MDIRect[1].bottom -6);

				DrawDC.MoveTo(m_MDIRect[1].left + 5, m_MDIRect[1].top + 5);
				DrawDC.LineTo(m_MDIRect[1].right - 4, m_MDIRect[1].top + 5);
				break;
			case MDI_CLOSE:
				// Manually Draw Close Button
				DrawDC.MoveTo(m_MDIRect[2].left + 4, m_MDIRect[2].top +5);
				DrawDC.LineTo(m_MDIRect[2].right - 4, m_MDIRect[2].bottom -3);

				DrawDC.MoveTo(m_MDIRect[2].left + 5, m_MDIRect[2].top +5);
				DrawDC.LineTo(m_MDIRect[2].right - 4, m_MDIRect[2].bottom -4);

				DrawDC.MoveTo(m_MDIRect[2].left + 4, m_MDIRect[2].top +6);
				DrawDC.LineTo(m_MDIRect[2].right - 5, m_MDIRect[2].bottom -3);

				DrawDC.MoveTo(m_MDIRect[2].right -5, m_MDIRect[2].top +5);
				DrawDC.LineTo(m_MDIRect[2].left + 3, m_MDIRect[2].bottom -3);

				DrawDC.MoveTo(m_MDIRect[2].right -5, m_MDIRect[2].top +6);
				DrawDC.LineTo(m_MDIRect[2].left + 4, m_MDIRect[2].bottom -3);

				DrawDC.MoveTo(m_MDIRect[2].right -6, m_MDIRect[2].top +5);
				DrawDC.LineTo(m_MDIRect[2].left + 3, m_MDIRect[2].bottom -4);
				break;
			}
		}
	}

	inline void CMenuBar::ExitMenu()
	{
		ReleaseFocus();
		m_bKeyMode = FALSE;
		m_bMenuActive = FALSE;
		SendMessage(TB_PRESSBUTTON, m_nHotItem, (LPARAM) MAKELONG (FALSE, 0));
		SetHotItem(-1);

		CPoint pt = GetCursorPos();
		ScreenToClient(pt);

		// Update mouse mouse position for hot tracking
		SendMessage(WM_MOUSEMOVE, 0L, MAKELONG(pt.x, pt.y));
	}

	inline HWND CMenuBar::GetActiveMDIChild()
	{
		HWND hwndMDIChild = NULL;
		if (IsMDIFrame())
		{
			hwndMDIChild = (HWND)::SendMessage(m_pFrame->GetView()->GetHwnd(), WM_MDIGETACTIVE, 0L, 0L);
		}

		return hwndMDIChild;
	}

	inline void CMenuBar::GrabFocus()
	{
		if (::GetFocus() != m_hWnd)
			m_hPrevFocus = ::SetFocus(m_hWnd);
		::SetCapture(m_hWnd);
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}

	inline BOOL CMenuBar::IsMDIChildMaxed() const
	{
		BOOL bMaxed = FALSE;

		if (IsMDIFrame() && m_pFrame->GetView()->IsWindow())
		{
			m_pFrame->GetView()->SendMessage(WM_MDIGETACTIVE, 0L, (LPARAM)&bMaxed);
		}

		return bMaxed;
	}

	inline BOOL CMenuBar::IsMDIFrame() const
	{
		return (m_pFrame->IsMDIFrame());	
	}

	inline void CMenuBar::OnMenuChar(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);

		if (!m_bMenuActive)
			DoAltKey(LOWORD(wParam));
	}

	inline void CMenuBar::OnCreate()
	{
		// We must send this message before sending the TB_ADDBITMAP or TB_ADDBUTTONS message
		SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);

		m_pFrame = (CFrame*)GetAncestor();
		assert(m_pFrame);
	}

	inline LRESULT CMenuBar::OnCustomDraw(NMHDR* pNMHDR)
	// CustomDraw is used to render the MenuBar's toolbar buttons
	{
		if (m_ThemeMenu.UseThemes)
		{
			LPNMTBCUSTOMDRAW lpNMCustomDraw = (LPNMTBCUSTOMDRAW)pNMHDR;

			switch (lpNMCustomDraw->nmcd.dwDrawStage)
			{
			// Begin paint cycle
			case CDDS_PREPAINT:
				// Send NM_CUSTOMDRAW item draw, and post-paint notification messages.
				return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT ;

			// An item is about to be drawn
			case CDDS_ITEMPREPAINT:
				{
					CDC* pDrawDC = FromHandle(lpNMCustomDraw->nmcd.hdc);
					CRect rcRect = lpNMCustomDraw->nmcd.rc;
					int nState = lpNMCustomDraw->nmcd.uItemState;
					DWORD dwItem = (DWORD)lpNMCustomDraw->nmcd.dwItemSpec;

					// Leave a pixel gap above and below the drawn rectangle
					if (IsAeroThemed())
						rcRect.InflateRect(0, -2);
					else
						rcRect.InflateRect(0, -1);

					if (IsMDIChildMaxed() && (0 == dwItem))
					// Draw over MDI Max button
					{
						HICON hIcon = (HICON)::SendMessage(GetActiveMDIChild(), WM_GETICON, ICON_SMALL, 0L);
						if (NULL == hIcon)
							hIcon = ::LoadIcon(NULL, IDI_APPLICATION);

						int cx = ::GetSystemMetrics (SM_CXSMICON);
						int cy = ::GetSystemMetrics (SM_CYSMICON);
						int y = 1 + (GetWindowRect().Height() - cy)/2;
						int x = (rcRect.Width() - cx)/2;
						pDrawDC->DrawIconEx(x, y, hIcon, cx, cy, 0, NULL, DI_NORMAL);

						pDrawDC->Detach();	// Optional, deletes GDI objects sooner
						return CDRF_SKIPDEFAULT;  // No further drawing
					}

					else if (nState & (CDIS_HOT | CDIS_SELECTED))
					{
						if ((nState & CDIS_SELECTED) || (GetButtonState(dwItem) & TBSTATE_PRESSED))
						{
							pDrawDC->GradientFill(m_ThemeMenu.clrPressed1, m_ThemeMenu.clrPressed2, rcRect, FALSE);
						}
						else if (nState & CDIS_HOT)
						{
							pDrawDC->GradientFill(m_ThemeMenu.clrHot1, m_ThemeMenu.clrHot2, rcRect, FALSE);
						}

						// Draw border
						pDrawDC->CreatePen(PS_SOLID, 1, m_ThemeMenu.clrOutline);
						pDrawDC->MoveTo(rcRect.left, rcRect.bottom);
						pDrawDC->LineTo(rcRect.left, rcRect.top);
						pDrawDC->LineTo(rcRect.right-1, rcRect.top);
						pDrawDC->LineTo(rcRect.right-1, rcRect.bottom);
						pDrawDC->MoveTo(rcRect.right-1, rcRect.bottom);
						pDrawDC->LineTo(rcRect.left, rcRect.bottom);

						TCHAR str[80] = _T("");
						int nLength = (int)SendMessage(TB_GETBUTTONTEXT, lpNMCustomDraw->nmcd.dwItemSpec, 0L);
						if ((nLength > 0) && (nLength < 80))
							SendMessage(TB_GETBUTTONTEXT, lpNMCustomDraw->nmcd.dwItemSpec, (LPARAM)str);

						// Draw highlight text
						pDrawDC->SelectObject(GetFont());
						rcRect.bottom += 1;
						int iMode = pDrawDC->SetBkMode(TRANSPARENT);
						pDrawDC->DrawText(str, lstrlen(str), rcRect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

						pDrawDC->SetBkMode(iMode);
						pDrawDC->Detach();	// Optional, deletes GDI objects sooner
						return CDRF_SKIPDEFAULT;  // No further drawing
					} 
				} 
				return CDRF_DODEFAULT ;   // Do default drawing

			// Painting cycle has completed
			case CDDS_POSTPAINT:
				// Draw MDI Minimise, Restore and Close buttons
				{
					CDC* pDrawDC = FromHandle(lpNMCustomDraw->nmcd.hdc);
					DrawAllMDIButtons(*pDrawDC);
					pDrawDC->Detach();	// Optional, deletes GDI objects sooner
				}
				break;
			}
		} 
		return 0L;
	}

	inline void CMenuBar::OnKeyDown(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);

		switch (wParam)
		{
		case VK_ESCAPE:
			ExitMenu();
			break;

		case VK_SPACE:
			ExitMenu();
			// Bring up the system menu
			GetAncestor()->PostMessage(WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE);
			break;

		// Handle VK_DOWN,VK_UP and VK_RETURN together
		case VK_DOWN:
		case VK_UP:
		case VK_RETURN:
			// Always use PostMessage for USER_POPUPMENU (not SendMessage)
			PostMessage(UWM_POPUPMENU, 0L, 0L);
			break;

		case VK_LEFT:
			// Move left to next topmenu item
			(m_nHotItem > 0)? SetHotItem(m_nHotItem -1) : SetHotItem(GetButtonCount()-1);
			break;

		case VK_RIGHT:
			// Move right to next topmenu item
			(m_nHotItem < GetButtonCount() -1)? SetHotItem(m_nHotItem +1) : SetHotItem(0);
			break;

		default:
			// Handle Accelerator keys with Alt toggled down
			if (m_bKeyMode)
			{
				UINT ID;
				if (SendMessage(TB_MAPACCELERATOR, wParam, (LPARAM) &ID))
				{
					m_nHotItem = ID;
					PostMessage(UWM_POPUPMENU, 0L, 0L);
				}
				else
					::MessageBeep(MB_OK);
			}
			break;
		} // switch (wParam)
	}

	inline void CMenuBar::OnLButtonDown(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		GrabFocus();
		m_nMDIButton = 0;
		CPoint pt;

		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		if (IsMDIFrame())
		{
			if (IsMDIChildMaxed())
			{
				CClientDC MenuBarDC(this);
				m_nMDIButton = -1;

				if (m_MDIRect[0].PtInRect(pt)) m_nMDIButton = 0;
				if (m_MDIRect[1].PtInRect(pt)) m_nMDIButton = 1;
				if (m_MDIRect[2].PtInRect(pt)) m_nMDIButton = 2;

				if (m_nMDIButton >= 0)
				{
					DrawMDIButton(MenuBarDC, MDI_MIN,     (0 == m_nMDIButton)? 2 : 0);
					DrawMDIButton(MenuBarDC, MDI_RESTORE, (1 == m_nMDIButton)? 2 : 0);
					DrawMDIButton(MenuBarDC, MDI_CLOSE,   (2 == m_nMDIButton)? 2 : 0);
				}

				// Bring up the MDI Child window's system menu when the icon is pressed
				if (0 == HitTest())
				{
					m_nHotItem = 0;
					PostMessage(UWM_POPUPMENU, 0L, 0L);
				}
			}
		}
	}

	inline void CMenuBar::OnLButtonUp(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);
		CPoint pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		if (IsMDIFrame())
		{
			HWND MDIClient = m_pFrame->GetView()->GetHwnd();
			HWND MDIChild = GetActiveMDIChild();

			if (IsMDIChildMaxed())
			{
				CPoint pt = GetCursorPos();
				ScreenToClient(pt);

				// Process the MDI button action when the left mouse button is up
				if (m_MDIRect[0].PtInRect(pt))
				{
					if (MDI_MIN == m_nMDIButton)
						::ShowWindow(MDIChild, SW_MINIMIZE);
				}

				if (m_MDIRect[1].PtInRect(pt))
				{
					if (MDI_RESTORE == m_nMDIButton)
					::PostMessage(MDIClient, WM_MDIRESTORE, (WPARAM)MDIChild, 0L);
				}

				if (m_MDIRect[2].PtInRect(pt))
				{
					if (MDI_CLOSE == m_nMDIButton)
						::PostMessage(MDIChild, WM_CLOSE, 0L, 0L);
				}
			}
		}
		m_nMDIButton = 0;
		ExitMenu();
	}

	inline BOOL CMenuBar::OnMenuInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// When a popup menu is active, StaticMsgHook directs all menu messages here
	{
		switch(uMsg)
		{
		case WM_KEYDOWN:
			m_bExitAfter = FALSE;
			{
				switch (wParam)
				{
				case VK_ESCAPE:
					// Use default processing if inside a Sub Menu
					if ((m_hSelMenu) &&(m_hSelMenu != m_hPopupMenu))
						return FALSE;

					m_bMenuActive = FALSE;
					m_bKeyMode = TRUE;
					SendMessage(WM_CANCELMODE, 0L, 0L);
					SendMessage(TB_PRESSBUTTON, m_nHotItem, MAKELONG(FALSE, 0));
					SendMessage(TB_SETHOTITEM, m_nHotItem, 0L);
					break;

				case VK_LEFT:
					// Use default processing if inside a Sub Menu
				    if ((m_hSelMenu) &&(m_hSelMenu != m_hPopupMenu))
						return FALSE;

					SendMessage(TB_PRESSBUTTON, m_nHotItem, MAKELONG(FALSE, 0));

					// Move left to next topmenu item
					(m_nHotItem > 0)? --m_nHotItem : m_nHotItem = GetButtonCount()-1;
					SendMessage(WM_CANCELMODE, 0L, 0L);

					// Always use PostMessage for USER_POPUPMENU (not SendMessage)
					PostMessage(UWM_POPUPMENU, 0L, 0L);
					PostMessage(WM_KEYDOWN, VK_DOWN, 0L);
					break;

				case VK_RIGHT:
					// Use default processing to open Sub Menu
					if (m_bSelPopup)
						return FALSE;

					SendMessage(TB_PRESSBUTTON, m_nHotItem, MAKELONG(FALSE, 0));

					// Move right to next topmenu item
					(m_nHotItem < GetButtonCount()-1)? ++m_nHotItem : m_nHotItem = 0;
					SendMessage(WM_CANCELMODE, 0L, 0L);

					// Always use PostMessage for USER_POPUPMENU (not SendMessage)
					PostMessage(UWM_POPUPMENU, 0L, 0L);
					PostMessage(WM_KEYDOWN, VK_DOWN, 0L);
					break;

				case VK_RETURN:
					m_bExitAfter = TRUE;
					break;

				} // switch (wParam)

			} // case WM_KEYDOWN

			return FALSE;

		case WM_CHAR:
			m_bExitAfter = TRUE;
			return FALSE;

		case WM_LBUTTONDOWN:
			{
				m_bExitAfter = TRUE;
				if (HitTest() >= 0)
				{
					// Cancel popup when we hit a button a second time
					SendMessage(WM_CANCELMODE, 0L, 0L);
					return TRUE;
				}
			}
			return FALSE;

		case WM_LBUTTONDBLCLK:
			// Perform default action for DblClick on MDI Maxed icon
			if (IsMDIChildMaxed() && (0 == HitTest()))
			{
				CWnd* pMDIChild = FromHandle(GetActiveMDIChild());
				CMenu* pChildMenu = pMDIChild->GetSystemMenu(FALSE);

				UINT nID = pChildMenu->GetDefaultItem(FALSE, 0);
				if (nID)
					pMDIChild->PostMessage(WM_SYSCOMMAND, nID, 0L);
			}

			m_bExitAfter = TRUE;
			return FALSE;

		case WM_MENUSELECT:
			{
				// store info about selected item
				m_hSelMenu = (HMENU)lParam;
				m_bSelPopup = ((HIWORD(wParam) & MF_POPUP) != 0);

				// Reflect message back to the frame window
				GetAncestor()->SendMessage(WM_MENUSELECT, wParam, lParam);
			}
			return TRUE;

		case WM_MOUSEMOVE:
			{
				CPoint pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);

				// Skip if mouse hasn't moved
				if ((pt.x == m_OldMousePos.x) && (pt.y == m_OldMousePos.y))
					return FALSE;

				m_OldMousePos.x = pt.x;
				m_OldMousePos.y = pt.y;
				ScreenToClient(pt);

				// Reflect messages back to the MenuBar for hot tracking
				SendMessage(WM_MOUSEMOVE, 0L, MAKELPARAM(pt.x, pt.y));
			}
			break;

		}
		return FALSE;
	}

	inline void CMenuBar::OnMouseLeave()
	{
		if (IsMDIFrame())
		{
			if (IsMDIChildMaxed())
			{
				CClientDC MenuBarDC(this);

				DrawMDIButton(MenuBarDC, MDI_MIN,     0);
				DrawMDIButton(MenuBarDC, MDI_RESTORE, 0);
				DrawMDIButton(MenuBarDC, MDI_CLOSE,   0);
			}
		}
	}

	inline void CMenuBar::OnMouseMove(WPARAM wParam, LPARAM lParam)
	{
		CPoint pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		if (IsMDIFrame())
		{
			if (IsMDIChildMaxed())
			{
				CClientDC MenuBarDC(this);
				int MDIButton = -1;
				if (m_MDIRect[0].PtInRect(pt)) MDIButton = 0;
				if (m_MDIRect[1].PtInRect(pt)) MDIButton = 1;
				if (m_MDIRect[2].PtInRect(pt)) MDIButton = 2;

				if (MK_LBUTTON == wParam)  // mouse moved with left mouse button is held down
				{
					// toggle the MDI button image pressed/unpressed as required
					if (MDIButton >= 0)
					{
						DrawMDIButton(MenuBarDC, MDI_MIN,     ((0 == MDIButton) && (0 == m_nMDIButton))? 2 : 0);
						DrawMDIButton(MenuBarDC, MDI_RESTORE, ((1 == MDIButton) && (1 == m_nMDIButton))? 2 : 0);
						DrawMDIButton(MenuBarDC, MDI_CLOSE,   ((2 == MDIButton) && (2 == m_nMDIButton))? 2 : 0);
					}
					else
					{
						DrawMDIButton(MenuBarDC, MDI_MIN,     0);
						DrawMDIButton(MenuBarDC, MDI_RESTORE, 0);
						DrawMDIButton(MenuBarDC, MDI_CLOSE,   0);
					}
				}
				else	// mouse moved without left mouse button held down
				{
					if (MDIButton >= 0)
					{
						DrawMDIButton(MenuBarDC, MDI_MIN,     (0 == MDIButton)? 1 : 0);
						DrawMDIButton(MenuBarDC, MDI_RESTORE, (1 == MDIButton)? 1 : 0);
						DrawMDIButton(MenuBarDC, MDI_CLOSE,   (2 == MDIButton)? 1 : 0);
					}
					else
					{
						DrawMDIButton(MenuBarDC, MDI_MIN,     0);
						DrawMDIButton(MenuBarDC, MDI_RESTORE, 0);
						DrawMDIButton(MenuBarDC, MDI_CLOSE,   0);
					}
				}
			}
		}
	}

	inline LRESULT CMenuBar::OnNotifyReflect(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CUSTOMDRAW:
			{
				return OnCustomDraw((LPNMHDR) lParam);
			}

		case TBN_DROPDOWN:
			// Always use PostMessage for USER_POPUPMENU (not SendMessage)
			PostMessage(UWM_POPUPMENU, 0L, 0L);
			break;

		case TBN_HOTITEMCHANGE:
			// This is the notification that a hot item change is about to occur
			// This is used to bring up a new popup menu when required
			{
				CPoint pt = GetCursorPos();
				if (this == WindowFromPoint(pt))	// MenuBar window must be on top
				{
					DWORD flag = ((LPNMTBHOTITEM)lParam)->dwFlags;
					if ((flag & HICF_MOUSE) && !(flag & HICF_LEAVING))
					{
						int nButton = HitTest();
						if ((m_bMenuActive) && (nButton != m_nHotItem))
						{
							SendMessage(TB_PRESSBUTTON, m_nHotItem, MAKELONG(FALSE, 0));
							m_nHotItem = nButton;
							SendMessage(WM_CANCELMODE, 0L, 0L);

							//Always use PostMessage for USER_POPUPMENU (not SendMessage)
							PostMessage(UWM_POPUPMENU, 0L, 0L);
						}
						m_nHotItem = nButton;
					}

					// Handle escape from popup menu
					if ((flag & HICF_LEAVING) && m_bKeyMode)
					{
						m_nHotItem = ((LPNMTBHOTITEM)lParam)->idOld;
						PostMessage(TB_SETHOTITEM, m_nHotItem, 0L);
					}

				}
				break;
			} //case TBN_HOTITEMCHANGE:

		} // switch(((LPNMHDR)lParam)->code)
		return 0L;
	} // CMenuBar::OnNotify(...)

	inline void CMenuBar::OnWindowPosChanged()
	{
		InvalidateRect(&m_MDIRect[0], TRUE);
		InvalidateRect(&m_MDIRect[1], TRUE);
		InvalidateRect(&m_MDIRect[2], TRUE);
		{
			CClientDC MenuBarDC(this);
			DrawAllMDIButtons(MenuBarDC);
		}
	}

	inline void CMenuBar::PreCreate(CREATESTRUCT &cs)
	{
		cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE;
	}

	inline void CMenuBar::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  TOOLBARCLASSNAME;
	}

	inline void CMenuBar::ReleaseFocus()
	{
		if (m_hPrevFocus)
			::SetFocus(m_hPrevFocus);

		m_hPrevFocus = NULL;
		::ReleaseCapture();
	}

	inline void CMenuBar::SetHotItem(int nHot)
	{
		m_nHotItem = nHot;
		SendMessage(TB_SETHOTITEM, m_nHotItem, 0L);
	}

	inline void CMenuBar::SetMenu(HMENU hMenu)
	{
		assert(::IsWindow(m_hWnd));

		m_hTopMenu = hMenu;
		int nMaxedOffset = (IsMDIChildMaxed()? 1:0);

		// Remove any existing buttons
		while (SendMessage(TB_BUTTONCOUNT,  0L, 0L) > 0)
		{
			if(!SendMessage(TB_DELETEBUTTON, 0L, 0L))
				break;
		}

		// Set the Bitmap size to zero
		SendMessage(TB_SETBITMAPSIZE, 0L, MAKELPARAM(0, 0));

		if (IsMDIChildMaxed())
		{
			// Create an extra button for the MDI child system menu
			// Later we will custom draw the window icon over this button
			TBBUTTON tbb = {0};
			tbb.fsState = TBSTATE_ENABLED;
			tbb.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE ;
			tbb.iString = (INT_PTR)_T(" ");
			SendMessage(TB_ADDBUTTONS, 1, (WPARAM)&tbb);
			SetButtonText(0, _T("    "));
		}

		for (int i = 0 ; i < ::GetMenuItemCount(hMenu); ++i)
		{
			// Assign the ToolBar Button struct
			TBBUTTON tbb = {0};
			tbb.idCommand = i  + nMaxedOffset;	// Each button needs a unique ID
			tbb.fsState = TBSTATE_ENABLED;
			tbb.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
			tbb.iString = (INT_PTR)_T(" ");
			SendMessage(TB_ADDBUTTONS, 1, (WPARAM)&tbb);

			// Add the menu title to the string table
			std::vector<TCHAR> vMenuName( MAX_MENU_STRING+1, _T('\0') );
			TCHAR* szMenuName = &vMenuName[0];
			GetMenuString(hMenu, i, szMenuName, MAX_MENU_STRING, MF_BYPOSITION);
			SetButtonText(i  + nMaxedOffset, szMenuName);
		}
	}

	inline void CMenuBar::SetMenuBarTheme(MenuTheme& Theme)
	{
		m_ThemeMenu.UseThemes   = Theme.UseThemes;
		m_ThemeMenu.clrHot1     = Theme.clrHot1;
		m_ThemeMenu.clrHot2     = Theme.clrHot2;
		m_ThemeMenu.clrPressed1 = Theme.clrPressed1;
		m_ThemeMenu.clrPressed2 = Theme.clrPressed2;
		m_ThemeMenu.clrOutline  = Theme.clrOutline;

		if (IsWindow())
			Invalidate();
	}

	inline LRESULT CALLBACK CMenuBar::StaticMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
	{
		assert(GetApp());
		MSG* pMsg = (MSG*)lParam;
		TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
		assert(pTLSData);
		CMenuBar* pMenuBar = (CMenuBar*)pTLSData->pMenuBar;

		if (pMenuBar && (MSGF_MENU == nCode))
		{
			// process menu message
			if (pMenuBar->OnMenuInput(pMsg->message, pMsg->wParam, pMsg->lParam))
			{
				return TRUE;
			}
		}

		return CallNextHookEx(pTLSData->hHook, nCode, wParam, lParam);
	}

	inline void CMenuBar::OnSysCommand(WPARAM wParam, LPARAM lParam)
	{
		if (SC_KEYMENU == wParam)
		{
			if (0 == lParam)
			{
				// Alt/F10 key toggled
				GrabFocus();
				m_bKeyMode = TRUE;
				int nMaxedOffset = (IsMDIChildMaxed()? 1:0);
				SetHotItem(nMaxedOffset);
			}
			else
				// Handle key pressed with Alt held down
				DoAltKey((WORD)lParam);
		}
	}

	inline LRESULT CMenuBar::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CHAR:
			return 0L;  // Discard these messages
		case WM_DRAWITEM:
			m_pFrame->OnDrawItem(wParam, lParam);
			return TRUE; // handled
		case WM_EXITMENULOOP:
			if (m_bExitAfter)
				ExitMenu();
			m_pFrame->OnExitMenuLoop();
			break;
		case WM_INITMENUPOPUP:
			m_pFrame->OnInitMenuPopup(wParam, lParam);
			break;
		case WM_KEYDOWN:
			OnKeyDown(wParam, lParam);
			return 0L;	// Discard these messages
		case WM_KILLFOCUS:
			ExitMenu();
			return 0L;
		case WM_LBUTTONDOWN:
			// Do default processing first
			CallWindowProc(GetPrevWindowProc(), uMsg, wParam, lParam);

			OnLButtonDown(wParam, lParam);
			return 0L;
		case WM_LBUTTONUP:
			OnLButtonUp(wParam, lParam);
			break;
		case WM_MEASUREITEM:
			m_pFrame->OnMeasureItem(wParam, lParam);
			return TRUE; // handled
		case WM_MOUSELEAVE:
			OnMouseLeave();
			break;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, lParam);
			break;
		case UWM_POPUPMENU:
			DoPopupMenu();
			return 0L;
		case WM_SYSKEYDOWN:
			if ((VK_MENU == wParam) || (VK_F10 == wParam))
				return 0L;
			break;
		case WM_SYSKEYUP:
			if ((VK_MENU == wParam) || (VK_F10 == wParam))
			{
				ExitMenu();
				return 0L;
			}
			break;
		case UWM_GETMENUTHEME:
			{
				MenuTheme& tm = GetMenuBarTheme();
				return (LRESULT)&tm;
			}
		case WM_WINDOWPOSCHANGED:
			OnWindowPosChanged();
			break;
		case WM_WINDOWPOSCHANGING:
			// Bypass CToolBar::WndProcDefault for this message
			return CWnd::WndProcDefault(uMsg, wParam, lParam);

		} // switch (uMsg)

		return CToolBar::WndProcDefault(uMsg, wParam, lParam);
	} // LRESULT CMenuBar::WndProcDefault(...)



	///////////////////////////////////
	// Definitions for the CFrame class
	//
	inline CFrame::CFrame() : m_tsStatusText(_T("Ready")), m_bShowIndicatorStatus(TRUE), m_bShowMenuStatus(TRUE),
		                m_bUseReBar(FALSE), m_bUseThemes(TRUE), m_bUpdateTheme(FALSE), m_bUseToolBar(TRUE), m_bUseCustomDraw(TRUE),
						m_bShowStatusBar(TRUE), m_bShowToolBar(TRUE), m_himlMenu(NULL), m_himlMenuDis(NULL),
						m_AboutDialog(IDW_ABOUT), m_pView(NULL), m_nMaxMRU(0), m_hOldFocus(0), m_nOldID(-1)
	{
		ZeroMemory(&m_ThemeMenu, sizeof(m_ThemeMenu));

		// Do either InitCommonControls or InitCommonControlsEx
		LoadCommonControls();

		// By default, we use the rebar if we can
		if (GetComCtlVersion() >= 470)
			m_bUseReBar = TRUE;

		for (int i = 0 ; i < 3 ; ++i)
			m_OldStatus[i] = _T('\0');
	}

	inline CFrame::~CFrame()
	{
		if (m_himlMenu) ImageList_Destroy(m_himlMenu);
		if (m_himlMenuDis) ImageList_Destroy(m_himlMenuDis);
	}

	inline BOOL CFrame::AddMenuIcon(int nID_MenuItem, HICON hIcon, int cx /*= 16*/, int cy /*= 16*/)
	{
		// Get ImageList image size
		int cxOld = 0;
		int cyOld = 0;
		ImageList_GetIconSize(m_himlMenu, &cxOld, &cyOld );

		// Create a new ImageList if required
		if ((cx != cxOld) || (cy != cyOld) || (NULL == m_himlMenu))
		{
			if (m_himlMenu) ImageList_Destroy(m_himlMenu);
			m_himlMenu = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 1, 0);
			m_vMenuIcons.clear();
		}

        if (ImageList_AddIcon(m_himlMenu, hIcon) != -1)
		{
			m_vMenuIcons.push_back(nID_MenuItem);

			// Recreate the Disabled imagelist
			if (m_himlMenuDis) ImageList_Destroy(m_himlMenuDis);
			m_himlMenuDis = NULL;
			m_himlMenuDis = CreateDisabledImageList(m_himlMenu);

			return TRUE;
		}

		return FALSE;
	}

	inline UINT CFrame::AddMenuIcons(const std::vector<UINT>& MenuData, COLORREF crMask, UINT ToolBarID, UINT ToolBarDisabledID)
	// Adds the icons from a bitmap resouce to an internal ImageList for use with popup menu items.
	// Note:  If existing are a different size to the new ones, the old ones will be removed!
	//        The ToolBarDisabledID is ignored unless ToolBarID and ToolBarDisabledID bitmaps are the same size.
	{
		// Count the MenuData entries excluding seperators
		int iImages = 0;
		for (UINT i = 0 ; i < MenuData.size(); ++i)
		{
			if (MenuData[i] != 0)	// Don't count seperators
			{
				++iImages;
			}
		}

		// Load the button images from Resouce ID
		CBitmap Bitmap(ToolBarID);

		if ((0 == iImages) || (!Bitmap))
			return (UINT)m_vMenuIcons.size();	// No valid images, so nothing to do!

		BITMAP bm = Bitmap.GetBitmapData();
		int iImageWidth  = bm.bmWidth / iImages;
		int iImageHeight = bm.bmHeight;

		// Create the ImageList if required
		if (NULL == m_himlMenu)
		{
			m_himlMenu = ImageList_Create(iImageWidth, iImageHeight, ILC_COLOR32 | ILC_MASK, iImages, 0);
			m_vMenuIcons.clear();
		}
		else
		{
			int Oldcx;
			int Oldcy;

			ImageList_GetIconSize(m_himlMenu, &Oldcx, &Oldcy);
			if (iImageHeight != Oldcy)
			{
				TRACE(_T("Unable to add icons. The new icons are a different size to the old ones\n"));
				return (UINT)m_vMenuIcons.size();
			}
		}

		// Add the resource IDs to the m_vMenuIcons vector
		for (UINT j = 0 ; j < MenuData.size(); ++j)
		{
			if (MenuData[j] != 0)
			{
				m_vMenuIcons.push_back(MenuData[j]);
			}
		}

		// Add the images to the ImageList
		ImageList_AddMasked(m_himlMenu, Bitmap, crMask);

		// Create the Disabled imagelist
		if (ToolBarDisabledID)
		{
			if (0 != m_himlMenuDis)
				m_himlMenuDis = ImageList_Create(iImageWidth, iImageHeight, ILC_COLOR32 | ILC_MASK, iImages, 0);

			CBitmap BitmapDisabled(ToolBarDisabledID);
			BITMAP bmDis = BitmapDisabled.GetBitmapData();

			int iImageWidthDis  = bmDis.bmWidth / iImages;
			int iImageHeightDis = bmDis.bmHeight;

			// Normal and Disabled icons must be the same size
			if ((iImageWidthDis == iImageWidth) && (iImageHeightDis == iImageHeight))
			{
				ImageList_AddMasked(m_himlMenu, BitmapDisabled, crMask);
			}
			else
			{
				ImageList_Destroy(m_himlMenuDis);
				m_himlMenuDis = CreateDisabledImageList(m_himlMenu);
			}
		}
		else
		{
			if (m_himlMenuDis) ImageList_Destroy(m_himlMenuDis);
			m_himlMenuDis = CreateDisabledImageList(m_himlMenu);
		}

		// return the number of menu icons
		return (UINT)m_vMenuIcons.size();
	}

	inline void CFrame::AddMenuBarBand()
	{
		// Adds a MenuBar to the rebar control
		REBARBANDINFO rbbi = {0};
		CSize sz = GetMenuBar().GetMaxSize();

		// Calculate the MenuBar height from the menu font
		CSize csMenuBar;
		CClientDC dcMenuBar(&GetMenuBar());
		dcMenuBar.SelectObject(GetMenuBar().GetFont());
		csMenuBar = dcMenuBar.GetTextExtentPoint32(_T("\tSomeText"), lstrlen(_T("\tSomeText")));
		int MenuBar_Height = csMenuBar.cy + 6;

		rbbi.fMask      = RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE | RBBIM_ID;
		rbbi.cxMinChild = sz.cx;
		rbbi.cx         = sz.cx;
		rbbi.cyMinChild = MenuBar_Height;
		rbbi.cyMaxChild = MenuBar_Height;
		rbbi.fStyle     = RBBS_BREAK | RBBS_VARIABLEHEIGHT | RBBS_GRIPPERALWAYS ;
		rbbi.hwndChild  = GetMenuBar();
		rbbi.wID        = IDW_MENUBAR;

		// Note: rbbi.cbSize is set inside the InsertBand function
		GetReBar().InsertBand(-1, rbbi);
		SetMenuBarBandSize();
		GetReBar().SetMenuBar(GetMenuBar());

		if (GetReBar().GetReBarTheme().LockMenuBand)
			GetReBar().ShowGripper(GetReBar().GetBand(GetMenuBar()), FALSE);
	}

	inline void CFrame::AddMRUEntry(LPCTSTR szMRUEntry)
	{
		// Erase possible duplicate entries from vector
		RemoveMRUEntry(szMRUEntry);

		// Insert the entry at the beginning of the vector
		m_vMRUEntries.insert(m_vMRUEntries.begin(), szMRUEntry);

		// Delete excessive MRU entries
		if (m_vMRUEntries.size() > m_nMaxMRU)
			m_vMRUEntries.erase(m_vMRUEntries.begin() + m_nMaxMRU, m_vMRUEntries.end());

		UpdateMRUMenu();
	}

	inline void CFrame::AddToolBarBand(CToolBar& TB, DWORD dwStyle, UINT nID)
	{
		// Adds a ToolBar to the rebar control

		// Create the ToolBar Window
		TB.Create(&GetReBar());

		// Fill the REBARBAND structure
		REBARBANDINFO rbbi = {0};
		CSize sz = TB.GetMaxSize();

		rbbi.fMask      = RBBIM_CHILDSIZE | RBBIM_STYLE |  RBBIM_CHILD | RBBIM_SIZE | RBBIM_ID;
		rbbi.cyMinChild = sz.cy;
		rbbi.cyMaxChild = sz.cy;
		rbbi.cx         = sz.cx +2;
		rbbi.cxMinChild = sz.cx +2;

		rbbi.fStyle     = dwStyle;
		rbbi.hwndChild  = TB;
		rbbi.wID        = nID;

		// Note: rbbi.cbSize is set inside the InsertBand function
		GetReBar().InsertBand(-1, rbbi);
	}

	inline void CFrame::AddToolBarButton(UINT nID, BOOL bEnabled /* = TRUE*/, LPCTSTR szText)
	// Adds Resource IDs to toolbar buttons.
	// A resource ID of 0 is a separator
	{
		GetToolBar().AddButton(nID, bEnabled);

		if(0 != szText)
			GetToolBar().SetButtonText(nID, szText);

		if (!IsWindow()) TRACE(_T("Warning ... Resource IDs for toolbars should be added in SetupToolBar\n"));
	}

	inline void CFrame::AdjustFrameRect(RECT rcView) const
	// Adjust the size of the frame to accommodate the View window's dimensions
	{
		// Adjust for the view styles
		CRect rc = rcView;
		DWORD dwStyle = (DWORD)GetView()->GetWindowLongPtr(GWL_STYLE);
		DWORD dwExStyle = (DWORD)GetView()->GetWindowLongPtr(GWL_EXSTYLE);
		AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

		// Calculate the new frame height
		CRect rcFrameBefore = GetWindowRect();
		CRect rcViewBefore = GetViewRect();
		int Height = rc.Height() + rcFrameBefore.Height() - rcViewBefore.Height();

		// Adjust for the frame styles
		dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);
		dwExStyle = (DWORD)GetWindowLongPtr(GWL_EXSTYLE);
		AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

		// Calculate final rect size, and reposition frame
		SetWindowPos(NULL, 0, 0, rc.Width(), Height, SWP_NOMOVE);
	}

	inline void CFrame::CreateToolBar()
	{
		if (IsReBarSupported() && m_bUseReBar)
			AddToolBarBand(GetToolBar(), RBBS_BREAK, IDW_TOOLBAR);	// Create the toolbar inside rebar
		else
			GetToolBar().Create(this);	// Create the toolbar without a rebar

		SetupToolBar();

		if (IsReBarSupported() && m_bUseReBar)
		{
			if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().LockMenuBand)
			{
				// Hide gripper for single toolbar
				if (GetReBar().GetBandCount() <= 2)
					GetReBar().ShowGripper(GetReBar().GetBand(GetToolBar()), FALSE);
			}
		}

		if (GetToolBar().GetToolBarData().size() > 0)
		{
			// Set the toolbar images (if not already set in SetupToolBar)
			// A mask of 192,192,192 is compatible with AddBitmap (for Win95)
			if (!GetToolBar().SendMessage(TB_GETIMAGELIST,  0L, 0L))
				SetToolBarImages(RGB(192,192,192), IDW_MAIN, 0, 0);

			// Add the icons for popup menu
			AddMenuIcons(GetToolBar().GetToolBarData(), RGB(192, 192, 192), IDW_MAIN, 0);
		}
		else
		{
			TRACE(_T("Warning ... No resource IDs assigned to the toolbar\n"));
		}
	}

	inline void CFrame::DrawCheckmark(LPDRAWITEMSTRUCT pdis, CDC& DrawDC)
	// Draws the checkmark or radiocheck transparently
	{
		CRect rc = pdis->rcItem;
		UINT fType = ((ItemData*)pdis->itemData)->fType;
		MenuTheme tm = GetMenuTheme();
		CRect rcBk;

		// Draw the checkmark's background rectangle first
		int Iconx = 16, Icony = 16;
		if (m_himlMenu) ImageList_GetIconSize(m_himlMenu, &Iconx, &Icony);
		int BarWidth = Iconx + 8;
		int left = (BarWidth - Iconx)/2;
		int top = rc.top + (rc.Height() - Icony)/2;
		rcBk.SetRect(left, top, left + Iconx, top + Icony);

		if (tm.UseThemes)
		{
			DrawDC.CreateSolidBrush(tm.clrHot2);
			DrawDC.CreatePen(PS_SOLID, 1, tm.clrOutline);

			// Draw the checkmark's background rectangle
			DrawDC.Rectangle(rcBk.left, rcBk.top, rcBk.right, rcBk.bottom);
		}

		CMemDC MemDC(FromHandle(pdis->hDC));
		int cxCheck = ::GetSystemMetrics(SM_CXMENUCHECK);
		int cyCheck = ::GetSystemMetrics(SM_CYMENUCHECK);
		MemDC.CreateBitmap(cxCheck, cyCheck, 1, 1, NULL);
		CRect rcCheck( 0, 0, cxCheck, cyCheck);

		// Copy the check mark bitmap to hdcMem
		if (MFT_RADIOCHECK == fType)
			MemDC.DrawFrameControl(rcCheck, DFC_MENU, DFCS_MENUBULLET);
		else
			MemDC.DrawFrameControl(rcCheck, DFC_MENU, DFCS_MENUCHECK);

		int xoffset = (rcBk.Width() - rcCheck.Width()-1)/2;
		int yoffset = (rcBk.Height() - rcCheck.Height()-1)/2;

		if (tm.UseThemes)
			xoffset += 2;

		// Draw a white or black check mark as required
		// Unfortunately MaskBlt isn't supported on Win95, 98 or ME, so we do it the hard way
		CMemDC MaskDC(FromHandle(pdis->hDC));
		MaskDC.CreateCompatibleBitmap(FromHandle(pdis->hDC), cxCheck, cyCheck);
		MaskDC.BitBlt(0, 0, cxCheck, cyCheck, &MaskDC, 0, 0, WHITENESS);
		
		if ((pdis->itemState & ODS_SELECTED) && (!tm.UseThemes))
		{
			// Draw a white checkmark
			MemDC.BitBlt(0, 0, cxCheck, cyCheck, &MemDC, 0, 0, DSTINVERT);
			MaskDC.BitBlt(0, 0, cxCheck, cyCheck, &MemDC, 0, 0, SRCAND);
			DrawDC.BitBlt(rcBk.left + xoffset, rcBk.top + yoffset, cxCheck, cyCheck, &MaskDC, 0, 0, SRCPAINT);
		}
		else
		{
			// Draw a black checkmark
			int BullitOffset = ((MFT_RADIOCHECK == fType) && tm.UseThemes)? 1 : 0;
			MaskDC.BitBlt( -BullitOffset, BullitOffset, cxCheck, cyCheck, &MemDC, 0, 0, SRCAND);
			DrawDC.BitBlt(rcBk.left + xoffset, rcBk.top + yoffset, cxCheck, cyCheck, &MaskDC, 0, 0, SRCAND);
		}
	}

	inline void CFrame::DrawMenuIcon(LPDRAWITEMSTRUCT pdis, CDC& DrawDC, BOOL bDisabled)
	{
		if (!m_himlMenu)
			return;
		// Get icon size
		int Iconx;
		int Icony;
		ImageList_GetIconSize(m_himlMenu, &Iconx, &Icony);
		int BarWidth = Iconx + 8;

		// get the drawing rectangle
		CRect rc = pdis->rcItem;
		int left = (BarWidth - Iconx)/2;
		int top = rc.top + (rc.Height() - Icony)/2;
		rc.SetRect(left, top, left + Iconx, top + Icony);

		// get the icon's location in the imagelist
		int iImage = -1;
		for (int i = 0 ; i < (int)m_vMenuIcons.size(); ++i)
		{
			if (pdis->itemID == m_vMenuIcons[i])
				iImage = i;
		}

		// draw the image
		if (iImage >= 0 )
		{
			if ((bDisabled) && (m_himlMenuDis))
				ImageList_Draw(m_himlMenuDis, iImage, DrawDC, rc.left, rc.top, ILD_TRANSPARENT);
			else
				ImageList_Draw(m_himlMenu, iImage, DrawDC, rc.left, rc.top, ILD_TRANSPARENT);
		}
	}

	inline void CFrame::DrawMenuText(CDC& DrawDC, LPCTSTR ItemText, CRect& rc, COLORREF colorText)
	{
		// find the position of tab character
		int nTab = -1;
		for(int i = 0; i < lstrlen(ItemText); ++i)
		{
			if(_T('\t') == ItemText[i])
			{
				nTab = i;
				break;
			}
		}

		// Draw the item text
		DrawDC.SetTextColor(colorText);
		DrawDC.DrawText(ItemText, nTab, rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

		// Draw text after tab, right aligned
		if(nTab != -1)
			DrawDC.DrawText( &ItemText[nTab + 1], -1, rc, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
	}

	inline int CFrame::GetMenuItemPos(HMENU hMenu, LPCTSTR szItem)
	// Returns the position of the menu item, given it's name
	{
		int nMenuItemCount = GetMenuItemCount(hMenu);
		MENUITEMINFO mii = {0};
		mii.cbSize = GetSizeofMenuItemInfo();

		for (int nItem = 0 ; nItem < nMenuItemCount; ++nItem)
		{
			std::vector<TCHAR> vTChar( MAX_MENU_STRING+1, _T('\0') );
			TCHAR* szStr = &vTChar[0];

			std::vector<TCHAR> vStripped( MAX_MENU_STRING+1, _T('\0') );
			TCHAR* szStripped = &vStripped[0];

			mii.fMask      = MIIM_TYPE;
			mii.fType      = MFT_STRING;
			mii.dwTypeData = szStr;
			mii.cch        = MAX_MENU_STRING;

			// Fill the contents of szStr from the menu item
			if (::GetMenuItemInfo(hMenu, nItem, TRUE, &mii) && (lstrlen(szStr) <= MAX_MENU_STRING))
			{
				// Strip out any & characters
				int j = 0;
				for (int i = 0; i < lstrlen(szStr); ++i)
				{
					if (szStr[i] != _T('&'))
						szStripped[j++] = szStr[i];
				}
				szStripped[j] = _T('\0');	// Append null tchar

				// Compare the strings
				if (0 == lstrcmp(szStripped, szItem))
					return nItem;
			}
		}

		return -1;
	}

	inline tString CFrame::GetMRUEntry(UINT nIndex)
	{
		tString tsPathName;
		if (nIndex < m_vMRUEntries.size())
		{
			tsPathName = m_vMRUEntries[nIndex];

			// Now put the selected entry at Index 0
			AddMRUEntry(tsPathName.c_str());
		}
		return tsPathName;
	}

	inline CRect CFrame::GetViewRect() const
	{
		// Get the frame's client area
		CRect rcFrame = GetClientRect();

		// Get the statusbar's window area
		CRect rcStatus;
		if (GetStatusBar().IsWindowVisible() || !IsWindowVisible())
			rcStatus = GetStatusBar().GetWindowRect();

		// Get the top rebar or toolbar's window area
		CRect rcTop;
		if (IsReBarSupported() && m_bUseReBar)
			rcTop = GetReBar().GetWindowRect();
		else
			if (GetToolBar().IsWindow() && GetToolBar().IsWindowVisible())
				rcTop = GetToolBar().GetWindowRect();

		// Return client size less the rebar and status windows
		int top = rcFrame.top + rcTop.Height();
		int left = rcFrame.left;
		int right = rcFrame.right;
		int bottom = rcFrame.Height() - (rcStatus.Height());
		if ((bottom <= top) ||( right <= left))
			top = left = right = bottom = 0;

		CRect rcView(left, top, right, bottom);
		return rcView;
	}

	inline void CFrame::LoadCommonControls()
	{
		HMODULE hComCtl;

		try
		{
			// Load the Common Controls DLL
			hComCtl = ::LoadLibrary(_T("COMCTL32.DLL"));
			if (!hComCtl)
				throw CWinException(_T("Failed to load COMCTL32.DLL"));

			if (GetComCtlVersion() > 470)
			{
				// Declare a pointer to the InItCommonControlsEx function
				typedef BOOL WINAPI INIT_EX(INITCOMMONCONTROLSEX*);
				INIT_EX* pfnInit = (INIT_EX*)::GetProcAddress(hComCtl, "InitCommonControlsEx");

				// Load the full set of common controls
				INITCOMMONCONTROLSEX InitStruct = {0};
				InitStruct.dwSize = sizeof(INITCOMMONCONTROLSEX);
				InitStruct.dwICC = ICC_COOL_CLASSES|ICC_DATE_CLASSES|ICC_INTERNET_CLASSES|ICC_NATIVEFNTCTL_CLASS|
							ICC_PAGESCROLLER_CLASS|ICC_USEREX_CLASSES|ICC_WIN95_CLASSES;

				// Call InitCommonControlsEx
				if(!((*pfnInit)(&InitStruct)))
					throw CWinException(_T("InitCommonControlsEx failed"));
			}
			else
			{
				::InitCommonControls();
			}

			::FreeLibrary(hComCtl);
		}

		catch (const CWinException &e)
		{
			e.what();
			if (hComCtl)
				::FreeLibrary(hComCtl);

			throw;
		}
	}

	inline BOOL CFrame::LoadRegistryMRUSettings(UINT nMaxMRU /*= 0*/)
	{
		// Load the MRU list from the registry

		assert(!m_tsKeyName.empty()); // KeyName must be set before calling LoadRegistryMRUSettings
		HKEY hKey = NULL;
		BOOL bRet = FALSE;

		try
		{
			m_nMaxMRU = MIN(nMaxMRU, 16);
			std::vector<tString> vMRUEntries;
			tString tsKey = _T("Software\\") + m_tsKeyName + _T("\\Recent Files");

			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, tsKey.c_str(), 0, KEY_READ, &hKey))
			{
				for (UINT i = 0; i < m_nMaxMRU; ++i)
				{
					DWORD dwType = REG_SZ;
					DWORD dwBufferSize = 0;
					TCHAR szSubKey[10] = _T("");
					wsprintf(szSubKey, _T("File %d\0"), i+1);

					if (ERROR_SUCCESS != RegQueryValueEx(hKey, szSubKey, NULL, &dwType, NULL, &dwBufferSize))
						throw CWinException(_T("RegQueryValueEx failed\n"));

					std::vector<TCHAR> PathName( dwBufferSize, _T('\0') );
					TCHAR* pTCharArray = &PathName[0];

					// load the entry from the registry
					if (ERROR_SUCCESS != RegQueryValueEx(hKey, szSubKey, NULL, &dwType, (LPBYTE)pTCharArray, &dwBufferSize))
						throw CWinException(_T("RegQueryValueEx failed\n"));

					if ( lstrlen( pTCharArray ) )
						vMRUEntries.push_back( pTCharArray );
				}

				// successfully loaded all MRU values, so store them
				m_vMRUEntries = vMRUEntries;
				RegCloseKey(hKey);
				bRet = TRUE;
			}
		}

		catch(const CWinException& e)
		{
			TRACE(_T("Failed to load MRU values from registry\n"));
			e.what();

			if (hKey)
				RegCloseKey(hKey);
		}

		return bRet;
	}

	inline BOOL CFrame::LoadRegistrySettings(LPCTSTR szKeyName)
	{
		assert (NULL != szKeyName);
		m_tsKeyName = szKeyName;

		tString tsKey = _T("Software\\") + m_tsKeyName + _T("\\Frame Settings");
		HKEY hKey = 0;
		BOOL bRet = FALSE;

		try
		{
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, tsKey.c_str(), 0, KEY_READ, &hKey))
			{
				DWORD dwType = REG_BINARY;
				DWORD BufferSize = sizeof(DWORD);
				DWORD dwTop, dwLeft, dwWidth, dwHeight, dwStatusBar, dwToolBar;
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Top"), NULL, &dwType, (LPBYTE)&dwTop, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Left"), NULL, &dwType, (LPBYTE)&dwLeft, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Width"), NULL, &dwType, (LPBYTE)&dwWidth, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Height"), NULL, &dwType, (LPBYTE)&dwHeight, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("StatusBar"), NULL, &dwType, (LPBYTE)&dwStatusBar, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));
				if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("ToolBar"), NULL, &dwType, (LPBYTE)&dwToolBar, &BufferSize))
					throw CWinException(_T("RegQueryValueEx Failed"));

				m_rcPosition.top = dwTop;
				m_rcPosition.left = dwLeft;
				m_rcPosition.bottom = m_rcPosition.top + dwHeight;
				m_rcPosition.right = m_rcPosition.left + dwWidth;
				m_bShowStatusBar = dwStatusBar & 1;
				m_bShowToolBar = dwToolBar & 1;

				RegCloseKey(hKey);
				bRet = TRUE;
			}
		}

		catch (const CWinException& e)
		{
			TRACE(_T("Failed to load values from registry, using defaults!\n"));
			e.what();

			if (hKey)
				RegCloseKey(hKey);
		}

		return bRet;
	}

	inline void CFrame::OnActivate(WPARAM wParam, LPARAM lParam)
	{
		// Do default processing first
		DefWindowProc(WM_ACTIVATE, wParam, lParam);

		if (LOWORD(wParam) == WA_INACTIVE)
		{
			// Save the hwnd of the window which currently has focus
			// (this must be CFrame window itself or a child window
			if (!IsIconic()) m_hOldFocus = ::GetFocus();

			// Send a notification to the view window
			int idCtrl = ::GetDlgCtrlID(m_hOldFocus);
			NMHDR nhdr={0};
			nhdr.hwndFrom = m_hOldFocus;
			nhdr.idFrom = idCtrl;
			nhdr.code = UWM_FRAMELOSTFOCUS;
			if (GetView()->IsWindow())
				GetView()->SendMessage(WM_NOTIFY, (WPARAM)idCtrl, (LPARAM)&nhdr);
		}
		else
		{
			// Now set the focus to the appropriate child window
			if (m_hOldFocus) ::SetFocus(m_hOldFocus);

			// Send a notification to the view window
			int idCtrl = ::GetDlgCtrlID(m_hOldFocus);
			NMHDR nhdr={0};
			nhdr.hwndFrom = m_hOldFocus;
			nhdr.idFrom = idCtrl;
			nhdr.code = UWM_FRAMEGOTFOCUS;
			if (GetView()->IsWindow())
				GetView()->SendMessage(WM_NOTIFY, (WPARAM)idCtrl, (LPARAM)&nhdr);
		}
	}

	inline void CFrame::OnClose()
	{
		// Called in response to a WM_CLOSE message for the frame.
		ShowWindow(SW_HIDE);
		SaveRegistrySettings();

		GetMenuBar().Destroy();
		GetToolBar().Destroy();
		GetReBar().Destroy();
		GetStatusBar().Destroy();
		GetView()->Destroy();
	}

	inline void CFrame::OnCreate()
	{
		// This is called when the frame window is being created.
		// Override this in CMainFrame if you wish to modify what happens here

		// Set the icon
		SetIconLarge(IDW_MAIN);
		SetIconSmall(IDW_MAIN);

		// Set the keyboard accelerators
		m_hAccel = LoadAccelerators(GetApp()->GetResourceHandle(), MAKEINTRESOURCE(IDW_MAIN));
		GetApp()->SetAccelerators(m_hAccel, this);

		// Set the Caption
		SetWindowText(LoadString(IDW_MAIN));

		// Set the theme for the frame elements
		SetTheme();

		// Create the rebar and menubar
		if (IsReBarSupported() && m_bUseReBar)
		{
			// Create the rebar
			GetReBar().Create(this);

			// Create the menu inside rebar
			GetMenuBar().Create(&GetReBar());
			AddMenuBarBand();
		}

		// Setup the menu
		SetFrameMenu(IDW_MAIN);
		UpdateMRUMenu();

		// Create the ToolBar
		if (m_bUseToolBar)
		{
			CreateToolBar();
			ShowToolBar(m_bShowToolBar);
		}
		else
		{
			::CheckMenuItem(GetFrameMenu(), IDW_VIEW_TOOLBAR, MF_UNCHECKED);
			::EnableMenuItem(GetFrameMenu(), IDW_VIEW_TOOLBAR, MF_GRAYED);
		}

		// Create the status bar
		GetStatusBar().Create(this);
		ShowStatusBar(m_bShowStatusBar);

		// Create the view window
		assert(GetView());			// Use SetView in CMainFrame's constructor to set the view window
		GetView()->Create(this);

		// Disable XP themes for the menubar
		if ( m_bUseThemes || (GetWinVersion() < 2600)  )	// themes or WinVersion < Vista
			GetMenuBar().SetWindowTheme(L" ", L" ");

		// Start timer for Status updates
		if (m_bShowIndicatorStatus || m_bShowMenuStatus)
			SetTimer(ID_STATUS_TIMER, 200, NULL);

		// Reposition the child windows
		RecalcLayout();
	}

	inline void CFrame::OnDestroy()
	{
		SetMenu(NULL);
		KillTimer(ID_STATUS_TIMER);
		::PostQuitMessage(0);	// Terminates the application
	}

	inline LRESULT CFrame::OnDrawItem(WPARAM wParam, LPARAM lParam)
	// OwnerDraw is used to render the popup menu items
	{
		LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT) lParam;
		if (pdis->CtlType != ODT_MENU)
			return CWnd::WndProcDefault(WM_DRAWITEM, wParam, lParam);

		CRect rc = pdis->rcItem;
		ItemData* pmd = (ItemData*)pdis->itemData;
		CDC* pDrawDC = FromHandle(pdis->hDC);
		MenuTheme tm = GetMenuTheme();

		int Iconx = 16;
		int Icony = 16;
		if (m_himlMenu)	ImageList_GetIconSize(m_himlMenu, &Iconx, &Icony);
		int BarWidth = tm.UseThemes? Iconx + 8 : 0;

		// Draw the side bar
		if (tm.UseThemes)
		{
			CRect rcBar = rc;
			rcBar.right = BarWidth;
			pDrawDC->GradientFill(tm.clrPressed1, tm.clrPressed2, rcBar, TRUE);
		}

		if (pmd->fType & MFT_SEPARATOR)
		{
			// draw separator
			CRect rcSep = rc;
			rcSep.left = BarWidth;
			if (tm.UseThemes)
				pDrawDC->SolidFill(RGB(255,255,255), rcSep);
			else
				pDrawDC->SolidFill(GetSysColor(COLOR_MENU), rcSep);
			rcSep.top += (rc.bottom - rc.top)/2;
			rcSep.left = BarWidth + 2;
			pDrawDC->DrawEdge(rcSep,  EDGE_ETCHED, BF_TOP);
		}
		else
		{
			// draw menu item
			BOOL bDisabled = pdis->itemState & ODS_GRAYED;
			BOOL bSelected = pdis->itemState & ODS_SELECTED;
			BOOL bChecked  = pdis->itemState & ODS_CHECKED;
			CRect rcDraw = rc;

			if ((bSelected) && (!bDisabled))
			{
				// draw selected item background
				if (tm.UseThemes)
				{
					pDrawDC->CreateSolidBrush(tm.clrHot1);
					pDrawDC->CreatePen(PS_SOLID, 1, tm.clrOutline);
					pDrawDC->Rectangle(rcDraw.left, rcDraw.top, rcDraw.right, rcDraw.bottom);
				}
				else
					pDrawDC->SolidFill(GetSysColor(COLOR_HIGHLIGHT), rcDraw);
			}
			else
			{
				// draw non-selected item background
				rcDraw.left = BarWidth;
				if (tm.UseThemes)
					pDrawDC->SolidFill(RGB(255,255,255), rcDraw);
				else
					pDrawDC->SolidFill(GetSysColor(COLOR_MENU), rcDraw);
			}

			if (bChecked)
				DrawCheckmark(pdis, *pDrawDC);
			else
				DrawMenuIcon(pdis, *pDrawDC, bDisabled);

			// Calculate the text rect size
			rc.left  = rc.bottom - rc.top + 2;
			if (_tcschr(pmd->GetItemText(), _T('\t')))
				rc.right -= POST_TEXT_GAP;	// Add POST_TEXT_GAP if the text includes a tab

			// Draw the text
			int iMode = pDrawDC->SetBkMode(TRANSPARENT);
			COLORREF colorText;
			if (tm.UseThemes)
			{
				rc.left += 8;
				colorText = GetSysColor(bDisabled ?  COLOR_GRAYTEXT : COLOR_MENUTEXT);
			}
			else
				colorText = GetSysColor(bDisabled ?  COLOR_GRAYTEXT : bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);

			DrawMenuText(*pDrawDC, pmd->GetItemText(), rc, colorText);
			pDrawDC->SetBkMode(iMode); 
		}

		pDrawDC->Detach();	// Optional, deletes GDI objects sooner
		return TRUE;
	}

	inline void CFrame::OnExitMenuLoop()
	{
		if (m_bUseCustomDraw)
		{
			for (UINT nItem = 0; nItem < m_vMenuItemData.size(); ++nItem)
			{
				// Undo OwnerDraw and put the text back
				MENUITEMINFO mii = {0};
				mii.cbSize = GetSizeofMenuItemInfo();

				mii.fMask = MIIM_TYPE | MIIM_DATA;
				mii.fType = m_vMenuItemData[nItem]->fType;
				mii.dwTypeData = m_vMenuItemData[nItem]->GetItemText();
				mii.cch = lstrlen(m_vMenuItemData[nItem]->GetItemText());
				mii.dwItemData = 0;
				::SetMenuItemInfo(m_vMenuItemData[nItem]->hMenu, m_vMenuItemData[nItem]->nPos, TRUE, &mii);
			}

			m_vMenuItemData.clear();
		}
	}

	inline void CFrame::OnHelp()
	{
		// Ensure only one dialog displayed even for multiple hits of the F1 button
		if (!m_AboutDialog.IsWindow())
		{
			// Store the window handle that currently has keyboard focus
			HWND hPrevFocus = ::GetFocus();
			if (hPrevFocus == GetMenuBar())
				hPrevFocus = m_hWnd;

			m_AboutDialog.SetDlgParent(this);
			m_AboutDialog.DoModal();

			::SetFocus(hPrevFocus);
		}
	}

	inline void CFrame::OnInitMenuPopup(WPARAM wParam, LPARAM lParam)
	{
		// The system menu shouldn't be owner drawn
		if (HIWORD(lParam)) return;

		if (m_bUseCustomDraw)
		{
			CMenu* pMenu = FromHandle((HMENU)wParam);

			for (UINT i = 0; i < pMenu->GetMenuItemCount(); ++i)
			{
				MENUITEMINFO mii = {0};
				mii.cbSize = GetSizeofMenuItemInfo();

				TCHAR szMenuItem[MAX_MENU_STRING] = _T("");

				// Use old fashioned MIIM_TYPE instead of MIIM_FTYPE for MS VC6 compatibility
				mii.fMask  = MIIM_TYPE | MIIM_DATA | MIIM_SUBMENU;
				mii.dwTypeData = szMenuItem;
				mii.cch = MAX_MENU_STRING -1;

				// Send message for menu updates
				UINT menuItem = pMenu->GetMenuItemID(i);
				SendMessage(UWM_UPDATE_COMMAND, (WPARAM)menuItem, 0);

				// Specify owner-draw for the menu item type
				if (pMenu->GetMenuItemInfo(i, &mii, TRUE))
				{
					if (0 == mii.dwItemData)
					{
						ItemData* pItem = new ItemData;		// deleted in OnExitMenuLoop
						pItem->hMenu = *pMenu;
						pItem->nPos = i;
						pItem->fType = mii.fType;
						pItem->hSubMenu = mii.hSubMenu;
						mii.fType |= MFT_OWNERDRAW;
						lstrcpyn(pItem->GetItemText(), szMenuItem, MAX_MENU_STRING);
						mii.dwItemData = (DWORD_PTR)pItem;

						m_vMenuItemData.push_back(ItemDataPtr(pItem));		// Store pItem in m_vMenuItemData
						pMenu->SetMenuItemInfo(i, &mii, TRUE); // Store pItem in mii
					}
				}
			}
		}
	}

	inline LRESULT CFrame::OnMeasureItem(WPARAM wParam, LPARAM lParam)
	// Called before the Popup menu is displayed, so that the MEASUREITEMSTRUCT
	//  values can be assigned with the menu item's dimensions.
	{
		LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT) lParam;
		if (pmis->CtlType != ODT_MENU)
			return CWnd::WndProcDefault(WM_MEASUREITEM, wParam, lParam);

		ItemData* pmd = (ItemData *) pmis->itemData;
		assert(::IsMenu(pmd->hMenu));	// Does itemData contain a valid ItemData struct?
		MenuTheme tm = GetMenuTheme();

		if (pmd->fType & MFT_SEPARATOR)
		{
			pmis->itemHeight = 7;
			pmis->itemWidth  = 0;
		}

		else
		{
			CClientDC DesktopDC(NULL);

			// Get the font used in menu items
			NONCLIENTMETRICS nm = {0};
			nm.cbSize = GetSizeofNonClientMetrics();
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(nm), &nm, 0);
			// Default menu items are bold, so take this into account
			if ((int)::GetMenuDefaultItem(pmd->hMenu, TRUE, GMDI_USEDISABLED) != -1)
				nm.lfMenuFont.lfWeight = FW_BOLD;

			TCHAR* pItemText = &(pmd->vItemText[0]);
			DesktopDC.CreateFontIndirect(&nm.lfMenuFont);

			// Calculate the size of the text
			CSize size = DesktopDC.GetTextExtentPoint32(pItemText, lstrlen(pItemText));

			// Calculate the size of the icon
			int Iconx = 16;
			int Icony = 16;
			if (m_himlMenu) ImageList_GetIconSize(m_himlMenu, &Iconx, &Icony);

			pmis->itemHeight = 2+ MAX(MAX(size.cy, GetSystemMetrics(SM_CYMENU)-2), Icony+2);
			pmis->itemWidth = size.cx + MAX(::GetSystemMetrics(SM_CXMENUSIZE), Iconx+2);

			// Allow extra width if the text includes a tab
			if (_tcschr(pItemText, _T('\t')))
				pmis->itemWidth += POST_TEXT_GAP;

			// Allow extra width if the menu item has a sub menu
			if (pmd->hSubMenu)
				pmis->itemWidth += 10;

			// Allow extra width for themed menu
			if (tm.UseThemes)
				pmis->itemWidth += 8;
		}
		 return TRUE;
	}

	inline LRESULT CFrame::OnMenuChar(WPARAM wParam, LPARAM lParam)
	{
		if ((IsMenuBarUsed()) && (LOWORD(wParam)!= VK_SPACE))
		{
			// Activate MenuBar for key pressed with Alt key held down
			GetMenuBar().OnMenuChar(wParam, lParam);
			return -1L;
		}
		return CWnd::WndProcDefault(WM_MENUCHAR, wParam, lParam);
	}

	inline void CFrame::OnMenuSelect(WPARAM wParam, LPARAM lParam)
	{
		// Set the StatusBar text when we hover over a menu
		// Only popup submenus have status strings
		if (m_bShowMenuStatus)
		{
			int nID = LOWORD (wParam);
			CMenu* pMenu = FromHandle((HMENU) lParam);

			if ((pMenu != GetMenu()) && (nID != 0) && !(HIWORD(wParam) & MF_POPUP))
				m_tsStatusText = LoadString(nID);
			else
				m_tsStatusText = _T("Ready");

			SetStatusText();
		}
	}

	inline LRESULT CFrame::OnNotify(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		switch (((LPNMHDR)lParam)->code)
		{
		case UWM_UNDOCKED:
			m_hOldFocus = 0;
			break;
		case RBN_HEIGHTCHANGE:
			RecalcLayout();
			Invalidate();
			break;
	//	case RBN_LAYOUTCHANGED:
	//		if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().BandsLeft)
	//			GetReBar().MoveBandsLeft();
	//		break;
		case RBN_MINMAX:
			if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().ShortBands)
				return 1L;	// Supress maximise or minimise rebar band
			break;

		// Display tooltips for the toolbar
		case TTN_GETDISPINFO:
			if (GetToolBar().IsWindow())
			{
				CToolBar* pToolBar = 0;
				if (IsReBarUsed())
				{
					// Get the ToolBar's CWnd
					CWnd* pWnd = FromHandle(GetReBar().HitTest(GetCursorPos()));
					if (pWnd && (lstrcmp(pWnd->GetClassName(), _T("ToolbarWindow32")) == 0))
					{
						pToolBar = (CToolBar*)pWnd;
					}
				}

				if (pToolBar)
				{
					LPNMTTDISPINFO lpDispInfo = (LPNMTTDISPINFO)lParam;
					int iIndex =  pToolBar->HitTest();
					if (iIndex >= 0)
					{
						int nID = pToolBar->GetCommandID(iIndex);
						if (nID > 0)
						{
							m_tsTooltip = LoadString(nID);
							lpDispInfo->lpszText = (LPTSTR)m_tsTooltip.c_str();
						}
						else
							m_tsTooltip = _T("");
					}
				}
			}
			break;
		} // switch LPNMHDR

		return 0L;

	} // CFrame::Onotify(...)

	inline void CFrame::OnSetFocus()
	{
		SetStatusText();
	}

	inline void CFrame::OnSysColorChange()
	{
		// Honour theme color changes
		for (int nBand = 0; nBand <= GetReBar().GetBandCount(); ++nBand)
		{
			GetReBar().SetBandColor(nBand, GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE));
		}

		// Update the status bar font and text
		NONCLIENTMETRICS nm = {0};
		nm.cbSize = GetSizeofNonClientMetrics();
		SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &nm, 0);
		LOGFONT lf = nm.lfStatusFont;
		CFont* pFont = FromHandle(CreateFontIndirect(&lf));
		GetStatusBar().SetFont(pFont, FALSE);
		SetStatusText();

		if ((m_bUpdateTheme) && (m_bUseThemes)) SetTheme();

		// Reposition and redraw everything
		RecalcLayout();
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

		// Forward the message to the view window
		m_pView->PostMessage(WM_SYSCOLORCHANGE, 0L, 0L);
	}

	inline LRESULT CFrame::OnSysCommand(WPARAM wParam, LPARAM lParam)
	{
		if ((SC_KEYMENU == wParam) && (VK_SPACE != lParam) && IsMenuBarUsed())
		{
			GetMenuBar().OnSysCommand(wParam, lParam);
			return 0L;
		}

		if (SC_MINIMIZE == wParam)
			m_hOldFocus = ::GetFocus();

		return CWnd::WndProcDefault(WM_SYSCOMMAND, wParam, lParam);
	}

	inline void CFrame::OnTimer(WPARAM wParam)
	{
		if (ID_STATUS_TIMER == wParam)
		{
			if (m_bShowMenuStatus)
			{
				// Get the toolbar the point is over
				CToolBar* pToolBar = 0;
				if (IsReBarUsed())
				{
					// Get the ToolBar's CWnd
					CWnd* pWnd = FromHandle(GetReBar().HitTest(GetCursorPos()));
					if (pWnd && (dynamic_cast<CToolBar*>(pWnd)) && !(dynamic_cast<CMenuBar*>(pWnd)))
						pToolBar = (CToolBar*)pWnd;
				}
				else
				{
					CPoint pt = GetCursorPos();
					CWnd* pWnd = WindowFromPoint(GetCursorPos());
					if (pWnd && (dynamic_cast<CToolBar*>(pWnd)))
						pToolBar = (CToolBar*)pWnd;
				}

				if ((pToolBar) && (WindowFromPoint(GetCursorPos()) == pToolBar))
				{
					// Which toolbar button is the mouse cursor hovering over?
					int nButton = pToolBar->HitTest();
					if (nButton >= 0)
					{
						int nID = pToolBar->GetCommandID(nButton);
						// Only update the statusbar if things have changed
						if (nID != m_nOldID)
						{
							if (nID != 0)
								m_tsStatusText = LoadString(nID);
							else
								m_tsStatusText = _T("Ready");

							if (GetStatusBar().IsWindow())
								SetStatusText();
						}
						m_nOldID = nID;
					}
				}
				else
				{
					if (m_nOldID != -1)
					{
						m_tsStatusText = _T("Ready");
						SetStatusText();
					}
					m_nOldID = -1;
				}
			}

			if (m_bShowIndicatorStatus)
				SetStatusIndicators();
		}
	}

	inline void CFrame::OnViewStatusBar()
	{
		m_bShowStatusBar = !m_bShowStatusBar;
		ShowStatusBar(m_bShowStatusBar);
	}

	inline void CFrame::OnViewToolBar()
	{
		m_bShowToolBar = !m_bShowToolBar;
		ShowToolBar(m_bShowToolBar);
	}

  	inline void CFrame::PreCreate(CREATESTRUCT& cs)
	{
		// Set the frame window styles
		cs.style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

		// Set the original window position
		cs.x  = m_rcPosition.left;
		cs.y  = m_rcPosition.top;
		cs.cx = m_rcPosition.Width();
		cs.cy = m_rcPosition.Height();
	}

	inline void CFrame::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  _T("Win32++ Frame");
	}

	inline void CFrame::RecalcLayout()
	{
		CWnd* pView = GetView();
		if ((!pView) || (!pView->GetHwnd()))
			return;

		// Resize the status bar
		if (GetStatusBar().IsWindow() && m_bShowStatusBar)
		{
			GetStatusBar().SetWindowPos(NULL, 0, 0, 0, 0, SWP_SHOWWINDOW);
			GetStatusBar().Invalidate();
			SetStatusText();
		}

		// Resize the rebar or toolbar
		if (IsReBarUsed())
		{
			GetReBar().SendMessage(WM_SIZE, 0L, 0L);
			GetReBar().Invalidate();
		}
		else if (m_bUseToolBar && m_bShowToolBar)
			GetToolBar().SendMessage(TB_AUTOSIZE, 0L, 0L);

		// Resize the View window
		CRect rClient = GetViewRect();
		if ((rClient.bottom - rClient.top) >= 0)
		{
			int x  = rClient.left;
			int y  = rClient.top;
			int cx = rClient.Width();
			int cy = rClient.Height();

			pView->SetWindowPos( NULL, x, y, cx, cy, SWP_SHOWWINDOW|SWP_ASYNCWINDOWPOS );
		}

		// Adjust rebar bands
		if (IsReBarUsed())
		{
			if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().BandsLeft)
				GetReBar().MoveBandsLeft();

			if (IsMenuBarUsed())
				SetMenuBarBandSize();
		}
	}

	inline void CFrame::RemoveMRUEntry(LPCTSTR szMRUEntry)
	{
		std::vector<tString>::iterator it;
		for (it = m_vMRUEntries.begin(); it != m_vMRUEntries.end(); ++it)
		{
			if ((*it) == szMRUEntry)
			{
				m_vMRUEntries.erase(it);
				break;
			}
		}

		UpdateMRUMenu();
	}

	inline BOOL CFrame::SaveRegistrySettings()
	{
		// Store the window position in the registry
		if (!m_tsKeyName.empty())
		{
			tString tsKeyName = _T("Software\\") + m_tsKeyName + _T("\\Frame Settings");
			HKEY hKey = NULL;

			try
			{
				if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, tsKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
					throw CWinException(_T("RegCreateKeyEx failed"));

				WINDOWPLACEMENT Wndpl = {0};
				Wndpl.length = sizeof(WINDOWPLACEMENT);

				if (GetWindowPlacement(Wndpl))
				{
					// Get the Frame's window position
					CRect rc = Wndpl.rcNormalPosition;
					DWORD dwTop = MAX(rc.top, 0);
					DWORD dwLeft = MAX(rc.left, 0);
					DWORD dwWidth = MAX(rc.Width(), 100);
					DWORD dwHeight = MAX(rc.Height(), 50);

					if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Top"), 0, REG_DWORD, (LPBYTE)&dwTop, sizeof(DWORD)))
						throw CWinException(_T("RegSetValueEx failed"));
					if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Left"), 0, REG_DWORD, (LPBYTE)&dwLeft, sizeof(DWORD)))
						throw CWinException(_T("RegSetValueEx failed"));
					if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Width"), 0, REG_DWORD, (LPBYTE)&dwWidth, sizeof(DWORD)))
						throw CWinException(_T("RegSetValueEx failed"));
					if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Height"), 0, REG_DWORD, (LPBYTE)&dwHeight, sizeof(DWORD)))
						throw CWinException(_T("RegSetValueEx failed"));
				}

				// Store the ToolBar and statusbar states
				DWORD dwShowToolBar = m_bShowToolBar;
				DWORD dwShowStatusBar = m_bShowStatusBar;

				if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("ToolBar"), 0, REG_DWORD, (LPBYTE)&dwShowToolBar, sizeof(DWORD)))
					throw CWinException(_T("RegSetValueEx failed"));
				if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("StatusBar"), 0, REG_DWORD, (LPBYTE)&dwShowStatusBar, sizeof(DWORD)))
					throw CWinException(_T("RegSetValueEx failed"));

				RegCloseKey(hKey);
			}

			catch (const CWinException& e)
			{
				TRACE(_T("Failed to save registry settings\n"));

				if (hKey)
				{
					// Roll back the registry changes by deleting this subkey
					RegDeleteKey(HKEY_CURRENT_USER ,tsKeyName.c_str());
					RegCloseKey(hKey);
				}

				e.what();
				return FALSE;
			}

			// Store the MRU entries in the registry
			if (m_nMaxMRU > 0)
			{
				tString tsKeyName = _T("Software\\") + m_tsKeyName + _T("\\Recent Files");
				HKEY hKey = NULL;

				try
				{
					if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, tsKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
						throw CWinException(_T("RegCreateKeyEx failed"));

					for (UINT i = 0; i < m_nMaxMRU; ++i)
					{
						TCHAR szSubKey[10];
						wsprintf(szSubKey, _T("File %d\0"), i+1);
						tString tsPathName;
						if (i < m_vMRUEntries.size())
							tsPathName = m_vMRUEntries[i];

						if (ERROR_SUCCESS != RegSetValueEx(hKey, szSubKey, 0, REG_SZ, (LPBYTE)tsPathName.c_str(), (1 + lstrlen(tsPathName.c_str()))*sizeof(TCHAR)))
							throw CWinException(_T("RegSetValueEx failed"));
					}

					RegCloseKey(hKey);
				}

				catch (const CWinException& e)
				{
					TRACE(_T("Failed to save registry MRU settings\n"));

					if (hKey)
					{
						// Roll back the registry changes by deleting this subkey
						RegDeleteKey(HKEY_CURRENT_USER ,tsKeyName.c_str());
						RegCloseKey(hKey);
					}

					e.what();
					return FALSE;
				}
			}
		}

		return TRUE;
	}

	inline void CFrame::SetFrameMenu(INT ID_MENU)
	{
		// Sets the frame's menu from a Resouce ID.
		// A resource ID of 0 removes the menu from the frame.
		HMENU hMenu = 0;
		if (ID_MENU != 0)
		{
		// Sets the frame's menu from a resource ID.
			hMenu = ::LoadMenu(GetApp()->GetResourceHandle(), MAKEINTRESOURCE(ID_MENU));
			assert (hMenu);
		}

		SetFrameMenu(hMenu);
 	}

	inline void CFrame::SetFrameMenu(HMENU hMenu)
	{
		// Sets the frame's menu from a HMENU.
		m_Menu.Attach(hMenu);

		if (IsMenuBarUsed())
		{
			GetMenuBar().SetMenu(GetFrameMenu());
			BOOL bShow = (hMenu != NULL);	// boolean expression
			ShowMenu(bShow);
		}
		else
			SetMenu(&m_Menu);
 	}

	inline UINT CFrame::SetMenuIcons(const std::vector<UINT>& MenuData, COLORREF crMask, UINT ToolBarID, UINT ToolBarDisabledID)
	{
		// Remove any existing menu icons
		if (m_himlMenu) ImageList_Destroy(m_himlMenu);
		if (m_himlMenuDis) ImageList_Destroy(m_himlMenuDis);
		m_himlMenu = NULL;
		m_himlMenuDis = NULL;
		m_vMenuIcons.clear();

		// Exit if no ToolBarID is specified
		if (ToolBarID == 0) return 0;

		// Add the menu icons from the bitmap IDs
		return AddMenuIcons(MenuData, crMask, ToolBarID, ToolBarDisabledID);
	}

	inline void CFrame::SetMenuBarBandSize()
	{
		// Sets the minimum width of the MenuBar band to the width of the rebar
		// This prevents other bands from moving to this MenuBar's row.

		CRect rcClient = GetClientRect();
		CReBar& RB = GetReBar();
		int nBand = RB.GetBand(GetMenuBar());
		CRect rcBorder = RB.GetBandBorders(nBand);

		REBARBANDINFO rbbi = {0};
		rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_SIZE;
		RB.GetBandInfo(nBand, rbbi);

		int Width;
		if ((GetReBar().GetReBarTheme().UseThemes) && (GetReBar().GetReBarTheme().LockMenuBand))
			Width = rcClient.Width() - rcBorder.Width() - 2;
		else
			Width = GetMenuBar().GetMaxSize().cx;

		rbbi.cxMinChild = Width;
		rbbi.cx         = Width;

		RB.SetBandInfo(nBand, rbbi);
	}

	inline void CFrame::SetMenuTheme(MenuTheme& Theme)
	{
		m_ThemeMenu.UseThemes   = Theme.UseThemes;
		m_ThemeMenu.clrHot1     = Theme.clrHot1;
		m_ThemeMenu.clrHot2     = Theme.clrHot2;
		m_ThemeMenu.clrPressed1 = Theme.clrPressed1;
		m_ThemeMenu.clrPressed2 = Theme.clrPressed2;
		m_ThemeMenu.clrOutline  = Theme.clrOutline;

		GetMenuBar().SetMenuBarTheme(Theme); // Sets the theme for MenuBar buttons
		Invalidate();
	}

	inline void CFrame::SetStatusIndicators()
	{
		if (::IsWindow(GetStatusBar()))
		{
			LPCTSTR Status1 = (::GetKeyState(VK_CAPITAL) & 0x0001)? _T("\tCAP") : _T("");
			LPCTSTR Status2 = (::GetKeyState(VK_NUMLOCK) & 0x0001)? _T("\tNUM") : _T("");
			LPCTSTR Status3 = (::GetKeyState(VK_SCROLL)  & 0x0001)? _T("\tSCRL"): _T("");

			// Only update indictors if the text has changed
			if (Status1 != m_OldStatus[0]) 	GetStatusBar().SetPartText(1, (Status1));
			if (Status2 != m_OldStatus[1])  GetStatusBar().SetPartText(2, (Status2));
			if (Status3 != m_OldStatus[2])  GetStatusBar().SetPartText(3, (Status3));

			m_OldStatus[0] = Status1;
			m_OldStatus[1] = Status2;
			m_OldStatus[2] = Status3;
		}
	}

	inline void CFrame::SetStatusText()
	{
		if (::IsWindow(GetStatusBar()))
		{
			// Calculate the width of the text indicators
			CClientDC dcStatus(&GetStatusBar());
			CSize csCAP  = dcStatus.GetTextExtentPoint32(_T("\tCAP"), lstrlen(_T("\tCAP")));
			CSize csNUM  = dcStatus.GetTextExtentPoint32(_T("\tNUM"), lstrlen(_T("\tNUM")));
			CSize csSCRL = dcStatus.GetTextExtentPoint32(_T("\tSCRL "), lstrlen(_T("\tSCRL ")));

			// Get the coordinates of the parent window's client area.
			CRect rcClient = GetClientRect();
			int width = MAX(300, rcClient.right);

			if (m_bShowIndicatorStatus)
			{
				// Create 4 panes
				GetStatusBar().SetPartWidth(0, width - (csCAP.cx+csNUM.cx+csSCRL.cx+20));
				GetStatusBar().SetPartWidth(1, csCAP.cx);
				GetStatusBar().SetPartWidth(2, csNUM.cx);
				GetStatusBar().SetPartWidth(3, csSCRL.cx);

				SetStatusIndicators();
			}

			// Place text in the 1st pane
			GetStatusBar().SetPartText(0, m_tsStatusText.c_str());
		}
	}

	inline void CFrame::SetTheme()
	{
		// Note: To modify theme colors, override this function in CMainframe,
		//        and make any modifications there.

		// Avoid themes if using less than 16 bit colors
		CClientDC DesktopDC(NULL);
		if (DesktopDC.GetDeviceCaps(BITSPIXEL) < 16)
			m_bUseThemes = FALSE;

		BOOL T = TRUE;
		BOOL F = FALSE;

		if (m_bUseThemes)
		{
			// Set a flag redo SetTheme when the theme changes
			m_bUpdateTheme = TRUE;

			// Detect the XP theme name
			WCHAR Name[30] = L"";
			HMODULE hMod = ::LoadLibrary(_T("uxtheme.dll"));
			if(hMod)
			{
				typedef HRESULT (__stdcall *PFNGETCURRENTTHEMENAME)(LPWSTR pszThemeFileName, int cchMaxNameChars,
					LPWSTR pszColorBuff, int cchMaxColorChars, LPWSTR pszSizeBuff, int cchMaxSizeChars);

				PFNGETCURRENTTHEMENAME pfn = (PFNGETCURRENTTHEMENAME)GetProcAddress(hMod, "GetCurrentThemeName");

				(*pfn)(0, 0, Name, 30, 0, 0);

				::FreeLibrary(hMod);
			}

			enum Themetype{ Modern, Grey, Blue, Silver, Olive };

			int Theme = Grey;
			if (GetWinVersion() < 2600) // Not for Vista and above
			{
				if (0 == wcscmp(L"NormalColor", Name))	Theme = Blue;
				if (0 == wcscmp(L"Metallic", Name))		Theme = Silver;
				if (0 == wcscmp(L"HomeStead", Name))	Theme = Olive;
			}
			else
				Theme = Modern;

			switch (Theme)
			{
			case Modern:
				{
					ToolBarTheme tt = {T, RGB(180, 250, 255), RGB(140, 190, 255), RGB(150, 220, 255), RGB(80, 100, 255), RGB(127, 127, 255)};
					ReBarTheme tr = {T, RGB(220, 225, 250), RGB(240, 242, 250), RGB(240, 240, 250), RGB(180, 200, 230), F, T, T, T, T, F};
					MenuTheme tm = {T, RGB(180, 250, 255), RGB(140, 190, 255), RGB(240, 250, 255), RGB(120, 170, 220), RGB(127, 127, 255)};

					GetToolBar().SetToolBarTheme(tt);
					SetMenuTheme(tm); // Sets the theme for popup menus and MenuBar

					GetReBar().SetReBarTheme(tr);
				}
				break;

			case Grey:	// A color scheme suitable for 16 bit colors. Suitable for Windows older than XP.
				{
					ToolBarTheme tt = {T, RGB(182, 189, 210), RGB(182, 189, 210), RGB(133, 146, 181), RGB(133, 146, 181), RGB(10, 36, 106)};
					ReBarTheme tr = {T, RGB(212, 208, 200), RGB(212, 208, 200), RGB(230, 226, 222), RGB(220, 218, 208), F, T, T, T, T, F};
					MenuTheme tm = {T, RGB(182, 189, 210), RGB( 182, 189, 210), RGB(200, 196, 190), RGB(200, 196, 190), RGB(100, 100, 100)};

					GetToolBar().SetToolBarTheme(tt);
					SetMenuTheme(tm); // Sets the theme for popup menus and MenuBar

					GetReBar().SetReBarTheme(tr);
				}
				break;
			case Blue:
				{
					// Used for XP default (blue) color scheme
					ToolBarTheme tt = {T, RGB(255, 230, 190), RGB(255, 190, 100), RGB(255, 140, 40), RGB(255, 180, 80), RGB(192, 128, 255)};
					ReBarTheme tr = {T, RGB(150,190,245), RGB(196,215,250), RGB(220,230,250), RGB( 70,130,220), F, T, T, T, T, F};
					MenuTheme tm = {T, RGB(255, 230, 190), RGB(255, 190, 100), RGB(220,230,250), RGB(150,190,245), RGB(128, 128, 200)};

					GetToolBar().SetToolBarTheme(tt);
					SetMenuTheme(tm); // Sets the theme for popup menus and MenuBar

					GetReBar().SetReBarTheme(tr);
				}
				break;

			case Silver:
				{
					// Used for XP Silver color scheme
					ToolBarTheme tt = {T, RGB(192, 210, 238), RGB(192, 210, 238), RGB(152, 181, 226), RGB(152, 181, 226), RGB(49, 106, 197)};
					ReBarTheme tr = {T, RGB(225, 220, 240), RGB(240, 240, 245), RGB(245, 240, 255), RGB(160, 155, 180), F, T, T, T, T, F};
					MenuTheme tm = {T, RGB(196, 215, 250), RGB( 120, 180, 220), RGB(240, 240, 245), RGB(170, 165, 185), RGB(128, 128, 150)};

					GetToolBar().SetToolBarTheme(tt);
					SetMenuTheme(tm); // Sets the theme for popup menus and MenuBar

					GetReBar().SetReBarTheme(tr);
				}
				break;

			case Olive:
				{
					// Used for XP Olive color scheme
					ReBarTheme tr = {T, RGB(215, 216, 182), RGB(242, 242, 230), RGB(249, 255, 227), RGB(178, 191, 145), F, T, T, T, T, F};
					ToolBarTheme tt = {T, RGB(255, 230, 190), RGB(255, 190, 100), RGB(255, 140, 40), RGB(255, 180, 80), RGB(200, 128, 128)};
					MenuTheme tm = {T, RGB(255, 230, 190), RGB(255, 190, 100), RGB(249, 255, 227), RGB(178, 191, 145), RGB(128, 128, 128)};

					GetToolBar().SetToolBarTheme(tt);
					SetMenuTheme(tm); // Sets the theme for popup menus and MenuBar

					GetReBar().SetReBarTheme(tr);
				}
				break;
			}
		}
		else
		{
			// Use a classic style by default
			ReBarTheme tr = {T, 0, 0, 0, 0, F, T, T, F, F, F};
			GetReBar().SetReBarTheme(tr);
		}

		RecalcLayout();
	}

	inline void CFrame::SetToolBarImages(COLORREF crMask, UINT ToolBarID, UINT ToolBarHotID, UINT ToolBarDisabledID)
	// Either sets the imagelist or adds/replaces bitmap depending on ComCtl32.dll version
	// Assumes the width of the button image = bitmap_size / buttons
	// Assumes buttons have been already been added via AdddToolBarButton
	// The colour mask is ignored for 32bit bitmaps, but is required for 24bit bitmaps
	// The colour mask is often grey RGB(192,192,192) or magenta (255,0,255)
	// The color mask is ignored for 32bit bitmap resources
	// The Hot and disabled bitmap resources can be 0
	{
		GetToolBar().SetImages(crMask, ToolBarID, ToolBarHotID, ToolBarDisabledID);
	}

	inline void CFrame::SetupToolBar()
	{
		// Use this function to set the Resource IDs for the toolbar(s).

/*		// Set the Resource IDs for the toolbar buttons
		AddToolBarButton( IDM_FILE_NEW   );
		AddToolBarButton( IDM_FILE_OPEN  );
		AddToolBarButton( IDM_FILE_SAVE  );
		AddToolBarButton( 0 );				// Separator
		AddToolBarButton( IDM_EDIT_CUT   );
		AddToolBarButton( IDM_EDIT_COPY  );
		AddToolBarButton( IDM_EDIT_PASTE );
		AddToolBarButton( 0 );				// Separator
		AddToolBarButton( IDM_FILE_PRINT );
		AddToolBarButton( 0 );				// Separator
		AddToolBarButton( IDM_HELP_ABOUT );
*/
	}

	inline void CFrame::SetView(CWnd& wndView)
	// Sets or changes the View window displayed within the frame
	{
		if (m_pView != &wndView)
		{
			// Destroy the existing view window (if any)
			if (m_pView) m_pView->Destroy();

			// Assign the view window
			m_pView = &wndView;

			if (m_hWnd)
			{
				// The frame is already created, so create and position the new view too
				assert(GetView());			// Use SetView in CMainFrame's constructor to set the view window
				GetView()->Create(this);
				RecalcLayout();
			}
		}
	}

	inline void CFrame::ShowMenu(BOOL bShow)
	{
		if (bShow)
		{
			if (IsReBarUsed())
				GetReBar().SendMessage(RB_SHOWBAND, GetReBar().GetBand(GetMenuBar()), TRUE);
			else
				SetMenu(&m_Menu);
		}
		else
		{
			if (IsReBarUsed())
				GetReBar().SendMessage(RB_SHOWBAND, GetReBar().GetBand(GetMenuBar()), FALSE);
			else
				SetMenu(NULL);
		}

		if (GetReBar().IsWindow())
		{
			if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().BandsLeft)
				GetReBar().MoveBandsLeft();
		}

		// Reposition the Windows
		RecalcLayout();
	}



	inline void CFrame::ShowStatusBar(BOOL bShow)
	{
		if (bShow)
		{
			m_Menu.CheckMenuItem(IDW_VIEW_STATUSBAR, MF_CHECKED);
			GetStatusBar().ShowWindow(SW_SHOW);
		}
		else
		{
			m_Menu.CheckMenuItem(IDW_VIEW_STATUSBAR, MF_UNCHECKED);
			GetStatusBar().ShowWindow(SW_HIDE);
		}

		// Reposition the Windows
		RecalcLayout();
	}

	inline void CFrame::ShowToolBar(BOOL bShow)
	{
		if (bShow)
		{
			m_Menu.CheckMenuItem(IDW_VIEW_TOOLBAR, MF_CHECKED);
			if (IsReBarUsed())
				GetReBar().SendMessage(RB_SHOWBAND, GetReBar().GetBand(GetToolBar()), TRUE);
			else
				GetToolBar().ShowWindow(SW_SHOW);
		}
		else
		{
			m_Menu.CheckMenuItem(IDW_VIEW_TOOLBAR, MF_UNCHECKED);
			if (IsReBarUsed())
				GetReBar().SendMessage(RB_SHOWBAND, GetReBar().GetBand(GetToolBar()), FALSE);
			else
				GetToolBar().ShowWindow(SW_HIDE);
		}

		if (GetReBar().IsWindow())
		{
			if (GetReBar().GetReBarTheme().UseThemes && GetReBar().GetReBarTheme().BandsLeft)
				GetReBar().MoveBandsLeft();
		}

		// Reposition the Windows
		RecalcLayout();
	}

	inline void CFrame::UpdateMRUMenu()
	{
		if (0 >= m_nMaxMRU) return;

		// Set the text for the MRU Menu
		tString tsMRUArray[16];
		UINT MaxMRUArrayIndex = 0;
		if (m_vMRUEntries.size() > 0)
		{
			for (UINT n = 0; ((n < m_vMRUEntries.size()) && (n <= m_nMaxMRU)); ++n)
			{
				tsMRUArray[n] = m_vMRUEntries[n];
				if (tsMRUArray[n].length() > MAX_MENU_STRING - 10)
				{
					// Truncate the string if its too long
					tsMRUArray[n].erase(0, tsMRUArray[n].length() - MAX_MENU_STRING +10);
					tsMRUArray[n] = _T("... ") + tsMRUArray[n];
				}

				// Prefix the string with its number
				TCHAR tVal[5];
				wsprintf(tVal, _T("%d "), n+1);
				tsMRUArray[n] = tVal + tsMRUArray[n];
				MaxMRUArrayIndex = n;
			}
		}
		else
		{
			tsMRUArray[0] = _T("Recent Files");
		}

		// Set MRU menu items
		MENUITEMINFO mii = {0};
		mii.cbSize = GetSizeofMenuItemInfo();

		int nFileItem = 0;  // We place the MRU items under the left most menu item
		CMenu* pFileMenu = GetFrameMenu().GetSubMenu(nFileItem);

		if (pFileMenu)
		{
			// Remove all but the first MRU Menu entry
			for (UINT u = IDW_FILE_MRU_FILE2; u <= IDW_FILE_MRU_FILE1 +16; ++u)
			{
				pFileMenu->DeleteMenu(u, MF_BYCOMMAND);
			}

			int MaxMRUIndex = (int)MIN(MaxMRUArrayIndex, m_nMaxMRU);

			for (int index = MaxMRUIndex; index >= 0; --index)
			{
				mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
				mii.fState = (0 == m_vMRUEntries.size())? MFS_GRAYED : 0;
				mii.fType = MFT_STRING;
				mii.wID = IDW_FILE_MRU_FILE1 + index;
				mii.dwTypeData = (LPTSTR)tsMRUArray[index].c_str();

				BOOL bResult;
				if (index == MaxMRUIndex)
					// Replace the last MRU entry first
					bResult = pFileMenu->SetMenuItemInfo(IDW_FILE_MRU_FILE1, &mii, FALSE);
				else
					// Insert the other MRU entries next
					bResult = pFileMenu->InsertMenuItem(IDW_FILE_MRU_FILE1 + index + 1, &mii, FALSE);

				if (!bResult)
				{
					TRACE(_T("Failed to set MRU menu item\n"));
					break;
				}
			}
		}

		DrawMenuBar();
	}

	inline LRESULT CFrame::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_ACTIVATE:
			OnActivate(wParam, lParam);
			return 0L;
		case WM_CLOSE:
			OnClose();
			break;
		case WM_DESTROY:
			OnDestroy();
			return 0L;
		case WM_ERASEBKGND:
			return 0L;
		case WM_HELP:
			OnHelp();
			return 0L;
		case WM_MENUCHAR:
			return OnMenuChar(wParam, lParam);
		case WM_MENUSELECT:
			OnMenuSelect(wParam, lParam);
			return 0L;
		case WM_SETFOCUS:
			OnSetFocus();
			break;
		case WM_SIZE:
			RecalcLayout();
			return 0L;
		case WM_SYSCOLORCHANGE:
			// Changing themes trigger this
			OnSysColorChange();
			return 0L;
		case WM_SYSCOMMAND:
			return OnSysCommand(wParam, lParam);
		case WM_TIMER:
			OnTimer(wParam);
			return 0L;
		case WM_DRAWITEM:
			// Owner draw menu items
			return OnDrawItem(wParam, lParam);
		case WM_INITMENUPOPUP:
			OnInitMenuPopup(wParam, lParam);
			break;
		case WM_MEASUREITEM:
			return OnMeasureItem(wParam, lParam);
		case WM_EXITMENULOOP:
			OnExitMenuLoop();
			break;
		case UWM_GETMENUTHEME:
			{
				MenuTheme& tm = GetMenuTheme();
				return (LRESULT)&tm;
			}
		case UWM_GETREBARTHEME:
			{
				ReBarTheme& rm = GetReBarTheme();
				return (LRESULT)&rm;
			}
		case UWM_GETTOOLBARTHEME:
			{
				ToolBarTheme& tt = GetToolBarTheme();
				return (LRESULT)&tt;
			}
		} // switch uMsg

		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	} // LRESULT CFrame::WndProcDefault(...)


} // namespace Win32xx

#endif // _WIN32XX_FRAME_H_
