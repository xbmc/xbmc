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


#ifndef _WIN32XX_TOOLBAR_H_
#define _WIN32XX_TOOLBAR_H_

#include "wincore.h"
#include "gdi.h"
#include "rebar.h"


namespace Win32xx
{

	struct ToolBarTheme
	{
		BOOL UseThemes;			// TRUE if themes are used
		COLORREF clrHot1;		// Colour 1 for hot button
		COLORREF clrHot2;		// Colour 2 for hot button
		COLORREF clrPressed1;	// Colour 1 for pressed button
		COLORREF clrPressed2;	// Colour 2 for pressed button
		COLORREF clrOutline;	// Colour for border outline
	};


	////////////////////////////////////
	// Declaration of the CToolBar class
	//
	class CToolBar : public CWnd
	{
	public:
		CToolBar();
		virtual ~CToolBar();

		// Operations
		virtual int  AddBitmap(UINT ToolBarID);
		virtual BOOL AddButton(UINT nID, BOOL bEnabled = TRUE);
		virtual void Destroy();
		virtual BOOL ReplaceBitmap(UINT NewToolBarID);
		virtual BOOL SetBitmap(UINT nID);
		virtual int  SetButtons(const std::vector<UINT>& vToolBarData) const;
		virtual BOOL SetButtonText(int idButton, LPCTSTR szText);
		virtual BOOL SetImages(COLORREF crMask, UINT ToolBarID, UINT ToolBarHotID, UINT ToolBarDisabledID);

		// Wrappers for Win32 API functions
		BOOL  AddButtons(UINT uNumButtons, LPTBBUTTON lpButtons) const;
		int   AddString(UINT nStringID) const;
		int   AddStrings(LPCTSTR lpszStrings) const;
		void  Autosize() const;
		void  CheckButton(int idButton, BOOL fCheck) const;
		int   CommandToIndex(int idButton) const;
		BOOL  DeleteButton(int iButton) const;
		BOOL  DisableButton(int idButton) const;
		BOOL  EnableButton(int idButton) const;
		BOOL  GetButton(int iButton, LPTBBUTTON lpButton) const;
		int   GetButtonCount() const;
		DWORD GetButtonSize() const;
		UINT  GetButtonState(int idButton) const;
		BYTE  GetButtonStyle(int idButton) const;
		CString GetButtonText(int idButton) const;
		int   GetCommandID(int iIndex) const;
		HIMAGELIST GetDisabledImageList() const;
		int   GetHotItem() const;
		HIMAGELIST GetHotImageList() const;
		HIMAGELIST GetImageList() const;
		CRect GetItemRect(int iIndex) const;
		CSize GetMaxSize() const;
		DWORD GetPadding() const;
		CRect GetRect(int idButton) const;
		int   GetRows() const;
		int   GetTextRows() const;
		HWND  GetToolTips() const;
		BOOL  HasText() const;
		BOOL  HideButton(int idButton, BOOL fShow) const;
		int   HitTest() const;
		BOOL  Indeterminate(int idButton, BOOL fIndeterminate) const;
		BOOL  InsertButton(int iButton, LPTBBUTTON lpButton) const;
		BOOL  IsButtonHidden(int idButton) const;
		BOOL  IsButtonHighlighted(int idButton) const;
		BOOL  IsButtonIndeterminate(int idButton) const;
		BOOL  IsButtonPressed(int idButton) const;
		int   MapAccelerator(TCHAR chAccel) const;
		BOOL  MarkButton(int idButton) const;
		BOOL  MoveButton(UINT uOldPos, UINT uNewPos) const;
		BOOL  PressButton(int idButton, BOOL fPress) const;
		void  SaveRestore(BOOL fSave, TBSAVEPARAMS* ptbsp) const;
		BOOL  SetBitmapSize(int cx, int cy) const;
		BOOL  SetButtonSize(int cx, int cy) const;
		BOOL  SetButtonState(int idButton, UINT State) const;
		BOOL  SetButtonStyle(int idButton, BYTE Style) const;
		BOOL  SetButtonWidth(int idButton, int nWidth) const;
		BOOL  SetCommandID(int iIndex, int idButton) const;
		HIMAGELIST SetDisableImageList(HIMAGELIST himlNewDisabled) const;
		DWORD SetDrawTextFlags(DWORD dwMask, DWORD dwDTFlags) const;
		DWORD SetExtendedStyle(DWORD dwExStyle) const;
		HIMAGELIST SetHotImageList(HIMAGELIST himlNewHot) const;
		int   SetHotItem(int iHot) const;
		HIMAGELIST SetImageList(HIMAGELIST himlNew) const;
		BOOL  SetIndent(int iIndent) const;
		BOOL  SetMaxTextRows(int iMaxRows) const;
		BOOL  SetPadding(int cx, int cy) const;
		void  SetToolTips(HWND hwndToolTip) const;

		// Attributes
		std::vector<UINT>& GetToolBarData() const {return (std::vector <UINT> &)m_vToolBarData;}
		ToolBarTheme& GetToolBarTheme() {return m_Theme;}
		void SetToolBarTheme(ToolBarTheme& Theme);

	protected:
	// Overridables
		virtual void OnCreate();
		virtual void OnDestroy();
		virtual void OnWindowPosChanging(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnCustomDraw(NMHDR* pNMHDR);
		virtual LRESULT OnNotifyReflect(WPARAM wParam, LPARAM lParam);
		virtual void PreCreate(CREATESTRUCT &cs);
		virtual void PreRegisterClass(WNDCLASS &wc);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CToolBar(const CToolBar&);				// Disable copy construction
		CToolBar& operator = (const CToolBar&); // Disable assignment operator

		std::vector<UINT> m_vToolBarData;	// vector of resource IDs for toolbar buttons
		std::map<tString, int> m_StringMap;	// a map of strings used in SetButtonText
		UINT m_OldToolBarID;				// Bitmap Resource ID, used in AddBitmap/ReplaceBitmap
		ToolBarTheme m_Theme;				// The theme structure
		BOOL m_bDrawArrowBkgrnd;			// True if a seperate arrow background is to be drawn

	};  // class CToolBar

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	////////////////////////////////////
	// Definitions for the CToolBar class
	//
	inline CToolBar::CToolBar() : m_OldToolBarID(0), m_bDrawArrowBkgrnd(FALSE)
	{
		ZeroMemory(&m_Theme, sizeof(ToolBarTheme));
	}

