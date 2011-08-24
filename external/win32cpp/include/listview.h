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



#ifndef _WIN32XX_LISTVIEW_H_
#define _WIN32XX_LISTVIEW_H_

#include "wincore.h"
#include "commctrl.h"

namespace Win32xx
{

	class CListView : public CWnd
	{
	public:
		CListView() {}
		virtual ~CListView() {}
		virtual void PreRegisterClass(WNDCLASS &wc);

		// Attributes
		CSize ApproximateViewRect(CSize sz = CSize(-1, -1), int iCount = -1) const;
		COLORREF GetBkColor( ) const;
		BOOL GetBkImage( LVBKIMAGE& lvbkImage ) const;
		UINT GetCallbackMask( ) const;
		BOOL GetCheckState( UINT nItem ) const;
		BOOL GetColumn( int iCol, LVCOLUMN& Column ) const;
		BOOL GetColumnOrderArray( LPINT piArray, int iCount = -1 );
		int GetColumnWidth( int iCol ) const;
		int GetCountPerPage( ) const;
		HWND GetEditControl( ) const;
		DWORD GetExtendedStyle( ) const;
		HWND GetHeader( ) const;
		HCURSOR GetHotCursor( );
		int GetHotItem( ) const;
		DWORD GetHoverTime( ) const;
		HIMAGELIST GetImageList( int nImageType ) const;
		BOOL GetItem( LVITEM& lvItem ) const;
		int GetItemCount( ) const;
		DWORD_PTR GetItemData( int iItem ) const;
		BOOL GetItemPosition( int iItem, CPoint& pt ) const;
		BOOL GetItemRect( int iItem, CRect& rc, UINT nCode ) const;
		UINT GetItemState( int iItem, UINT nMask ) const;
		tString GetItemText( int iItem, int iSubItem, UINT nTextMax = 260 ) const;
		int GetNextItem( int iItem, int iFlags ) const;
		UINT GetNumberOfWorkAreas( ) const;
		BOOL GetOrigin( CPoint& pt ) const;
		UINT GetSelectedCount( ) const;
		int GetSelectionMark( ) const;
		int GetStringWidth( LPCTSTR pszString ) const;
		BOOL GetSubItemRect( int iItem, int iSubItem, int iCode, CRect& rc ) const;
		COLORREF GetTextBkColor( ) const;
		COLORREF GetTextColor( ) const;
		HWND GetToolTips( ) const;
		int GetTopIndex( ) const;
		BOOL GetViewRect( CRect& rc ) const;
		void GetWorkAreas( int iWorkAreas, LPRECT pRectArray ) const;
		BOOL SetBkColor( COLORREF clrBk ) const;
		BOOL SetBkImage( LVBKIMAGE& plvbkImage ) const;
		BOOL SetCallbackMask( UINT nMask ) const;
		void SetCheckState( int iItem, BOOL fCheck = TRUE ) const;
		BOOL SetColumn( int iCol, const LVCOLUMN& pColumn ) const;
		BOOL SetColumnOrderArray( int iCount, LPINT piArray ) const;
		BOOL SetColumnWidth( int iCol, int cx ) const;
		DWORD SetExtendedStyle( DWORD dwNewStyle ) const;
		HCURSOR SetHotCursor( HCURSOR hCursor ) const;
		int SetHotItem( int nIndex ) const;
		DWORD SetHoverTime( DWORD dwHoverTime = (DWORD)-1 ) const;
		CSize SetIconSpacing( int cx, int cy ) const;
		CSize SetIconSpacing( CSize sz ) const;
		HIMAGELIST SetImageList( HIMAGELIST himl, int iImageListType ) const;
		BOOL SetItem( LVITEM& pItem ) const;
		BOOL SetItem( int iItem, int iSubItem, UINT nMask, LPCTSTR pszText, int iImage,
						UINT nState, UINT nStateMask, LPARAM lParam, int iIndent ) const;
		void SetItemCount( int iCount ) const;
		void SetItemCountEx( int iCount, DWORD dwFlags = LVSICF_NOINVALIDATEALL ) const;
		BOOL SetItemData( int iItem, DWORD_PTR dwData ) const;
		BOOL SetItemPosition( int iItem, CPoint& pt ) const;
		BOOL SetItemState( int iItem, LVITEM& Item ) const;
        void SetItemState( int iItem, UINT nState, UINT nMask ) const;
		void SetItemText( int iItem, int iSubItem, LPCTSTR pszText ) const;
		int SetSelectionMark( int iIndex ) const;
		BOOL SetTextBkColor( COLORREF clrBkText ) const;
		BOOL SetTextColor( COLORREF clrText ) const;
		HWND SetToolTips( HWND hWndToolTip ) const;
		void SetWorkAreas( int nWorkAreas, CRect& pRectArray ) const;
		int SubItemHitTest( LVHITTESTINFO& htInfo ) const;

