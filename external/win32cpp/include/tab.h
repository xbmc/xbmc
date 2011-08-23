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
// tab.h
//  Declaration of the CTab and CMDITab classes

#ifndef _WIN32XX_TAB_H_
#define _WIN32XX_TAB_H_

#include "wincore.h"
#include "dialog.h"
#include "gdi.h"
#include "default_resource.h"

namespace Win32xx
{

	struct TabPageInfo
	{
		TCHAR szTabText[MAX_MENU_STRING];
		int iImage;			// index of this tab's image
		int idTab;			// identifier for this tab (optional)
		CWnd* pView;		// pointer to the view window
	};

	class CTab : public CWnd
	{
	protected:
		// Declaration of the CSelectDialog class, a nested class of CTab
		// It creates the dialog to choose which tab to activate
		class CSelectDialog : public CDialog
		{
		public:
			CSelectDialog(LPCDLGTEMPLATE lpTemplate, CWnd* pParent = NULL);
			virtual ~CSelectDialog() {}
			virtual void AddItem(LPCTSTR szString);
			virtual BOOL IsTab() const { return FALSE; }

		protected:
			virtual BOOL OnInitDialog();
			virtual void OnOK();
			virtual void OnCancel() { EndDialog(-2); }

		private:
			CSelectDialog(const CSelectDialog&);				// Disable copy construction
			CSelectDialog& operator = (const CSelectDialog&); // Disable assignment operator

			std::vector<tString> m_vItems;
			int IDC_LIST;

		};
	public:
		CTab();
		virtual ~CTab();
		virtual int  AddTabPage(WndPtr pView, LPCTSTR szTabText, HICON hIcon, UINT idTab);
		virtual int  AddTabPage(WndPtr pView, LPCTSTR szTabText, int nID_Icon, UINT idTab = 0);
		virtual int  AddTabPage(WndPtr pView, LPCTSTR szTabText);
		virtual CRect GetCloseRect() const;
		virtual CRect GetListRect() const;
		virtual HMENU GetListMenu();
		virtual BOOL GetTabsAtTop() const;
		virtual int  GetTabIndex(CWnd* pWnd) const;
		virtual TabPageInfo GetTabPageInfo(UINT nTab) const;
		virtual int GetTextHeight() const;
		virtual void RecalcLayout();
		virtual void RemoveTabPage(int nPage);
		virtual void SelectPage(int nPage);
		virtual void SetFixedWidth(BOOL bEnabled);
		virtual void SetOwnerDraw(BOOL bEnabled);
		virtual void SetShowButtons(BOOL bShow);
		virtual void SetTabIcon(int i, HICON hIcon);
		virtual void SetTabsAtTop(BOOL bTop);
		virtual void SetTabText(UINT nTab, LPCTSTR szText);
		virtual void SwapTabs(UINT nTab1, UINT nTab2);

		// Attributes
		std::vector <TabPageInfo>& GetAllTabs() const { return (std::vector <TabPageInfo>&) m_vTabPageInfo; }
		HIMAGELIST GetImageList() const { return m_himlTab; }
		BOOL GetShowButtons() const { return m_bShowButtons; }
		int GetTabHeight() const { return m_nTabHeight; }
		CWnd* GetActiveView() const		{ return m_pActiveView; }
		void SetTabHeight(int nTabHeight) { m_nTabHeight = nTabHeight; NotifyChanged();}

		// Wrappers for Win32 Macros
		void AdjustRect(BOOL fLarger, RECT *prc) const;
		int  GetCurFocus() const;
		int  GetCurSel() const;
		BOOL GetItem(int iItem, LPTCITEM pitem) const;
		int  GetItemCount() const;
		int  HitTest(TCHITTESTINFO& info) const;
		void SetCurFocus(int iItem) const;
		int  SetCurSel(int iItem) const;
		DWORD SetItemSize(int cx, int cy) const;
		int  SetMinTabWidth(int cx) const;
		void SetPadding(int cx, int cy) const;

	protected:
		virtual void	DrawCloseButton(CDC& DrawDC);
		virtual void	DrawListButton(CDC& DrawDC);
		virtual void	DrawTabs(CDC& dcMem);
		virtual void	DrawTabBorders(CDC& dcMem, CRect& rcTab);
		virtual void    OnCreate();
		virtual void    OnLButtonDown(WPARAM wParam, LPARAM lParam);
		virtual void    OnLButtonUp(WPARAM wParam, LPARAM lParam);
		virtual void    OnMouseLeave(WPARAM wParam, LPARAM lParam);
		virtual void    OnMouseMove(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNCHitTest(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotifyReflect(WPARAM wParam, LPARAM lParam);
		virtual void	NotifyChanged();
		virtual void	Paint();
		virtual void    PreCreate(CREATESTRUCT& cs);
		virtual void	PreRegisterClass(WNDCLASS &wc);
		virtual void    SetTabSize();
		virtual void	ShowListDialog();
		virtual void	ShowListMenu();
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CTab(const CTab&);				// Disable copy construction
		CTab& operator = (const CTab&); // Disable assignment operator

		SIZE  GetMaxTabSize() const;
		void ShowActiveView(CWnd* pView);

		std::vector<TabPageInfo> m_vTabPageInfo;
		std::vector<WndPtr> m_vTabViews;
		CFont m_Font;
		HIMAGELIST m_himlTab;
		HMENU m_hListMenu;
		CWnd* m_pActiveView;
		BOOL m_bShowButtons;	// Show or hide the close and list button
		BOOL m_IsTracking;
		BOOL m_IsClosePressed;
		BOOL m_IsListPressed;
		BOOL m_IsListMenuActive;
		int m_nTabHeight;
	};

	////////////////////////////////////////
	// Declaration of the CTabbedMDI class
	class CTabbedMDI : public CWnd
	{
	public:
		CTabbedMDI();
		virtual ~CTabbedMDI();
		virtual CWnd* AddMDIChild(CWnd* pView, LPCTSTR szTabText, int idMDIChild = 0);
		virtual void  CloseActiveMDI();
		virtual void  CloseAllMDIChildren();
		virtual void  CloseMDIChild(int nTab);
		virtual CWnd* GetActiveMDIChild() const;
		virtual int	  GetActiveMDITab() const;
		virtual CWnd* GetMDIChild(int nTab) const;
		virtual int   GetMDIChildCount() const;
		virtual int   GetMDIChildID(int nTab) const;
		virtual LPCTSTR GetMDIChildTitle(int nTab) const;
		virtual HMENU GetListMenu() const { return GetTab().GetListMenu(); }
		virtual CTab& GetTab() const	{return (CTab&)m_Tab;}
		virtual BOOL LoadRegistrySettings(tString tsRegistryKeyName);
		virtual void RecalcLayout();
		virtual BOOL SaveRegistrySettings(tString tsRegistryKeyName);
		virtual void SetActiveMDIChild(CWnd* pWnd);
		virtual void SetActiveMDITab(int nTab);

	protected:
		virtual HWND    Create(CWnd* pParent);
		virtual CWnd*   NewMDIChildFromID(int idMDIChild);
		virtual void	OnCreate();
		virtual void    OnDestroy(WPARAM wParam, LPARAM lParam);
		virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
		virtual void    OnWindowPosChanged(WPARAM wParam, LPARAM lParam);
		virtual LRESULT WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		CTabbedMDI(const CTabbedMDI&);				// Disable copy construction
		CTabbedMDI& operator = (const CTabbedMDI&); // Disable assignment operator

		CTab m_Tab;
	};

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	/////////////////////////////////////////////////////////////
	// Definitions for the CSelectDialog class nested within CTab
	//
	inline CTab::CSelectDialog::CSelectDialog(LPCDLGTEMPLATE lpTemplate, CWnd* pParent) :
					CDialog(lpTemplate, pParent), IDC_LIST(121)
	{
	}

	inline BOOL CTab::CSelectDialog::OnInitDialog()
	{
		for (UINT u = 0; u < m_vItems.size(); ++u)
		{
			SendDlgItemMessage(IDC_LIST, LB_ADDSTRING, 0, (LPARAM) m_vItems[u].c_str());
		}

		return true;
	}

	inline void CTab::CSelectDialog::AddItem(LPCTSTR szString)
	{
		m_vItems.push_back(szString);
	}

	inline void CTab::CSelectDialog::OnOK()
	{
		int iSelect = (int)SendDlgItemMessage(IDC_LIST, LB_GETCURSEL, 0, 0);
		if (iSelect != LB_ERR) 
			EndDialog(iSelect);
		else
			EndDialog(-2);
	}


	//////////////////////////////////////////////////////////
	// Definitions for the CTab class
	//
	inline CTab::CTab() : m_hListMenu(NULL), m_pActiveView(NULL), m_bShowButtons(FALSE), m_IsTracking(FALSE), m_IsClosePressed(FALSE),
							m_IsListPressed(FALSE), m_IsListMenuActive(FALSE), m_nTabHeight(0)
	{
		// Create and assign the image list
		m_himlTab = ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 0, 0);

		// Set the tab control's font
		NONCLIENTMETRICS info = {0};
		info.cbSize = GetSizeofNonClientMetrics();
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);
		m_Font.CreateFontIndirect(&info.lfStatusFont);
	}

