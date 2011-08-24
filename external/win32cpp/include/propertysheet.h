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
// propertysheet.h
//  Declaration of the following classes:
//  CPropertyPage and CPropertySheet

// These classes add support for property sheets to Win32++. A property sheet
// will have one or more property pages. These pages are much like dialogs
// which are presented within a tabbed dialog or within a wizard. The data
// on a property page can be validated before the next page is presented.
// Property sheets have three modes of use: Modal, Modeless, and Wizard.
//
// Refer to the PropertySheet demo program for an example of how propert sheets
// can be used.


#ifndef _WIN32XX_PROPERTYSHEET_H_
#define _WIN32XX_PROPERTYSHEET_H_

#include "dialog.h"

#define ID_APPLY_NOW   0x3021
#define ID_WIZBACK     0x3023
#define ID_WIZNEXT     0x3024
#define ID_WIZFINISH   0x3025
#define ID_HELP        0xE146

#ifndef PROPSHEETHEADER_V1_SIZE
 #define PROPSHEETHEADER_V1_SIZE 40
#endif

namespace Win32xx
{
    class CPropertyPage;
	typedef Shared_Ptr<CPropertyPage> PropertyPagePtr;

	class CPropertyPage : public CWnd
	{
	public:
		CPropertyPage (UINT nIDTemplate, LPCTSTR szTitle = NULL);
		virtual ~CPropertyPage() {}

		virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual INT_PTR DialogProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual int  OnApply();
		virtual void OnCancel();
		virtual void OnHelp();
		virtual BOOL OnInitDialog();
		virtual BOOL OnKillActive();
		virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
		virtual int  OnOK();
		virtual BOOL OnQueryCancel();
		virtual BOOL OnQuerySiblings(WPARAM wParam, LPARAM lParam);
		virtual int  OnSetActive();
		virtual int  OnWizardBack();
		virtual INT_PTR OnWizardFinish();
		virtual int  OnWizardNext();
		virtual	BOOL PreTranslateMessage(MSG* pMsg);

		static UINT CALLBACK StaticPropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
		static INT_PTR CALLBACK StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void CancelToClose() const;
		PROPSHEETPAGE GetPSP() const {return m_PSP;}
		BOOL IsButtonEnabled(int iButton) const;
		LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam) const;
		void SetModified(BOOL bChanged) const;
		void SetTitle(LPCTSTR szTitle);
		void SetWizardButtons(DWORD dwFlags) const;

	protected:
		PROPSHEETPAGE m_PSP;

	private:
		CPropertyPage(const CPropertyPage&);				// Disable copy construction
		CPropertyPage& operator = (const CPropertyPage&);	// Disable assignment operator

		tString m_Title;
	};

	class CPropertySheet : public CWnd
	{
	public:
		CPropertySheet(UINT nIDCaption, CWnd* pParent = NULL);
		CPropertySheet(LPCTSTR pszCaption = NULL, CWnd* pParent = NULL);
		virtual ~CPropertySheet() {}

		// Operations
		virtual CPropertyPage* AddPage(CPropertyPage* pPage);
		virtual HWND Create(CWnd* pParent = 0);
		virtual INT_PTR CreatePropertySheet(LPCPROPSHEETHEADER ppsph);
		virtual void DestroyButton(int iButton);
		virtual void Destroy();
		virtual int DoModal();
		virtual void RemovePage(CPropertyPage* pPage);

		// State functions
		BOOL IsModeless() const;
		BOOL IsWizard() const;

		//Attributes
		CPropertyPage* GetActivePage() const;
		int GetPageCount() const;
		int GetPageIndex(CPropertyPage* pPage) const;
		HWND GetTabControl() const;
		virtual BOOL SetActivePage(int nPage);
		virtual BOOL SetActivePage(CPropertyPage* pPage);
		virtual void SetIcon(UINT idIcon);
		virtual void SetTitle(LPCTSTR szTitle);
		virtual void SetWizardMode(BOOL bWizard);

	protected:
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CPropertySheet(const CPropertySheet&);				// Disable copy construction
		CPropertySheet& operator = (const CPropertySheet&); // Disable assignment operator
		void BuildPageArray();
		static void CALLBACK Callback(HWND hwnd, UINT uMsg, LPARAM lParam);

		tString m_Title;
		std::vector<PropertyPagePtr> m_vPages;	// vector of CPropertyPage
		std::vector<PROPSHEETPAGE> m_vPSP;		// vector of PROPSHEETPAGE
		BOOL m_bInitialUpdate;
		PROPSHEETHEADER m_PSH;
	};

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

