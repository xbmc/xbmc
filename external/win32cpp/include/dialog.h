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
// dialog.h
//  Declaration of the CDialog class

// CDialog adds support for dialogs to Win32++. Dialogs are specialised
// windows which are a parent window for common controls. Common controls
// are special window types such as buttons, edit controls, tree views,
// list views, static text etc.

// The layout of a dialog is typically defined in a resource script file
// (often Resource.rc). While this script file can be constructed manually,
// it is often created using a resource editor. If your compiler doesn't
// include a resource editor, you might find ResEdit useful. It is a free
// resource editor available for download at:
// http://www.resedit.net/

// CDialog supports modal and modeless dialogs. It also supports the creation
// of dialogs defined in a resource script file, as well as those defined in
// a dialog template.

// Use the Dialog generic program as the starting point for your own dialog
// applications.
// The DlgSubclass sample demonstrates how to use subclassing to customise
// the behaviour of common controls in a dialog.


#ifndef _WIN32XX_DIALOG_H_
#define _WIN32XX_DIALOG_H_

#include "wincore.h"

#ifndef SWP_NOCOPYBITS
	#define SWP_NOCOPYBITS      0x0100
#endif

namespace Win32xx
{

	class CDialog : public CWnd
	{
	public:
		CDialog(UINT nResID, CWnd* pParent = NULL);
		CDialog(LPCTSTR lpszResName, CWnd* pParent = NULL);
		CDialog(LPCDLGTEMPLATE lpTemplate, CWnd* pParent = NULL);
		virtual ~CDialog();

		// You probably won't need to override these functions
		virtual void AttachItem(int nID, CWnd& Wnd);
		virtual HWND Create(CWnd* pParent = NULL);
		virtual INT_PTR DoModal();
		virtual HWND DoModeless();
		virtual void SetDlgParent(CWnd* pParent);
		BOOL IsModal() const { return m_IsModal; }
		BOOL IsIndirect() const { return (NULL != m_lpTemplate); }

	protected:
		// These are the functions you might wish to override
		virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual INT_PTR DialogProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual void EndDialog(INT_PTR nResult);
		virtual void OnCancel();
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		virtual BOOL PreTranslateMessage(MSG* pMsg);

		// Can't override these functions
		static INT_PTR CALLBACK StaticDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	#ifndef _WIN32_WCE
		static LRESULT CALLBACK StaticMsgHook(int nCode, WPARAM wParam, LPARAM lParam);
	#endif

	private:
		CDialog(const CDialog&);			  // Disable copy construction
		CDialog& operator = (const CDialog&); // Disable assignment operator

		BOOL m_IsModal;					// a flag for modal dialogs
		LPCTSTR m_lpszResName;			// the resource name for the dialog
		LPCDLGTEMPLATE m_lpTemplate;	// the dialog template for indirect dialogs
		HWND m_hParent;					// handle to the dialogs's parent window
	};


#ifndef _WIN32_WCE

    //////////////////////////////////////
    // Declaration of the CResizer class
    //
    // The CResizer class can be used to rearrange a dialog's child
    // windows when the dialog is resized.

    // To use CResizer, follow the following steps:
    // 1) Use Initialize to specify the dialog's CWnd, and min and max size.
    // 3) Use AddChild for each child window
    // 4) Call HandleMessage from within DialogProc.
    //

	// Resize Dialog Styles
#define RD_STRETCH_WIDTH		0x0001	// The item has a variable width
#define RD_STRETCH_HEIGHT		0x0002	// The item has a variable height

	// Resize Dialog alignments
	enum Alignment { topleft, topright, bottomleft, bottomright };

    class CResizer
    {
	public:
		CResizer() : m_pParent(0), m_xScrollPos(0), m_yScrollPos(0) {}
		virtual ~CResizer() {}

        virtual void AddChild(CWnd* pWnd, Alignment corner, DWORD dwStyle);
		virtual void AddChild(HWND hWnd, Alignment corner, DWORD dwStyle);
		virtual void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    	virtual void Initialize(CWnd* pParent, RECT rcMin, RECT rcMax = CRect(0,0,0,0));
		virtual void OnHScroll(WPARAM wParam, LPARAM lParam);
		virtual void OnVScroll(WPARAM wParam, LPARAM lParam);
		virtual void RecalcLayout();
		CRect GetMinRect() const { return m_rcMin; }
		CRect GetMaxRect() const { return m_rcMax; }