	inline CTab::~CTab()
	{
		ImageList_Destroy(m_himlTab);
		
		if (IsMenu(m_hListMenu)) ::DestroyMenu(m_hListMenu);
	}

	inline int CTab::AddTabPage(WndPtr pView, LPCTSTR szTabText, HICON hIcon, UINT idTab)
	{
		assert(pView.get());
		assert(lstrlen(szTabText) < MAX_MENU_STRING);

		m_vTabViews.push_back(pView);

		TabPageInfo tpi = {0};
		tpi.pView = pView.get();
		tpi.idTab = idTab;
		lstrcpyn(tpi.szTabText, szTabText, MAX_MENU_STRING);
		if (hIcon)
			tpi.iImage = ImageList_AddIcon(GetImageList(), hIcon);
		else
			tpi.iImage = -1;

		int iNewPage = (int)m_vTabPageInfo.size();
		m_vTabPageInfo.push_back(tpi);

		if (m_hWnd)
		{
			TCITEM tie = {0};
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = tpi.iImage;
			tie.pszText = tpi.szTabText;
			TabCtrl_InsertItem(m_hWnd, iNewPage, &tie);

			SetTabSize();
			SelectPage(iNewPage);
			NotifyChanged();
		}

		return iNewPage;
	}

	inline int CTab::AddTabPage(WndPtr pView, LPCTSTR szTabText, int idIcon, UINT idTab /* = 0*/)
	{
		HICON hIcon = (HICON)LoadImage(GetApp()->GetResourceHandle(), MAKEINTRESOURCE(idIcon), IMAGE_ICON, 0, 0, LR_SHARED);
		return AddTabPage(pView, szTabText, hIcon, idTab);
	}

	inline int CTab::AddTabPage(WndPtr pView, LPCTSTR szTabText)
	{
		return AddTabPage(pView, szTabText, (HICON)0, 0);
	}

