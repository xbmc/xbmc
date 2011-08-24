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
// mdi.h
//  Declaration of the CMDIChild and CMDIFrame classes

// The classes defined here add MDI frames support to Win32++. MDI
// (Multiple Document Interface) frames host one or more child windows. The
// child windows hosted by a MDI frame can be different types. For example,
// some MDI child windows could be used to edit text, while others could be
// used to display a bitmap. Four classes are defined here to support MDI
// frames:


// 1) CMDIFrame. This class inherits from CFrame, and adds the functionality
//    required by MDI frames. It keeps track of the MDI children created and
//    destroyed, and adjusts the menu when a MDI child is activated. Use the
//    AddMDIChild function to add MDI child windows to the MDI frame. Inherit
//    from CMDIFrame to create your own MDI frame.
//
// 2) CMDIChild: All MDI child windows (ie. CWnd classes) should inherit from
//    this class. Each MDI child type can have a different frame menu.

// Use the MDIFrame generic application as the starting point for your own MDI
// frame applications.
// Refer to the MDIDemo sample for an example on how to use these classes to
// create a MDI frame application with different types of MDI child windows.


#ifndef _WIN32XX_MDI_H_
#define _WIN32XX_MDI_H_

#include "frame.h"
#include <vector>



namespace Win32xx
{
    class CMDIChild;
    class CMDIFrame;
	typedef Shared_Ptr<CMDIChild> MDIChildPtr;

	/////////////////////////////////////
	// Declaration of the CMDIChild class
	//
	class CMDIChild : public CWnd
	{
		friend class CMDIFrame;
	public:
		CMDIChild();
		virtual ~CMDIChild();

		// These are the functions you might wish to override
		virtual HWND Create(CWnd* pParent = NULL);
		virtual void RecalcLayout();

		// These functions aren't virtual, and shouldn't be overridden
		void SetHandles(HMENU MenuName, HACCEL AccelName);
		CMDIFrame* GetMDIFrame() const;
		CWnd* GetView() const	{return m_pView;}
		void SetView(CWnd& pwndView);
		void MDIActivate() const;
		void MDIDestroy() const;
		void MDIMaximize() const;
		void MDIRestore() const;

	protected:
		// Its unlikely you would need to override these functions
		virtual LRESULT FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual void OnCreate();
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CMDIChild(const CMDIChild&);				// Disable copy construction
		CMDIChild& operator = (const CMDIChild&); // Disable assignment operator

		CWnd* m_pView;				// pointer to the View CWnd object
		HMENU m_hChildMenu;
		HACCEL m_hChildAccel;
	};


	/////////////////////////////////////
	// Declaration of the CMDIFrame class
	//
	class CMDIFrame : public CFrame
	{
		friend class CMDIChild;     // CMDIChild uses m_hOrigMenu
		typedef Shared_Ptr<CMDIChild> MDIChildPtr;

	public:
		class CMDIClient : public CWnd  // a nested class within CMDIFrame
		{
		public:
			CMDIClient() {}
			virtual ~CMDIClient() {}
			virtual HWND Create(CWnd* pParent = NULL);
			virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
			CMDIFrame* GetMDIFrame() const { return (CMDIFrame*)GetParent(); }

		private:
			CMDIClient(const CMDIClient&);				// Disable copy construction
			CMDIClient& operator = (const CMDIClient&); // Disable assignment operator
		};


		CMDIFrame();
		virtual ~CMDIFrame() {}

		virtual CMDIChild* AddMDIChild(MDIChildPtr pMDIChild);
		virtual CMDIClient& GetMDIClient() const { return (CMDIClient&)m_MDIClient; }
		virtual BOOL IsMDIFrame() const { return TRUE; }
		virtual void RemoveMDIChild(HWND hWnd);
		virtual BOOL RemoveAllMDIChildren();
		virtual void UpdateCheckMarks();

