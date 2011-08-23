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




#ifndef _WIN32XX_TREEVIEW_H_
#define _WIN32XX_TREEVIEW_H_

#include "wincore.h"
#include "commctrl.h"

// Disable macros from Windowsx.h
#undef GetNextSibling
#undef GetPrevSibling

namespace Win32xx
{

	class CTreeView : public CWnd
	{
	public:
		CTreeView() {}
		virtual ~CTreeView() {}
		virtual void PreRegisterClass(WNDCLASS &wc);

// Attributes
		COLORREF GetBkColor() const;
		HTREEITEM GetChild(HTREEITEM hItem) const;
		UINT  GetCount() const;
		HTREEITEM GetDropHiLightItem() const;
		HWND GetEditControl() const;
		HTREEITEM GetFirstVisible() const;
		HIMAGELIST GetImageList(int iImageType) const;
		UINT  GetIndent() const;
		COLORREF GetInsertMarkColor() const;
		BOOL GetItem(TVITEM& Item) const;
		DWORD_PTR GetItemData(HTREEITEM hItem) const;
		int  GetItemHeight() const;
		BOOL GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage ) const;
		BOOL GetItemRect(HTREEITEM hItem, CRect& rc, BOOL bTextOnly) const;
		tString GetItemText(HTREEITEM hItem, UINT nTextMax /* = 260 */) const;
		HTREEITEM GetLastVisible() const;
		HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode) const;
		HTREEITEM GetNextSibling(HTREEITEM hItem) const;
		HTREEITEM GetNextVisible(HTREEITEM hItem) const;
		HTREEITEM GetParentItem(HTREEITEM hItem) const;
		HTREEITEM GetPrevSibling(HTREEITEM hItem) const;
		HTREEITEM GetPrevVisible(HTREEITEM hItem) const;
		HTREEITEM GetRootItem() const;
		int GetScrollTime() const;
		HTREEITEM GetSelection() const;
		COLORREF GetTextColor() const;
		HWND GetToolTips() const;
		UINT GetVisibleCount() const;
		BOOL ItemHasChildren(HTREEITEM hItem) const;
		COLORREF SetBkColor(COLORREF clrBk) const;
		HIMAGELIST SetImageList(HIMAGELIST himl, int nType) const;
		void SetIndent(int indent) const;
		BOOL SetInsertMark(HTREEITEM hItem, BOOL fAfter = TRUE) const;
		COLORREF SetInsertMarkColor(COLORREF clrInsertMark) const;
		BOOL SetItem(TVITEM& Item) const;
		BOOL SetItem(HTREEITEM hItem, UINT nMask, LPCTSTR szText, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam) const;
		BOOL SetItemData(HTREEITEM hItem, DWORD_PTR dwData) const;
		int  SetItemHeight(SHORT cyItem) const;
		BOOL SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage) const;
		BOOL SetItemText(HTREEITEM hItem, LPCTSTR szText) const;
		UINT SetScrollTime(UINT uScrollTime) const;
		COLORREF SetTextColor(COLORREF clrText) const;
		HWND SetToolTips(HWND hwndTooltip) const;

// Operations
		HIMAGELIST CreateDragImage(HTREEITEM hItem) const;
		BOOL DeleteAllItems() const;
		BOOL DeleteItem(HTREEITEM hItem) const;
		HWND EditLabel(HTREEITEM hItem) const;
		BOOL EndEditLabelNow(BOOL fCancel) const;
		BOOL EnsureVisible(HTREEITEM hItem) const;
		BOOL Expand(HTREEITEM hItem, UINT nCode) const;
		HTREEITEM HitTest(TVHITTESTINFO& ht) const;
		HTREEITEM InsertItem(TVINSERTSTRUCT& tvIS) const;
		BOOL Select(HTREEITEM hitem, UINT flag) const;
		BOOL SelectDropTarget(HTREEITEM hItem) const;
		BOOL SelectItem(HTREEITEM hItem) const;
		BOOL SelectSetFirstVisible(HTREEITEM hItem) const;
		BOOL SortChildren(HTREEITEM hItem, BOOL fRecurse) const;
		BOOL SortChildrenCB(TVSORTCB& sort, BOOL fRecurse) const;

	private:
		CTreeView(const CTreeView&);				// Disable copy construction
		CTreeView& operator = (const CTreeView&); // Disable assignment operator

	};
	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Win32xx
{

	inline void CTreeView::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  WC_TREEVIEW;
	}