	inline void CTab::DrawCloseButton(CDC& DrawDC)
	{
		// The close button isn't displayed on Win95
		if (GetWinVersion() == 1400)  return;

		if (!m_bShowButtons) return;
		if (!GetActiveView()) return;
		if (!(GetWindowLongPtr(GWL_STYLE) & TCS_FIXEDWIDTH)) return;
		if (!(GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED)) return;

		// Determine the close button's drawing position relative to the window
		CRect rcClose = GetCloseRect();

		CPoint pt = GetCursorPos();
		ScreenToClient(pt);
		UINT uState = rcClose.PtInRect(pt)? m_IsClosePressed? 2: 1: 0;

		// Draw the outer highlight for the close button
		if (!IsRectEmpty(&rcClose))
		{
			switch (uState)
			{
			case 0:
				{
					DrawDC.CreatePen(PS_SOLID, 1, RGB(232, 228, 220));

					DrawDC.MoveTo(rcClose.left, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.top);
					DrawDC.LineTo(rcClose.left, rcClose.top);
					DrawDC.LineTo(rcClose.left, rcClose.bottom);
					break;
				}

			case 1:
				{
					// Draw outline, white at top, black on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.MoveTo(rcClose.left, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.LineTo(rcClose.left, rcClose.top);
					DrawDC.LineTo(rcClose.left, rcClose.bottom);
				}

				break;
			case 2:
				{
					// Draw outline, black on top, white on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.MoveTo(rcClose.left, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.bottom);
					DrawDC.LineTo(rcClose.right, rcClose.top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.LineTo(rcClose.left, rcClose.top);
					DrawDC.LineTo(rcClose.left, rcClose.bottom);
				}
				break;
			}

			// Manually draw close button
			DrawDC.CreatePen(PS_SOLID, 1, RGB(64, 64, 64));

			DrawDC.MoveTo(rcClose.left + 3, rcClose.top +3);
			DrawDC.LineTo(rcClose.right - 2, rcClose.bottom -2);

			DrawDC.MoveTo(rcClose.left + 4, rcClose.top +3);
			DrawDC.LineTo(rcClose.right - 2, rcClose.bottom -3);

			DrawDC.MoveTo(rcClose.left + 3, rcClose.top +4);
			DrawDC.LineTo(rcClose.right - 3, rcClose.bottom -2);

			DrawDC.MoveTo(rcClose.right -3, rcClose.top +3);
			DrawDC.LineTo(rcClose.left + 2, rcClose.bottom -2);

			DrawDC.MoveTo(rcClose.right -3, rcClose.top +4);
			DrawDC.LineTo(rcClose.left + 3, rcClose.bottom -2);

			DrawDC.MoveTo(rcClose.right -4, rcClose.top +3);
			DrawDC.LineTo(rcClose.left + 2, rcClose.bottom -3);
		}
	}

	inline void CTab::DrawListButton(CDC& DrawDC)
	{
		// The list button isn't displayed on Win95
		if (GetWinVersion() == 1400)  return;

		if (!m_bShowButtons) return;
		if (!GetActiveView()) return;
		if (!(GetWindowLongPtr(GWL_STYLE) & TCS_FIXEDWIDTH)) return;
		if (!(GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED)) return;

		// Determine the list button's drawing position relative to the window
		CRect rcList = GetListRect();

		CPoint pt = GetCursorPos();
		ScreenToClient(pt);
		UINT uState = rcList.PtInRect(pt)? 1: 0;
		if (m_IsListMenuActive) uState = 2;

		// Draw the outer highlight for the list button
		if (!IsRectEmpty(&rcList))
		{
			switch (uState)
			{
			case 0:
				{
					DrawDC.CreatePen(PS_SOLID, 1, RGB(232, 228, 220));

					DrawDC.MoveTo(rcList.left, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.top);
					DrawDC.LineTo(rcList.left, rcList.top);
					DrawDC.LineTo(rcList.left, rcList.bottom);
					break;
				}

			case 1:
				{
					// Draw outline, white at top, black on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.MoveTo(rcList.left, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.LineTo(rcList.left, rcList.top);
					DrawDC.LineTo(rcList.left, rcList.bottom);
				}

				break;
			case 2:
				{
					// Draw outline, black on top, white on bottom
					DrawDC.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
					DrawDC.MoveTo(rcList.left, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.bottom);
					DrawDC.LineTo(rcList.right, rcList.top);
					DrawDC.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
					DrawDC.LineTo(rcList.left, rcList.top);
					DrawDC.LineTo(rcList.left, rcList.bottom);
				}
				break;
			}

			// Manually draw list button
			DrawDC.CreatePen(PS_SOLID, 1, RGB(64, 64, 64));

			int MaxLength = (int)(0.65 * rcList.Width());
			int topGap = 1 + rcList.Height()/3;
			for (int i = 0; i <= MaxLength/2; i++)
			{
				int Length = MaxLength - 2*i;
				DrawDC.MoveTo(rcList.left +1 + (rcList.Width() - Length)/2, rcList.top +topGap +i);
				DrawDC.LineTo(rcList.left +1 + (rcList.Width() - Length)/2 + Length, rcList.top +topGap +i);
			}
		}
	}

	inline void CTab::DrawTabs(CDC& dcMem)
	{
		// Draw the tab buttons:
		for (int i = 0; i < TabCtrl_GetItemCount(m_hWnd); ++i)
		{
			CRect rcItem;
			TabCtrl_GetItemRect(m_hWnd, i, &rcItem);
			if (!rcItem.IsRectEmpty())
			{
				if (i == TabCtrl_GetCurSel(m_hWnd))
				{
					dcMem.CreateSolidBrush(RGB(248,248,248));
					dcMem.SetBkColor(RGB(248,248,248));
				}
				else
				{
					dcMem.CreateSolidBrush(RGB(200,200,200));
					dcMem.SetBkColor(RGB(200,200,200));
				}

				dcMem.CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
				dcMem.RoundRect(rcItem.left+1, rcItem.top, rcItem.right+2, rcItem.bottom, 6, 6);

				if (rcItem.Width() >= 24)
				{
					TCHAR szText[30];
					TCITEM tcItem = {0};
					tcItem.mask = TCIF_TEXT | TCIF_IMAGE;
					tcItem.cchTextMax = 30;
					tcItem.pszText = szText;
					TabCtrl_GetItem(m_hWnd, i, &tcItem);
					int xImage;
					int yImage;
					int yOffset = 0;
					if (ImageList_GetIconSize(m_himlTab, &xImage, &yImage))
						yOffset = (rcItem.Height() - yImage)/2;

					// Draw the icon
					ImageList_Draw(m_himlTab, tcItem.iImage, dcMem, rcItem.left+5, rcItem.top+yOffset, ILD_NORMAL);

					// Draw the text
					dcMem.SelectObject(&m_Font);

					// Calculate the size of the text
					CRect rcText = rcItem;

					int iImageSize = 20;
					int iPadding = 4;
					if (tcItem.iImage >= 0)
						rcText.left += iImageSize;

					rcText.left += iPadding;
					dcMem.DrawText(szText, -1, rcText, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
				}
			}
		}
	}

	inline void CTab::DrawTabBorders(CDC& dcMem, CRect& rcTab)
	{
		BOOL IsBottomTab = (BOOL)GetWindowLongPtr(GWL_STYLE) & TCS_BOTTOM;

		// Draw a lighter rectangle touching the tab buttons
		CRect rcItem;
		TabCtrl_GetItemRect(m_hWnd, 0, &rcItem);
		int left = rcItem.left +1;
		int right = rcTab.right;
		int top = rcTab.bottom;
		int bottom = top + 3;

		if (!IsBottomTab)
		{
			bottom = MAX(rcTab.top, m_nTabHeight +4);
			top = bottom -3;
		}

		dcMem.CreateSolidBrush(RGB(248,248,248));
		dcMem.CreatePen(PS_SOLID, 1, RGB(248,248,248));
		if (!rcItem.IsRectEmpty())
		{
			dcMem.Rectangle(left, top, right, bottom);

			// Draw a darker line below the rectangle
			dcMem.CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
			if (IsBottomTab)
			{
				dcMem.MoveTo(left-1, bottom);
				dcMem.LineTo(right, bottom);
			}
			else
			{
				dcMem.MoveTo(left-1, top-1);
				dcMem.LineTo(right, top-1);
			}

			// Draw a lighter line over the darker line for the selected tab
			dcMem.CreatePen(PS_SOLID, 1, RGB(248,248,248));
			TabCtrl_GetItemRect(m_hWnd, TabCtrl_GetCurSel(m_hWnd), &rcItem);
			OffsetRect(&rcItem, 1, 1);

			if (IsBottomTab)
			{
				dcMem.MoveTo(rcItem.left, bottom);
				dcMem.LineTo(rcItem.right, bottom);
			}
			else
			{
				dcMem.MoveTo(rcItem.left, top-1);
				dcMem.LineTo(rcItem.right, top-1);
			}
		}
	}

	inline CRect CTab::GetCloseRect() const
	{
		CRect rcClose;
		if (GetShowButtons())
		{
			rcClose= GetClientRect();
			int Gap = 2;
			int cx = GetSystemMetrics(SM_CXSMICON) -1;
			int cy = GetSystemMetrics(SM_CYSMICON) -1;
			rcClose.right -= Gap;
			rcClose.left = rcClose.right - cx;

			if (GetTabsAtTop())
				rcClose.top = Gap;
			else
				rcClose.top = MAX(Gap, rcClose.bottom - m_nTabHeight);

			rcClose.bottom = rcClose.top + cy;
		}
		return rcClose;
	}

	inline HMENU CTab::GetListMenu()
	{
		if (IsMenu(m_hListMenu))
			::DestroyMenu(m_hListMenu);
		
		m_hListMenu = CreatePopupMenu();

		// Add the menu items
		for(UINT u = 0; u < MIN(GetAllTabs().size(), 9); ++u)
		{
			TCHAR szMenuString[MAX_MENU_STRING+1];
			TCHAR szTabText[MAX_MENU_STRING];
			lstrcpyn(szTabText, GetAllTabs()[u].szTabText, MAX_MENU_STRING -4);
			wsprintf(szMenuString, _T("&%d %s"), u+1, szTabText);
			AppendMenu(m_hListMenu, MF_STRING, IDW_FIRSTCHILD +u, szMenuString);
		}
		if (GetAllTabs().size() >= 10)
			AppendMenu(m_hListMenu, MF_STRING, IDW_FIRSTCHILD +9, _T("More Windows"));

		// Add a checkmark to the menu
		int iSelected = GetCurSel();
		if (iSelected < 9)
			CheckMenuItem(m_hListMenu, iSelected, MF_BYPOSITION|MF_CHECKED);

		return m_hListMenu;
	}

	inline CRect CTab::GetListRect() const
	{
		CRect rcList;
		if (GetShowButtons())
		{
			CRect rcClose = GetCloseRect();
			rcList = rcClose;
			rcList.OffsetRect( -(rcClose.Width() + 2), 0);
			rcList.InflateRect(-1, 0);
		}
		return rcList;
	}

	inline SIZE CTab::GetMaxTabSize() const
	{
		CSize Size;

		for (int i = 0; i < TabCtrl_GetItemCount(m_hWnd); i++)
		{
			CClientDC dcClient(this);
			dcClient.SelectObject(&m_Font);
			std::vector<TCHAR> vTitle(MAX_MENU_STRING, _T('\0'));
			TCHAR* pszTitle = &vTitle.front();
			TCITEM tcItem = {0};
			tcItem.mask = TCIF_TEXT |TCIF_IMAGE;
			tcItem.cchTextMax = MAX_MENU_STRING;
			tcItem.pszText = pszTitle;
			TabCtrl_GetItem(m_hWnd, i, &tcItem);
			CSize TempSize = dcClient.GetTextExtentPoint32(pszTitle, lstrlen(pszTitle));

			int iImageSize = 0;
			int iPadding = 6;
			if (tcItem.iImage >= 0)
				iImageSize = 20;
			TempSize.cx += iImageSize + iPadding;

			if (TempSize.cx > Size.cx)
				Size = TempSize;
		}

		return Size;
	}

	inline BOOL CTab::GetTabsAtTop() const
	// Returns TRUE if the contol's tabs are placed at the top
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);
		return (!(dwStyle & TCS_BOTTOM));
	}