namespace Win32xx
{

	//////////////////////////////////////////
	// Definitions for the CPropertyPage class
	//
	inline CPropertyPage::CPropertyPage(UINT nIDTemplate, LPCTSTR szTitle /* = NULL*/)
	{
		ZeroMemory(&m_PSP, sizeof(PROPSHEETPAGE));
		SetTitle(szTitle);

		m_PSP.dwSize        = sizeof(PROPSHEETPAGE);
		m_PSP.dwFlags       |= PSP_USECALLBACK;
		m_PSP.hInstance     = GetApp()->GetResourceHandle();
		m_PSP.pszTemplate   = MAKEINTRESOURCE(nIDTemplate);
		m_PSP.pszTitle      = m_Title.c_str();
		m_PSP.pfnDlgProc    = (DLGPROC)CPropertyPage::StaticDialogProc;
		m_PSP.lParam        = (LPARAM)this;
		m_PSP.pfnCallback   = CPropertyPage::StaticPropSheetPageProc;
	}

	inline void CPropertyPage::CancelToClose() const
	// Disables the Cancel button and changes the text of the OK button to "Close."
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(PSM_CANCELTOCLOSE, 0L, 0L);
	}


	inline INT_PTR CPropertyPage::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// Override this function in your class derrived from CPropertyPage if you wish to handle messages
		// A typical function might look like this:

		//	switch (uMsg)
		//	{
		//	case MESSAGE1:		// Some Win32 API message
		//		OnMessage1();	// A user defined function
		//		break;			// Also do default processing
		//	case MESSAGE2:
		//		OnMessage2();
		//		return x;		// Don't do default processing, but instead return
		//						//  a value recommended by the Win32 API documentation
		//	}

		// Always pass unhandled messages on to DialogProcDefault
		return DialogProcDefault(uMsg, wParam, lParam);
	}

	inline INT_PTR CPropertyPage::DialogProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	// All DialogProc functions should pass unhandled messages to this function
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

	    case WM_INITDIALOG:
		    return OnInitDialog();

		case PSM_QUERYSIBLINGS:
			return (BOOL)OnQuerySiblings(wParam, lParam);

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

	} // INT_PTR CALLBACK CPropertyPage::DialogProc(...)

	inline BOOL CPropertyPage::IsButtonEnabled(int iButton) const
	{
		assert(::IsWindow(m_hWnd));
		return GetParent()->GetDlgItem(iButton)->IsWindowEnabled();
	}

	inline int CPropertyPage::OnApply()
	{
		// This function is called for each page when the Apply button is pressed
		// Override this function in your derived class if required.

		// The possible return values are:
		// PSNRET_NOERROR. The changes made to this page are valid and have been applied
		// PSNRET_INVALID. The property sheet will not be destroyed, and focus will be returned to this page.
		// PSNRET_INVALID_NOCHANGEPAGE. The property sheet will not be destroyed, and focus will be returned;

		return PSNRET_NOERROR;
	}

	inline void CPropertyPage::OnCancel()
	{
		// This function is called for each page when the Cancel button is pressed
		// Override this function in your derived class if required.
	}

	inline void CPropertyPage::OnHelp()
	{
		// This function is called in response to the PSN_HELP notification.
		SendMessage(m_hWnd, WM_COMMAND, ID_HELP, 0L);
	}

	inline BOOL CPropertyPage::OnQueryCancel()
	{
		// Called when the cancel button is pressed, and before the cancel has taken place
		// Returns TRUE to prevent the cancel operation, or FALSE to allow it.

		return FALSE;    // Allow cancel to proceed
	}

	inline BOOL CPropertyPage::OnQuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);
		UNREFERENCED_PARAMETER(lParam);

		// Responds to a query request from the Property Sheet.
		// The values for wParam and lParam are the ones set by
		// the CPropertySheet::QuerySiblings call

		// return FALSE to allow other siblings to be queried, or
		// return TRUE to stop query at this page.

		return FALSE;
	}

	inline BOOL CPropertyPage::OnInitDialog()
	{
		// Called when the property page is created
		// Override this function in your derived class if required.

		return TRUE; // Pass Keyboard control to handle in WPARAM
	}

	inline BOOL CPropertyPage::OnKillActive()
	{
		// This is called in response to a PSN_KILLACTIVE notification, which
		// is sent whenever the OK or Apply button is pressed.
		// It provides an opportunity to validate the page contents before it's closed.
		// Return TRUE to prevent the page from losing the activation, or FALSE to allow it.

		return FALSE;
	}

	inline int CPropertyPage::OnOK()
	{
		// Called for each page when the OK button is pressed
		// Override this function in your derived class if required.

		// The possible return values are:
		// PSNRET_NOERROR. The changes made to this page are valid and have been applied
		// PSNRET_INVALID. The property sheet will not be destroyed, and focus will be returned to this page.
		// PSNRET_INVALID_NOCHANGEPAGE. The property sheet will not be destroyed, and focus will be returned;

		return PSNRET_NOERROR;
	}

	inline LRESULT CPropertyPage::OnNotify(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		LPPSHNOTIFY pNotify = (LPPSHNOTIFY)lParam;
		switch(pNotify->hdr.code)
		{
		case PSN_SETACTIVE:
			return OnSetActive();
		case PSN_KILLACTIVE:
			return OnKillActive();
		case PSN_APPLY:
			if (pNotify->lParam)
				return OnOK();
			else
				return OnApply();
		case PSN_RESET:
			OnCancel();
			return FALSE;
		case PSN_QUERYCANCEL:
			return OnQueryCancel();
		case PSN_WIZNEXT:
			return OnWizardNext();
		case PSN_WIZBACK:
			return OnWizardBack();
		case PSN_WIZFINISH:
			return OnWizardFinish();
		case PSN_HELP:
			OnHelp();
			return TRUE;
		}
		return FALSE;
	}

	inline int CPropertyPage::OnSetActive()
	{
		// Called when a page becomes active
		// Override this function in your derived class if required.

		// Returns zero to accept the activation, or -1 to activate the next or the previous page (depending
		// on whether the user clicked the Next or Back button). To set the activation to a particular page,
		// return the resource identifier of the page.

		return 0;
	}

	inline int CPropertyPage::OnWizardBack()
	{
		// This function is called when the Back button is pressed on a wizard page
		// Override this function in your derived class if required.

		// Returns 0 to allow the wizard to go to the previous page. Returns -1 to prevent the wizard
		// from changing pages. To display a particular page, return its dialog resource identifier.

		return 0;
	}

	inline INT_PTR CPropertyPage::OnWizardFinish()
	{
		// This function is called when the Finish button is pressed on a wizard page
		// Override this function in your derived class if required.

		// Return Value:
		// Return non-zero to prevent the wizard from finishing.
		// Version 5.80. and later. Return a window handle to prevent the wizard from finishing. The wizard will set the focus to that window. The window must be owned by the wizard page.
		// Return 0 to allow the wizard to finish.

		return 0; // Allow wizard to finish
	}

	inline int CPropertyPage::OnWizardNext()
	{
		// This function is called when the Next button is pressed on a wizard page
		// Override this function in your derived class if required.

		// Return 0 to allow the wizard to go to the next page. Return -1 to prevent the wizard from
		// changing pages. To display a particular page, return its dialog resource identifier.

		return 0;
	}

	inline BOOL CPropertyPage::PreTranslateMessage(MSG* pMsg)
	{
		// allow the tab control to translate keyboard input
		if (pMsg->message == WM_KEYDOWN && GetAsyncKeyState(VK_CONTROL) < 0 &&
			(pMsg->wParam == VK_TAB || pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT))
		{
			CWnd* pWndParent = GetParent();
			if (pWndParent->SendMessage(PSM_ISDIALOGMESSAGE, 0L, (LPARAM)pMsg))
				return TRUE;
		}

		// allow the dialog to translate keyboard input
		if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
		{
			if (IsDialogMessage(pMsg))
				return TRUE;
		}

		return CWnd::PreTranslateMessage(pMsg);
	}

	inline LRESULT CPropertyPage::QuerySiblings(WPARAM wParam, LPARAM lParam) const
	{
		// Sent to a property sheet, which then forwards the message to each of its pages.
		// Set wParam and lParam to values you want passed to the property pages.
		// Returns the nonzero value from a page in the property sheet, or zero if no page returns a nonzero value.

		assert(::IsWindow(m_hWnd));
		return GetParent()->SendMessage(PSM_QUERYSIBLINGS, wParam, lParam);
	}

	inline void CPropertyPage::SetModified(BOOL bChanged) const
	{
		// The property sheet will enable the Apply button if bChanged is TRUE.

		assert(::IsWindow(m_hWnd));

		if (bChanged)
			GetParent()->SendMessage(PSM_CHANGED, (WPARAM)m_hWnd, 0L);
		else
			GetParent()->SendMessage(PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
	}

	inline void CPropertyPage::SetTitle(LPCTSTR szTitle)
	{
		if (szTitle)
		{
			m_Title = szTitle;
			m_PSP.dwFlags |= PSP_USETITLE;
		}
		else
		{
			m_Title.erase();
			m_PSP.dwFlags &= ~PSP_USETITLE;
		}

		m_PSP.pszTitle = m_Title.c_str();
	}

	inline void CPropertyPage::SetWizardButtons(DWORD dwFlags) const
	{
		// dwFlags:  A value that specifies which wizard buttons are enabled. You can combine one or more of the following flags.
		//	PSWIZB_BACK				Enable the Back button. If this flag is not set, the Back button is displayed as disabled.
		//	PSWIZB_DISABLEDFINISH	Display a disabled Finish button.
		//	PSWIZB_FINISH			Display an enabled Finish button.
		//	PSWIZB_NEXT				Enable the Next button. If this flag is not set, the Next button is displayed as disabled.

		assert (::IsWindow(m_hWnd));
		PropSheet_SetWizButtons(::GetParent(m_hWnd), dwFlags);
	}

	inline UINT CALLBACK CPropertyPage::StaticPropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		assert( GetApp() );
		UNREFERENCED_PARAMETER(hwnd);

		// Note: the hwnd is always NULL

		switch (uMsg)
		{
		case PSPCB_CREATE:
			{
				TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
				assert(pTLSData);

				// Store the CPropertyPage pointer in Thread Local Storage
				pTLSData->pCWnd = (CWnd*)ppsp->lParam;
			}
			break;
		}

		return TRUE;
	}

	inline INT_PTR CALLBACK CPropertyPage::StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		assert( GetApp() );

		// Find matching CWnd pointer for this HWND
		CPropertyPage* pPage = (CPropertyPage*)GetApp()->GetCWndFromMap(hwndDlg);
		if (0 == pPage)
		{
			// matching CWnd pointer not found, so add it to HWNDMap now
			TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
			pPage = (CPropertyPage*)pTLSData->pCWnd;

			// Set the hWnd members and call DialogProc for this message
			pPage->m_hWnd = hwndDlg;
			pPage->AddToMap();
		}

		return pPage->DialogProc(uMsg, wParam, lParam);
	}


	///////////////////////////////////////////
	// Definitions for the CPropertySheet class
	//
	inline CPropertySheet::CPropertySheet(UINT nIDCaption, CWnd* pParent /* = NULL*/)
	{
		ZeroMemory(&m_PSH, sizeof (PROPSHEETHEADER));
		SetTitle(LoadString(nIDCaption));
		m_bInitialUpdate = FALSE;

#ifdef _WIN32_WCE
		m_PSH.dwSize = sizeof(PROPSHEETHEADER);
#else
		if (GetComCtlVersion() >= 471)
			m_PSH.dwSize = sizeof(PROPSHEETHEADER);
		else
			m_PSH.dwSize = PROPSHEETHEADER_V1_SIZE;
#endif

		m_PSH.dwFlags          = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
		m_PSH.hwndParent       = pParent? pParent->GetHwnd() : 0;
		m_PSH.hInstance        = GetApp()->GetInstanceHandle();
		m_PSH.pfnCallback      = (PFNPROPSHEETCALLBACK)CPropertySheet::Callback;
	}

	inline CPropertySheet::CPropertySheet(LPCTSTR pszCaption /*= NULL*/, CWnd* pParent /* = NULL*/)
	{
		ZeroMemory(&m_PSH, sizeof (PROPSHEETHEADER));
		SetTitle(pszCaption);
		m_bInitialUpdate = FALSE;

#ifdef _WIN32_WCE
		m_PSH.dwSize = PROPSHEETHEADER_V1_SIZE;
#else
		if (GetComCtlVersion() >= 471)
			m_PSH.dwSize = sizeof(PROPSHEETHEADER);
		else
			m_PSH.dwSize = PROPSHEETHEADER_V1_SIZE;
#endif

		m_PSH.dwFlags          = PSH_PROPSHEETPAGE | PSH_USECALLBACK;
		m_PSH.hwndParent       = pParent? pParent->GetHwnd() : 0;;
		m_PSH.hInstance        = GetApp()->GetInstanceHandle();
		m_PSH.pfnCallback      = (PFNPROPSHEETCALLBACK)CPropertySheet::Callback;
	}

	inline CPropertyPage* CPropertySheet::AddPage(CPropertyPage* pPage)
	// Adds a Property Page to the Property Sheet
	{
		assert(NULL != pPage);

		m_vPages.push_back(PropertyPagePtr(pPage));

		if (m_hWnd)
		{
			// property sheet already exists, so add page to it
			PROPSHEETPAGE psp = pPage->GetPSP();
			HPROPSHEETPAGE hpsp = ::CreatePropertySheetPage(&psp);
			PropSheet_AddPage(m_hWnd, hpsp);
		}

		m_PSH.nPages = (int)m_vPages.size();

		return pPage;
	}

	inline void CPropertySheet::BuildPageArray()
	// Builds the PROPSHEETPAGE array
	{
		m_vPSP.clear();
		std::vector<PropertyPagePtr>::iterator iter;
		for (iter = m_vPages.begin(); iter < m_vPages.end(); ++iter)
			m_vPSP.push_back((*iter)->GetPSP());

		PROPSHEETPAGE* pPSPArray = &m_vPSP.front();	// Array of PROPSHEETPAGE
		m_PSH.ppsp = pPSPArray;
	}

	inline void CALLBACK CPropertySheet::Callback(HWND hwnd, UINT uMsg, LPARAM lParam)
	{
		assert( GetApp() );

		switch(uMsg)
		{
		//called before the dialog is created, hwnd = NULL, lParam points to dialog resource
		case PSCB_PRECREATE:
			{
				LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;

				if(!(lpTemplate->style & WS_SYSMENU))
				{
					lpTemplate->style |= WS_SYSMENU;
				}
			}
			break;

		//called after the dialog is created
		case PSCB_INITIALIZED:
			{
				// Retrieve pointer to CWnd object from Thread Local Storage
				TLSData* pTLSData = (TLSData*)TlsGetValue(GetApp()->GetTlsIndex());
				assert(pTLSData);

				CPropertySheet* w = (CPropertySheet*)pTLSData->pCWnd;
				assert(w);

				w->Attach(hwnd);
				w->OnCreate();
			}
			break;
		}
	}


	inline HWND CPropertySheet::Create(CWnd* pParent /*= 0*/)
	// Creates a modeless Property Sheet
	{
		assert( GetApp() );

		if (pParent)
		{
			m_PSH.hwndParent = pParent->GetHwnd();
		}

		BuildPageArray();
		PROPSHEETPAGE* pPSPArray = &m_vPSP.front();
		m_PSH.ppsp = pPSPArray;

		// Create a modeless Property Sheet
		m_PSH.dwFlags &= ~PSH_WIZARD;
		m_PSH.dwFlags |= PSH_MODELESS;
		HWND hWnd = (HWND)CreatePropertySheet(&m_PSH);

		return hWnd;
	}

	inline INT_PTR CPropertySheet::CreatePropertySheet(LPCPROPSHEETHEADER ppsph)
	{
		assert( GetApp() );

		INT_PTR ipResult = 0;

		// Only one window per CWnd instance allowed
		assert(!::IsWindow(m_hWnd));

		// Ensure this thread has the TLS index set
		TLSData* pTLSData = GetApp()->SetTlsIndex();

		// Store the 'this' pointer in Thread Local Storage
		pTLSData->pCWnd = this;

		// Create the property sheet
		ipResult = PropertySheet(ppsph);

		return ipResult;
	}

	inline void CPropertySheet::DestroyButton(int IDButton)
	{
		assert(::IsWindow(m_hWnd));

		HWND hwndButton = ::GetDlgItem(m_hWnd, IDButton);
		if (hwndButton != NULL)
		{
			// Hide and disable the button
			::ShowWindow(hwndButton, SW_HIDE);
			::EnableWindow(hwndButton, FALSE);
		}
	}

	inline void CPropertySheet::Destroy()
	{
		CWnd::Destroy();
		m_vPages.clear();
	}

	inline int CPropertySheet::DoModal()
	{
		assert( GetApp() );

		BuildPageArray();
		PROPSHEETPAGE* pPSPArray = &m_vPSP.front();
		m_PSH.ppsp = pPSPArray;

		// Create the Property Sheet
		int nResult = (int)CreatePropertySheet(&m_PSH);

		m_vPages.clear();

		return nResult;
	}

	inline CPropertyPage* CPropertySheet::GetActivePage() const
	{
		assert(::IsWindow(m_hWnd));

		CPropertyPage* pPage = NULL;
		if (m_hWnd != NULL)
		{
			HWND hPage = (HWND)SendMessage(PSM_GETCURRENTPAGEHWND, 0L, 0L);
			pPage = (CPropertyPage*)FromHandle(hPage);
		}

		return pPage;
	}

	inline int CPropertySheet::GetPageCount() const
	// Returns the number of Property Pages in this Property Sheet
	{
		assert(::IsWindow(m_hWnd));
		return (int)m_vPages.size();
	}

	inline int CPropertySheet::GetPageIndex(CPropertyPage* pPage) const
	{
		assert(::IsWindow(m_hWnd));

		for (int i = 0; i < GetPageCount(); i++)
		{
			if (m_vPages[i].get() == pPage)
				return i;
		}
		return -1;
	}

	inline HWND CPropertySheet::GetTabControl() const
	// Returns the handle to the Property Sheet's tab control
	{
		assert(::IsWindow(m_hWnd));
		return (HWND)SendMessage(PSM_GETTABCONTROL, 0L, 0L);
	}

	inline BOOL CPropertySheet::IsModeless() const
	{
		return (m_PSH.dwFlags & PSH_MODELESS);
	}

	inline BOOL CPropertySheet::IsWizard() const
	{
		return (m_PSH.dwFlags & PSH_WIZARD);
	}

	inline void CPropertySheet::RemovePage(CPropertyPage* pPage)
	// Removes a Property Page from the Property Sheet
	{
		assert(::IsWindow(m_hWnd));

		int nPage = GetPageIndex(pPage);
		if (m_hWnd != NULL)
			SendMessage(m_hWnd, PSM_REMOVEPAGE, nPage, 0L);

		m_vPages.erase(m_vPages.begin() + nPage, m_vPages.begin() + nPage+1);
		m_PSH.nPages = (int)m_vPages.size();
	}

	inline BOOL CPropertySheet::PreTranslateMessage(MSG* pMsg)
	{
		// allow sheet to translate Ctrl+Tab, Shift+Ctrl+Tab, Ctrl+PageUp, and Ctrl+PageDown
		if (pMsg->message == WM_KEYDOWN && GetAsyncKeyState(VK_CONTROL) < 0 &&
			(pMsg->wParam == VK_TAB || pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT))
		{
			if (SendMessage(PSM_ISDIALOGMESSAGE, 0L, (LPARAM)pMsg))
				return TRUE;
		}

		// allow the dialog to translate keyboard input
		if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
		{
			return GetActivePage()->PreTranslateMessage(pMsg);
		}

		return CWnd::PreTranslateMessage(pMsg);
	}

	inline BOOL CPropertySheet::SetActivePage(int nPage)
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(m_hWnd, PSM_SETCURSEL, nPage, 0L);
	}

	inline BOOL CPropertySheet::SetActivePage(CPropertyPage* pPage)
	{
		assert(::IsWindow(m_hWnd));
		int nPage = GetPageIndex(pPage);
		if ((nPage >= 0))
			return SetActivePage(nPage);

		return FALSE;
	}

	inline void CPropertySheet::SetIcon(UINT idIcon)
	{
		m_PSH.pszIcon = MAKEINTRESOURCE(idIcon);
		m_PSH.dwFlags |= PSH_USEICONID;
	}

	inline void CPropertySheet::SetTitle(LPCTSTR szTitle)
	{
		if (szTitle)
			m_Title = szTitle;
		else
			m_Title.erase();

		m_PSH.pszCaption = m_Title.c_str();
	}

	inline void CPropertySheet::SetWizardMode(BOOL bWizard)
	{
		if (bWizard)
			m_PSH.dwFlags |= PSH_WIZARD;
		else
			m_PSH.dwFlags &= ~PSH_WIZARD;
	}

	inline LRESULT CPropertySheet::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{

		case WM_WINDOWPOSCHANGED:
			{
				LPWINDOWPOS lpWinPos = (LPWINDOWPOS)lParam;
				if (lpWinPos->flags & SWP_SHOWWINDOW)
				{
					if (!m_bInitialUpdate)
						// The first window positioning with the window visible
						OnInitialUpdate();
					m_bInitialUpdate = TRUE;
				}
			}
			break;

		case WM_DESTROY:
			m_bInitialUpdate = FALSE;
			break;

		case WM_SYSCOMMAND:
			if ((SC_CLOSE == wParam) && (m_PSH.dwFlags &  PSH_MODELESS))
			{
				Destroy();
				return 0L;
			}
			break;
		}

		// pass unhandled messages on for default processing
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}

}

#endif // _WIN32XX_PROPERTYSHEET_H_
