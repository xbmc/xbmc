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


#ifndef _WIN32XX_REBAR_H_
#define _WIN32XX_REBAR_H_

#include "wincore.h"
#include "gdi.h"


namespace Win32xx
{

	struct ReBarTheme
	{
		BOOL UseThemes;			// TRUE if themes are used
		COLORREF clrBkgnd1;		// Colour 1 for rebar background
		COLORREF clrBkgnd2;		// Colour 2 for rebar background
		COLORREF clrBand1;		// Colour 1 for rebar band background. Use NULL if not required
		COLORREF clrBand2;		// Colour 2 for rebar band background. Use NULL if not required
		BOOL FlatStyle;			// Bands are rendered with flat rather than raised style
		BOOL BandsLeft;			// Position bands left on rearrange
		BOOL LockMenuBand;		// Lock MenuBar's band in dedicated top row, without gripper
		BOOL RoundBorders;		// Use rounded band borders
		BOOL ShortBands;        // Allows bands to be shorter than maximum available width
		BOOL UseLines;			// Displays horizontal lines between bands
	};

	////////////////////////////////////
	// Declaration of the CReBar class
	//
	class CReBar : public CWnd
	{
	public:
		CReBar();
		virtual ~CReBar();

		// Operations
		BOOL DeleteBand(const int nBand) const;
		int  HitTest(RBHITTESTINFO& rbht);
		HWND HitTest(POINT pt);
		int  IDToIndex(UINT uBandID) const;
		BOOL InsertBand(const int nBand, REBARBANDINFO& rbbi) const;
		BOOL IsBandVisible(int nBand) const;
		void MaximizeBand(UINT uBand, BOOL fIdeal = FALSE);
		void MinimizeBand(UINT uBand);
		BOOL MoveBand(UINT uFrom, UINT uTo);
		void MoveBandsLeft();
		BOOL ResizeBand(const int nBand, const CSize& sz) const;
		BOOL ShowGripper(int nBand, BOOL fShow) const;
		BOOL ShowBand(int nBand, BOOL fShow) const;
		BOOL SizeToRect(CRect& rect) const;
		
		// Attributes
		int  GetBand(const HWND hWnd) const;
		CRect GetBandBorders(int nBand) const;
		int  GetBandCount() const;
		BOOL GetBandInfo(const int nBand, REBARBANDINFO& rbbi) const;
		CRect GetBandRect(int i) const;
		UINT GetBarHeight() const;
		BOOL GetBarInfo(REBARINFO& rbi) const;
		HWND GetMenuBar() {return m_hMenuBar;}
		ReBarTheme& GetReBarTheme() {return m_Theme;}
		UINT GetRowCount() const;
		int  GetRowHeight(int nRow) const;
		UINT GetSizeofRBBI() const;
		HWND GetToolTips() const;
		BOOL SetBandBitmap(const int nBand, const CBitmap* pBackground) const;
		BOOL SetBandColor(const int nBand, const COLORREF clrFore, const COLORREF clrBack) const;
		BOOL SetBandInfo(const int nBand, REBARBANDINFO& rbbi) const;
		BOOL SetBarInfo(REBARINFO& rbi) const;
		void SetMenuBar(HWND hMenuBar) {m_hMenuBar = hMenuBar;}
		void SetReBarTheme(ReBarTheme& Theme);

	protected:
	//Overridables
		virtual BOOL OnEraseBkgnd(CDC* pDC);
		virtual void PreCreate(CREATESTRUCT& cs);
		virtual void PreRegisterClass(WNDCLASS &wc);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CReBar(const CReBar&);				// Disable copy construction
		CReBar& operator = (const CReBar&); // Disable assignment operator

		ReBarTheme m_Theme;
		BOOL m_bIsDragging;
		HWND m_hMenuBar;
		LPARAM m_Orig_lParam;
	};

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	///////////////////////////////////
	// Definitions for the CReBar class
	//
	inline CReBar::CReBar() : m_bIsDragging(FALSE), m_hMenuBar(0), m_Orig_lParam(0L)
	{
		ZeroMemory(&m_Theme, sizeof(ReBarTheme));
	}