		struct ResizeData
		{
			CRect rcInit;
			CRect rcOld;
			Alignment corner;
			BOOL bFixedWidth;
			BOOL bFixedHeight;
    		HWND hWnd;
		};

    private:
        CWnd* m_pParent;
    	std::vector<ResizeData> m_vResizeData;

    	CRect m_rcInit;
    	CRect m_rcMin;
    	CRect m_rcMax;

		int m_xScrollPos;
		int m_yScrollPos;
    };

#endif

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{
    ////////////////////////////////////
	// Definitions for the CDialog class
	//
	inline CDialog::CDialog(LPCTSTR lpszResName, CWnd* pParent/* = NULL*/)
		: m_IsModal(TRUE), m_lpszResName(lpszResName), m_lpTemplate(NULL)
	{
		m_hParent = pParent? pParent->GetHwnd() : NULL;
		::InitCommonControls();
	}

	inline CDialog::CDialog(UINT nResID, CWnd* pParent/* = NULL*/)
		: m_IsModal(TRUE), m_lpszResName(MAKEINTRESOURCE (nResID)), m_lpTemplate(NULL)
	{
		m_hParent = pParent? pParent->GetHwnd() : NULL;
		::InitCommonControls();
	}

	//For indirect dialogs - created from a dialog box template in memory.
	inline CDialog::CDialog(LPCDLGTEMPLATE lpTemplate, CWnd* pParent/* = NULL*/)
		: m_IsModal(TRUE), m_lpszResName(NULL), m_lpTemplate(lpTemplate)
	{
		m_hParent = pParent? pParent->GetHwnd() : NULL;
		::InitCommonControls();
	}

	inline CDialog::~CDialog()
	{
		if (m_hWnd != NULL)
		{
			if (IsModal())
				::EndDialog(m_hWnd, 0);
			else
				Destroy();
		}
	}

	inline void CDialog::AttachItem(int nID, CWnd& Wnd)
	// Attach a dialog item to a CWnd
	{
		Wnd.AttachDlgItem(nID, this);
	}

	inline HWND CDialog::Create(CWnd* pParent /* = NULL */)
	{
		// Allow a dialog to be used as a child window

		assert(GetApp());
		SetDlgParent(pParent);
		return DoModeless();
	}

	inline INT_PTR CDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Override this function in your class derrived from CDialog if you wish to handle messages
		// A typical function might look like this:

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

		// Always pass unhandled messages on to DialogProcDefault
		return DialogProcDefault(uMsg, wParam, lParam);
	}

	inline INT_PTR CDialog::DialogProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// All DialogProc functions should pass unhandled messages to this function
	{
		LRESULT lr = 0;

		switch (uMsg)
	    {
		case UWM_CLEANUPTEMPS:
			{
				TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
				pTLSData->vTmpWnds.clear();
			}
			break;
	    case WM_INITDIALOG:
			{
				// Center the dialog
				CenterWindow();
			}
		    return OnInitDialog();
	    case WM_COMMAND:
	        switch (LOWORD (wParam))
	        {
	        case IDOK:
				OnOK();
				return TRUE;
			case IDCANCEL:
				OnCancel();
				return TRUE;
			default:
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
				break;  // Some commands require default processing
	        }
	        break;

		case WM_NOTIFY:
			{
				// Do Notification reflection if it came from a CWnd object
				HWND hwndFrom = ((LPNMHDR)lParam)->hwndFrom;
				CWnd* pWndFrom = GetApp()->GetCWndFromMap(hwndFrom);

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
	
				// Handle user notifications
				if (!lr) lr = OnNotify(wParam, lParam);

				// Set the return code for notifications
				if (IsWindow())
					SetWindowLongPtr(DWLP_MSGRESULT, (LONG_PTR)lr);

				return (BOOL)lr;
			}

		case WM_PAINT:
			{
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

				break;
			}

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
			return MessageReflect(m_hWnd, uMsg, wParam, lParam);

	    } // switch(uMsg)
	    return FALSE;

	} // INT_PTR CALLBACK CDialog::DialogProc(...)