		// Operations
		BOOL Arrange( UINT nCode ) const;
		HIMAGELIST CreateDragImage( int iItem, CPoint& pt ) const;
		BOOL DeleteAllItems( ) const;
		BOOL DeleteColumn( int iCol ) const;
		BOOL DeleteItem( int iItem ) const;
		HWND EditLabel( int iItem ) const;
		BOOL EnsureVisible( int iItem, BOOL fPartialOK ) const;
		int FindItem( LVFINDINFO& FindInfo, int iStart = -1 ) const;
		int HitTest( LVHITTESTINFO& HitTestInfo ) const;
        int HitTest( CPoint pt, UINT* pFlags = NULL ) const;
		int InsertColumn( int iCol, const LVCOLUMN& pColumn ) const;
        int InsertColumn( int iCol, LPCTSTR pszColumnHeading, int iFormat = LVCFMT_LEFT,
							int iWidth = -1, int iSubItem = -1 ) const;
		int InsertItem( const LVITEM& pItem ) const;
        int InsertItem( int iItem, LPCTSTR pszText ) const;
        int InsertItem( int iItem, LPCTSTR pszText, int iImage ) const;
		BOOL RedrawItems( int iFirst, int iLast ) const;
		BOOL Scroll( CSize sz ) const;
		BOOL SortItems( PFNLVCOMPARE pfnCompare, DWORD_PTR dwData ) const;
		BOOL Update( int iItem ) const;

	private:
		CListView(const CListView&);				// Disable copy construction
		CListView& operator = (const CListView&); // Disable assignment operator
	};

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Win32xx
{

	inline void CListView::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  WC_LISTVIEW;
	}

	inline CSize CListView::ApproximateViewRect(CSize sz /*= CSize(-1, -1)*/, int iCount /* = -1*/) const
	// Calculates the approximate width and height required to display a given number of items.
	{
		assert(::IsWindow(m_hWnd));
		return CSize( ListView_ApproximateViewRect( m_hWnd, sz.cx, sz.cy, iCount ) );
	}