		// These functions aren't virtual, so don't override them
		std::vector <MDIChildPtr>& GetAllMDIChildren() {return m_vMDIChild;}
		CMDIChild* GetActiveMDIChild() const;
		BOOL IsMDIChildMaxed() const;
		void MDICascade(int nType = 0) const;
		void MDIIconArrange() const;
		void MDIMaximize() const;
		void MDINext() const;
		void MDIPrev() const;
		void MDIRestore() const;
		void MDITile(int nType = 0) const;
		void SetActiveMDIChild(CMDIChild* pChild);

	protected:
		// These are the functions you might wish to override
		virtual void OnClose();
		virtual void OnViewStatusBar();
		virtual void OnViewToolBar();
		virtual void OnWindowPosChanged();
		virtual void RecalcLayout();
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CMDIFrame(const CMDIFrame&);				// Disable copy construction
		CMDIFrame& operator = (const CMDIFrame&); // Disable assignment operator
		void AppendMDIMenu(HMENU hMenuWindow);
		LRESULT FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		void UpdateFrameMenu(HMENU hMenu);

		CMDIClient m_MDIClient;
		std::vector <MDIChildPtr> m_vMDIChild;
		HWND m_hActiveMDIChild;
	};

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	/////////////////////////////////////
	// Definitions for the CMDIFrame class
	//
	inline CMDIFrame::CMDIFrame() : m_hActiveMDIChild(NULL)
	{
		SetView(GetMDIClient());
	}

	inline CMDIChild* CMDIFrame::AddMDIChild(MDIChildPtr pMDIChild)
	{
		assert(NULL != pMDIChild.get()); // Cannot add Null MDI Child

		m_vMDIChild.push_back(pMDIChild);
		pMDIChild->Create(GetView());

		return pMDIChild.get();
	}

	inline void CMDIFrame::AppendMDIMenu(HMENU hMenuWindow)
	{
		// Adds the additional menu items the the "Window" submenu when
		//  MDI child windows are created

		if (!IsMenu(hMenuWindow))
			return;

		// Delete previously appended items
		int nItems = ::GetMenuItemCount(hMenuWindow);
		UINT uLastID = ::GetMenuItemID(hMenuWindow, --nItems);
		if ((uLastID >= IDW_FIRSTCHILD) && (uLastID < IDW_FIRSTCHILD + 10))
		{
			while ((uLastID >= IDW_FIRSTCHILD) && (uLastID < IDW_FIRSTCHILD + 10))
			{
				::DeleteMenu(hMenuWindow, nItems, MF_BYPOSITION);
				uLastID = ::GetMenuItemID(hMenuWindow, --nItems);
			}
			//delete the separator too
			::DeleteMenu(hMenuWindow, nItems, MF_BYPOSITION);
		}

		int nWindow = 0;

		// Allocate an iterator for our MDIChild vector
		std::vector <MDIChildPtr>::iterator v;

		for (v = GetAllMDIChildren().begin(); v < GetAllMDIChildren().end(); ++v)
		{
			if ((*v)->GetWindowLongPtr(GWL_STYLE) & WS_VISIBLE)	// IsWindowVisible is unreliable here
			{
				// Add Separator
				if (0 == nWindow)
					::AppendMenu(hMenuWindow, MF_SEPARATOR, 0, NULL);

				// Add a menu entry for each MDI child (up to 9)
				if (nWindow < 9)
				{
					tString tsMenuItem ( (*v)->GetWindowText() );

					if (tsMenuItem.length() > MAX_MENU_STRING -10)
					{
						// Truncate the string if its too long
						tsMenuItem.erase(tsMenuItem.length() - MAX_MENU_STRING +10);
						tsMenuItem += _T(" ...");
					}

					TCHAR szMenuString[MAX_MENU_STRING+1];
					wsprintf(szMenuString, _T("&%d %s"), nWindow+1, tsMenuItem.c_str());

					::AppendMenu(hMenuWindow, MF_STRING, IDW_FIRSTCHILD + nWindow, szMenuString);

					if (GetActiveMDIChild() == (*v).get())
						::CheckMenuItem(hMenuWindow, IDW_FIRSTCHILD+nWindow, MF_CHECKED);

					++nWindow;
				}
				else if (9 == nWindow)
				// For the 10th MDI child, add this menu item and return
				{
					::AppendMenu(hMenuWindow, MF_STRING, IDW_FIRSTCHILD + nWindow, _T("&Windows..."));
					return;
				}
			}
		}
	}