	inline INT_PTR CDialog::DoModal()
	{
		// Create a modal dialog
		// A modal dialog box must be closed by the user before the application continues

		assert( GetApp() );		// Test if Win32++ has been started
		assert(!::IsWindow(m_hWnd));	// Only one window per CWnd instance allowed

		INT_PTR nResult = 0;

		try
		{
			m_IsModal=TRUE;

			// Ensure this thread has the TLS index set
			TLSData* pTLSData = GetApp()->SetTlsIndex();

		#ifndef _WIN32_WCE
			BOOL IsHookedHere = FALSE;
			if (NULL == pTLSData->hHook )
			{
				pTLSData->hHook = ::SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC)StaticMsgHook, NULL, ::GetCurrentThreadId());
				IsHookedHere = TRUE;
			}
		#endif

			HINSTANCE hInstance = GetApp()->GetInstanceHandle();
			pTLSData->pCWnd = this;

			// Create a modal dialog
			if (IsIndirect())
				nResult = ::DialogBoxIndirect(hInstance, m_lpTemplate, m_hParent, (DLGPROC)CDialog::StaticDialogProc);
			else
			{
				if (::FindResource(GetApp()->GetResourceHandle(), m_lpszResName, RT_DIALOG))
					hInstance = GetApp()->GetResourceHandle();
				nResult = ::DialogBox(hInstance, m_lpszResName, m_hParent, (DLGPROC)CDialog::StaticDialogProc);
			}

			// Tidy up
			m_hWnd = NULL;
			pTLSData->pCWnd = NULL;
			GetApp()->CleanupTemps();

		#ifndef _WIN32_WCE
			if (IsHookedHere)
			{
				::UnhookWindowsHookEx(pTLSData->hHook);
				pTLSData->hHook = NULL;
			}
		#endif

			if (nResult == -1)
				throw CWinException(_T("Failed to create modal dialog box"));

		}

		catch (const CWinException &e)
		{
			TRACE(_T("\n*** Failed to create dialog ***\n"));
			e.what();	// Display the last error message.

			// eat the exception (don't rethrow)
		}

		return nResult;
	}

	inline HWND CDialog::DoModeless()
	{
		assert( GetApp() );		// Test if Win32++ has been started
		assert(!::IsWindow(m_hWnd));	// Only one window per CWnd instance allowed

		try
		{
			m_IsModal=FALSE;

			// Ensure this thread has the TLS index set
			TLSData* pTLSData = GetApp()->SetTlsIndex();

			// Store the CWnd pointer in Thread Local Storage
			pTLSData->pCWnd = this;

			HINSTANCE hInstance = GetApp()->GetInstanceHandle();

			// Create a modeless dialog
			if (IsIndirect())
				m_hWnd = ::CreateDialogIndirect(hInstance, m_lpTemplate, m_hParent, (DLGPROC)CDialog::StaticDialogProc);
			else
			{
				if (::FindResource(GetApp()->GetResourceHandle(), m_lpszResName, RT_DIALOG))
					hInstance = GetApp()->GetResourceHandle();

				m_hWnd = ::CreateDialog(hInstance, m_lpszResName, m_hParent, (DLGPROC)CDialog::StaticDialogProc);
			}

			// Tidy up
			pTLSData->pCWnd = NULL;

			// Now handle dialog creation failure
			if (!m_hWnd)
				throw CWinException(_T("Failed to create dialog"));
		}

		catch (const CWinException &e)
		{
			TRACE(_T("\n*** Failed to create dialog ***\n"));
			e.what();	// Display the last error message.

			// eat the exception (don't rethrow)
		}

		return m_hWnd;
	}

	inline void CDialog::EndDialog(INT_PTR nResult)
	{
		assert(::IsWindow(m_hWnd));

		if (IsModal())
			::EndDialog(m_hWnd, nResult);
		else
			Destroy();

		m_hWnd = NULL;
	}

	inline void CDialog::OnCancel()
	{
		// Override to customize OnCancel behaviour
		EndDialog(IDCANCEL);
	}

	inline BOOL CDialog::OnInitDialog()
	{
		// Called when the dialog is initialized
		// Override it in your derived class to automatically perform tasks
		// The return value is used by WM_INITDIALOG

		return TRUE;
	}

	inline void CDialog::OnOK()
	{
		// Override to customize OnOK behaviour
		EndDialog(IDOK);
	}

	inline BOOL CDialog::PreTranslateMessage(MSG* pMsg)
	{
		// allow the dialog to translate keyboard input
		if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
		{
			// Process dialog keystrokes for modeless dialogs
			if (!IsModal())
			{
				TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
				if (NULL == pTLSData->hHook)
				{
					if (IsDialogMessage(pMsg))
						return TRUE;
				}
				else
				{
					// A modal message loop is running so we can't do IsDialogMessage.
					// Avoid having modal dialogs create other windows, because those
					// windows will then use the modal dialog's special message loop.
				}
			}
		}

		return FALSE;
	}

	inline void CDialog::SetDlgParent(CWnd* pParent)
	// Allows the parent of the dialog to be set before the dialog is created
	{
		m_hParent = pParent? pParent->GetHwnd() : NULL;
	}

	inline INT_PTR CALLBACK CDialog::StaticDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Find the CWnd pointer mapped to this HWND
		CDialog* w = (CDialog*)GetApp()->GetCWndFromMap(hWnd);
		if (0 == w)
		{
			// The HWND wasn't in the map, so add it now
			TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
			assert(pTLSData);

			// Retrieve pointer to CWnd object from Thread Local Storage TLS
			w = (CDialog*)pTLSData->pCWnd;
			assert(w);
			pTLSData->pCWnd = NULL;

			// Store the Window pointer into the HWND map
			w->m_hWnd = hWnd;
			w->AddToMap();
		}

		return w->DialogProc(uMsg, wParam, lParam);

	} // INT_PTR CALLBACK CDialog::StaticDialogProc(...)