	inline CToolBar::~CToolBar()
	{
	}

	inline int CToolBar::AddBitmap(UINT ToolBarID)
	// Adds one or more images to the list of button images available for a toolbar.

	// Note: AddBitmap supports a maximum colour depth of 8 bits (256 colours)
	//       For more colours, use an ImageList instead
	{
		assert(::IsWindow(m_hWnd));

		int iNumButtons = 0;
		std::vector<UINT>::iterator iter;
		for (iter = GetToolBarData().begin(); iter < GetToolBarData().end(); ++iter)
			if ((*iter) != 0) ++iNumButtons;

		TBADDBITMAP tbab = {0};
		tbab.hInst = GetApp()->GetResourceHandle();
		tbab.nID   = ToolBarID;
		int iResult = (int)SendMessage(TB_ADDBITMAP, iNumButtons, (LPARAM)&tbab);

		if (-1 != iResult)
			m_OldToolBarID = ToolBarID;

		return iResult;
	}

	inline BOOL CToolBar::AddButton(UINT nID, BOOL bEnabled /* = TRUE */)
	// Adds Resource IDs to toolbar buttons.
	// A resource ID of 0 is a separator
	{
		assert(::IsWindow(m_hWnd));

		m_vToolBarData.push_back(nID);

		// TBBUTTON structure for each button in the toolbar
		TBBUTTON tbb = {0};

		std::vector<UINT>::iterator iter;
		int iImages = 0;
		for(iter = m_vToolBarData.begin(); iter < m_vToolBarData.end(); ++iter)
			if (0 != *iter) iImages++;

		ZeroMemory(&tbb, sizeof(TBBUTTON));

		if (0 == nID)
		{
			tbb.fsStyle = TBSTYLE_SEP;
		}
		else
		{
			tbb.dwData  = iImages -1;
			tbb.iBitmap = iImages -1;
			tbb.idCommand = nID;
			tbb.fsState = bEnabled? TBSTATE_ENABLED : 0;
			tbb.fsStyle = TBSTYLE_BUTTON;
		}

		// Add the button to the toolbar
		return (BOOL)SendMessage(TB_ADDBUTTONS, 1L, (LPARAM)&tbb);
	}

	inline BOOL CToolBar::AddButtons(UINT uNumButtons, LPTBBUTTON lpButtons) const
	// Adds one or more buttons to a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ADDBUTTONS, (LPARAM)uNumButtons, (WPARAM)lpButtons);
	}