	inline CReBar::~CReBar()
	{
	}

	inline BOOL CReBar::DeleteBand(int nBand) const
	// Deletes a band from a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(RB_DELETEBAND, nBand, 0L);
	}

	inline int CReBar::GetBand(HWND hWnd) const
	// Returns the zero based band number for this window handle
	{
		assert(::IsWindow(m_hWnd));

		int nResult = -1;
		if (NULL == hWnd) return nResult;

		for (int nBand = 0; nBand < GetBandCount(); ++nBand)
		{
			REBARBANDINFO rbbi = {0};
			rbbi.cbSize = GetSizeofRBBI();
			rbbi.fMask = RBBIM_CHILD;
			GetBandInfo(nBand, rbbi);
			if (rbbi.hwndChild == hWnd)
				nResult = nBand;
		}

		return nResult;
	}

	inline CRect CReBar::GetBandBorders(int nBand) const
	// Retrieves the borders of a band.
	{
		assert(::IsWindow(m_hWnd));

		CRect rc;
		SendMessage(RB_GETBANDBORDERS, nBand, (LPARAM)&rc);
		return rc;
	}

	inline int  CReBar::GetBandCount() const
	// Retrieves the count of bands currently in the rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(RB_GETBANDCOUNT, 0L, 0L);
	}

	inline BOOL CReBar::GetBandInfo(int nBand, REBARBANDINFO& rbbi) const
	// Retrieves information about a specified band in a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		assert(nBand >=  0);

		// REBARBANDINFO describes individual BAND characteristics
		rbbi.cbSize = GetSizeofRBBI();
		return (BOOL)SendMessage(RB_GETBANDINFO, nBand, (LPARAM)&rbbi);
	}

	inline CRect CReBar::GetBandRect(int i) const
	// Retrieves the bounding rectangle for a given band in a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		CRect rc;
		SendMessage(RB_GETRECT, i, (LPARAM)&rc);
		return rc;
	}

	inline UINT CReBar::GetBarHeight() const
	// Retrieves the height of the rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)SendMessage(RB_GETBARHEIGHT, 0L, 0L);
	}

	inline BOOL CReBar::GetBarInfo(REBARINFO& rbi) const
	// Retrieves information about the rebar control and the image list it uses.
	{
		assert(::IsWindow(m_hWnd));

		// REBARINFO describes overall rebar control characteristics
		rbi.cbSize = GetSizeofRBBI();
		return (BOOL)SendMessage(RB_GETBARINFO, 0L, (LPARAM)&rbi);
	}

	inline UINT CReBar::GetRowCount() const
	// Retrieves the number of rows of bands in a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)SendMessage(RB_GETROWCOUNT, 0L, 0L);
	}

	inline int CReBar::GetRowHeight(int nRow) const
	// Retrieves the height of a specified row in a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(RB_GETROWHEIGHT, nRow, 0L);
	}

	inline UINT CReBar::GetSizeofRBBI() const
	// The size of the REBARBANDINFO struct changes according to _WIN32_WINNT
	// sizeof(REBARBANDINFO) can report an incorrect size for older Window versions,
	// or newer Window version without XP themes enabled.
	// Use this function to get a safe size for REBARBANDINFO.
	{
		assert(::IsWindow(m_hWnd));

		UINT uSizeof = sizeof(REBARBANDINFO);

	#if defined REBARBANDINFO_V6_SIZE	// only defined for VS2008 or higher
	  #if !defined (_WIN32_WINNT) || _WIN32_WINNT >= 0x0600
		if ((GetWinVersion() < 2600) || (GetComCtlVersion() < 610)) // Vista and Vista themes?
			uSizeof = REBARBANDINFO_V6_SIZE;
	  #endif
	#endif

		return uSizeof;
	}

	inline HWND CReBar::GetToolTips() const
	// Retrieves the handle to any ToolTip control associated with the rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (HWND)SendMessage(RB_GETTOOLTIPS, 0L, 0L);
	}

	inline int CReBar::HitTest(RBHITTESTINFO& rbht)
	// Determines which portion of a rebar band is at a given point on the screen,
	//  if a rebar band exists at that point.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(RB_HITTEST, 0L, (LPARAM)&rbht);
	}

	inline HWND CReBar::HitTest(POINT pt)
	// Return the child HWND at the given point
	{
		assert(::IsWindow(m_hWnd));

		// Convert the point to client co-ordinates
		ScreenToClient(pt);

		// Get the rebar band with the point
		RBHITTESTINFO rbhti = {0};
		rbhti.pt = pt;
		int iBand = HitTest(rbhti);

		if (iBand >= 0)
		{
			// Get the rebar band's hWnd
			REBARBANDINFO rbbi = {0};
			rbbi.cbSize = GetSizeofRBBI();
			rbbi.fMask = RBBIM_CHILD;
			GetBandInfo(iBand, rbbi);

			return rbbi.hwndChild;
		}
		else
			return NULL;
	}

	inline int CReBar::IDToIndex(UINT uBandID) const
	// Converts a band identifier to a band index in a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(RB_IDTOINDEX, (WPARAM)uBandID, 0L);
	}

	inline BOOL CReBar::InsertBand(int nBand, REBARBANDINFO& rbbi) const
	// Inserts a new band in a rebar control.
	{
		assert(::IsWindow(m_hWnd));

		rbbi.cbSize = GetSizeofRBBI();
		return (BOOL)SendMessage(RB_INSERTBAND, nBand, (LPARAM)&rbbi);
	}

	inline BOOL CReBar::IsBandVisible(int nBand) const
	// Returns true if the band is visible
	{
		assert(::IsWindow(m_hWnd));

		REBARBANDINFO rbbi = {0};
		rbbi.cbSize = GetSizeofRBBI();
		rbbi.fMask = RBBIM_STYLE;
		GetBandInfo(nBand, rbbi);

		return !(rbbi.fStyle & RBBS_HIDDEN);
	}

	inline BOOL CReBar::OnEraseBkgnd(CDC* pDC)
	{
		BOOL Erase = TRUE;
		if (!m_Theme.UseThemes)
			Erase = FALSE;

		if (!m_Theme.clrBkgnd1 && !m_Theme.clrBkgnd2 && !m_Theme.clrBand1 && !m_Theme.clrBand2)
			Erase = FALSE;

		if (Erase)
		{
			CRect rcReBar = GetClientRect();
			int BarWidth = rcReBar.Width();
			int BarHeight = rcReBar.Height();

			// Create and set up our memory DC
			CMemDC MemDC(pDC);
			MemDC.CreateCompatibleBitmap(pDC, BarWidth, BarHeight);

			// Draw to ReBar background to the memory DC
			rcReBar.right = 600;
			MemDC.GradientFill(m_Theme.clrBkgnd1, m_Theme.clrBkgnd2, rcReBar, TRUE);
			if (BarWidth >= 600)
			{
				rcReBar.left = 600;
				rcReBar.right = BarWidth;
				MemDC.SolidFill(m_Theme.clrBkgnd2, rcReBar);
			}

			if (m_Theme.clrBand1 || m_Theme.clrBand2)
			{
				// Draw the individual band backgrounds
				for (int nBand = 0 ; nBand < GetBandCount(); ++nBand)
				{
					if (IsBandVisible(nBand))
					{
						if (nBand != GetBand(m_hMenuBar))
						{
							// Determine the size of this band
							CRect rcBand = GetBandRect(nBand);

							// Determine the size of the child window
							REBARBANDINFO rbbi = {0};
							rbbi.cbSize = GetSizeofRBBI();
							rbbi.fMask = RBBIM_CHILD ;
							GetBandInfo(nBand, rbbi);
							CRect rcChild;
							::GetWindowRect(rbbi.hwndChild, &rcChild);
							int ChildWidth = rcChild.right - rcChild.left;

							// Determine our drawing rectangle
							CRect rcDraw = rcBand;
							rcDraw.bottom = rcDraw.top + (rcBand.bottom - rcBand.top)/2;
							int xPad = IsXPThemed()? 2: 0;
							rcDraw.left -= xPad;

							// Fill the Source CDC with the band's background
							CMemDC SourceDC(pDC);
							SourceDC.CreateCompatibleBitmap(pDC, BarWidth, BarHeight);
							CRect rcBorder = GetBandBorders(nBand);
							rcDraw.right = rcBand.left + ChildWidth + rcBorder.left;
							SourceDC.SolidFill(m_Theme.clrBand1, rcDraw);
							rcDraw.top = rcDraw.bottom;
							rcDraw.bottom = rcBand.bottom;
							SourceDC.GradientFill(m_Theme.clrBand1, m_Theme.clrBand2, rcDraw, FALSE);

							// Set Curve amount for rounded edges
							int Curve = m_Theme.RoundBorders? 12 : 0;

							// Create our mask for rounded edges using RoundRect
							CMemDC MaskDC(pDC);
							MaskDC.CreateCompatibleBitmap(pDC, BarWidth, BarHeight);

							rcDraw.top = rcBand.top;
							if (!m_Theme.FlatStyle)
								::InflateRect(&rcDraw, 1, 1);

							int left = rcDraw.left;
							int right = rcDraw.right;
							int top = rcDraw.top;
							int bottom = rcDraw.bottom;
							int cx = rcDraw.right - rcBand.left + xPad;
							int cy = rcDraw.bottom - rcBand.top;

							if (m_Theme.FlatStyle)
							{
								MaskDC.SolidFill(RGB(0,0,0), rcDraw);
								MaskDC.BitBlt(left, top, cx, cy, &MaskDC, left, top, PATINVERT);
								MaskDC.RoundRect(left, top, right, bottom, Curve, Curve);
							}
							else
							{
								MaskDC.SolidFill(RGB(0,0,0), rcDraw);
								MaskDC.RoundRect(left, top, right, bottom, Curve, Curve);
								MaskDC.BitBlt(left, top, cx, cy, &MaskDC, left, top, PATINVERT);
							}

							// Copy Source DC to Memory DC using the RoundRect mask
							MemDC.BitBlt(left, top, cx, cy, &SourceDC, left, top, SRCINVERT);
							MemDC.BitBlt(left, top, cx, cy, &MaskDC,   left, top, SRCAND);
							MemDC.BitBlt(left, top, cx, cy, &SourceDC, left, top, SRCINVERT);

							// Extra drawing to prevent jagged edge while moving bands
							if (m_bIsDragging)
							{
								CClientDC ReBarDC(this);
								ReBarDC.BitBlt(rcDraw.right - ChildWidth, rcDraw.top, ChildWidth, cy, &MemDC, rcDraw.right - ChildWidth, rcDraw.top, SRCCOPY);
							}
						}
					}
				}
			}

			if (m_Theme.UseLines)
			{
				// Draw lines between bands
				for (int j = 0; j < GetBandCount()-1; ++j)
				{
					rcReBar = GetBandRect(j);
					rcReBar.left = MAX(0, rcReBar.left - 4);
					rcReBar.bottom +=2;
					MemDC.DrawEdge(rcReBar, EDGE_ETCHED, BF_BOTTOM | BF_ADJUST);
				}
			}

			// Copy the Memory DC to the window's DC
			pDC->BitBlt(0, 0, BarWidth, BarHeight, &MemDC, 0, 0, SRCCOPY);
		}
		
		return Erase;
	}

	inline void CReBar::PreCreate(CREATESTRUCT &cs)
	// Sets the CREATESTRUCT paramaters prior to window creation
	{
		cs.style = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                         CCS_NODIVIDER | RBS_VARHEIGHT | RBS_BANDBORDERS ;

		cs.cy = 100;
	}

	inline void CReBar::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  REBARCLASSNAME;
	}

	inline void CReBar::MaximizeBand(UINT uBand, BOOL fIdeal /*= FALSE*/)
	// Resizes a band in a rebar control to either its ideal or largest size.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(RB_MAXIMIZEBAND, (WPARAM)uBand, (LPARAM)fIdeal);
	}

	inline void CReBar::MinimizeBand(UINT uBand)
	// Resizes a band in a rebar control to its smallest size.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(RB_MINIMIZEBAND, (WPARAM)uBand, 0L);
	}

	inline BOOL CReBar::MoveBand(UINT uFrom, UINT uTo)
	// Moves a band from one index to another.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(RB_MOVEBAND, (WPARAM)uFrom, (LPARAM)uTo);
	}

	inline void CReBar::MoveBandsLeft()
	// Repositions the bands so they are left justified
	{
		assert(::IsWindow(m_hWnd));

		int OldrcTop = -1;
		for (int nBand = GetBandCount() -1; nBand >= 0; --nBand)
		{
			CRect rc = GetBandRect(nBand);
			if (rc.top != OldrcTop)
			{
				// Maximize the last band on each row
				if (IsBandVisible(nBand))
				{
					::SendMessage(GetHwnd(), RB_MAXIMIZEBAND, nBand, 0L);
					OldrcTop = rc.top;
				}
			}
		}
	}

	inline BOOL CReBar::ResizeBand(int nBand, const CSize& sz) const
	// Sets a band's size
	{
		assert(::IsWindow(m_hWnd));

		REBARBANDINFO rbbi = {0};
		rbbi.cbSize = GetSizeofRBBI();
		rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_SIZE;

		GetBandInfo(nBand, rbbi);
		rbbi.cx         = sz.cx + 2;
		rbbi.cxMinChild = sz.cx + 2;
		rbbi.cyMinChild = sz.cy;
		rbbi.cyMaxChild = sz.cy;

		return SetBandInfo(nBand, rbbi );
	}

	inline BOOL CReBar::SetBandBitmap(int nBand, const CBitmap* pBackground) const
	// Sets the band's bitmaps
	{
		assert(::IsWindow(m_hWnd));
		assert(pBackground);

		REBARBANDINFO rbbi = {0};
		rbbi.cbSize = GetSizeofRBBI();
		rbbi.fMask  = RBBIM_STYLE;
		GetBandInfo(nBand, rbbi);
		rbbi.fMask  |= RBBIM_BACKGROUND;
		rbbi.hbmBack = *pBackground;

		return (BOOL)SendMessage(RB_SETBANDINFO, nBand, (LPARAM)&rbbi);
	}

	inline BOOL CReBar::SetBandColor(int nBand, COLORREF clrFore, COLORREF clrBack) const
	// Sets the band's color
	// Note:	No effect with XP themes enabled
	//			No effect if a bitmap has been set
	{
		assert(::IsWindow(m_hWnd));

		REBARBANDINFO rbbi = {0};
		rbbi.cbSize = GetSizeofRBBI();
		rbbi.fMask = RBBIM_COLORS;
		rbbi.clrFore = clrFore;
		rbbi.clrBack = clrBack;

		return (BOOL)SendMessage(RB_SETBANDINFO, nBand, (LPARAM)&rbbi);
	}

	inline BOOL CReBar::SetBandInfo(int nBand, REBARBANDINFO& rbbi) const
	// Sets the characteristics of a rebar control.
	{
		assert(::IsWindow(m_hWnd));
		assert(nBand >= 0);

		// REBARBANDINFO describes individual BAND characteristics0
		rbbi.cbSize = GetSizeofRBBI();
		return (BOOL)SendMessage(RB_SETBANDINFO, nBand, (LPARAM)&rbbi);
	}

	inline BOOL CReBar::SetBarInfo(REBARINFO& rbi) const
	// REBARINFO associates an image list with the rebar
	// A band will also need to set RBBIM_IMAGE
	{
		assert(::IsWindow(m_hWnd));

		rbi.cbSize = GetSizeofRBBI();
		return (BOOL)SendMessage(RB_SETBARINFO, 0L, (LPARAM)&rbi);
	}

	inline void CReBar::SetReBarTheme(ReBarTheme& Theme)
	{
		m_Theme.UseThemes    = Theme.UseThemes;
		m_Theme.clrBkgnd1    = Theme.clrBkgnd1;
		m_Theme.clrBkgnd2    = Theme.clrBkgnd2;
		m_Theme.clrBand1     = Theme.clrBand1;
		m_Theme.clrBand2     = Theme.clrBand2;
		m_Theme.BandsLeft    = Theme.BandsLeft;
		m_Theme.LockMenuBand = Theme.LockMenuBand;
		m_Theme.ShortBands   = Theme.ShortBands;
		m_Theme.UseLines     = Theme.UseLines;
		m_Theme.FlatStyle    = Theme.FlatStyle;
		m_Theme.RoundBorders = Theme.RoundBorders;

		if (IsWindow())
		{
			if (m_Theme.LockMenuBand)
				ShowGripper(GetBand(m_hMenuBar), FALSE);
			else
				ShowGripper(GetBand(m_hMenuBar), TRUE);
		
			Invalidate();
		}
	}

	inline BOOL CReBar::ShowBand(int nBand, BOOL fShow) const
	// Show or hide a band
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(RB_SHOWBAND, (WPARAM)nBand, (LPARAM)fShow);
	}

	inline BOOL CReBar::ShowGripper(int nBand, BOOL fShow) const
	// Show or hide the band's gripper
	{
		assert(::IsWindow(m_hWnd));

		REBARBANDINFO rbbi = {0};
		rbbi.cbSize = GetSizeofRBBI();
		rbbi.fMask = RBBIM_STYLE;
		GetBandInfo(nBand, rbbi);
		if (fShow)
		{
			rbbi.fStyle |= RBBS_GRIPPERALWAYS;
			rbbi.fStyle &= ~RBBS_NOGRIPPER;
		}
		else
		{
			rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
			rbbi.fStyle |= RBBS_NOGRIPPER;
		}

		return SetBandInfo(nBand, rbbi);
	}

	inline BOOL CReBar::SizeToRect(CRect& rect) const
	// Attempts to find the best layout of the bands for the given rectangle.
	// The rebar bands will be arranged and wrapped as necessary to fit the rectangle.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(RB_SIZETORECT, 0, (LPARAM) (LPRECT)rect);
	}

	inline LRESULT CReBar::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
			if (m_Theme.UseThemes && m_Theme.LockMenuBand)
			{
				// We want to lock the first row in place, but allow other bands to move!
				// Use move messages to limit the resizing of bands
				int y = GET_Y_LPARAM(lParam);

				if (y <= GetRowHeight(0))
					return 0L;	// throw this message away
			}
			break;
		case WM_LBUTTONDOWN:
			m_Orig_lParam = lParam;	// Store the x,y position
			m_bIsDragging = TRUE;
			break;
		case WM_LBUTTONUP:
			if (m_Theme.UseThemes && m_Theme.LockMenuBand)
			{
				// Use move messages to limit the resizing of bands
				int y = GET_Y_LPARAM(lParam);

				if (y <= GetRowHeight(0))
				{
					// Use x,y from WM_LBUTTONDOWN for WM_LBUTTONUP position
					lParam = m_Orig_lParam;
				}
			}
			m_bIsDragging = FALSE;
			break;
		case UWM_GETREBARTHEME:
			{
				ReBarTheme& rm = GetReBarTheme();
				return (LRESULT)&rm;
			}
		case UWM_TOOLBAR_RESIZE:
			{
				HWND hToolBar = (HWND)wParam;
				LPSIZE pToolBarSize = (LPSIZE)lParam;
				ResizeBand(GetBand(hToolBar), *pToolBarSize);
			}
			break;
		}

		// pass unhandled messages on for default processing
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}

} // namespace Win32xx

#endif // #ifndef _WIN32XX_REBAR_H_
