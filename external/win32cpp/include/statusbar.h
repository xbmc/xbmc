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


#ifndef _WIN32XX_STATUSBAR_H_
#define _WIN32XX_STATUSBAR_H_

#include "wincore.h"

namespace Win32xx
{

	//////////////////////////////////////
	// Declaration of the CStatusBar class
	//
	class CStatusBar : public CWnd
	{
	public:
		CStatusBar();
		virtual ~CStatusBar() {}

	// Overridables
		virtual void PreCreate(CREATESTRUCT& cs);
		virtual void PreRegisterClass(WNDCLASS &wc);

	// Attributes
		int GetParts();
		HICON GetPartIcon(int iPart);
		CRect GetPartRect(int iPart);
		tString GetPartText(int iPart) const;
		BOOL IsSimple();
		BOOL SetPartIcon(int iPart, HICON hIcon);
		BOOL SetPartText(int iPart, LPCTSTR szText, UINT Style = 0) const;
		BOOL SetPartWidth(int iPart, int iWidth) const;

	// Operations
		CStatusBar(const CStatusBar&);				// Disable copy construction
		CStatusBar& operator = (const CStatusBar&); // Disable assignment operator

		BOOL CreateParts(int iParts, const int iPaneWidths[]) const;
		void SetSimple(BOOL fSimple = TRUE);
	};

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	//////////////////////////////////////
	// Definitions for the CStatusBar class
	//
	inline CStatusBar::CStatusBar()
	{
	}

	inline BOOL CStatusBar::CreateParts(int iParts, const int iPaneWidths[]) const
	// Sets the number of parts in a status window and the coordinate of the right edge of each part. 
	// If an element of iPaneWidths is -1, the right edge of the corresponding part extends
	//  to the border of the window
	{
		assert(::IsWindow(m_hWnd));
		assert(iParts <= 256);	
		
		return (BOOL)SendMessage(SB_SETPARTS, iParts, (LPARAM)iPaneWidths);		
	}

	inline int CStatusBar::GetParts()
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(SB_GETPARTS, 0L, 0L);
	}

	inline HICON CStatusBar::GetPartIcon(int iPart)
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(SB_GETICON, (WPARAM)iPart, 0L);
	}

	inline CRect CStatusBar::GetPartRect(int iPart)
	{
		assert(::IsWindow(m_hWnd));
		
		CRect rc;
		SendMessage(SB_GETRECT, (WPARAM)iPart, (LPARAM)&rc);
		return rc;
	}

	inline tString CStatusBar::GetPartText(int iPart) const
	{
		assert(::IsWindow(m_hWnd));
		tString PaneText;
		
		// Get size of Text array
		int iChars = LOWORD (SendMessage(SB_GETTEXTLENGTH, iPart, 0L));

		std::vector<TCHAR> Text( iChars +1, _T('\0') );
		TCHAR* pTextArray = &Text[0];

		SendMessage(SB_GETTEXT, iPart, (LPARAM)pTextArray);
		PaneText = pTextArray;			
		return PaneText;
	}

	inline BOOL CStatusBar::IsSimple()
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(SB_ISSIMPLE, 0L, 0L);
	}

	inline void CStatusBar::PreCreate(CREATESTRUCT &cs)
	{
		cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM | SBARS_SIZEGRIP;
	}

	inline void CStatusBar::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  STATUSCLASSNAME;
	}

	inline BOOL CStatusBar::SetPartText(int iPart, LPCTSTR szText, UINT Style) const
	// Available Styles: Combinations of ...
	//0					The text is drawn with a border to appear lower than the plane of the window.
	//SBT_NOBORDERS		The text is drawn without borders.
	//SBT_OWNERDRAW		The text is drawn by the parent window.
	//SBT_POPOUT		The text is drawn with a border to appear higher than the plane of the window.
	//SBT_RTLREADING	The text will be displayed in the opposite direction to the text in the parent window.
	{
		assert(::IsWindow(m_hWnd));
		
		BOOL bResult = FALSE;
		if (SendMessage(SB_GETPARTS, 0L, 0L) >= iPart)
			bResult = (BOOL)SendMessage(SB_SETTEXT, iPart | Style, (LPARAM)szText);

		return bResult;
	}

	inline BOOL CStatusBar::SetPartIcon(int iPart, HICON hIcon)
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(SB_SETICON, (WPARAM)iPart, (LPARAM) hIcon);
	}

	inline BOOL CStatusBar::SetPartWidth(int iPart, int iWidth) const
	{
		// This changes the width of an existing pane, or creates a new pane
		// with the specified width.
		// A width of -1 for the last part sets the width to the border of the window.

		assert(::IsWindow(m_hWnd));
		assert(iPart >= 0 && iPart <= 255);

		// Fill the PartWidths vector with the current width of the statusbar parts
		int PartsCount = (int)SendMessage(SB_GETPARTS, 0L, 0L);
		std::vector<int> PartWidths(PartsCount, 0);
		int* pPartWidthArray = &PartWidths[0];
		SendMessage(SB_GETPARTS, PartsCount, (LPARAM)pPartWidthArray);

		// Fill the NewPartWidths vector with the new width of the statusbar parts
		int NewPartsCount = MAX(iPart+1, PartsCount);	
		std::vector<int> NewPartWidths(NewPartsCount, 0);;
		NewPartWidths = PartWidths;
		int* pNewPartWidthArray = &NewPartWidths[0];
		
		if (0 == iPart)
			pNewPartWidthArray[iPart] = iWidth;
		else
		{
			if (iWidth >= 0)
				pNewPartWidthArray[iPart] = pNewPartWidthArray[iPart -1] + iWidth;
			else
				pNewPartWidthArray[iPart] = -1;
		}

		// Set the statusbar parts with our new parts count and part widths
		BOOL bResult = (BOOL)SendMessage(SB_SETPARTS, NewPartsCount, (LPARAM)pNewPartWidthArray);

		return bResult;
	}

	inline void CStatusBar::SetSimple(BOOL fSimple /* = TRUE*/)
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(SB_SIMPLE, (WPARAM)fSimple, 0L);
	}

} // namespace Win32xx

#endif // #ifndef _WIN32XX_STATUSBAR_H_