	inline LRESULT CMDIFrame::FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefFrameProc(m_hWnd, GetMDIClient(), uMsg, wParam, lParam);
	}

	inline CMDIChild* CMDIFrame::GetActiveMDIChild() const
	{
		return (CMDIChild*)FromHandle(m_hActiveMDIChild);
	}

	inline BOOL CMDIFrame::IsMDIChildMaxed() const
	{
		BOOL bMaxed = FALSE;
		GetMDIClient().SendMessage(WM_MDIGETACTIVE, 0L, (LPARAM)&bMaxed);
		return bMaxed;
	}

	inline void CMDIFrame::MDICascade(int nType /* = 0*/) const
	{
		// Possible values for nType are:
		// MDITILE_SKIPDISABLED	Prevents disabled MDI child windows from being cascaded.

		assert(::IsWindow(m_hWnd));
		GetView()->SendMessage(WM_MDICASCADE, (WPARAM)nType, 0L);
	}

	inline void CMDIFrame::MDIIconArrange() const
	{
		assert(::IsWindow(m_hWnd));
		GetView()->SendMessage(WM_MDIICONARRANGE, 0L, 0L);
	}

	inline void CMDIFrame::MDIMaximize() const
	{
		assert(::IsWindow(m_hWnd));
		GetView()->SendMessage(WM_MDIMAXIMIZE, 0L, 0L);
	}

	inline void CMDIFrame::MDINext() const
	{
		assert(::IsWindow(m_hWnd));
		HWND hMDIChild = GetActiveMDIChild()->GetHwnd();
		GetView()->SendMessage(WM_MDINEXT, (WPARAM)hMDIChild, FALSE);
	}

	inline void CMDIFrame::MDIPrev() const
	{
		assert(::IsWindow(m_hWnd));
		HWND hMDIChild = GetActiveMDIChild()->GetHwnd();
		GetView()->SendMessage(WM_MDINEXT, (WPARAM)hMDIChild, TRUE);
	}

	inline void CMDIFrame::MDIRestore() const
	{
		assert(::IsWindow(m_hWnd));
		GetView()->SendMessage(WM_MDIRESTORE, 0L, 0L);
	}

	inline void CMDIFrame::MDITile(int nType /* = 0*/) const
	{
		// Possible values for nType are:
		// MDITILE_HORIZONTAL	Tiles MDI child windows so that one window appears above another.
		// MDITILE_SKIPDISABLED	Prevents disabled MDI child windows from being tiled.
		// MDITILE_VERTICAL		Tiles MDI child windows so that one window appears beside another.

		assert(::IsWindow(m_hWnd));
		GetView()->SendMessage(WM_MDITILE, (WPARAM)nType, 0L);
	}

	inline void CMDIFrame::OnClose()
	{
		if (RemoveAllMDIChildren())
		{
			CFrame::OnClose();
			Destroy();
		}
	}

	inline void CMDIFrame::OnViewStatusBar()
	{
		CFrame::OnViewStatusBar();
		UpdateCheckMarks();
		GetView()->RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}

	inline void CMDIFrame::OnViewToolBar()
	{
		CFrame::OnViewToolBar();
		UpdateCheckMarks();
		GetView()->RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}

	inline void CMDIFrame::OnWindowPosChanged()
	{
		if (IsMenuBarUsed())
		{
			// Refresh MenuBar Window
			HMENU hMenu= GetMenuBar().GetMenu();
			GetMenuBar().SetMenu(hMenu);
			UpdateCheckMarks();
		}
	}

	inline BOOL CMDIFrame::PreTranslateMessage(MSG* pMsg)
	{
		if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
		{
			if (TranslateMDISysAccel(GetView()->GetHwnd(), pMsg))
				return TRUE;
		}

		return CFrame::PreTranslateMessage(pMsg);
	}

	inline void CMDIFrame::RecalcLayout()
	{
		CFrame::RecalcLayout();

		if (GetView()->IsWindow())
			MDIIconArrange();
	}

	inline BOOL CMDIFrame::RemoveAllMDIChildren()
	{
		BOOL bResult = TRUE;
		int Children = (int)m_vMDIChild.size();

		// Remove the children in reverse order
		for (int i = Children-1; i >= 0; --i)
		{
			if (IDNO == m_vMDIChild[i]->SendMessage(WM_CLOSE, 0L, 0L))	// Also removes the MDI child
				bResult = FALSE;
		}

		return bResult;
	}

	inline void CMDIFrame::RemoveMDIChild(HWND hWnd)
	{
		// Allocate an iterator for our HWND map
		std::vector <MDIChildPtr>::iterator v;

		for (v = m_vMDIChild.begin(); v!= m_vMDIChild.end(); ++v)
		{
			if ((*v)->GetHwnd() == hWnd)
			{
				m_vMDIChild.erase(v);
				break;
			}
		}

		if (GetActiveMDIChild())
		{
			if (GetActiveMDIChild()->m_hChildMenu)
				UpdateFrameMenu(GetActiveMDIChild()->m_hChildMenu);
			if (GetActiveMDIChild()->m_hChildAccel)
				GetApp()->SetAccelerators(GetActiveMDIChild()->m_hChildAccel, this);
		}
		else
		{
			if (IsMenuBarUsed())
				GetMenuBar().SetMenu(GetFrameMenu());
			else
				SetMenu(FromHandle(GetFrameMenu()));

			GetApp()->SetAccelerators(GetFrameAccel(), this);
		}
	}

	inline void CMDIFrame::SetActiveMDIChild(CMDIChild* pChild)
	{
		assert ( pChild->IsWindow() );

		GetMDIClient().SendMessage(WM_MDIACTIVATE, (WPARAM)pChild->GetHwnd(), 0L);

		// Verify
		assert ( m_hActiveMDIChild == pChild->GetHwnd() );
	}

	inline void CMDIFrame::UpdateCheckMarks()
	{
		if ((GetActiveMDIChild()) && GetActiveMDIChild()->m_hChildMenu)
		{
			HMENU hMenu = GetActiveMDIChild()->m_hChildMenu;

			UINT uCheck = GetToolBar().IsWindowVisible()? MF_CHECKED : MF_UNCHECKED;
			::CheckMenuItem(hMenu, IDW_VIEW_TOOLBAR, uCheck);

			uCheck = GetStatusBar().IsWindowVisible()? MF_CHECKED : MF_UNCHECKED;
			::CheckMenuItem (hMenu, IDW_VIEW_STATUSBAR, uCheck);
		}
	}

	inline void CMDIFrame::UpdateFrameMenu(HMENU hMenu)
	{
		int nMenuItems = GetMenuItemCount(hMenu);
		if (nMenuItems > 0)
		{
			// The Window menu is typically second from the right
			int nWindowItem = MAX (nMenuItems -2, 0);
			HMENU hMenuWindow = ::GetSubMenu (hMenu, nWindowItem);

			if (hMenuWindow)
			{
				if (IsMenuBarUsed())
				{
					AppendMDIMenu(hMenuWindow);
					GetMenuBar().SetMenu(hMenu);
				}
				else
				{
					GetView()->SendMessage (WM_MDISETMENU, (WPARAM) hMenu, (LPARAM)hMenuWindow);
					DrawMenuBar();
				}
			}
		}
		UpdateCheckMarks();
	}

	inline LRESULT CMDIFrame::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			OnClose();
			return 0;

		case WM_WINDOWPOSCHANGED:
			// MDI Child or MDI frame has been resized
			OnWindowPosChanged();
			break; // Continue with default processing

		} // switch uMsg
		return CFrame::WndProcDefault(uMsg, wParam, lParam);
	}

	inline HWND CMDIFrame::CMDIClient::Create(CWnd* pParent)
	{
		assert(pParent != 0);

		CLIENTCREATESTRUCT clientcreate ;
		clientcreate.hWindowMenu  = m_hWnd;
		clientcreate.idFirstChild = IDW_FIRSTCHILD ;
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES;

		// Create the view window
		CreateEx(WS_EX_CLIENTEDGE, _T("MDICLient"), TEXT(""), dwStyle, 0, 0, 0, 0, pParent, NULL, (PSTR) &clientcreate);

		return m_hWnd;
	}

	inline LRESULT CMDIFrame::CMDIClient::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_MDIDESTROY:
			{
				// Do default processing first
				CallWindowProc(GetPrevWindowProc(), uMsg, wParam, lParam);

				// Now remove MDI child
				GetMDIFrame()->RemoveMDIChild((HWND) wParam);
			}
			return 0; // Discard message

		case WM_MDISETMENU:
			{
				if (GetMDIFrame()->IsMenuBarUsed())
				{
					return 0L;
				}
			}
			break;

		case WM_MDIACTIVATE:
			{
				// Suppress redraw to avoid flicker when activating maximised MDI children
				SendMessage(WM_SETREDRAW, FALSE, 0L);
				LRESULT lr = CallWindowProc(GetPrevWindowProc(), WM_MDIACTIVATE, wParam, lParam);
				SendMessage(WM_SETREDRAW, TRUE, 0L);
				RedrawWindow(0, 0, RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

				return lr;
			}
		}
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}


	/////////////////////////////////////
	//Definitions for the CMDIChild class
	//
	inline CMDIChild::CMDIChild() : m_pView(NULL), m_hChildMenu(NULL)
	{
		// Set the MDI Child's menu and accelerator in the constructor, like this ...
		//   HMENU hChildMenu = LoadMenu(GetApp()->GetResourceHandle(), _T("MdiMenuView"));
		//   HACCEL hChildAccel = LoadAccelerators(GetApp()->GetResourceHandle(), _T("MDIAccelView"));
		//   SetHandles(hChildMenu, hChildAccel);
	}

	inline CMDIChild::~CMDIChild()
	{
		if (IsWindow())
			GetParent()->SendMessage(WM_MDIDESTROY, (WPARAM)m_hWnd, 0L);

		if (m_hChildMenu)
			::DestroyMenu(m_hChildMenu);
	}

	inline HWND CMDIChild::Create(CWnd* pParent /*= NULL*/)
	// We create the MDI child window and then maximize if required.
	// This technique avoids unnecessary flicker when creating maximized MDI children.
	{
		//Call PreCreate in case its overloaded
		PreCreate(*m_pcs);

		//Determine if the window should be created maximized
		BOOL bMax = FALSE;
		pParent->SendMessage(WM_MDIGETACTIVE, 0L, (LPARAM)&bMax);
		bMax = bMax | (m_pcs->style & WS_MAXIMIZE);

		// Set the Window Class Name
		TCHAR szClassName[MAX_STRING_SIZE + 1] = _T("Win32++ MDI Child");
		if (m_pcs->lpszClass)
			lstrcpyn(szClassName, m_pcs->lpszClass, MAX_STRING_SIZE);

		// Set the window style
		DWORD dwStyle;
		dwStyle = m_pcs->style & ~WS_MAXIMIZE;
		dwStyle |= WS_VISIBLE | WS_OVERLAPPEDWINDOW ;

		// Set window size and position
		int x = CW_USEDEFAULT;
		int	y = CW_USEDEFAULT;
		int cx = CW_USEDEFAULT;
		int cy = CW_USEDEFAULT;
		if(m_pcs->cx && m_pcs->cy)
		{
			x = m_pcs->x;
			y = m_pcs->y;
			cx = m_pcs->cx;
			cy = m_pcs->cy;
		}

		// Set the extended style
		DWORD dwExStyle = m_pcs->dwExStyle | WS_EX_MDICHILD;

		// Turn off redraw while creating the window
		pParent->SendMessage(WM_SETREDRAW, FALSE, 0L);

		// Create the window
		if (!CreateEx(dwExStyle, szClassName, m_pcs->lpszName, dwStyle, x, y,
			cx, cy, pParent, FromHandle(m_pcs->hMenu), m_pcs->lpCreateParams))
			throw CWinException(_T("CMDIChild::Create ... CreateEx failed"));

		if (bMax)
			ShowWindow(SW_MAXIMIZE);

		// Turn redraw back on
		pParent->SendMessage(WM_SETREDRAW, TRUE, 0L);
		pParent->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

		// Ensure bits revealed by round corners (XP themes) are redrawn
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);

		if (m_hChildMenu)
			GetMDIFrame()->UpdateFrameMenu(m_hChildMenu);
		if (m_hChildAccel)
			GetApp()->SetAccelerators(m_hChildAccel, this);

		return m_hWnd;
	}

	inline CMDIFrame* CMDIChild::GetMDIFrame() const
	{
		CMDIFrame* pMDIFrame = (CMDIFrame*)GetParent()->GetParent();
		assert(dynamic_cast<CMDIFrame*>(pMDIFrame));
		return pMDIFrame;
	}

	inline LRESULT CMDIChild::FinalWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefMDIChildProc(m_hWnd, uMsg, wParam, lParam);
	}

	inline void CMDIChild::MDIActivate() const
	{
		GetParent()->SendMessage(WM_MDIACTIVATE, (WPARAM)m_hWnd, 0L);
	}

	inline void CMDIChild::MDIDestroy() const
	{
		GetParent()->SendMessage(WM_MDIDESTROY, (WPARAM)m_hWnd, 0L);
	}

	inline void CMDIChild::MDIMaximize() const
	{
		GetParent()->SendMessage(WM_MDIMAXIMIZE, (WPARAM)m_hWnd, 0L);
	}

	inline void CMDIChild::MDIRestore() const
	{
		GetParent()->SendMessage(WM_MDIRESTORE, (WPARAM)m_hWnd, 0L);
	}

	inline void CMDIChild::OnCreate()
	{
		// Create the view window
		assert(GetView());			// Use SetView in CMDIChild's constructor to set the view window
		GetView()->Create(this);
		RecalcLayout();
	}

	inline void CMDIChild::RecalcLayout()
	{
		// Resize the View window
		CRect rc = GetClientRect();
		m_pView->SetWindowPos( NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_SHOWWINDOW );
	}

	inline void CMDIChild::SetHandles(HMENU hMenu, HACCEL hAccel)
	{
		m_hChildMenu = hMenu;
		m_hChildAccel = hAccel;

		// Note: It is valid to call SetChildMenu before the window is created
		if (IsWindow())
		{
			CWnd* pWnd = GetMDIFrame()->GetActiveMDIChild();
			if (pWnd == this)
			{
				if (m_hChildMenu)
					GetMDIFrame()->UpdateFrameMenu(m_hChildMenu);

				if (m_hChildAccel)
					GetApp()->SetAccelerators(m_hChildAccel, GetMDIFrame());
			}
		}
	}

	inline void CMDIChild::SetView(CWnd& wndView)
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
				assert(GetView());			// Use SetView in CMDIChild's constructor to set the view window
				GetView()->Create(this);
				RecalcLayout();
			}
		}
	}

	inline LRESULT CMDIChild::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_MDIACTIVATE:
			{
				// This child is being activated
				if (lParam == (LPARAM) m_hWnd)
				{
					GetMDIFrame()->m_hActiveMDIChild = m_hWnd;
					// Set the menu to child default menu
					if (m_hChildMenu)
						GetMDIFrame()->UpdateFrameMenu(m_hChildMenu);
					if (m_hChildAccel)
						GetApp()->SetAccelerators(m_hChildAccel, this);
				}

				// No child is being activated
				if (0 == lParam)
				{
					GetMDIFrame()->m_hActiveMDIChild = NULL;
					// Set the menu to frame's original menu
					GetMDIFrame()->UpdateFrameMenu(GetMDIFrame()->GetFrameMenu());
					GetApp()->SetAccelerators(GetMDIFrame()->GetFrameAccel(), this);
				}
			}
			return 0L ;

		case WM_WINDOWPOSCHANGED:
			{
				RecalcLayout();
				break;
			}
		}
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}


} // namespace Win32xx

#endif // _WIN32XX_MDI_H_