#ifndef _WIN32_WCE
	inline LRESULT CALLBACK CDialog::StaticMsgHook(int nCode, WPARAM wParam, LPARAM lParam)
	{
		// Used by Modal Dialogs to PreTranslate Messages
		TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());

		if (nCode == MSGF_DIALOGBOX)
		{
			MSG* lpMsg = (MSG*) lParam;

			// only pre-translate keyboard events
			if ((lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST))
			{
				for (HWND hWnd = lpMsg->hwnd; hWnd != NULL; hWnd = ::GetParent(hWnd))
				{
					CDialog* pDialog = (CDialog*)GetApp()->GetCWndFromMap(hWnd);
					if (pDialog && (lstrcmp(pDialog->GetClassName(), _T("#32770")) == 0))	// only for dialogs
					{
						pDialog->PreTranslateMessage(lpMsg);
						break;
					}
				}
			}
		}

		return ::CallNextHookEx(pTLSData->hHook, nCode, wParam, lParam);
	}
#endif



#ifndef _WIN32_WCE

    /////////////////////////////////////
	// Definitions for the CResizer class
	//

	void inline CResizer::AddChild(CWnd* pWnd, Alignment corner, DWORD dwStyle)
    // Adds a child window (usually a dialog control) to the set of windows managed by
	// the Resizer.
	//
	// The alignment corner should be set to the closest corner of the dialog. Allowed
	// values are topleft, topright, bottomleft, and bottomright.
	// Set bFixedWidth to TRUE if the width should be fixed instead of variable.
	// Set bFixedHeight to TRUE if the height should be fixed instead of variable.
	{
    	ResizeData rd;
    	rd.corner = corner;
    	rd.bFixedWidth  = !(dwStyle & RD_STRETCH_WIDTH);
    	rd.bFixedHeight = !(dwStyle & RD_STRETCH_HEIGHT);
		CRect rcInit = pWnd->GetWindowRect();
		m_pParent->ScreenToClient(rcInit);
		rd.rcInit = rcInit;
		rd.hWnd = pWnd->GetHwnd();

		m_vResizeData.insert(m_vResizeData.begin(), rd);
    }

	void inline CResizer::AddChild(HWND hWnd, Alignment corner, DWORD dwStyle)
    // Adds a child window (usually a dialog control) to the set of windows managed by
	// the Resizer.	
	{
		AddChild(FromHandle(hWnd), corner, dwStyle);
	}

	inline void CResizer::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_SIZE:
			RecalcLayout();
			break;

		case WM_HSCROLL:
			if (0 == lParam)
				OnHScroll(wParam, lParam);
			break;

		case WM_VSCROLL:
			if (0 == lParam)
				OnVScroll(wParam, lParam);
			break;
		}
	}

    void inline CResizer::Initialize(CWnd* pParent, RECT rcMin, RECT rcMax)
	// Sets up the Resizer by specifying the parent window (usually a dialog),
	//  and the minimum and maximum allowed rectangle sizes.
    {
    	assert (NULL != pParent);

    	m_pParent = pParent;
    	m_rcInit = pParent->GetClientRect();
    	m_rcMin = rcMin;
    	m_rcMax = rcMax;

		// Add scroll bar support to the parent window
		DWORD dwStyle = (DWORD)m_pParent->GetClassLongPtr(GCL_STYLE);
		dwStyle |= WS_HSCROLL | WS_VSCROLL;
		m_pParent->SetClassLongPtr(GCL_STYLE, dwStyle);
    }

	void inline CResizer::OnHScroll(WPARAM wParam, LPARAM /*lParam*/)
	{
		int xNewPos;

		switch (LOWORD(wParam))
		{
			case SB_PAGEUP: // User clicked the scroll bar shaft left of the scroll box.
				xNewPos = m_xScrollPos - 50;
				break;

			case SB_PAGEDOWN: // User clicked the scroll bar shaft right of the scroll box.
				xNewPos = m_xScrollPos + 50;
				break;

			case SB_LINEUP: // User clicked the left arrow.
				xNewPos = m_xScrollPos - 5;
				break;

			case SB_LINEDOWN: // User clicked the right arrow.
				xNewPos = m_xScrollPos + 5;
				break;

			case SB_THUMBPOSITION: // User dragged the scroll box.
				xNewPos = HIWORD(wParam);
				break;

			case SB_THUMBTRACK: // User dragging the scroll box.
				xNewPos = HIWORD(wParam);
				break;

			default:
				xNewPos = m_xScrollPos;
		}

		// Scroll the window.
		xNewPos = MAX(0, xNewPos);
		xNewPos = MIN( xNewPos, GetMinRect().Width() - m_pParent->GetClientRect().Width() );
		int xDelta = xNewPos - m_xScrollPos;
		m_xScrollPos = xNewPos;
		m_pParent->ScrollWindow(-xDelta, 0, NULL, NULL);

		// Reset the scroll bar.
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask  = SIF_POS;
		si.nPos   = m_xScrollPos;
		m_pParent->SetScrollInfo(SB_HORZ, si, TRUE);
	}

	void inline CResizer::OnVScroll(WPARAM wParam, LPARAM /*lParam*/)
	{
		int yNewPos;

		switch (LOWORD(wParam))
		{
			case SB_PAGEUP: // User clicked the scroll bar shaft above the scroll box.
				yNewPos = m_yScrollPos - 50;
				break;

			case SB_PAGEDOWN: // User clicked the scroll bar shaft below the scroll box.
				yNewPos = m_yScrollPos + 50;
				break;

			case SB_LINEUP: // User clicked the top arrow.
				yNewPos = m_yScrollPos - 5;
				break;

			case SB_LINEDOWN: // User clicked the bottom arrow.
				yNewPos = m_yScrollPos + 5;
				break;

			case SB_THUMBPOSITION: // User dragged the scroll box.
				yNewPos = HIWORD(wParam);
				break;

			case SB_THUMBTRACK: // User dragging the scroll box.
				yNewPos = HIWORD(wParam);
				break;

			default:
				yNewPos = m_yScrollPos;
		}

		// Scroll the window.
		yNewPos = MAX(0, yNewPos);
		yNewPos = MIN( yNewPos, GetMinRect().Height() - m_pParent->GetClientRect().Height() );
		int yDelta = yNewPos - m_yScrollPos;
		m_yScrollPos = yNewPos;
		m_pParent->ScrollWindow(0, -yDelta, NULL, NULL);

		// Reset the scroll bar.
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask  = SIF_POS;
		si.nPos   = m_yScrollPos;
		m_pParent->SetScrollInfo(SB_VERT, si, TRUE);
	}

    void inline CResizer::RecalcLayout()
    // Repositions the child windows. Call this function when handling
	// the WM_SIZE message in the parent window.
	{
    	assert (m_rcInit.Width() > 0 && m_rcInit.Height() > 0);
    	assert (NULL != m_pParent);

		CRect rcCurrent = m_pParent->GetClientRect();

		// Adjust the scrolling if required
		m_xScrollPos = MIN(m_xScrollPos, MAX(0, m_rcMin.Width()  - rcCurrent.Width() ) );
		m_yScrollPos = MIN(m_yScrollPos, MAX(0, m_rcMin.Height() - rcCurrent.Height()) );
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMax   =	m_rcMin.Width();
		si.nPage  = rcCurrent.Width();
		si.nPos   = m_xScrollPos;
		m_pParent->SetScrollInfo(SB_HORZ, si, TRUE);
		si.nMax   =	m_rcMin.Height();
		si.nPage  = rcCurrent.Height();
		si.nPos   = m_yScrollPos;
		m_pParent->SetScrollInfo(SB_VERT, si, TRUE);

    	rcCurrent.right  = MAX( rcCurrent.Width(),  m_rcMin.Width() );
    	rcCurrent.bottom = MAX( rcCurrent.Height(), m_rcMin.Height() );
    	if (!m_rcMax.IsRectEmpty())
    	{
    		rcCurrent.right  = MIN( rcCurrent.Width(),  m_rcMax.Width() );
    		rcCurrent.bottom = MIN( rcCurrent.Height(), m_rcMax.Height() );
    	}

		// Declare an iterator to step through the vector
		std::vector<ResizeData>::iterator iter;

    	for (iter = m_vResizeData.begin(); iter < m_vResizeData.end(); ++iter)
    	{
    		int left   = 0;
    		int top    = 0;
    		int width  = 0;
    		int height = 0;

    		// Calculate the new size and position of the child window
			switch( (*iter).corner )
    		{
    		case topleft:
				width  = (*iter).bFixedWidth?  (*iter).rcInit.Width()  : (*iter).rcInit.Width()  - m_rcInit.Width() + rcCurrent.Width();
    			height = (*iter).bFixedHeight? (*iter).rcInit.Height() : (*iter).rcInit.Height() - m_rcInit.Height() + rcCurrent.Height();
    			left   = (*iter).rcInit.left;
    			top    = (*iter).rcInit.top;
    			break;
    		case topright:
    			width  = (*iter).bFixedWidth?  (*iter).rcInit.Width()  : (*iter).rcInit.Width()  - m_rcInit.Width() + rcCurrent.Width();
    			height = (*iter).bFixedHeight? (*iter).rcInit.Height() : (*iter).rcInit.Height() - m_rcInit.Height() + rcCurrent.Height();
    			left   = (*iter).rcInit.right - width - m_rcInit.Width() + rcCurrent.Width();
    			top    = (*iter).rcInit.top;
    			break;
    		case bottomleft:
				width  = (*iter).bFixedWidth?  (*iter).rcInit.Width()  : (*iter).rcInit.Width()  - m_rcInit.Width() + rcCurrent.Width();
    			height = (*iter).bFixedHeight? (*iter).rcInit.Height() : (*iter).rcInit.Height() - m_rcInit.Height() + rcCurrent.Height();
    			left   = (*iter).rcInit.left;
    			top    = (*iter).rcInit.bottom - height - m_rcInit.Height() + rcCurrent.Height();
    			break;
    		case bottomright:
    			width  = (*iter).bFixedWidth?  (*iter).rcInit.Width()  : (*iter).rcInit.Width()  - m_rcInit.Width() + rcCurrent.Width();
    			height = (*iter).bFixedHeight? (*iter).rcInit.Height() : (*iter).rcInit.Height() - m_rcInit.Height() + rcCurrent.Height();
    			left   = (*iter).rcInit.right   - width - m_rcInit.Width() + rcCurrent.Width();
    			top    = (*iter).rcInit.bottom  - height - m_rcInit.Height() + rcCurrent.Height();
    			break;
    		}

			// Position the child window.
			CRect rc(left - m_xScrollPos, top - m_yScrollPos, left + width - m_xScrollPos, top + height - m_yScrollPos);
			if ( rc != (*iter).rcOld)
			{
				CWnd* pWnd = FromHandle((*iter).hWnd);
				CWnd *pWndPrev = pWnd->GetWindow(GW_HWNDPREV); // Trick to maintain the original tab order.
				HWND hWnd = pWndPrev ? pWndPrev->GetHwnd():NULL;
				pWnd->SetWindowPos(hWnd, rc, SWP_NOCOPYBITS);
				(*iter).rcOld = rc;
			}
    	}
    }

#endif // #ifndef _WIN32_WCE

} // namespace Win32xx



#endif // _WIN32XX_DIALOG_H_