// Attributes
	inline COLORREF CTreeView::GetBkColor() const
	// Retrieves the current background color of the control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetBkColor( m_hWnd );
	}

	inline HTREEITEM CTreeView::GetChild(HTREEITEM hItem) const
	// Retrieves the first child item of the specified tree-view item.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetChild(m_hWnd, hItem);
	}

	inline UINT  CTreeView::GetCount() const
	// Retrieves a count of the items in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetCount( m_hWnd );
	}

	inline HTREEITEM CTreeView::GetDropHiLightItem() const
	// Retrieves the tree-view item that is the target of a drag-and-drop operation.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetDropHilight(m_hWnd);
	}

	inline HWND CTreeView::GetEditControl() const
	// Retrieves the handle to the edit control being used to edit a tree-view item's text.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetEditControl( m_hWnd );
	}

	inline HTREEITEM CTreeView::GetFirstVisible() const
	// Retrieves the first visible item in a tree-view control window.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetFirstVisible(m_hWnd);
	}

	inline HIMAGELIST CTreeView::GetImageList(int iImageType) const
	// Retrieves the handle to the normal or state image list associated with a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetImageList( m_hWnd, iImageType );
	}

	inline UINT  CTreeView::GetIndent() const
	// Retrieves the amount, in pixels, that child items are indented relative to their parent items.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetIndent( m_hWnd );
	}

	inline COLORREF CTreeView::GetInsertMarkColor() const
	// Retrieves the color used to draw the insertion mark for the tree view.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetInsertMarkColor( m_hWnd );
	}

	inline BOOL CTreeView::GetItem(TVITEM& Item) const
	// Retrieves some or all of a tree-view item's attributes.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetItem( m_hWnd, &Item );
	}

	inline DWORD_PTR CTreeView::GetItemData(HTREEITEM hItem) const
	// Retrieves a tree-view item's application data.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItem;
		TreeView_GetItem( m_hWnd, &tvi );
		return tvi.lParam;
	}

	inline int  CTreeView::GetItemHeight() const
	// Retrieves the current height of the tree-view item.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetItemHeight( m_hWnd );
	}

	inline BOOL CTreeView::GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage ) const
	// Retrieves the index of the tree-view item's image and selected image.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.hItem = hItem;
		BOOL bResult = TreeView_GetItem( m_hWnd, &tvi );
		nImage = tvi.iImage;
		nSelectedImage = tvi.iSelectedImage;
		return bResult;
	}

	inline BOOL CTreeView::GetItemRect(HTREEITEM hItem, CRect& rc, BOOL bTextOnly) const
	// Retrieves the bounding rectangle for a tree-view item and indicates whether the item is visible.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetItemRect( m_hWnd, hItem, &rc, bTextOnly );
	}

	inline tString CTreeView::GetItemText(HTREEITEM hItem, UINT nTextMax /* = 260 */) const
	// Retrieves the text for a tree-view item.
	// Note: Although the tree-view control allows any length string to be stored 
	//       as item text, only the first 260 characters are displayed.
	{
		assert(::IsWindow(m_hWnd));

		tString t;
		if (nTextMax > 0)
		{
			TVITEM tvi = {0};
			tvi.hItem = hItem;
			tvi.mask = TVIF_TEXT;
			tvi.cchTextMax = nTextMax;
			std::vector<TCHAR> vTChar(nTextMax +1, _T('\0'));
			TCHAR* pTCharArray = &vTChar.front();
			tvi.pszText = pTCharArray;
			::SendMessage(m_hWnd, TVM_GETITEM, 0L, (LPARAM)&tvi);
			t = tvi.pszText;
		}
		return t;
	}

	inline HTREEITEM CTreeView::GetLastVisible() const
	// Retrieves the last expanded item in a tree-view control.
	// This does not retrieve the last item visible in the tree-view window.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetLastVisible(m_hWnd);
	}

	inline HTREEITEM CTreeView::GetNextItem(HTREEITEM hItem, UINT nCode) const
	// Retrieves the tree-view item that bears the specified relationship to a specified item.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetNextItem( m_hWnd, hItem, nCode);
	}

	inline HTREEITEM CTreeView::GetNextSibling(HTREEITEM hItem) const
	// Retrieves the next sibling item of a specified item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetNextSibling(m_hWnd, hItem);
	}

	inline HTREEITEM CTreeView::GetNextVisible(HTREEITEM hItem) const
	// Retrieves the next visible item that follows a specified item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetNextVisible(m_hWnd, hItem);
	}

	inline HTREEITEM CTreeView::GetParentItem(HTREEITEM hItem) const
	// Retrieves the parent item of the specified tree-view item.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetParent(m_hWnd, hItem);
	}

	inline HTREEITEM CTreeView::GetPrevSibling(HTREEITEM hItem) const
	// Retrieves the previous sibling item of a specified item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetPrevSibling(m_hWnd, hItem);
	}

	inline HTREEITEM CTreeView::GetPrevVisible(HTREEITEM hItem) const
	// Retrieves the first visible item that precedes a specified item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetPrevSibling(m_hWnd, hItem);
	}

	inline HTREEITEM CTreeView::GetRootItem() const
	// Retrieves the topmost or very first item of the tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetRoot(m_hWnd);
	}

	inline int CTreeView::GetScrollTime() const
	// Retrieves the maximum scroll time for the tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetScrollTime( m_hWnd );
	}

	inline HTREEITEM CTreeView::GetSelection() const
	// Retrieves the currently selected item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetSelection(m_hWnd);
	}

	inline COLORREF CTreeView::GetTextColor() const
	// Retrieves the current text color of the control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetTextColor( m_hWnd );
	}

	inline HWND CTreeView::GetToolTips() const
	// Retrieves the handle to the child ToolTip control used by a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetToolTips( m_hWnd );
	}

	inline UINT CTreeView::GetVisibleCount() const
	// Obtains the number of items that can be fully visible in the client window of a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_GetVisibleCount( m_hWnd );
	}

	inline BOOL CTreeView::ItemHasChildren(HTREEITEM hItem) const
	// Returns true of the tree-view item has one or more children
	{
		assert(::IsWindow(m_hWnd));

		if (TreeView_GetChild( m_hWnd, hItem ))
			return TRUE;

		return FALSE;
	}

	inline COLORREF CTreeView::SetBkColor(COLORREF clrBk) const
	// Sets the background color of the control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetBkColor( m_hWnd, clrBk );
	}

	inline HIMAGELIST CTreeView::SetImageList(HIMAGELIST himl, int nType) const
	// Sets the normal or state image list for a tree-view control
	//  and redraws the control using the new images.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetImageList( m_hWnd, himl, nType );
	}

	inline void CTreeView::SetIndent(int indent) const
	// Sets the width of indentation for a tree-view control
	//  and redraws the control to reflect the new width.
	{
		assert(::IsWindow(m_hWnd));
		TreeView_SetIndent( m_hWnd, indent );
	}

	inline BOOL CTreeView::SetInsertMark(HTREEITEM hItem, BOOL fAfter/* = TRUE*/) const
	// Sets the insertion mark in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetInsertMark( m_hWnd, hItem, fAfter );
	}

	inline COLORREF CTreeView::SetInsertMarkColor(COLORREF clrInsertMark) const
	// Sets the color used to draw the insertion mark for the tree view.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetInsertMarkColor( m_hWnd, clrInsertMark );
	}

	inline BOOL CTreeView::SetItem(TVITEM& Item) const
	// Sets some or all of a tree-view item's attributes.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetItem( m_hWnd, &Item );
	}

	inline BOOL CTreeView::SetItem(HTREEITEM hItem, UINT nMask, LPCTSTR szText, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam) const
	// Sets some or all of a tree-view item's attributes.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.mask  = nMask;
		tvi.pszText = (LPTSTR)szText;
		tvi.iImage  = nImage;
		tvi.iSelectedImage = nSelectedImage;
		tvi.state = nState;
		tvi.stateMask = nStateMask;
		tvi.lParam = lParam;
		return TreeView_SetItem( m_hWnd, &tvi );
	}

	inline BOOL CTreeView::SetItemData(HTREEITEM hItem, DWORD_PTR dwData) const
	// Sets the tree-view item's application data.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.mask = TVIF_PARAM;
		tvi.lParam = dwData;
		return TreeView_SetItem( m_hWnd, &tvi );
	}

	inline int  CTreeView::SetItemHeight(SHORT cyItem) const
	// Sets the height of the tree-view items.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetItemHeight( m_hWnd, cyItem );
	}

	inline BOOL CTreeView::SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage) const
	// Sets the tree-view item's application image.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.iImage = nImage;
		tvi.iSelectedImage = nSelectedImage;
		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		return TreeView_SetItem(m_hWnd, &tvi );
	}

	inline BOOL CTreeView::SetItemText(HTREEITEM hItem, LPCTSTR szText) const
	// Sets the tree-view item's application text.
	{
		assert(::IsWindow(m_hWnd));

		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.pszText = (LPTSTR)szText;
		tvi.mask = TVIF_TEXT;
		return TreeView_SetItem(m_hWnd, &tvi );
	}

	inline UINT CTreeView::SetScrollTime(UINT uScrollTime) const
	// Sets the maximum scroll time for the tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetScrollTime( m_hWnd, uScrollTime );
	}

	inline COLORREF CTreeView::SetTextColor(COLORREF clrText) const
	// Sets the text color of the control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetTextColor( m_hWnd, clrText );
	}

	inline HWND CTreeView::SetToolTips(HWND hwndTooltip) const
	// Sets a tree-view control's child ToolTip control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SetToolTips( m_hWnd, hwndTooltip );
	}

	// Operations

	inline HIMAGELIST CTreeView::CreateDragImage(HTREEITEM hItem) const
	// Creates a dragging bitmap for the specified item in a tree-view control.
	// It also creates an image list for the bitmap and adds the bitmap to the image list.
	// An application can display the image when dragging the item by using the image list functions.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_CreateDragImage( m_hWnd, hItem );
	}

	inline BOOL CTreeView::DeleteAllItems() const
	// Deletes all items from a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_DeleteAllItems( m_hWnd );
	}

	inline BOOL CTreeView::DeleteItem(HTREEITEM hItem) const
	// Removes an item and all its children from a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_DeleteItem( m_hWnd, hItem );
	}

	inline HWND CTreeView::EditLabel(HTREEITEM hItem) const
	// Begins in-place editing of the specified item's text, replacing the text of the item
	// with a single-line edit control containing the text.
	// The specified item  is implicitly selected and focused.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_EditLabel( m_hWnd, hItem );
	}

	inline BOOL CTreeView::EndEditLabelNow(BOOL fCancel) const
	// Ends the editing of a tree-view item's label.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_EndEditLabelNow(m_hWnd, fCancel);
	}

	inline BOOL CTreeView::EnsureVisible(HTREEITEM hItem) const
	// Ensures that a tree-view item is visible, expanding the parent item or
	// scrolling the tree-view control, if necessary.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_EnsureVisible( m_hWnd, hItem );
	}

	inline BOOL CTreeView::Expand(HTREEITEM hItem, UINT nCode) const
	// The TreeView_Expand macro expands or collapses the list of child items associated
	// with the specified parent item, if any.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_Expand( m_hWnd, hItem, nCode );
	}

	inline HTREEITEM CTreeView::HitTest(TVHITTESTINFO& ht) const
	// Determines the location of the specified point relative to the client area of a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_HitTest( m_hWnd, &ht );
	}

	inline HTREEITEM CTreeView::InsertItem(TVINSERTSTRUCT& tvIS) const
	// Inserts a new item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_InsertItem( m_hWnd, &tvIS );
	}

	inline BOOL CTreeView::Select(HTREEITEM hitem, UINT flag) const
	// Selects the specified tree-view item, scrolls the item into view, or redraws
	// the item in the style used to indicate the target of a drag-and-drop operation.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_Select(m_hWnd, hitem, flag );
	}

	inline BOOL CTreeView::SelectDropTarget(HTREEITEM hItem) const
	// Redraws a specified tree-view control item in the style used to indicate the
	// target of a drag-and-drop operation.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SelectDropTarget(m_hWnd, hItem);
	}

	inline BOOL CTreeView::SelectItem(HTREEITEM hItem) const
	// Selects the specified tree-view item.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SelectItem(m_hWnd, hItem);
	}

	inline BOOL CTreeView::SelectSetFirstVisible(HTREEITEM hItem) const
	// Scrolls the tree-view control vertically to ensure that the specified item is visible.
	// If possible, the specified item becomes the first visible item at the top of the control's window.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SelectSetFirstVisible(m_hWnd, hItem);
	}

	inline BOOL CTreeView::SortChildren(HTREEITEM hItem, BOOL fRecurse) const
	// Sorts the child items of the specified parent item in a tree-view control.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SortChildren( m_hWnd, hItem, fRecurse );
	}

	inline BOOL CTreeView::SortChildrenCB(TVSORTCB& sort, BOOL fRecurse) const
	// Sorts tree-view items using an application-defined callback function that compares the items.
	{
		assert(::IsWindow(m_hWnd));
		return TreeView_SortChildrenCB( m_hWnd, &sort, fRecurse );
	}


} // namespace Win32xx

#endif // #ifndef _WIN32XX_TREEVIEW_H_