	inline COLORREF CListView::GetBkColor( ) const
	// Retrieves the background color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetBkColor( m_hWnd );
	}

	inline BOOL CListView::GetBkImage( LVBKIMAGE& lvbkImage ) const
	// Retrieves the background image in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetBkImage( m_hWnd, &lvbkImage );
	}

	inline UINT CListView::GetCallbackMask( ) const
	// Retrieves the callback mask for a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetCallbackMask( m_hWnd );
	}

	inline BOOL CListView::GetCheckState( UINT nItem ) const
	// Determines if an item in a list-view control is selected.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetCheckState( m_hWnd, nItem );
	}

	inline BOOL CListView::GetColumn( int iCol, LVCOLUMN& Column ) const
	// Retrieves the attributes of a list-view control's column.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetColumn( m_hWnd, iCol, &Column );
	}

	inline BOOL CListView::GetColumnOrderArray( LPINT piArray, int iCount /*= -1*/ )
	// Retrieves the current left-to-right order of columns in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetColumnOrderArray( m_hWnd, iCount, piArray );
	}

	inline int CListView::GetColumnWidth( int iCol ) const
	// Retrieves the width of a column in report or list view.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetColumnWidth( m_hWnd, iCol );
	}

	inline int CListView::GetCountPerPage( ) const
	// Calculates the number of items that can fit vertically in the visible area of a
	// list-view control when in list or report view. Only fully visible items are counted.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetCountPerPage( m_hWnd );
	}

	inline HWND CListView::GetEditControl( ) const
	// Retrieves the handle to the edit control being used to edit a list-view item's text.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetEditControl( m_hWnd );
	}

	inline DWORD CListView::GetExtendedStyle( ) const
	// Retrieves the extended styles that are currently in use for a given list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetExtendedListViewStyle( m_hWnd );
	}

	inline HWND CListView::GetHeader( ) const
	// Retrieves the handle to the header control used by a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetHeader( m_hWnd );
	}

	inline HCURSOR CListView::GetHotCursor( )
	// Retrieves the HCURSOR used when the pointer is over an item while hot tracking is enabled.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetHotCursor( m_hWnd );
	}

	inline int CListView::GetHotItem( ) const
	// Retrieves the index of the hot item.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetHotItem( m_hWnd );
	}

	inline DWORD CListView::GetHoverTime( ) const
	// Retrieves the amount of time that the mouse cursor must hover over an item before it is selected.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetHoverTime( m_hWnd );
	}

	inline HIMAGELIST CListView::GetImageList( int nImageType ) const
	// Retrieves the handle to an image list used for drawing list-view items.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetImageList( m_hWnd, nImageType );
	}

	inline BOOL CListView::GetItem( LVITEM& Item ) const
	// Retrieves some or all of a list-view item's attributes.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetItem( m_hWnd, &Item );
	}

	inline int CListView::GetItemCount( ) const
	// Retrieves the number of items in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetItemCount( m_hWnd );
	}

	inline DWORD_PTR CListView::GetItemData( int iItem ) const
	// Retrieves the value(lParam) specific to the item.
	{
		assert(::IsWindow(m_hWnd));

		LVITEM lvi = {0};
		lvi.iItem = iItem;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(m_hWnd, &lvi);
		return lvi.lParam;
	}

	inline BOOL CListView::GetItemPosition( int iItem, CPoint& pt ) const
	// Retrieves the position of a list-view item.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetItemPosition( m_hWnd, iItem, &pt );
	}

	inline BOOL CListView::GetItemRect( int iItem, CRect& rc, UINT nCode ) const
	// Retrieves the bounding rectangle for all or part of an item in the current view.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetItemRect( m_hWnd, iItem, &rc, nCode );
	}

	inline UINT CListView::GetItemState( int iItem, UINT nMask ) const
	// Retrieves the state of a list-view item.

	// Possible values of nMask:
	// LVIS_CUT				The item is marked for a cut-and-paste operation.
	// LVIS_DROPHILITED		The item is highlighted as a drag-and-drop target.
	// LVIS_FOCUSED			The item has the focus, so it is surrounded by a standard focus rectangle.
	// LVIS_SELECTED		The item is selected.
	// LVIS_OVERLAYMASK		Use this mask to retrieve the item's overlay image index.
	// LVIS_STATEIMAGEMASK	Use this mask to retrieve the item's state image index.
	{
		assert(::IsWindow(m_hWnd));
		return  ListView_GetItemState( m_hWnd, iItem, nMask );
	}

	inline tString CListView::GetItemText( int iItem, int iSubItem, UINT nTextMax /* = 260 */ ) const
	// Retrieves the text of a list-view item.
	// Note: Although the list-view control allows any length string to be stored
	//       as item text, only the first 260 characters are displayed.
	{
		assert(::IsWindow(m_hWnd));

		tString t;
		if (nTextMax > 0)
		{
			std::vector<TCHAR> vTChar(nTextMax +1, _T('\0'));
			TCHAR* pszText = &vTChar.front();
			LVITEM lvi = {0};
			lvi.iItem = iItem;
			lvi.iSubItem = iSubItem;
			lvi.mask = LVIF_TEXT;
			lvi.cchTextMax = nTextMax;
			lvi.pszText = pszText;
			ListView_GetItem( m_hWnd, &lvi );
			t = lvi.pszText;
		}
		return t;
	}

	inline int CListView::GetNextItem( int iItem, int iFlags ) const
	// Searches for a list-view item that has the specified properties and
	// bears the specified relationship to a specified item.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetNextItem( m_hWnd, iItem, iFlags );
	}

	inline UINT CListView::GetNumberOfWorkAreas( ) const
	// Retrieves the working areas from a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		UINT nWorkAreas = 0;
		ListView_GetWorkAreas( m_hWnd, nWorkAreas, NULL );
		return nWorkAreas;
	}

	inline BOOL CListView::GetOrigin( CPoint& pt ) const
	// Retrieves the current view origin for a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetOrigin( m_hWnd, &pt );
	}

	inline UINT CListView::GetSelectedCount( ) const
	// Determines the number of selected items in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)::SendMessage( m_hWnd, LVM_GETSELECTEDCOUNT, 0L, 0L );
	}

	inline int CListView::GetSelectionMark( ) const
	// Retrieves the selection mark from a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)::SendMessage( m_hWnd, LVM_GETSELECTIONMARK, 0L, 0L );
	}

	inline int CListView::GetStringWidth( LPCTSTR pszString ) const
	// Determines the width of a specified string using the specified list-view control's current font.
	{
		assert(::IsWindow(m_hWnd));
		return (int)::SendMessage( m_hWnd, LVM_GETSTRINGWIDTH, 0L, (LPARAM)pszString );
	}

	inline BOOL CListView::GetSubItemRect( int iItem, int iSubItem, int iCode, CRect& rc ) const
	// Retrieves information about the rectangle that surrounds a subitem in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetSubItemRect( m_hWnd, iItem, iSubItem, iCode, &rc );
	}

	inline COLORREF CListView::GetTextBkColor( ) const
	// Retrieves the text background color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetTextBkColor( m_hWnd );
	}

	inline COLORREF CListView::GetTextColor( ) const
	// Retrieves the text color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetTextColor( m_hWnd );
	}

	inline HWND CListView::GetToolTips( ) const
	// Retrieves the ToolTip control that the list-view control uses to display ToolTips.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetToolTips( m_hWnd );
	}

	inline int CListView::GetTopIndex( ) const
	// Retrieves the index of the topmost visible item when in list or report view.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetTopIndex( m_hWnd );
	}

	inline BOOL CListView::GetViewRect( CRect& rc ) const
	// Retrieves the bounding rectangle of all items in the list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_GetViewRect( m_hWnd, &rc );
	}

	inline void CListView::GetWorkAreas( int iWorkAreas, LPRECT pRectArray ) const
	// Retrieves the working areas from a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		ListView_GetWorkAreas( m_hWnd, iWorkAreas, pRectArray );
	}

	inline BOOL CListView::SetBkColor( COLORREF clrBk ) const
	// Sets the background color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetBkColor( m_hWnd, clrBk );
	}

	inline BOOL CListView::SetBkImage( LVBKIMAGE& lvbkImage ) const
	// Sets the background image in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetBkImage( m_hWnd, &lvbkImage );
	}

	inline BOOL CListView::SetCallbackMask( UINT nMask ) const
	// Changes the callback mask for a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetCallbackMask( m_hWnd, nMask );
	}

	inline void CListView::SetCheckState( int iItem, BOOL fCheck /*= TRUE*/ ) const
	// Used to select or deselect an item in a list-view control.
	// This macro should only be used for list-view controls with the LVS_EX_CHECKBOXES style.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetItemState(m_hWnd, iItem, INDEXTOSTATEIMAGEMASK((fCheck==TRUE)?2:1),LVIS_STATEIMAGEMASK);
	}

	inline BOOL CListView::SetColumn( int iCol, const LVCOLUMN& Column ) const
	// Sets the attributes of a list-view column.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetColumn( m_hWnd, iCol, &Column );
	}

	inline BOOL CListView::SetColumnOrderArray( int iCount, LPINT piArray ) const
	// Sets the left-to-right order of columns in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetColumnOrderArray( m_hWnd, iCount, piArray );
	}

	inline BOOL CListView::SetColumnWidth( int iCol, int cx ) const
	// Used to change the width of a column in report view or the width of all columns in list-view mode.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetColumnWidth( m_hWnd, iCol, cx );
	}

	inline DWORD CListView::SetExtendedStyle( DWORD dwNewStyle ) const
	// Sets extended styles for list-view controls.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetExtendedListViewStyle( m_hWnd, dwNewStyle );
	}

	inline HCURSOR CListView::SetHotCursor( HCURSOR hCursor ) const
	// Sets the HCURSOR that the list-view control uses when the pointer is
	// over an item while hot tracking is enabled.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetHotCursor( m_hWnd, hCursor );
	}

	inline int CListView::SetHotItem( int nIndex ) const
	// Sets the hot item in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetHotItem( m_hWnd, nIndex );
	}

	inline DWORD CListView::SetHoverTime( DWORD dwHoverTime /*= (DWORD)-1*/ ) const
	// Sets the amount of time that the mouse cursor must hover over an item before it is selected.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetHoverTime( m_hWnd, dwHoverTime );
	}

	inline CSize CListView::SetIconSpacing( int cx, int cy ) const
	// Sets the spacing between icons in list-view controls set to the LVS_ICON style.
	{
		assert(::IsWindow(m_hWnd));
		return CSize( ListView_SetIconSpacing( m_hWnd, cx, cy ) );
	}

	inline CSize CListView::SetIconSpacing( CSize sz ) const
	// Sets the spacing between icons in list-view controls set to the LVS_ICON style.
	{
		assert(::IsWindow(m_hWnd));
		return CSize( ListView_SetIconSpacing( m_hWnd, sz.cx, sz.cy ) );
	}

	inline HIMAGELIST CListView::SetImageList( HIMAGELIST himl, int iImageListType ) const
	// Assigns an image list to a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetImageList( m_hWnd, himl, iImageListType );
	}

	inline BOOL CListView::SetItem( LVITEM& Item ) const
	// Sets some or all of a list-view item's attributes.

	// The declaration for TVITEM:
	//	typedef struct _LVITEM {
	//		UINT mask;
	//		int iItem;
	//		int iSubItem;
	//		UINT state;
	//		UINT stateMask;
	//		LPTSTR pszText;
	//		int cchTextMax;
    //		int iImage;
	//		LPARAM lParam;
	// } LVITEM, *LVITEM&;
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetItem( m_hWnd, &Item );
	}

	inline BOOL CListView::SetItem( int iItem, int iSubItem, UINT nMask, LPCTSTR pszText, int iImage,
					UINT nState, UINT nStateMask, LPARAM lParam, int iIndent ) const
	// Sets some or all of a list-view item's attributes.
	{
		assert(::IsWindow(m_hWnd));

		LVITEM lvi = {0};
		lvi.iItem = iItem;
		lvi.iSubItem = iSubItem;
		lvi.mask = nMask;
		lvi.pszText = (LPTSTR)pszText;
		lvi.iImage = iImage;
		lvi.state = nState;
		lvi.stateMask = nStateMask;
		lvi.lParam = lParam;
		lvi.iIndent = iIndent;

		return ListView_SetItem( m_hWnd, &lvi);
	}

	inline void CListView::SetItemCount( int iCount ) const
	// Causes the list-view control to allocate memory for the specified number of items.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetItemCount( m_hWnd, iCount );
	}

	inline void CListView::SetItemCountEx( int iCount, DWORD dwFlags /*= LVSICF_NOINVALIDATEALL*/ ) const
	// Sets the virtual number of items in a virtual list view.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetItemCountEx( m_hWnd, iCount, dwFlags );
	}

	inline BOOL CListView::SetItemData( int iItem, DWORD_PTR dwData ) const
	// Sets the value(lParam) specific to the item.
	{
		assert(::IsWindow(m_hWnd));

		LVITEM lvi = {0};
		lvi.iItem = iItem;
		lvi.lParam = dwData;
		lvi.mask = LVIF_PARAM;
		return ListView_SetItem(m_hWnd, &lvi);
	}

	inline BOOL CListView::SetItemPosition( int iItem, CPoint& pt ) const
	// Moves an item to a specified position in a list-view control (in icon or small icon view).
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetItemPosition( m_hWnd, iItem, pt.x, pt.y );
	}

	inline BOOL CListView::SetItemState( int iItem, LVITEM& Item ) const
	// Changes the state of an item in a list-view control.

	// Possible values of nMask:
	// LVIS_CUT				The item is marked for a cut-and-paste operation.
	// LVIS_DROPHILITED		The item is highlighted as a drag-and-drop target.
	// LVIS_FOCUSED			The item has the focus, so it is surrounded by a standard focus rectangle.
	// LVIS_SELECTED		The item is selected.
	// LVIS_OVERLAYMASK		Use this mask to retrieve the item's overlay image index.
	// LVIS_STATEIMAGEMASK	Use this mask to retrieve the item's state image index.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, LVM_SETITEMSTATE, (WPARAM)iItem, (LPARAM)&Item);
	}

    inline void CListView::SetItemState( int iItem, UINT nState, UINT nMask ) const
	// Changes the state of an item in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetItemState(m_hWnd, iItem, nState, nMask);
	}

	inline void CListView::SetItemText( int iItem, int iSubItem, LPCTSTR pszText ) const
	// Sets the text color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetItemText(m_hWnd, iItem, iSubItem, (LPTSTR)pszText );
	}

	inline int CListView::SetSelectionMark( int iIndex ) const
	// Sets the selection mark in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetSelectionMark( m_hWnd, iIndex );
	}

	inline BOOL CListView::SetTextBkColor( COLORREF clrBkText ) const
	// Sets the background color of text in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetTextBkColor( m_hWnd, clrBkText );
	}

	inline BOOL CListView::SetTextColor( COLORREF clrText ) const
	// Sets the text color of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SetTextColor( m_hWnd, clrText );
	}

	inline HWND CListView::SetToolTips( HWND hWndToolTip ) const
	// Sets the ToolTip control that the list-view control will use to display ToolTips.
	{
		assert(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, LVM_SETTOOLTIPS, (WPARAM)hWndToolTip, 0L);
	}

	inline void CListView::SetWorkAreas( int nWorkAreas, CRect& pRectArray ) const
	// Sets the working area within a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		ListView_SetWorkAreas( m_hWnd, nWorkAreas, pRectArray );
	}

	inline int CListView::SubItemHitTest( LVHITTESTINFO& htInfo ) const
	// Determines which list-view item or subitem is located at a given position.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SubItemHitTest( m_hWnd, &htInfo );
	}

	// Operations

	inline BOOL CListView::Arrange( UINT nCode ) const
	// Arranges items in icon view.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_Arrange( m_hWnd, nCode );
	}

	inline HIMAGELIST CListView::CreateDragImage( int iItem, CPoint& pt ) const
	// Creates a drag image list for the specified item.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_CreateDragImage( m_hWnd, iItem, &pt );
	}

	inline BOOL CListView::DeleteAllItems( ) const
	// ListView_DeleteAllItems
	{
		assert(::IsWindow(m_hWnd));
		return ListView_DeleteAllItems( m_hWnd );
	}

	inline BOOL CListView::DeleteColumn( int iCol ) const
	// Removes a column from a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_DeleteColumn( m_hWnd, iCol );
	}

	inline BOOL CListView::DeleteItem( int iItem ) const
	// Removes an item from a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_DeleteItem( m_hWnd, iItem );
	}

	inline HWND CListView::EditLabel( int iItem ) const
	// Begins in-place editing of the specified list-view item's text.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_EditLabel( m_hWnd, iItem );
	}

	inline BOOL CListView::EnsureVisible( int iItem, BOOL fPartialOK ) const
	// Ensures that a list-view item is either entirely or partially visible,
	// scrolling the list-view control if necessary.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(LVM_ENSUREVISIBLE, (WPARAM)iItem, (LPARAM)fPartialOK );
	}

	inline int CListView::FindItem( LVFINDINFO& FindInfo, int iStart /*= -1*/ ) const
	// Searches for a list-view item with the specified characteristics.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_FindItem( m_hWnd, iStart, &FindInfo );
	}

	inline int CListView::HitTest( LVHITTESTINFO& HitTestInfo ) const
	// Determines which list-view item, if any, is at a specified position.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_HitTest( m_hWnd, &HitTestInfo );
	}

    inline int CListView::HitTest( CPoint pt, UINT* pFlags /*= NULL*/ ) const
	// Determines which list-view item, if any, is at a specified position.
	{
		assert(::IsWindow(m_hWnd));

		LVHITTESTINFO hti = {0};
		hti.flags = *pFlags;
		hti.pt = pt;
		return ListView_HitTest( m_hWnd, &hti );
	}

	inline int CListView::InsertColumn( int iCol, const LVCOLUMN& Column ) const
	// Inserts a new column in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_InsertColumn( m_hWnd, iCol, &Column );
	}

    inline int CListView::InsertColumn( int iCol, LPCTSTR pszColumnHeading, int iFormat /*= LVCFMT_LEFT*/,
						int iWidth /*= -1*/, int iSubItem /*= -1*/ ) const
	// Inserts a new column in a list-view control.
	{
		assert(::IsWindow(m_hWnd));

		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_TEXT|LVCF_ORDER|LVCF_FMT;
		if (-1 != iWidth)
		{
			lvc.mask |= LVCF_WIDTH; 
			lvc.cx = iWidth; 
		}
		if (-1 !=  iSubItem)
		{
			lvc.mask |= LVCF_SUBITEM; 
			lvc.iSubItem = iSubItem;
		}

		lvc.iOrder = iCol;
		lvc.pszText = (LPTSTR)pszColumnHeading;
		lvc.fmt = iFormat;
		lvc.iSubItem = iSubItem;
		return ListView_InsertColumn( m_hWnd, iCol, &lvc );
	}

	inline int CListView::InsertItem( const LVITEM& Item ) const
	// Inserts a new item in a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_InsertItem( m_hWnd, &Item );
	}

    inline int CListView::InsertItem( int iItem, LPCTSTR pszText ) const
	// Inserts a new item in a list-view control.
	{
		assert(::IsWindow(m_hWnd));

		LVITEM lvi = {0};
		lvi.iItem = iItem;
		lvi.pszText = (LPTSTR)pszText;
		lvi.mask = LVIF_TEXT;
		return ListView_InsertItem( m_hWnd, &lvi );
	}

    inline int CListView::InsertItem( int iItem, LPCTSTR pszText, int iImage ) const
	// Inserts a new item in a list-view control.
	{
		assert(::IsWindow(m_hWnd));

		LVITEM lvi = {0};
		lvi.iItem = iItem;
		lvi.pszText = (LPTSTR)pszText;
		lvi.iImage = iImage;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		return ListView_InsertItem( m_hWnd, &lvi );
	}

	inline BOOL CListView::RedrawItems( int iFirst, int iLast ) const
	// Forces a list-view control to redraw a range of items.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_RedrawItems( m_hWnd, iFirst, iLast );
	}

	inline BOOL CListView::Scroll( CSize sz ) const
	// Scrolls the content of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_Scroll( m_hWnd, sz.cx, sz.cy );
	}

	inline BOOL CListView::SortItems( PFNLVCOMPARE pfnCompare, DWORD_PTR dwData ) const
	// Uses an application-defined comparison function to sort the items of a list-view control.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_SortItems( m_hWnd, pfnCompare, dwData );
	}

	inline BOOL CListView::Update( int iItem ) const
	// Updates a list-view item. If the list-view control has the LVS_AUTOARRANGE style,
	// the list-view control is rearranged.
	{
		assert(::IsWindow(m_hWnd));
		return ListView_Update( m_hWnd, iItem );
	}

} // namespace Win32xx

#endif // #ifndef _WIN32XX_LISTVIEW_H_