	inline int CTab::GetTextHeight() const
	{
		CClientDC dcClient(this);
		dcClient.SelectObject(&m_Font);
		CSize szText = dcClient.GetTextExtentPoint32(_T("Text"), lstrlen(_T("Text")));
		return szText.cy;
	}

	inline int CTab::GetTabIndex(CWnd* pWnd) const
	{
		assert(pWnd);

		for (int i = 0; i < (int)m_vTabPageInfo.size(); ++i)
		{
			if (m_vTabPageInfo[i].pView == pWnd)
				return i;
		}

		return -1;
	}

	inline TabPageInfo CTab::GetTabPageInfo(UINT nTab) const
	{
		assert (nTab < m_vTabPageInfo.size());

		return m_vTabPageInfo[nTab];
	}

	inline void CTab::NotifyChanged()
	{
		NMHDR nmhdr = {0};
		nmhdr.hwndFrom = m_hWnd;
		nmhdr.code = UWM_TAB_CHANGED;
		GetParent()->SendMessage(WM_NOTIFY, 0L, (LPARAM)&nmhdr);
	}

	inline void CTab::OnCreate()
	{
		SetFont(&m_Font, TRUE);
		
		// Assign ImageList unless we are owner drawn
		if (!(GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED))
			TabCtrl_SetImageList(m_hWnd, m_himlTab);

		for (int i = 0; i < (int)m_vTabPageInfo.size(); ++i)
		{
			// Add tabs for each view.
			TCITEM tie = {0};
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = m_vTabPageInfo[i].iImage;
			tie.pszText = m_vTabPageInfo[i].szTabText;
			TabCtrl_InsertItem(m_hWnd, i, &tie);
		}

		int HeightGap = 5;
		SetTabHeight(MAX(20, (GetTextHeight() + HeightGap)));
		SelectPage(0);
	}

	inline void CTab::OnLButtonDown(WPARAM /*wParam*/, LPARAM lParam)
	{
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		if (GetCloseRect().PtInRect(pt))
		{
			m_IsClosePressed = TRUE;
			SetCapture();
			CClientDC dc(this);
			DrawCloseButton(dc);
		}
		else
			m_IsClosePressed = FALSE;

		if (GetListRect().PtInRect(pt))
		{
			ShowListMenu();
		}
	}

	inline void CTab::OnLButtonUp(WPARAM /*wParam*/, LPARAM lParam)
	{
		ReleaseCapture();
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		if (m_IsClosePressed && GetCloseRect().PtInRect(pt))
		{
			RemoveTabPage(GetCurSel());
			if (GetActiveView())
				GetActiveView()->RedrawWindow();
		}

		m_IsClosePressed = FALSE;
	}

	inline void CTab::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		CClientDC dc(this);
		DrawCloseButton(dc);
		DrawListButton(dc);