	inline int CToolBar::AddString(UINT nStringID) const
	// Adds a new string, passed as a resource ID, to the toolbar's internal list of strings.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_ADDSTRING, (LPARAM)GetApp()->GetResourceHandle(), (WPARAM)nStringID);
	}

	inline int CToolBar::AddStrings(LPCTSTR lpszStrings) const
	// Adds a new string or strings to the list of strings available for a toolbar control.
	// Strings in the buffer must be separated by a null character. You must ensure that the last string has two null terminators.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_ADDSTRING, 0L, (WPARAM)lpszStrings);
	}

	inline void CToolBar::Autosize() const
	// Causes a toolbar to be resized.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(TB_AUTOSIZE, 0L, 0L);
	}

	inline void CToolBar::CheckButton(int idButton, BOOL fCheck) const
	// Checks or unchecks a given button in a toolbar.
	// When a button is checked, it is displayed in the pressed state.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0));
	}

	inline int CToolBar::CommandToIndex(int idButton) const
	// Retrieves the zero-based index for the button associated with the specified command identifier
	{
		assert(::IsWindow(m_hWnd));

		// returns -1 on fail
		return (int)SendMessage(TB_COMMANDTOINDEX, (WPARAM)idButton, 0L);
	}

	inline BOOL CToolBar::DeleteButton(int iButton) const
	// Deletes a button from the toolbar.
	// iButton is the Zero-based index of the button to delete.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_DELETEBUTTON, (WPARAM)iButton, 0L);
	}

	inline void CToolBar::Destroy()
	// Allows CToolBar to be reused after the window is destroyed
	{
		CWnd::Destroy();
		m_StringMap.clear();
	}

	inline BOOL CToolBar::DisableButton(int idButton) const
	// Disables the specified button in a toolbar
	// An example of idButton would be IDM_FILE_OPEN
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ENABLEBUTTON, (WPARAM)idButton, (LPARAM) MAKELONG(FALSE, 0));
	}

	inline BOOL CToolBar::EnableButton(int idButton) const
	// Enables the specified button in a toolbar
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ENABLEBUTTON, (WPARAM)idButton, (LPARAM) MAKELONG(TRUE,0 ));
	}

	inline BOOL CToolBar::GetButton(int iButton, LPTBBUTTON lpButton) const
	// Recieves the TBBUTTON structure information from the specified button
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_GETBUTTON, (LPARAM)iButton, (WPARAM)lpButton);
	}

	inline int CToolBar::GetButtonCount() const
	// Retrieves a count of the buttons currently in the toolbar
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_BUTTONCOUNT, 0L, 0L);
	}

	inline DWORD CToolBar::GetButtonSize() const
	// Retrieves the current width and height of toolbar buttons, in pixels.
	// Returns a DWORD value that contains the width and height values in the low word and high word, respectively.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(TB_GETBUTTONSIZE, 0L, 0L);
	}

	inline UINT CToolBar::GetButtonState(int idButton) const
	// Get the state of an individual button
	//	TBSTATE_CHECKED		The button has the TBSTYLE_CHECK style and is being clicked.
	//	TBSTATE_ELLIPSES	The button's text is cut off and an ellipsis is displayed.
	//	TBSTATE_ENABLED		The button accepts user input. A button that doesn't have this state is grayed.
	//	TBSTATE_HIDDEN		The button is not visible and cannot receive user input.
	//	TBSTATE_INDETERMINATE	The button is grayed.
	//	TBSTATE_MARKED		The button is marked. The interpretation of a marked item is dependent upon the application.
	//	TBSTATE_PRESSED		The button is being clicked.
	//	TBSTATE_WRAP		The button is followed by a line break.
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)SendMessage(TB_GETSTATE, (WPARAM) idButton, 0L);
	}

	inline BYTE CToolBar::GetButtonStyle(int idButton) const
	//	Get the the style of the toolbar control. The following button styles are supported:
	//	TBSTYLE_BUTTON		Standard pushbutton (default)
	//	TBSTYLE_SEP			Separator
	//	TBSTYLE_CHECK		Auto check-box button
	//	TBSTYLE_GROUP		Marks the start of a group of buttons
	//	TBSTYLE_CHECKGROUP	Marks the start of a group of check-box buttons
	//	TBSTYLE_DROPDOWN	Creates a drop-down list button
	//	TBSTYLE_AUTOSIZE	The button's width will be calculated based on the text of the button, not on the size of the image
	//	TBSTYLE_NOPREFIX	The button text will not have an accelerator prefix associated with it
	{
		assert(::IsWindow(m_hWnd));

		int iIndex = CommandToIndex(idButton);
		TBBUTTON tbb = {0};
		SendMessage(TB_GETBUTTON, iIndex, (LPARAM) &tbb);

		return tbb.fsStyle;
	}

	inline CString CToolBar::GetButtonText(int idButton) const
	// Retrieves the display text of a button on a toolbar.
	{
		assert(::IsWindow(m_hWnd));

		int Length = (int)SendMessage(TB_GETBUTTONTEXT, idButton, 0);
		CString str;
		LPTSTR szStr = str.GetBuffer(Length +1);
		SendMessage(TB_GETBUTTONTEXT, (LPARAM)idButton, (WPARAM)szStr);
		str.ReleaseBuffer();
		return str;
	}

	inline int CToolBar::GetCommandID(int iIndex) const
	// Retrieves information about the specified button in a toolbar
	{
		assert(::IsWindow(m_hWnd));
		TBBUTTON tbb = {0};
		SendMessage(TB_GETBUTTON, iIndex, (WPARAM) &tbb);

		// returns zero if failed
		return tbb.idCommand;
	}

	inline HIMAGELIST CToolBar::GetDisabledImageList() const
	// Retrieves the image list that a toolbar control uses to display inactive buttons.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_GETDISABLEDIMAGELIST, 0L, 0L);
	}
	
	inline HIMAGELIST CToolBar::GetHotImageList() const
	// Retrieves the image list that a toolbar control uses to display hot buttons.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_GETHOTIMAGELIST, 0L, 0L);	
	}

	inline int CToolBar::GetHotItem() const
	// Retrieves the index of the hot item in a toolbar, or -1 if no hot item is set.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_GETHOTITEM, 0L, 0L);
	}

	inline HIMAGELIST CToolBar::GetImageList() const
	// Retrieves the image list that a toolbar control uses to display buttons in their default state.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_GETIMAGELIST, 0L, 0L);
	}

	inline CRect CToolBar::GetItemRect(int iIndex) const
	// Retrieves the bounding rectangle of a button in a toolbar
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		int iCount = (int)SendMessage(TB_BUTTONCOUNT, 0L, 0L);

		if (iCount >= iIndex)
			SendMessage(TB_GETITEMRECT, (WPARAM)iIndex, (LPARAM)&rc);

		return rc;
	}

	inline CSize CToolBar::GetMaxSize() const
	// Retrieves the total size of all of the visible buttons and separators in the toolbar
	{
		assert(::IsWindow(m_hWnd));
		CSize sz;
		SendMessage(TB_GETMAXSIZE, 0L, (LPARAM)&sz);

		// This fixes a Windows bug calculating the size when TBSTYLE_DROPDOWN is used.
		int xMaxSize = 0;
		for (int i= 0 ; i < GetButtonCount(); ++i)
		{
			xMaxSize += GetItemRect(i).Width();
		}

		sz.cx = xMaxSize;
		return sz;
	}

	inline DWORD CToolBar::GetPadding() const
	// Returns a DWORD value that contains the horizontal padding in the low word and the vertical padding in the high word, in pixels.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(TB_GETPADDING, 0L, 0L);
	}

	inline CRect CToolBar::GetRect(int idButton) const
	// Retrieves the bounding rectangle for a specified toolbar button.
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		SendMessage(TB_GETRECT, (WPARAM)idButton, (LPARAM)&rc);
		return rc;
	}

	inline int CToolBar::GetRows() const
	// Retrieves the number of rows of buttons in a toolbar with the TBSTYLE_WRAPABLE style.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_GETROWS, 0L, 0L);
	}

	inline int CToolBar::GetTextRows() const
	// Retrieves the maximum number of text rows that can be displayed on a toolbar button.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_GETTEXTROWS, 0L, 0L);
	}

	inline HWND CToolBar::GetToolTips() const
	// Retrieves the handle to the ToolTip control, if any, associated with the toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (HWND)SendMessage(TB_GETTOOLTIPS, 0L, 0L);
	}

	inline BOOL CToolBar::HasText() const
	{
		assert(::IsWindow(m_hWnd));
		BOOL bReturn = FALSE;

		for (int i = 0 ; i < GetButtonCount(); ++i)
		{
			if (SendMessage(TB_GETBUTTONTEXT, GetCommandID(i), 0L) != -1)
				bReturn = TRUE;
		}

		// return TRUE if any button has text
		return bReturn;
	}

	inline BOOL CToolBar::HideButton(int idButton, BOOL fShow) const
	//Hides or shows the specified button in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_HIDEBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG (fShow, 0));
	}

	inline int CToolBar::HitTest() const
	// Determines where a point lies in a toolbar control.

	// We do our own hit test since TB_HITTEST is a bit buggy,
	// and also doesn't work at all on earliest versions of Win95
	{
		assert(::IsWindow(m_hWnd));
		CPoint pt = GetCursorPos();
		ScreenToClient(pt);

		int nButtons = (int)SendMessage(TB_BUTTONCOUNT, 0L, 0L);
		int iButton = -1;

		for (int i = 0 ; i < nButtons; ++i)
		{
			CRect rc = GetItemRect(i);
			if (rc.PtInRect(pt))
				iButton = i;
		}

		return iButton;
	}

	inline BOOL CToolBar::Indeterminate(int idButton, BOOL fIndeterminate) const
	//Hides or shows the specified button in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_INDETERMINATE, (WPARAM)idButton, (LPARAM)MAKELONG (fIndeterminate, 0));
	}

	inline BOOL CToolBar::InsertButton(int iButton, LPTBBUTTON lpButton) const
	// Inserts a button in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_INSERTBUTTON, (WPARAM)iButton, (LPARAM)lpButton);
	}

	inline BOOL CToolBar::IsButtonHidden(int idButton) const
	// Determines whether the specified button in a toolbar is hidden.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ISBUTTONHIDDEN, (WPARAM)idButton, 0L);
	}

	inline BOOL CToolBar::IsButtonHighlighted(int idButton) const
	// Checks the highlight state of a toolbar button.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ISBUTTONHIGHLIGHTED, (WPARAM)idButton, 0L);
	}

	inline BOOL CToolBar::IsButtonIndeterminate(int idButton) const
	// Determines whether the specified button in a toolbar is indeterminate.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ISBUTTONINDETERMINATE, (WPARAM)idButton, 0L);
	}

	inline BOOL CToolBar::IsButtonPressed(int idButton) const
	// Determines whether the specified button in a toolbar is pressed.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_ISBUTTONPRESSED, (WPARAM)idButton, 0L);
	}

	inline int CToolBar::MapAccelerator(TCHAR chAccel) const
	// Determines whether the specified button in a toolbar is pressed.
	{
		assert(::IsWindow(m_hWnd));
		int uButtonID;
		int idButton;
		if (SendMessage(TB_MAPACCELERATOR, (WPARAM)chAccel, (LPARAM)&uButtonID))
			idButton = uButtonID;
		else
			idButton = -1;

		return idButton;
	}

	inline BOOL CToolBar::MarkButton(int idButton) const
	// Sets the highlight state of a given button in a toolbar control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_MARKBUTTON, (WPARAM)idButton, 0L);
	}

	inline BOOL CToolBar::MoveButton(UINT uOldPos, UINT uNewPos) const
	// Moves a button from one index to another.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_MOVEBUTTON, (WPARAM)uOldPos, (LPARAM)uNewPos);
	}


	inline void CToolBar::OnCreate()
	{
		// We must send this message before sending the TB_ADDBITMAP or TB_ADDBUTTONS message
		SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);

		// allows buttons to have a separate dropdown arrow
		// Note: TBN_DROPDOWN notification is sent by a toolbar control when the user clicks a dropdown button
		SendMessage(TB_SETEXTENDEDSTYLE, 0L, TBSTYLE_EX_DRAWDDARROWS);

		// Turn of Double click processing (i.e. treat a double click as two single clicks)
		DWORD dwStyle = (DWORD)GetClassLongPtr(GCL_STYLE);
		dwStyle &= 	~CS_DBLCLKS;
		SetClassLongPtr(GCL_STYLE, dwStyle);

		// Add extra styles for toolbars inside a rebar
		if (lstrcmp(GetParent()->GetClassName(), _T("ReBarWindow32")) == 0)
		{
			DWORD style = (DWORD)GetWindowLongPtr(GWL_STYLE);
			style |= CCS_NODIVIDER | CCS_NORESIZE;
			SetWindowLongPtr(GWL_STYLE, style);
		}

		SetButtons(m_vToolBarData);

		// Set rows of text to zero
		SendMessage(TB_SETMAXTEXTROWS, 0L, 0L);
	}

	inline LRESULT CToolBar::OnCustomDraw(NMHDR* pNMHDR)
	// With CustomDraw we manually control the drawing of each toolbar button
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
				DWORD dwTBStyle = (DWORD)SendMessage(TB_GETSTYLE, 0L, 0L);
				int nStyle = GetButtonStyle(dwItem);

				int nButton = (int)SendMessage(TB_COMMANDTOINDEX, (WPARAM) dwItem, 0L);
				TBBUTTON tbb = {0};
				SendMessage(TB_GETBUTTON, nButton, (LPARAM)&tbb);
				int iImage = (int)tbb.dwData;

				// Calculate text size
				std::vector<TCHAR> vText(MAX_MENU_STRING, _T('\0'));
				TCHAR* pszText = &vText[0];
				CSize TextSize;
				if (HasText())	// Does any button have text?
				{
					pDrawDC->SelectObject(GetFont());
					if (SendMessage(TB_GETBUTTONTEXT, dwItem, (LPARAM)pszText)> 0)
					{
						TextSize = pDrawDC->GetTextExtentPoint32(pszText, lstrlen(pszText));
					}
				}

				// Draw outline rectangle
				if (nState & (CDIS_HOT | CDIS_SELECTED | CDIS_CHECKED))
				{
					pDrawDC->CreatePen(PS_SOLID, 1, m_Theme.clrOutline);
					pDrawDC->MoveTo(rcRect.left, rcRect.top);
					pDrawDC->LineTo(rcRect.left, rcRect.bottom-1);
					pDrawDC->LineTo(rcRect.right-1, rcRect.bottom-1);
					pDrawDC->LineTo(rcRect.right-1, rcRect.top);
					pDrawDC->LineTo(rcRect.left, rcRect.top);
				}

				// Draw filled gradient background
				rcRect.InflateRect(-1, -1);
				if ((nState & (CDIS_SELECTED|CDIS_CHECKED)) || (GetButtonState(dwItem) & TBSTATE_PRESSED))
				{
					pDrawDC->GradientFill(m_Theme.clrPressed1, m_Theme.clrPressed2, rcRect, FALSE);
				}
				else if (nState & CDIS_HOT)
				{
					pDrawDC->GradientFill(m_Theme.clrHot1, m_Theme.clrHot2, rcRect, FALSE);
				}

				// Get the appropriate image list depending on the button state
				HIMAGELIST himlToolBar;
				if (nState & CDIS_DISABLED)
				{
					himlToolBar = (HIMAGELIST)SendMessage(TB_GETDISABLEDIMAGELIST, 0L, 0L);
				}
				else if (nState & (CDIS_HOT | CDIS_SELECTED | CDIS_CHECKED))
				{
					himlToolBar = (HIMAGELIST)SendMessage(TB_GETHOTIMAGELIST, 0L, 0L);
					if (0 == himlToolBar)
						himlToolBar = (HIMAGELIST)SendMessage(TB_GETIMAGELIST, 0L, 0L);
				}
				else
				{
					himlToolBar = (HIMAGELIST)SendMessage(TB_GETIMAGELIST, 0L, 0L);
				}

				BOOL IsWin95 = (1400 == (GetWinVersion()) || (2400 == GetWinVersion()));

				// Calculate image position
				int cxImage = 0;
				int cyImage = 0;
				ImageList_GetIconSize(himlToolBar, &cxImage, &cyImage);

				int yImage = (rcRect.bottom - rcRect.top - cyImage - TextSize.cy +2)/2;
				int xImage = (rcRect.right + rcRect.left - cxImage)/2 + ((nState & (CDIS_SELECTED|CDIS_CHECKED))? 1:0);
				if (dwTBStyle & TBSTYLE_LIST)
				{
					xImage = rcRect.left + (IsXPThemed()?2:4) + ((nState & CDIS_SELECTED)? 1:0);
					yImage = (rcRect.bottom -rcRect.top - cyImage +2)/2 + ((nState & (CDIS_SELECTED|CDIS_CHECKED))? 1:0);
				}

				// Handle the TBSTYLE_DROPDOWN and BTNS_WHOLEDROPDOWN styles
				if ((nStyle & TBSTYLE_DROPDOWN) || ((nStyle & 0x0080) && (!IsWin95)))
				{
					// Calculate the dropdown arrow position
					int xAPos = (nStyle & TBSTYLE_DROPDOWN)? rcRect.right -6 : (rcRect.right + rcRect.left + cxImage + 4)/2;
					int yAPos = (nStyle & TBSTYLE_DROPDOWN)? (rcRect.bottom - rcRect.top +1)/2 : (cyImage)/2;
					if (dwTBStyle & TBSTYLE_LIST)
					{
						xAPos = (nStyle & TBSTYLE_DROPDOWN)?rcRect.right -6:rcRect.right -5;
						yAPos =	(rcRect.bottom - rcRect.top +1)/2 + ((nStyle & TBSTYLE_DROPDOWN)?0:1);
					}

					xImage -= (nStyle & TBSTYLE_DROPDOWN)?((dwTBStyle & TBSTYLE_LIST)? (IsXPThemed()?-4:0):6):((dwTBStyle & TBSTYLE_LIST)? 0:4);

					// Draw separate background for dropdown arrow
					if ((m_bDrawArrowBkgrnd) && (nState & CDIS_HOT))
					{
						CRect rcArrowBkgnd = rcRect;
						rcArrowBkgnd.left = rcArrowBkgnd.right - 13;
						pDrawDC->GradientFill(m_Theme.clrPressed1, m_Theme.clrPressed2, rcArrowBkgnd, FALSE);
					}

					m_bDrawArrowBkgrnd = FALSE;

					// Manually draw the dropdown arrow
					pDrawDC->CreatePen(PS_SOLID, 1, RGB(0,0,0));
					for (int i = 2; i >= 0; --i)
					{
						pDrawDC->MoveTo(xAPos -i-1, yAPos - i+1);
						pDrawDC->LineTo(xAPos +i,   yAPos - i+1);
					}

					// Draw line between icon and dropdown arrow
					if ((nStyle & TBSTYLE_DROPDOWN) && ((nState & CDIS_SELECTED) || nState & CDIS_HOT))
					{
						pDrawDC->CreatePen(PS_SOLID, 1, m_Theme.clrOutline);
						pDrawDC->MoveTo(rcRect.right - 13, rcRect.top);
						pDrawDC->LineTo(rcRect.right - 13, rcRect.bottom);
					}
				}

				// Draw the button image
				if (xImage > 0)
				{
					ImageList_Draw(himlToolBar, iImage, *pDrawDC, xImage, yImage, ILD_TRANSPARENT);
				}

				//Draw Text
				if (lstrlen(pszText) > 0)
				{
					int iWidth = rcRect.right - rcRect.left - ((nStyle & TBSTYLE_DROPDOWN)?13:0);
					CRect rcText(0, 0, MIN(TextSize.cx, iWidth), TextSize.cy);

					int xOffset = (rcRect.right + rcRect.left - rcText.right + rcText.left - ((nStyle & TBSTYLE_DROPDOWN)? 11 : 1))/2;
					int yOffset = yImage + cyImage +1;

					if (dwTBStyle & TBSTYLE_LIST)
					{
						xOffset = rcRect.left + cxImage + ((nStyle & TBSTYLE_DROPDOWN)?(IsXPThemed()?10:6): 6) + ((nState & CDIS_SELECTED)? 1:0);
						yOffset = (2+rcRect.bottom - rcRect.top - rcText.bottom + rcText.top)/2 + ((nState & CDIS_SELECTED)? 1:0);
						rcText.right = MIN(rcText.right,  rcRect.right - xOffset);
					}

					OffsetRect(&rcText, xOffset, yOffset);

					int iMode = pDrawDC->SetBkMode(TRANSPARENT);
					pDrawDC->SelectObject(GetFont());

					if (nState & (CDIS_DISABLED))
					{
						// Draw text twice for embossed look
						rcText.OffsetRect(1, 1);
						pDrawDC->SetTextColor(RGB(255,255,255));
						pDrawDC->DrawText(pszText, lstrlen(pszText), rcText, DT_LEFT);
						rcText.OffsetRect(-1, -1);
						pDrawDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
						pDrawDC->DrawText(pszText, lstrlen(pszText), rcText, DT_LEFT);
					}
					else
					{
						pDrawDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
						pDrawDC->DrawText(pszText, lstrlen(pszText), rcText, DT_LEFT | DT_END_ELLIPSIS);
					}
					pDrawDC->SetBkMode(iMode);
				}
			}
			return CDRF_SKIPDEFAULT;  // No further drawing
		}
		return 0L;
	}

	inline void CToolBar::OnDestroy()
	{
		HIMAGELIST himlToolBar    = (HIMAGELIST)SendMessage(TB_GETIMAGELIST,    0L, 0L);
		HIMAGELIST himlToolBarHot = (HIMAGELIST)SendMessage(TB_GETHOTIMAGELIST, 0L, 0L);
		HIMAGELIST himlToolBarDis = (HIMAGELIST)SendMessage(TB_GETDISABLEDIMAGELIST, 0L, 0L);
		ImageList_Destroy(himlToolBar);
		ImageList_Destroy(himlToolBarHot);
		ImageList_Destroy(himlToolBarDis);
	}

	inline LRESULT CToolBar::OnNotifyReflect(WPARAM wParam, LPARAM lParam)
	// Notifications sent to the parent window are reflected back here
	{
		UNREFERENCED_PARAMETER(wParam);

		switch (((LPNMHDR)lParam)->code)
		{
			case NM_CUSTOMDRAW:
			{
				if (m_Theme.UseThemes)
					return OnCustomDraw((LPNMHDR) lParam);
			}
			break;

			case TBN_DROPDOWN:
			{
				int iItem = ((LPNMTOOLBAR) lParam)->iItem;

				// a boolean expression
				m_bDrawArrowBkgrnd = (GetButtonStyle(iItem) & TBSTYLE_DROPDOWN);
			}
			break;
		}
		return 0L;
	}

	inline void CToolBar::OnWindowPosChanging(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		// Adjust size for toolbars inside a rebar
		CWnd* pParent = GetParent();
		if (lstrcmp(pParent->GetClassName(), _T("ReBarWindow32")) == 0)
		{
			ReBarTheme* pTheme = (ReBarTheme*)pParent->SendMessage(UWM_GETREBARTHEME, 0, 0);

			if (pTheme && pTheme->UseThemes && pTheme->ShortBands)
			{
				LPWINDOWPOS pWinPos = (LPWINDOWPOS)lParam;
				pWinPos->cx = GetMaxSize().cx+2;
			}
		}
	}

	inline void CToolBar::PreCreate(CREATESTRUCT &cs)
	{
		// Sets the CREATESTRUCT parameters prior to window creation
		cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT;
	}

	inline void CToolBar::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  TOOLBARCLASSNAME;
	}

	inline BOOL CToolBar::PressButton(int idButton, BOOL fPress) const
	// Presses or releases the specified button in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_PRESSBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fPress, 0));
	}

	inline BOOL CToolBar::ReplaceBitmap(UINT NewToolBarID)
	// Replaces an existing bitmap with a new bitmap.

	// Note: ReplaceBitmap supports a maximum colour depth of 8 bits (256 colours)
	//       For more colours, use an ImageList instead
	{
		assert(::IsWindow(m_hWnd));

		int iNumButtons = 0;
		std::vector<UINT>::iterator iter;
		for (iter = GetToolBarData().begin(); iter < GetToolBarData().end(); ++iter)
			if ((*iter) != 0) ++iNumButtons;

		TBREPLACEBITMAP tbrb = {0};
		tbrb.hInstNew = GetApp()->GetResourceHandle();
		tbrb.hInstOld = GetApp()->GetResourceHandle();
		tbrb.nIDNew = NewToolBarID;
		tbrb.nIDOld = m_OldToolBarID;
		tbrb.nButtons  = iNumButtons;

		BOOL bResult = (BOOL)SendMessage(TB_REPLACEBITMAP, iNumButtons, (LPARAM)&tbrb);
		if (bResult)
			m_OldToolBarID = NewToolBarID;

		return bResult;
	}

	inline void CToolBar::SaveRestore(BOOL fSave, TBSAVEPARAMS* ptbsp) const
	// Presses or releases the specified button in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(TB_PRESSBUTTON, (WPARAM)fSave, (LPARAM)ptbsp);
	}

	inline BOOL CToolBar::SetBitmap(UINT nID)
	// Set the button images
	{
		assert(::IsWindow(m_hWnd));

		CBitmap Bitmap(nID);
		assert (Bitmap.GetHandle());
		BITMAP bm = Bitmap.GetBitmapData();

		int iNumButtons = 0;
		std::vector<UINT>::iterator iter;
		for (iter = GetToolBarData().begin(); iter < GetToolBarData().end(); ++iter)
			if ((*iter) != 0) ++iNumButtons;

		int iImageWidth  = bm.bmWidth / iNumButtons;
		int iImageHeight = bm.bmHeight;

		// Set the bitmap size first
		SetBitmapSize(iImageWidth, iImageHeight);

		BOOL bResult = FALSE;
		if (m_OldToolBarID)
			bResult = ReplaceBitmap(nID);
		else
			bResult = (BOOL)AddBitmap(nID);

		return bResult;
	}

	inline BOOL CToolBar::SetBitmapSize(int cx, int cy) const
	// Sets the size of the bitmapped images to be added to a toolbar.

	// Needs to be used when the image size is not the default 16 x 15
	// Call this function before using AddBitmap or ReplaceBitmap
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETBITMAPSIZE, 0L, MAKELONG(cx, cy));
	}

	inline int CToolBar::SetButtons(const std::vector<UINT>& vToolBarData) const
	// Assigns a resource ID to each toolbar button
	{
		assert(::IsWindow(m_hWnd));

		int iImages = 0;
		UINT iNumButtons = (UINT)vToolBarData.size();

		// Remove any existing buttons
		while (SendMessage(TB_BUTTONCOUNT,  0L, 0L) > 0)
		{
			if(!SendMessage(TB_DELETEBUTTON, 0L, 0L))
				break;
		}

		if (iNumButtons > 0)
		{
			// TBBUTTON structure for each button in the toolbar
			TBBUTTON tbb = {0};

			for (UINT j = 0 ; j < iNumButtons; ++j)
			{
				ZeroMemory(&tbb, sizeof(TBBUTTON));

				if (0 == vToolBarData[j])
				{
					tbb.fsStyle = TBSTYLE_SEP;
				}
				else
				{
					tbb.dwData  = iImages;
					tbb.iBitmap = iImages;
					tbb.idCommand = vToolBarData[j];
					tbb.fsState = TBSTATE_ENABLED;
					tbb.fsStyle = TBSTYLE_BUTTON;
				}

				// Add the button to the toolbar
				if (SendMessage(TB_ADDBUTTONS, 1L, (LPARAM)&tbb))
					iImages++;
				else
					break;
			}
		}

		return iImages;
	}

	inline BOOL CToolBar::SetButtonSize(int cx, int cy) const
	// Sets the size of the buttons to be added to a toolbar
	// The size can be set only before adding any buttons to the toolbar
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETBUTTONSIZE, 0L, MAKELONG(cx, cy));
	}

	inline BOOL CToolBar::SetButtonState(int idButton, UINT State) const
	{
	// Set the state of an individual button
	//	TBSTATE_CHECKED		The button has the TBSTYLE_CHECK style and is being clicked.
	//	TBSTATE_ELLIPSES	The button's text is cut off and an ellipsis is displayed.
	//	TBSTATE_ENABLED		The button accepts user input. A button that doesn't have this state is grayed.
	//	TBSTATE_HIDDEN		The button is not visible and cannot receive user input.
	//	TBSTATE_INDETERMINATE	The button is grayed.
	//	TBSTATE_MARKED		The button is marked. The interpretation of a marked item is dependent upon the application.
	//	TBSTATE_PRESSED		The button is being clicked.
	//	TBSTATE_WRAP		The button is followed by a line break.

		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETSTATE, (WPARAM) idButton, (LPARAM)MAKELONG (State, 0));
 	}

	inline BOOL CToolBar::SetButtonStyle(int idButton, BYTE Style) const
	//	The the style of the toolbar control. The following button styles are supported:
	//	TBSTYLE_BUTTON		Standard pushbutton (default)
	//	TBSTYLE_SEP			Separator
	//	TBSTYLE_CHECK		Auto check-box button
	//	TBSTYLE_GROUP		Marks the start of a group of buttons
	//	TBSTYLE_CHECKGROUP	Marks the start of a group of check-box buttons
	//	TBSTYLE_DROPDOWN	Creates a drop-down list button
	//	TBSTYLE_AUTOSIZE	The button's width will be calculated based on the text of the button, not on the size of the image
	//	TBSTYLE_NOPREFIX	The button text will not have an accelerator prefix associated with it
	{
		assert(::IsWindow(m_hWnd));

		TBBUTTONINFO tbbi = {0};
		tbbi.cbSize = sizeof(TBBUTTONINFO);
		tbbi.dwMask = TBIF_STYLE;
		tbbi.fsStyle = Style;

		// Note:  TB_SETBUTTONINFO requires comctl32.dll version 4.71 or later
		//        i.e. Win95 with IE4 / NT with IE4   or later
		return (BOOL)SendMessage(TB_SETBUTTONINFO, idButton, (LPARAM) &tbbi);
	}

	inline BOOL CToolBar::SetButtonText(int idButton, LPCTSTR szText)
	// This rather convoluted approach to setting toolbar button text supports
	// all versions of Windows, including Win95 with COMCTL32.DLL version 4.0
	{
		assert(::IsWindow(m_hWnd));
		int iIndex = CommandToIndex(idButton);
		assert(-1 != iIndex);

		BOOL Succeeded = TRUE;
		tString sString = szText;
		std::map<tString, int>::iterator m;
		int iString;

		// Check to see if the string is already added
		m = m_StringMap.find(sString);
		if (m_StringMap.end() == m)
		{
			if (0 == m_StringMap.size())
			{
				// Place a blank string first in the string table, in case some
				// buttons don't have text
				TCHAR szString[3] = _T(" ");
				szString[2] = _T('\0');		// Double-null terminate
				SendMessage(TB_ADDSTRING, 0L, (LPARAM)szString);
			}

			// No index for this string exists, so create it now
			TCHAR szBuf[80] = _T("");
			lstrcpyn(szBuf, szText, 79);
			szBuf[lstrlen(szBuf)+1] = _T('\0');		// Double-null terminate

			iString = (int)SendMessage(TB_ADDSTRING, 0L, (LPARAM)szBuf);
			if (-1 == iString )
				Succeeded = FALSE;

			// Save the string its index in our map
			m_StringMap.insert(std::make_pair(sString, iString));
		}
		else
		{
			// String found, use the index from our map
			iString = m->second;
		}

		if (Succeeded)
		{
			TBBUTTON tbb = {0};
			Succeeded = (BOOL)SendMessage(TB_GETBUTTON, iIndex, (LPARAM)&tbb);

			tbb.iString = iString;

			// Turn off ToolBar drawing
			SendMessage(WM_SETREDRAW, FALSE, 0L);

			if (Succeeded)
				Succeeded = (BOOL)SendMessage(TB_DELETEBUTTON, iIndex, 0L);

			if (Succeeded)
				Succeeded = (BOOL)SendMessage(TB_INSERTBUTTON, iIndex, (LPARAM)&tbb);

			// Ensure the button now includes some text rows
			if (0 == SendMessage(TB_GETTEXTROWS, 0L, 0L))
				SendMessage(TB_SETMAXTEXTROWS, 1L, 0L);

			// Turn on ToolBar drawing
			SendMessage(WM_SETREDRAW, TRUE, 0L);
		}
		// Redraw button
		CRect r = GetItemRect(iIndex);
		InvalidateRect(&r, TRUE);

		return Succeeded;
	}

	inline BOOL CToolBar::SetButtonWidth(int idButton, int nWidth) const
	// The set button width can adjust the width of the button after it is created.
	// This is useful when replacing a button with a ComboBox or other control.
	// Note:  TB_SETBUTTONINFO requires comctl32.dll version 4.71 or later
	//        i.e. Win95 with IE4 / NT with IE4   or later
	{
		assert(::IsWindow(m_hWnd));

		TBBUTTONINFO tbbi = {0};
		tbbi.cbSize = sizeof(TBBUTTONINFO);
		tbbi.dwMask = TBIF_SIZE;
		tbbi.cx = (WORD)nWidth;
		BOOL bResult = (BOOL)SendMessage(TB_SETBUTTONINFO, (WPARAM)idButton, (LPARAM)&tbbi);

		// Send a changed message to the parent (used by the rebar)
		SIZE MaxSize = GetMaxSize();
		GetParent()->SendMessage(UWM_TOOLBAR_RESIZE, (WPARAM)m_hWnd, (LPARAM)&MaxSize);

		return bResult;
	}

	inline BOOL CToolBar::SetCommandID(int iIndex, int idButton) const
	// Sets the command identifier of a toolbar button
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETCMDID, iIndex, idButton);
	}

	inline HIMAGELIST CToolBar::SetDisableImageList(HIMAGELIST himlNewDisabled) const
	// Sets the image list that the toolbar control will use to display disabled buttons.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlNewDisabled);
	}

	inline DWORD CToolBar::SetDrawTextFlags(DWORD dwMask, DWORD dwDTFlags) const
	// Sets the text drawing flags for the toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(TB_SETDRAWTEXTFLAGS, (WPARAM)dwMask, (LPARAM)dwDTFlags);
	}

	inline DWORD CToolBar::SetExtendedStyle(DWORD dwExStyle) const
	// Sets the text drawing flags for the toolbar.
	// Extended styles include: TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_HIDECLIPPEDBUTTONS, TBSTYLE_EX_DOUBLEBUFFER and TBSTYLE_EX_MIXEDBUTTONS
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(TB_SETEXTENDEDSTYLE, 0L, (LPARAM)dwExStyle);
	}

	inline HIMAGELIST CToolBar::SetHotImageList(HIMAGELIST himlNewHot) const
	// Sets the image list that the toolbar control will use to display hot buttons.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_SETHOTIMAGELIST, 0L, (LPARAM)himlNewHot);
	}
	
	inline int CToolBar::SetHotItem(int iHot) const
	// Sets the hot item in a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(TB_SETHOTITEM, (WPARAM)iHot, 0L);
	}

	inline HIMAGELIST CToolBar::SetImageList(HIMAGELIST himlNew) const
	// Sets the image list that the toolbar will use to display buttons that are in their default state.
	{
		assert(::IsWindow(m_hWnd));
		return (HIMAGELIST)SendMessage(TB_SETIMAGELIST, 0L, (LPARAM)himlNew);
	}

	inline BOOL CToolBar::SetImages(COLORREF crMask, UINT ToolBarID, UINT ToolBarHotID, UINT ToolBarDisabledID)
	// Either sets the imagelist or adds/replaces bitmap depending on ComCtl32.dll version
	// Assumes the width of the button image = bitmap_size / buttons
	// Assumes buttons have been already been added via AdddToolBarButton
	// The colour mask is often grey RGB(192,192,192) or magenta (255,0,255);
	// The color mask is ignored for 32bit bitmap resources
	// The Hot and disiabled bitmap resources can be 0
	{
		assert(::IsWindow(m_hWnd));

		// ToolBar ImageLists require Comctl32.dll version 4.7 or later
		if (400 == GetComCtlVersion())
		{
			// We are using COMCTL32.DLL version 4.0, so we can't use an imagelist.
			// Instead we simply set the bitmap.
			return SetBitmap(ToolBarID);
		}

		int iNumButtons = 0;
		std::vector<UINT>::iterator iter;
		for (iter = GetToolBarData().begin(); iter < GetToolBarData().end(); ++iter)
			if ((*iter) != 0) ++iNumButtons;

		if (iNumButtons > 0)
		{
			// Set the button images
			CBitmap Bitmap(ToolBarID);
			assert(Bitmap.GetHandle());

			BITMAP bm = Bitmap.GetBitmapData();
			int iImageWidth  = bm.bmWidth / iNumButtons;
			int iImageHeight = bm.bmHeight;

			HIMAGELIST himlToolBar    = (HIMAGELIST)SendMessage(TB_GETIMAGELIST,    0L, 0L);
			HIMAGELIST himlToolBarHot = (HIMAGELIST)SendMessage(TB_GETHOTIMAGELIST, 0L, 0L);
			HIMAGELIST himlToolBarDis = (HIMAGELIST)SendMessage(TB_GETDISABLEDIMAGELIST, 0L, 0L);
			ImageList_Destroy(himlToolBar);
			ImageList_Destroy(himlToolBarHot);
			ImageList_Destroy(himlToolBarDis);

			himlToolBar = ImageList_Create(iImageWidth, iImageHeight, ILC_COLOR32 | ILC_MASK, iNumButtons, 0);
			assert(himlToolBar);

			ImageList_AddMasked(himlToolBar, Bitmap, crMask);
			SendMessage(TB_SETIMAGELIST, 0L, (LPARAM)himlToolBar);

			if (ToolBarHotID)
			{
				CBitmap BitmapHot(ToolBarHotID);
				assert(BitmapHot);

				himlToolBarHot = ImageList_Create(iImageWidth, iImageHeight, ILC_COLOR32 | ILC_MASK, iNumButtons, 0);
				assert(himlToolBarHot);

				ImageList_AddMasked(himlToolBarHot, BitmapHot, crMask);
				SendMessage(TB_SETHOTIMAGELIST, 0L, (LPARAM)himlToolBarHot);
			}

			if (ToolBarDisabledID)
			{
				CBitmap BitmapDisabled(ToolBarDisabledID);
				assert(BitmapDisabled);

				himlToolBarDis = ImageList_Create(iImageWidth, iImageHeight, ILC_COLOR32 | ILC_MASK, iNumButtons, 0);
				assert(himlToolBarDis);

				ImageList_AddMasked(himlToolBarDis, BitmapDisabled, crMask);
				SendMessage(TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlToolBarDis);
			}
			else
			{
				himlToolBarDis = CreateDisabledImageList(himlToolBar);
				SendMessage(TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlToolBarDis);
			}

			// Inform the parent of the change (rebar needs this)
			SIZE MaxSize = GetMaxSize();
			GetParent()->SendMessage(UWM_TOOLBAR_RESIZE, (WPARAM)m_hWnd, (LPARAM)&MaxSize);
		}

		return TRUE;
	}

	inline BOOL CToolBar::SetIndent(int iIndent) const
	// Sets the indentation for the first button in a toolbar control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETINDENT, (WPARAM)iIndent, 0L);
	}

	inline BOOL CToolBar::SetMaxTextRows(int iMaxRows) const
	// Sets the maximum number of text rows displayed on a toolbar button.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETMAXTEXTROWS, (WPARAM)iMaxRows, 0L);
	}

	inline BOOL CToolBar::SetPadding(int cx, int cy) const
	// Sets the padding for a toolbar control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(TB_SETPADDING, 0L, (WPARAM)MAKELONG(cx, cy));
	}

	inline void CToolBar::SetToolBarTheme(ToolBarTheme& Theme)
	{
		m_Theme.UseThemes   = Theme.UseThemes;
		m_Theme.clrHot1     = Theme.clrHot1;
		m_Theme.clrHot2     = Theme.clrHot2;
		m_Theme.clrPressed1 = Theme.clrPressed1;
		m_Theme.clrPressed2 = Theme.clrPressed2;
		m_Theme.clrOutline  = Theme.clrOutline;

		if (IsWindow())
			Invalidate();
	}

	inline void CToolBar::SetToolTips(HWND hwndToolTip) const
	// Associates a ToolTip control with a toolbar.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(TB_SETTOOLTIPS, (WPARAM)hwndToolTip, 0L);
	}

	inline LRESULT CToolBar::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_DESTROY:
			OnDestroy();
			break;
		case UWM_GETTOOLBARTHEME:
			{
				ToolBarTheme& tt = GetToolBarTheme();
				return (LRESULT)&tt;
			}
		case WM_WINDOWPOSCHANGING:
			OnWindowPosChanging(wParam, lParam);
			break;
		}

		// pass unhandled messages on for default processing
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}

} // namespace Win32xx

#endif // #ifndef _WIN32XX_TOOLBAR_H_