		m_IsTracking = FALSE;
	}

	inline void CTab::OnMouseMove(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		if (!m_IsListMenuActive && m_IsListPressed)
		{
			m_IsListPressed = FALSE;
		}

		if (!m_IsTracking)
		{
			TRACKMOUSEEVENT TrackMouseEventStruct = {0};
			TrackMouseEventStruct.cbSize = sizeof(TrackMouseEventStruct);
			TrackMouseEventStruct.dwFlags = TME_LEAVE;
			TrackMouseEventStruct.hwndTrack = m_hWnd;
			_TrackMouseEvent(&TrackMouseEventStruct);
			m_IsTracking = TRUE;
		}

		CClientDC dc(this);
		DrawCloseButton(dc);
		DrawListButton(dc);
	}

	inline LRESULT CTab::OnNCHitTest(WPARAM wParam, LPARAM lParam)
	{
		// Ensure we have an arrow cursor when the tab has no view window
		if (0 == GetAllTabs().size())
			SetCursor(LoadCursor(NULL, IDC_ARROW));

		// Cause WM_LBUTTONUP and WM_LBUTTONDOWN messages to be sent for buttons
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		ScreenToClient(pt);
		if (GetCloseRect().PtInRect(pt)) return HTCLIENT;
		if (GetListRect().PtInRect(pt))  return HTCLIENT;

		return CWnd::WndProcDefault(WM_NCHITTEST, wParam, lParam);
	}

	inline LRESULT CTab::OnNotifyReflect(WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(wParam);

		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
			{
				// Display the newly selected tab page
				int nPage = GetCurSel();
				ShowActiveView(m_vTabPageInfo[nPage].pView);
			}
			break;
		}

		return 0L;
	}

	inline void CTab::Paint()
	{
		// Microsoft's drawing for a tab control is rubbish, so we do our own.
		// We use double buffering and regions to eliminate flicker

		// Create the memory DC and bitmap
		CClientDC dcView(this);
		CMemDC dcMem(&dcView);
		CRect rcClient = GetClientRect();
		dcMem.CreateCompatibleBitmap(&dcView, rcClient.Width(), rcClient.Height());

		if (0 == GetItemCount())
		{
			// No tabs, so simply display a grey background and exit
			COLORREF rgbDialog = GetSysColor(COLOR_BTNFACE);
			dcView.SolidFill(rgbDialog, rcClient);
			return;
		}

		// Create a clipping region. Its the overall tab window's region,
		//  less the region belonging to the individual tab view's client area
		CRgn rgnSrc1 = ::CreateRectRgn(rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
		CRect rcTab = GetClientRect();
		TabCtrl_AdjustRect(m_hWnd, FALSE, &rcTab);
		if (rcTab.Height() < 0)
			rcTab.top = rcTab.bottom;
		if (rcTab.Width() < 0)
			rcTab.left = rcTab.right;

		CRgn rgnSrc2 = ::CreateRectRgn(rcTab.left, rcTab.top, rcTab.right, rcTab.bottom);
		CRgn rgnClip = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(rgnClip, rgnSrc1, rgnSrc2, RGN_DIFF);

		// Use the region in the memory DC to paint the grey background
		dcMem.SelectClipRgn(&rgnClip);
		HWND hWndParent = ::GetParent(m_hWnd);
		CDC dcParent = ::GetDC(hWndParent);
		HBRUSH hBrush = (HBRUSH) SendMessage(hWndParent, WM_CTLCOLORDLG, (WPARAM)dcParent.GetHDC(), (LPARAM)hWndParent);
		dcMem.SelectObject(FromHandle(hBrush));
		dcMem.PaintRgn(&rgnClip);

		// Draw the tab buttons on the memory DC:
		DrawTabs(dcMem);

		// Draw buttons and tab borders
		DrawCloseButton(dcMem);
		DrawListButton(dcMem);
		DrawTabBorders(dcMem, rcTab);

		// Now copy our from our memory DC to the window DC
		dcView.SelectClipRgn(&rgnClip);
		dcView.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &dcMem, 0, 0, SRCCOPY);
	}

	inline void CTab::PreCreate(CREATESTRUCT &cs)
	{
		// For Tabs on the bottom, add the TCS_BOTTOM style
		cs.style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
	}

	inline void CTab::PreRegisterClass(WNDCLASS &wc)
	{
		wc.lpszClassName = WC_TABCONTROL;
	}

	inline void CTab::RecalcLayout()
	{
		if (IsWindow())
		{
			if (GetActiveView())
			{
				// Set the tab sizes
				SetTabSize();

				// Position the View over the tab control's display area
				CRect rc = GetClientRect();
				TabCtrl_AdjustRect(m_hWnd, FALSE, &rc);
				GetActiveView()->SetWindowPos(NULL, rc, SWP_SHOWWINDOW);
			}
			else
				RedrawWindow();
		}
	}

	inline void CTab::RemoveTabPage(int nPage)
	{
		if ((nPage < 0) || (nPage > (int)m_vTabPageInfo.size() -1))
			return;

		// Remove the tab
		TabCtrl_DeleteItem(m_hWnd, nPage);

		// Remove the TapPageInfo entry
		std::vector<TabPageInfo>::iterator itTPI = m_vTabPageInfo.begin() + nPage;
		CWnd* pView = (*itTPI).pView;
		int iImage = (*itTPI).iImage;
		if (iImage >= 0)
			TabCtrl_RemoveImage(m_hWnd, iImage);

		if (pView == m_pActiveView)
			m_pActiveView = 0;

		(*itTPI).pView->Destroy();
		m_vTabPageInfo.erase(itTPI);

		std::vector<WndPtr>::iterator itView;
		for (itView = m_vTabViews.begin(); itView < m_vTabViews.end(); ++itView)
		{
			if ((*itView).get() == pView)
			{
				m_vTabViews.erase(itView);
				break;
			}
		}

		if (IsWindow())
		{
			if (m_vTabPageInfo.size() > 0)
			{
				SetTabSize();
				SelectPage(0);
			}
			else
				ShowActiveView(NULL);

			NotifyChanged();
		}
	}

	inline void CTab::SelectPage(int nPage)
	{
		if ((nPage >= 0) && (nPage < GetItemCount()))
		{
			if (nPage != GetCurSel())
				SetCurSel(nPage);
			
			ShowActiveView(m_vTabPageInfo[nPage].pView);
		}
	}

	inline void CTab::SetFixedWidth(BOOL bEnabled)
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);
		if (bEnabled)
			SetWindowLongPtr(GWL_STYLE, dwStyle | TCS_FIXEDWIDTH);
		else
			SetWindowLongPtr(GWL_STYLE, dwStyle & ~TCS_FIXEDWIDTH);

		RecalcLayout();
	}

	inline void CTab::SetOwnerDraw(BOOL bEnabled)
	// Enable or disable owner draw
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);
		if (bEnabled)
		{
			SetWindowLongPtr(GWL_STYLE, dwStyle | TCS_OWNERDRAWFIXED);
			TabCtrl_SetImageList(m_hWnd, NULL);
		}
		else
		{
			SetWindowLongPtr(GWL_STYLE, dwStyle & ~TCS_OWNERDRAWFIXED);
			TabCtrl_SetImageList(m_hWnd, m_himlTab);
		}

		RecalcLayout();
	}

	inline void CTab::SetShowButtons(BOOL bShow)
	{
		m_bShowButtons = bShow;
		RecalcLayout();
	}

	inline void CTab::SetTabIcon(int i, HICON hIcon)
	// Changes or sets the tab's icon
	{
		assert (GetItemCount() > i);
		TCITEM tci = {0};
		tci.mask = TCIF_IMAGE;
		GetItem(i, &tci);
		if (tci.iImage >= 0)
		{
			ImageList_ReplaceIcon(GetImageList(), i, hIcon);
		}
		else
		{
			int iImage = ImageList_AddIcon(GetImageList(), hIcon);
			tci.iImage = iImage;
			TabCtrl_SetItem(m_hWnd, i, &tci);
			m_vTabPageInfo[i].iImage = iImage;
		}
	}	

	inline void CTab::SetTabsAtTop(BOOL bTop)
	// Positions the tabs at the top or botttom of the control
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);

		if (bTop)
			dwStyle &= ~TCS_BOTTOM;
		else
			dwStyle |= TCS_BOTTOM;

		SetWindowLongPtr(GWL_STYLE, dwStyle);
		RecalcLayout();
	}

	inline void CTab::SetTabSize()
	{
		if (GetItemCount() > 0)
		{
			CRect rc = GetClientRect();
			TabCtrl_AdjustRect(m_hWnd, FALSE, &rc);

			int xGap = 2;
			if (m_bShowButtons) xGap += GetCloseRect().Width() + GetListRect().Width() +2;

			int nItemWidth = MIN( GetMaxTabSize().cx, (rc.Width() - xGap)/GetItemCount() );
			nItemWidth = MAX(nItemWidth, 0);
			SendMessage(TCM_SETITEMSIZE, 0L, MAKELPARAM(nItemWidth, m_nTabHeight));
			NotifyChanged();
		} 
	}

	inline void CTab::SetTabText(UINT nTab, LPCTSTR szText)
	{
		// Allows the text to be changed on an existing tab
		if (nTab < GetAllTabs().size())
		{
			TCITEM Item = {0};
			std::vector<TCHAR> vTChar(MAX_MENU_STRING+1, _T('\0'));
			TCHAR* pTChar = &vTChar.front();
			lstrcpyn(pTChar, szText, MAX_MENU_STRING);
			Item.mask = TCIF_TEXT;
			Item.pszText = pTChar;

			if (TabCtrl_SetItem(m_hWnd, nTab, &Item))
				lstrcpyn(m_vTabPageInfo[nTab].szTabText, pTChar, MAX_MENU_STRING);
		}
	}

	inline void CTab::ShowActiveView(CWnd* pView)
	// Sets or changes the View window displayed within the tab page
	{
		// Hide the old view
		if (GetActiveView() && (GetActiveView()->IsWindow()))
			GetActiveView()->ShowWindow(SW_HIDE);

		// Assign the view window
		m_pActiveView = pView;

		if (m_pActiveView && m_hWnd)
		{
			if (!m_pActiveView->IsWindow())
			{
				// The tab control is already created, so create the new view too
				GetActiveView()->Create(this);
			}
	
			// Position the View over the tab control's display area
			CRect rc = GetClientRect();
			TabCtrl_AdjustRect(m_hWnd, FALSE, &rc);
			GetActiveView()->SetWindowPos(HWND_TOP, rc, SWP_SHOWWINDOW);
			GetActiveView()->SetFocus();
		}
	}

	inline void CTab::ShowListMenu()
	// Displays the list of windows in a popup menu
	{
		if (!m_IsListPressed)
		{
			m_IsListPressed = TRUE;
			HMENU hMenu = GetListMenu();
	
			CPoint pt(GetListRect().left, GetListRect().top + GetTabHeight());
			ClientToScreen(pt);

			// Choosing the frame's hwnd for the menu's messages will automatically theme the popup menu
			HWND MenuHwnd = GetAncestor()->GetHwnd();
			int nPage = 0;
			m_IsListMenuActive = TRUE;
			nPage = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, MenuHwnd, NULL) - IDW_FIRSTCHILD;
			if ((nPage >= 0) && (nPage < 9)) SelectPage(nPage);
			if (nPage == 9) ShowListDialog();
			m_IsListMenuActive = FALSE;
		}

		CClientDC dc(this);
		DrawListButton(dc);
	}

	inline void CTab::ShowListDialog()
	{
		// Definition of a dialog template which displays a List Box
		unsigned char dlg_Template[] =
		{
			0x01,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0xc8,0x00,0xc8,0x90,0x03,
			0x00,0x00,0x00,0x00,0x00,0xdc,0x00,0x8e,0x00,0x00,0x00,0x00,0x00,0x53,0x00,0x65,
			0x00,0x6c,0x00,0x65,0x00,0x63,0x00,0x74,0x00,0x20,0x00,0x57,0x00,0x69,0x00,0x6e,
			0x00,0x64,0x00,0x6f,0x00,0x77,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x01,0x4d,
			0x00,0x53,0x00,0x20,0x00,0x53,0x00,0x68,0x00,0x65,0x00,0x6c,0x00,0x6c,0x00,0x20,
			0x00,0x44,0x00,0x6c,0x00,0x67,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x01,0x00,0x01,0x50,0x40,0x00,0x7a,0x00,0x25,0x00,0x0f,0x00,0x01,
			0x00,0x00,0x00,0xff,0xff,0x80,0x00,0x4f,0x00,0x4b,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x50,0x7a,0x00,0x7a,0x00,0x25,
			0x00,0x0f,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0x80,0x00,0x43,0x00,0x61,0x00,0x6e,
			0x00,0x63,0x00,0x65,0x00,0x6c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x02,0x00,0x01,0x01,0x21,0x50,0x06,0x00,0x06,0x00,0xcf,0x00,0x6d,0x00,0x79,
			0x00,0x00,0x00,0xff,0xff,0x83,0x00,0x00,0x00,0x00,0x00
		};

		// Display the modal dialog. The dialog is defined in the dialog template rather
		// than in the resource script (rc) file.
		CSelectDialog MyDialog((LPCDLGTEMPLATE) dlg_Template);
		for(UINT u = 0; u < GetAllTabs().size(); ++u)
		{
			MyDialog.AddItem(GetAllTabs()[u].szTabText);
		}

		int iSelected = (int)MyDialog.DoModal();
		if (iSelected >= 0) SelectPage(iSelected);
	}

	inline void CTab::SwapTabs(UINT nTab1, UINT nTab2)
	{
		if ((nTab1 < GetAllTabs().size()) && (nTab2 < GetAllTabs().size()) && (nTab1 != nTab2))
		{
			int nPage = GetCurSel();
			TabPageInfo T1 = GetTabPageInfo(nTab1);
			TabPageInfo T2 = GetTabPageInfo(nTab2);

			TCITEM Item1 = {0};
			Item1.mask = TCIF_IMAGE | TCIF_PARAM | TCIF_RTLREADING | TCIF_STATE | TCIF_TEXT;
			GetItem(nTab1, &Item1);
			TCITEM Item2 = {0};
			Item2.mask = TCIF_IMAGE | TCIF_PARAM | TCIF_RTLREADING | TCIF_STATE | TCIF_TEXT;
			GetItem(nTab2, &Item2);
			TabCtrl_SetItem(m_hWnd, nTab1, &Item2);
			TabCtrl_SetItem(m_hWnd, nTab2, &Item1);

			m_vTabPageInfo[nTab1] = T2;
			m_vTabPageInfo[nTab2] = T1;
			SelectPage(nPage);
		}
	}

	inline LRESULT CTab::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_PAINT:
			if (GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED)
			{
				// Remove all pending paint requests
				PAINTSTRUCT ps;
				BeginPaint(ps);
				EndPaint(ps);

				// Now call our local Paint
				Paint();
				return 0;
			}
			break;

		case WM_ERASEBKGND:
			if (GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED)
				return 0;
			break;
		case WM_KILLFOCUS:
			m_IsClosePressed = FALSE;
			break;
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			OnLButtonDown(wParam, lParam);
			break;
		case WM_LBUTTONUP:
			OnLButtonUp(wParam, lParam);
			break;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, lParam);
			break;
		case WM_MOUSELEAVE:
			OnMouseLeave(wParam, lParam);
			break;
		case WM_NCHITTEST:
			return OnNCHitTest(wParam, lParam);

		case WM_WINDOWPOSCHANGING:
			// A little hack to reduce tab flicker
			if (IsWindowVisible() && (GetWindowLongPtr(GWL_STYLE) & TCS_OWNERDRAWFIXED))
			{
				LPWINDOWPOS pWinPos = (LPWINDOWPOS)lParam;
				pWinPos->flags |= SWP_NOREDRAW;

				Paint();
			}

			break;

		case WM_WINDOWPOSCHANGED:
			RecalcLayout();
			break;
		}

		// pass unhandled messages on for default processing
		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}

	// Wrappers for Win32 Macros
	inline void CTab::AdjustRect(BOOL fLarger, RECT *prc) const
	{
		assert(::IsWindow(m_hWnd));
		TabCtrl_AdjustRect(m_hWnd, fLarger, prc);
	}

	inline int CTab::GetCurFocus() const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_GetCurFocus(m_hWnd);
	}

	inline int CTab::GetCurSel() const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_GetCurSel(m_hWnd);
	}

	inline BOOL CTab::GetItem(int iItem, LPTCITEM pitem) const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_GetItem(m_hWnd, iItem, pitem);
	}

	inline int CTab::GetItemCount() const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_GetItemCount(m_hWnd);
	}

	inline int CTab::HitTest(TCHITTESTINFO& info) const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_HitTest(m_hWnd, &info);
	}

	inline void CTab::SetCurFocus(int iItem) const
	{
		assert(::IsWindow(m_hWnd));
		TabCtrl_SetCurFocus(m_hWnd, iItem);
	}

	inline int CTab::SetCurSel(int iItem) const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_SetCurSel(m_hWnd, iItem);
	}

	inline DWORD CTab::SetItemSize(int cx, int cy) const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_SetItemSize(m_hWnd, cx, cy);
	}

	inline int CTab::SetMinTabWidth(int cx) const
	{
		assert(::IsWindow(m_hWnd));
		return TabCtrl_SetMinTabWidth(m_hWnd, cx);
	}

	inline void CTab::SetPadding(int cx, int cy) const
	{
		assert(::IsWindow(m_hWnd));
		TabCtrl_SetPadding(m_hWnd, cx, cy);
	}

	////////////////////////////////////////
	// Definitions for the CTabbedMDI class
	inline CTabbedMDI::CTabbedMDI()
	{
		GetTab().SetShowButtons(TRUE);
	}

	inline CTabbedMDI::~CTabbedMDI()
	{
	}

	inline CWnd* CTabbedMDI::AddMDIChild(CWnd* pView, LPCTSTR szTabText, int idMDIChild /*= 0*/)
	{
		assert(pView);
		assert(lstrlen(szTabText) < MAX_MENU_STRING);

		GetTab().AddTabPage(WndPtr(pView), szTabText, 0, idMDIChild);

		// Fake a WM_MOUSEACTIVATE to propogate focus change to dockers
		if (IsWindow())
			GetParent()->SendMessage(WM_MOUSEACTIVATE, (WPARAM)GetAncestor(), MAKELPARAM(HTCLIENT,WM_LBUTTONDOWN));

		return pView;
	}

	inline void CTabbedMDI::CloseActiveMDI()
	{
		int nTab = GetTab().GetCurSel();
		if (nTab >= 0)
			GetTab().RemoveTabPage(nTab);

		RecalcLayout();
	}

	inline void CTabbedMDI::CloseAllMDIChildren()
	{
		while (GetMDIChildCount() > 0)
		{
			GetTab().RemoveTabPage(0);
		}
	}

	inline void CTabbedMDI::CloseMDIChild(int nTab)
	{
		GetTab().RemoveTabPage(nTab);

		if (GetActiveMDIChild())
			GetActiveMDIChild()->RedrawWindow();
	}

	inline HWND CTabbedMDI::Create(CWnd* pParent /* = NULL*/)
	{
		CLIENTCREATESTRUCT clientcreate ;
		clientcreate.hWindowMenu  = m_hWnd;
		clientcreate.idFirstChild = IDW_FIRSTCHILD ;
		DWORD dwStyle = WS_CHILD | WS_VISIBLE | MDIS_ALLCHILDSTYLES;

		// Create the MDICLIENT view window
		if (!CreateEx(0, _T("MDICLIENT"), _T(""),
			dwStyle, 0, 0, 0, 0, pParent, NULL, (PSTR) &clientcreate))
				throw CWinException(_T("CMDIClient::Create ... CreateEx failed"));

		return m_hWnd;
	}

	inline CWnd* CTabbedMDI::GetActiveMDIChild() const
	{
		CWnd* pView = NULL;
		int nTab = GetTab().GetCurSel();
		if (nTab >= 0)
		{
			TabPageInfo tbi = GetTab().GetTabPageInfo(nTab);
			pView = tbi.pView;
		}

		return pView;
	}

	inline int CTabbedMDI::GetActiveMDITab() const
	{
		return GetTab().GetCurSel();
	}

	inline CWnd* CTabbedMDI::GetMDIChild(int nTab) const
	{
		assert(nTab >= 0);
		assert(nTab < GetMDIChildCount());
		return GetTab().GetTabPageInfo(nTab).pView;
	}

	inline int CTabbedMDI::GetMDIChildCount() const
	{
		return (int) GetTab().GetAllTabs().size();
	}

	inline int   CTabbedMDI::GetMDIChildID(int nTab) const
	{
		assert(nTab >= 0);
		assert(nTab < GetMDIChildCount());
		return GetTab().GetTabPageInfo(nTab).idTab;
	}

	inline LPCTSTR CTabbedMDI::GetMDIChildTitle(int nTab) const
	{
		assert(nTab >= 0);
		assert(nTab < GetMDIChildCount());
		return GetTab().GetTabPageInfo(nTab).szTabText;
	}

	inline BOOL CTabbedMDI::LoadRegistrySettings(tString tsRegistryKeyName)
	{
		BOOL bResult = FALSE;

		if (0 != tsRegistryKeyName.size())
		{
			tString tsKey = _T("Software\\") + tsRegistryKeyName + _T("\\MDI Children");
			HKEY hKey = 0;
			RegOpenKeyEx(HKEY_CURRENT_USER, tsKey.c_str(), 0, KEY_READ, &hKey);
			if (hKey)
			{
				DWORD dwType = REG_BINARY;
				DWORD BufferSize = sizeof(TabPageInfo);
				TabPageInfo tbi = {0};
				int i = 0;
				TCHAR szNumber[16];
				tString tsSubKey = _T("MDI Child ");
				tsSubKey += _itot(i, szNumber, 10);

				// Fill the DockList vector from the registry
				while (0 == RegQueryValueEx(hKey, tsSubKey.c_str(), NULL, &dwType, (LPBYTE)&tbi, &BufferSize))
				{
					CWnd* pWnd = NewMDIChildFromID(tbi.idTab);
					if (pWnd)
					{
						AddMDIChild(pWnd, tbi.szTabText, tbi.idTab);
						i++;
						tsSubKey = _T("MDI Child ");
						tsSubKey += _itot(i, szNumber, 10);
						bResult = TRUE;
					}
					else
					{
						TRACE(_T("Failed to get TabbedMDI info from registry"));
						bResult = FALSE;
						break;
					}
				}

				// Load Active MDI Tab from the registry
				tsSubKey = _T("Active MDI Tab");
				int nTab;
				dwType = REG_DWORD;
				BufferSize = sizeof(int);
				if(ERROR_SUCCESS == RegQueryValueEx(hKey, tsSubKey.c_str(), NULL, &dwType, (LPBYTE)&nTab, &BufferSize))
					SetActiveMDITab(nTab);
				else
					SetActiveMDITab(0);

				RegCloseKey(hKey);
			}
		}

		if (!bResult)
			CloseAllMDIChildren();

		return bResult;
	}

	inline CWnd* CTabbedMDI::NewMDIChildFromID(int /*idMDIChild*/)
	{
		// Override this function to create new MDI children from IDs as shown below
		CWnd* pView = NULL;
	/*	switch(idTab)
		{
		case ID_SIMPLE:
			pView = new CViewSimple;
			break;
		case ID_RECT:
			pView = new CViewRect;
			break;
		default:
			TRACE(_T("Unknown MDI child ID\n"));
			break;
		} */

		return pView;
	}

	inline void CTabbedMDI::OnCreate()
	{
		GetTab().Create(this);
		GetTab().SetFixedWidth(TRUE);
		GetTab().SetOwnerDraw(TRUE);
	}

	inline void CTabbedMDI::OnDestroy(WPARAM /*wParam*/, LPARAM /*lParam*/ )
	{
		CloseAllMDIChildren();
	}

	inline LRESULT CTabbedMDI::OnNotify(WPARAM /*wParam*/, LPARAM lParam)
	{
		LPNMHDR pnmhdr = (LPNMHDR)lParam;
		if (pnmhdr->code == UWM_TAB_CHANGED)
			RecalcLayout();

		return 0L;
	}

	inline void CTabbedMDI::OnWindowPosChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		RecalcLayout();
	}

	inline void CTabbedMDI::RecalcLayout()
	{
		if (GetTab().IsWindow())
		{
			if (GetTab().GetItemCount() >0)
			{
				CRect rcClient = GetClientRect();
				GetTab().SetWindowPos(NULL, rcClient, SWP_SHOWWINDOW);
				GetTab().UpdateWindow();
			}
			else
			{
				CRect rcClient = GetClientRect();
				GetTab().SetWindowPos(NULL, rcClient, SWP_HIDEWINDOW);
				Invalidate();
			}
		}
	}

	inline BOOL CTabbedMDI::SaveRegistrySettings(tString tsRegistryKeyName)
	{
		if (0 != tsRegistryKeyName.size())
		{
			tString tsKeyName = _T("Software\\") + tsRegistryKeyName;
			HKEY hKey = NULL;
			HKEY hKeyMDIChild = NULL;

			try
			{
				if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, tsKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL))
					throw (CWinException(_T("RegCreateKeyEx Failed")));

				RegDeleteKey(hKey, _T("MDI Children"));
				if (ERROR_SUCCESS != RegCreateKeyEx(hKey, _T("MDI Children"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyMDIChild, NULL))
					throw (CWinException(_T("RegCreateKeyEx Failed")));

				for (int i = 0; i < GetMDIChildCount(); ++i)
				{
					TCHAR szNumber[16];
					tString tsSubKey = _T("MDI Child ");
					tsSubKey += _itot(i, szNumber, 10);
					TabPageInfo pdi = GetTab().GetTabPageInfo(i);
					if (ERROR_SUCCESS != RegSetValueEx(hKeyMDIChild, tsSubKey.c_str(), 0, REG_BINARY, (LPBYTE)&pdi, sizeof(TabPageInfo)))
						throw (CWinException(_T("RegSetValueEx Failed")));
				}

				// Add Active Tab to the registry
				tString tsSubKey = _T("Active MDI Tab");
				int nTab = GetActiveMDITab();
				if(ERROR_SUCCESS != RegSetValueEx(hKeyMDIChild, tsSubKey.c_str(), 0, REG_DWORD, (LPBYTE)&nTab, sizeof(int)))
					throw (CWinException(_T("RegSetValueEx failed")));

				RegCloseKey(hKeyMDIChild);
				RegCloseKey(hKey);
			}
			catch (const CWinException& e)
			{
				// Roll back the registry changes by deleting the subkeys
				if (hKey)
				{
					if (hKeyMDIChild)
					{
						RegDeleteKey(hKeyMDIChild, _T("MDI Children"));
						RegCloseKey(hKeyMDIChild);
					}

					RegDeleteKey(HKEY_CURRENT_USER ,tsKeyName.c_str());
					RegCloseKey(hKey);
				}

				e.what();
				return FALSE;
			}
		}

		return TRUE;
	}

	inline void CTabbedMDI::SetActiveMDIChild(CWnd* pWnd)
	{
		assert(pWnd);
		int nPage = GetTab().GetTabIndex(pWnd);
		if (nPage >= 0)
			GetTab().SelectPage(nPage);
	}

	inline void CTabbedMDI::SetActiveMDITab(int iTab)
	{
		assert(::IsWindow(m_hWnd));
		assert(GetTab().IsWindow());
		GetTab().SelectPage(iTab);
	}

	inline LRESULT CTabbedMDI::WndProcDefault(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_DESTROY:
			OnDestroy(wParam, lParam);
			break;

		case WM_WINDOWPOSCHANGED:
			OnWindowPosChanged(wParam, lParam);
			break;
		}

		return CWnd::WndProcDefault(uMsg, wParam, lParam);
	}

} // namespace Win32xx

#endif  // _WIN32XX_TAB_H_
