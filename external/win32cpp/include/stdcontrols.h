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
// stdcontrols.h
//  Declaration of the CButton, CEdit, CListBox and CStatic classes

// The Button, Edit, ListBox and Static controls are often referred to 
// as "standard controls". These set of older controls were originally 
// developed for Win16 operating systems (Windows 3.1 and 3.11). They use an
// older form of notification, and send their notifications via a WM_COMMAND
// message. Newer controls send their notifications via a WM_NOTIFY message.


#ifndef _WIN32XX_STDCONTROLS_H_
#define _WIN32XX_STDCONTROLS_H_

#include "wincore.h"


namespace Win32xx
{
	class CButton : public CWnd
	{
	public:
		CButton() {}
		virtual ~CButton() {}

		// Attributes
		HBITMAP GetBitmap() const;
		UINT GetButtonStyle() const;
		int GetCheck() const;
		HCURSOR GetCursor() const;
		HICON GetIcon() const;
		UINT GetState() const;
		HBITMAP SetBitmap(HBITMAP hBitmap) const;
		void SetButtonStyle(DWORD dwStyle, BOOL bRedraw) const;
		void SetCheck(int nCheckState) const;
		HCURSOR SetCursor(HCURSOR hCursor) const;
		HICON SetIcon(HICON hIcon) const;
		void SetState(BOOL bHighlight) const;

	protected:
		// Overridables
		virtual void PreCreate(CREATESTRUCT& cs);
	};

	class CEdit : public CWnd
	{
	public:
		// Construction
		CEdit() {}
		virtual ~CEdit() {}

		// Attributes
		BOOL CanUndo() const;
		int CharFromPos(CPoint pt) const;
		int GetFirstVisibleLine() const;
		HLOCAL GetHandle() const;
		UINT GetLimitText() const;
		int GetLine(int nIndex, LPTSTR lpszBuffer) const;
		int GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const;
		int GetLineCount() const;
		DWORD GetMargins() const;
		BOOL GetModify() const;
		TCHAR GetPasswordChar() const;
		void GetRect(LPRECT lpRect) const;
		void GetSel(int& nStartChar, int& nEndChar) const;
		DWORD GetSel() const;
		CPoint PosFromChar(UINT nChar) const;
		void SetHandle(HLOCAL hBuffer) const;
		void SetLimitText(UINT nMax) const;
		void SetMargins(UINT nLeft, UINT nRight) const;
		void SetModify(BOOL bModified = TRUE) const;

		// Operations
		void EmptyUndoBuffer() const;
		BOOL FmtLines(BOOL bAddEOL) const;
		void LimitText(int nChars = 0) const;
		int LineFromChar(int nIndex = -1) const;
		int LineIndex(int nLine = -1) const;
		int LineLength(int nLine = -1) const;
		void LineScroll(int nLines, int nChars = 0) const;
		void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo) const;
		void SetPasswordChar(TCHAR ch) const;
		BOOL SetReadOnly(BOOL bReadOnly = TRUE) const;
		void SetRect(LPCRECT lpRect) const;
		void SetRectNP(LPCRECT lpRect) const;
		void SetSel(DWORD dwSelection, BOOL bNoScroll) const;
		void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll) const;
		BOOL SetTabStops(int nTabStops, LPINT rgTabStops) const;
		BOOL SetTabStops() const;
		BOOL SetTabStops(const int& cxEachStop) const;

		//Clipboard Operations
		void Clear() const;
		void Copy() const;
		void Cut() const;
		void Paste() const;
		void Undo() const;

	protected:
		// Overridables
		virtual void PreRegisterClass(WNDCLASS &wc);
	};

	class CListBox : public CWnd
	{
	public:
		CListBox() {}
		virtual ~CListBox() {}

		// General Operations
		int  GetCount() const;
		int  GetHorizontalExtent() const;
		DWORD GetItemData(int nIndex) const;
		void* GetItemDataPtr(int nIndex) const;
		int  GetItemHeight(int nIndex) const;
		int  GetItemRect(int nIndex, LPRECT lpRect) const;
		LCID GetLocale() const;
		int  GetSel(int nIndex) const;
		int  GetText(int nIndex, LPTSTR lpszBuffer) const;
		int  GetTextLen(int nIndex) const;
		int  GetTopIndex() const;
		UINT ItemFromPoint(CPoint pt, BOOL& bOutside ) const;
		void SetColumnWidth(int cxWidth) const;
		void SetHorizontalExtent(int cxExtent) const;
		int  SetItemData(int nIndex, DWORD dwItemData) const;
		int  SetItemDataPtr(int nIndex, void* pData) const;
		int  SetItemHeight(int nIndex, UINT cyItemHeight) const;
		LCID SetLocale(LCID nNewLocale) const;
		BOOL SetTabStops(int nTabStops, LPINT rgTabStops) const;
		void SetTabStops() const;
		BOOL SetTabStops(const int& cxEachStop) const;
		int  SetTopIndex(int nIndex) const;

		// Single-Selection Operations
		int  GetCurSel() const;
		int  SetCurSel(int nSelect) const;

		// Multiple-Selection Operations
		int  GetAnchorIndex() const;
		int  GetCaretIndex() const;
		int  GetSelCount() const;
		int  GetSelItems(int nMaxItems, LPINT rgIndex) const;
		int  SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem) const;
		void SetAnchorIndex(int nIndex) const;
		int  SetCaretIndex(int nIndex, BOOL bScroll) const;
		int  SetSel(int nIndex, BOOL bSelect) const;

		// String Operations
		int  AddString(LPCTSTR lpszItem) const;
		int  DeleteString(UINT nIndex) const;
		int  Dir(UINT attr, LPCTSTR lpszWildCard) const;
		int  FindString(int nStartAfter, LPCTSTR lpszItem) const;
		int  FindStringExact(int nIndexStart, LPCTSTR lpszFind) const;
		int  InsertString(int nIndex, LPCTSTR lpszItem) const;
		void ResetContent() const;
		int  SelectString(int nStartAfter, LPCTSTR lpszItem) const;

	protected:
		// Overridables
		virtual void PreRegisterClass(WNDCLASS &wc);
	};

	class CStatic : public CWnd
	{
	public:
		CStatic() {}
		virtual ~CStatic() {}

		// Operations
		HBITMAP  GetBitmap() const;
		HCURSOR GetCursor() const;
		HENHMETAFILE GetEnhMetaFile() const;
		HICON  GetIcon() const;
		HBITMAP SetBitmap(HBITMAP hBitmap) const;
		HCURSOR SetCursor(HCURSOR hCursor) const;
		HENHMETAFILE SetEnhMetaFile(HENHMETAFILE hMetaFile) const;
		HICON SetIcon(HICON hIcon) const;

	protected:
		// Overridables
		virtual void PreRegisterClass(WNDCLASS &wc);

	};

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	////////////////////////////////////////
	// Definitions for the CButton class
	//
	inline HBITMAP CButton::GetBitmap() const
	// returns the handle to the bitmap associated with the button
	{
		assert(::IsWindow(m_hWnd));
		return (HBITMAP)SendMessage(BM_GETIMAGE, IMAGE_BITMAP, 0);
	}

	inline UINT CButton::GetButtonStyle() const
	// returns the style of the button
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)GetWindowLongPtr(GWL_STYLE) & 0xff;
	}

	inline int CButton::GetCheck() const
	// returns the check state of the button
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(BM_GETCHECK, 0, 0);
	}

	inline HCURSOR CButton::GetCursor() const
	// returns the handle to the cursor associated withe the button
	{
		assert(::IsWindow(m_hWnd));
		return (HCURSOR)::SendMessage(m_hWnd, BM_GETIMAGE, IMAGE_CURSOR, 0L);
	}

	inline HICON CButton::GetIcon() const
	// returns the handle to the icon associated withe the button
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(BM_GETIMAGE, IMAGE_ICON, 0);
	}

	inline UINT CButton::GetState() const
	// returns the state of the button
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)SendMessage(BM_GETSTATE, 0, 0);
	}

	inline HBITMAP CButton::SetBitmap(HBITMAP hBitmap) const
	// sets the bitmap associated with the button
	{
		assert(::IsWindow(m_hWnd));
		return (HBITMAP)SendMessage(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
	}

	inline void CButton::SetButtonStyle(DWORD dwStyle, BOOL bRedraw) const
	// sets the button style
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(BM_SETSTYLE, dwStyle, bRedraw);
	}

	inline void CButton::SetCheck(int nCheckState) const
	// sets the button check state
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(BM_SETCHECK, nCheckState, 0);
	}

	inline HCURSOR CButton::SetCursor(HCURSOR hCursor) const
	// sets the cursor associated with the button
	{
		assert(::IsWindow(m_hWnd));
		return (HCURSOR)SendMessage(STM_SETIMAGE, IMAGE_CURSOR, (LPARAM)hCursor);
	}

	inline HICON CButton::SetIcon(HICON hIcon) const
	// sets the icon associated with the button
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage( BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
	}

	inline void CButton::SetState(BOOL bHighlight) const
	// sets the button state
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(BM_SETSTATE, bHighlight, 0);
	}

	inline void CButton::PreCreate(CREATESTRUCT& cs)
	{
		cs.lpszClass = _T("Button");
	}


	////////////////////////////////////////
	// Definitions for the CEdit class
	//
	inline BOOL CEdit::CanUndo() const
	// Returns TRUE if the edit control operation can be undone.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(EM_CANUNDO, 0, 0);
	}

	inline int CEdit::CharFromPos(CPoint pt) const
	// Returns the character index and line index of the character nearest the specified point.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_CHARFROMPOS, 0, MAKELPARAM(pt.x, pt.y));
	}

	inline int CEdit::GetFirstVisibleLine() const
	// Returns the zero-based index of the first visible character in a single-line edit control 
	//  or the zero-based index of the uppermost visible line in a multiline edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_GETFIRSTVISIBLELINE, 0, 0);
	}

	inline HLOCAL CEdit::GetHandle() const
	// Returns a handle identifying the buffer containing the multiline edit control's text. 
	//  It is not processed by single-line edit controls.
	{
		assert(::IsWindow(m_hWnd));
		return (HLOCAL)SendMessage(EM_GETHANDLE, 0, 0);
	}

	inline UINT CEdit::GetLimitText() const
	// Returns the current text limit, in characters.
	{
		assert(::IsWindow(m_hWnd));
		return (UINT)SendMessage(EM_GETLIMITTEXT, 0, 0);
	}

	inline int CEdit::GetLine(int nIndex, LPTSTR lpszBuffer) const
	// Copies characters to a buffer and returns the number of characters copied.
	{
		assert(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
	}

	inline int CEdit::GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const
	// Copies characters to a buffer and returns the number of characters copied.
	{
		assert(::IsWindow(m_hWnd));
		*(LPWORD)lpszBuffer = (WORD)nMaxLength;
		return (int)SendMessage(EM_GETLINE, nIndex, (LPARAM)lpszBuffer);
	}

	inline int CEdit::GetLineCount() const
	// Returns the number of lines in the edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_GETLINECOUNT, 0, 0);
	}

	inline DWORD CEdit::GetMargins() const
	// Returns the widths of the left and right margins.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(EM_GETMARGINS, 0, 0);
	}

	inline BOOL CEdit::GetModify() const
	// Returns a flag indicating whether the content of an edit control has been modified.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(EM_GETMODIFY, 0, 0);
	}

	inline TCHAR CEdit::GetPasswordChar() const
	// Returns the character that edit controls use in conjunction with the ES_PASSWORD style.
	{
		assert(::IsWindow(m_hWnd));
		return (TCHAR)SendMessage(EM_GETPASSWORDCHAR, 0, 0);
	}

	inline void CEdit::GetRect(LPRECT lpRect) const
	// Returns the coordinates of the formatting rectangle in an edit control.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_GETRECT, 0, (LPARAM)lpRect);
	}

	inline void CEdit::GetSel(int& nStartChar, int& nEndChar) const
	// Returns the starting and ending character positions of the current selection in the edit control.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_GETSEL, (WPARAM)&nStartChar,(LPARAM)&nEndChar);
	}

	inline DWORD CEdit::GetSel() const
	// Returns the starting and ending character positions of the current selection in the edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(EM_GETSEL, 0, 0);
	}

	inline CPoint CEdit::PosFromChar(UINT nChar) const
	// Returns the client coordinates of the specified character.
	{
		assert(::IsWindow(m_hWnd));
		return CPoint( (DWORD)SendMessage(EM_POSFROMCHAR, nChar, 0));
	}

	inline void CEdit::SetHandle(HLOCAL hBuffer) const
	// Sets a handle to the memory used as a text buffer, empties the undo buffer, 
	//  resets the scroll positions to zero, and redraws the window.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETHANDLE, (WPARAM)hBuffer, 0);
	}

	inline void CEdit::SetLimitText(UINT nMax) const
	// Sets the maximum number of characters the user may enter in the edit control.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETLIMITTEXT, (WPARAM)nMax, 0);
	}

	inline void CEdit::SetMargins(UINT nLeft, UINT nRight) const
	// Sets the widths of the left and right margins, and redraws the edit control to reflect the new margins.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELONG(nLeft, nRight));
	}

	inline void CEdit::SetModify(BOOL bModified) const
	// Sets or clears the modification flag to indicate whether the edit control has been modified.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETMODIFY, bModified, 0);
	}

	inline void CEdit::EmptyUndoBuffer() const
	// Empties the undo buffer and sets the undo flag retrieved by the EM_CANUNDO message to FALSE.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_EMPTYUNDOBUFFER, 0, 0);
	}

	inline BOOL CEdit::FmtLines(BOOL bAddEOL) const
	// Adds or removes soft line-break characters (two carriage returns and a line feed) to the ends of wrapped lines 
	//  in a multiline edit control. It is not processed by single-line edit controls.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(EM_FMTLINES, bAddEOL, 0);
	}

	inline void CEdit::LimitText(int nChars) const
	// Sets the text limit of an edit control. The text limit is the maximum amount of text, in TCHARs, 
	//  that the user can type into the edit control.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_LIMITTEXT, nChars, 0);
	}

	inline int CEdit::LineFromChar(int nIndex) const
	// Returns the zero-based number of the line in a multiline edit control that contains a specified character index.
	//  This message is the reverse of the EM_LINEINDEX message.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_LINEFROMCHAR, (WPARAM)nIndex, 0);
	}

	inline int CEdit::LineIndex(int nLine) const
	// Returns the character of a line in a multiline edit control. 
	// This message is the reverse of the EM_LINEFROMCHAR message
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_LINEINDEX, (WPARAM)nLine, 0);
	}

	inline int CEdit::LineLength(int nLine) const
	// Returns the length, in characters, of a single-line edit control. In a multiline edit control, 
	//	returns the length, in characters, of a specified line.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(EM_LINELENGTH, (WPARAM)nLine, 0);
	}

	inline void CEdit::LineScroll(int nLines, int nChars) const
	// Scrolls the text vertically in a single-line edit control or horizontally in a multiline edit control.
	{
		assert(::IsWindow(m_hWnd)); 
		SendMessage(EM_LINESCROLL, (WPARAM)nChars, (LPARAM)nLines);
	}

	inline void CEdit::ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo) const
	// Replaces the current selection with the text in an application-supplied buffer, sends the parent window 
	//  EN_UPDATE and EN_CHANGE messages, and updates the undo buffer.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM)lpszNewText);
	}

	inline void CEdit::SetPasswordChar(TCHAR ch) const
	// Defines the character that edit controls use in conjunction with the ES_PASSWORD style.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETPASSWORDCHAR, ch, 0);
	}

	inline BOOL CEdit::SetReadOnly(BOOL bReadOnly) const
	// Sets or removes the read-only style (ES_READONLY) in an edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(EM_SETREADONLY, bReadOnly, 0);
	}

	inline void CEdit::SetRect(LPCRECT lpRect) const
	// Sets the formatting rectangle for the multiline edit control and redraws the window.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETRECT, 0, (LPARAM)lpRect);
	}

	inline void CEdit::SetRectNP(LPCRECT lpRect) const
	// Sets the formatting rectangle for the multiline edit control but does not redraw the window.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETRECTNP, 0, (LPARAM)lpRect);
	}

	inline void CEdit::SetSel(DWORD dwSelection, BOOL bNoScroll) const
	// Selects a range of characters in the edit control by setting the starting and ending positions to be selected.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_SETSEL, LOWORD(dwSelection), HIWORD(dwSelection));
		if (!bNoScroll)
			SendMessage(EM_SCROLLCARET, 0, 0);
	}

	inline void CEdit::SetSel(int nStartChar, int nEndChar, BOOL bNoScroll) const
	// Selects a range of characters in the edit control by setting the starting and ending positions to be selected.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(m_hWnd, EM_SETSEL, nStartChar, nEndChar);
		if (!bNoScroll)
			SendMessage(EM_SCROLLCARET, 0, 0);
	}

	inline BOOL CEdit::SetTabStops(int nTabStops, LPINT rgTabStops) const
	// Sets tab-stop positions in the multiline edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, EM_SETTABSTOPS, nTabStops, (LPARAM)rgTabStops);
	}

	inline BOOL CEdit::SetTabStops() const
	// Sets tab-stop positions in the multiline edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage( EM_SETTABSTOPS, 0, 0);
	}

	inline BOOL CEdit::SetTabStops(const int& cxEachStop) const
	// Sets tab-stop positions in the multiline edit control.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(EM_SETTABSTOPS, 1, (LPARAM)(LPINT)&cxEachStop);
	}

	inline void CEdit::Clear() const
	// Clears the current selection, if any, in an edit control.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(WM_CLEAR, 0, 0);
	}

	inline void CEdit::Copy() const
	// Copies text to the clipboard unless the style is ES_PASSWORD, in which case the message returns zero.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(WM_COPY,  0, 0);
	}

	inline void CEdit::Cut() const
	// Cuts the selection to the clipboard, or deletes the character to the left of the cursor if there is no selection.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(WM_CUT,   0, 0);
	}

	inline void CEdit::Paste() const
	// Pastes text from the clipboard into the edit control window at the caret position.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(WM_PASTE, 0, 0);
	}

	inline void CEdit::Undo() const
	// Removes any text that was just inserted or inserts any deleted characters and sets the selection to the inserted text.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(EM_UNDO,  0, 0);
	}

	inline void CEdit::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  _T("Edit");
	}


	////////////////////////////////////////
	// Definitions for the CListbox class
	//
	inline int CListBox::GetCount() const
	// Returns the number of items in the list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETCOUNT, 0, 0);
	}

	inline int CListBox::GetHorizontalExtent() const
	// Returns the scrollable width, in pixels, of a list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETHORIZONTALEXTENT,	0, 0);
	}

	inline DWORD CListBox::GetItemData(int nIndex) const
	// Returns the value associated with the specified item.
	{
		assert(::IsWindow(m_hWnd));
		return (DWORD)SendMessage(LB_GETITEMDATA, nIndex, 0);
	}

	inline void* CListBox::GetItemDataPtr(int nIndex) const
	// Returns the value associated with the specified item.
	{
		assert(::IsWindow(m_hWnd));
		return (LPVOID)SendMessage(LB_GETITEMDATA, nIndex, 0);
	}

	inline int CListBox::GetItemHeight(int nIndex) const
	// Returns the height, in pixels, of an item in a list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETITEMHEIGHT, nIndex, 0L);
	}

	inline int CListBox::GetItemRect(int nIndex, LPRECT lpRect) const
	// Retrieves the client coordinates of the specified list box item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETITEMRECT, nIndex, (LPARAM)lpRect);
	}

	inline LCID CListBox::GetLocale() const
	// Retrieves the locale of the list box. The high-order word contains the country/region code 
	//  and the low-order word contains the language identifier.
	{
		assert(::IsWindow(m_hWnd));
		return (LCID)::SendMessage(m_hWnd, LB_GETLOCALE, 0, 0);
	}

	inline int CListBox::GetSel(int nIndex) const
	// Returns the selection state of a list box item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETSEL, nIndex, 0);
	}

	inline int CListBox::GetText(int nIndex, LPTSTR lpszBuffer) const
	// Retrieves the string associated with a specified item and the length of the string.
	{
		assert(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszBuffer);
	}

	inline int CListBox::GetTextLen(int nIndex) const
	// Returns the length, in characters, of the string associated with a specified item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage( LB_GETTEXTLEN, nIndex, 0);
	}

	inline int CListBox::GetTopIndex() const
	// Returns the index of the first visible item in a list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETTOPINDEX, 0, 0);
	}

	inline UINT CListBox::ItemFromPoint(CPoint pt, BOOL& bOutside) const
	// Retrieves the zero-based index of the item nearest the specified point in a list box.
	{
		assert(::IsWindow(m_hWnd));
		DWORD dw = (DWORD)::SendMessage(m_hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
		bOutside = !!HIWORD(dw);
		return LOWORD(dw);
	}

	inline void CListBox::SetColumnWidth(int cxWidth) const
	// Sets the width, in pixels, of all columns in a list box.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(LB_SETCOLUMNWIDTH, cxWidth, 0);
	}

	inline void CListBox::SetHorizontalExtent(int cxExtent) const
	// Sets the scrollable width, in pixels, of a list box.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(LB_SETHORIZONTALEXTENT, cxExtent, 0);
	}

	inline int CListBox::SetItemData(int nIndex, DWORD dwItemData) const
	// Associates a value with a list box item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETITEMDATA, nIndex, (LPARAM)dwItemData);
	}

	inline int CListBox::SetItemDataPtr(int nIndex, void* pData) const
	// Associates a value with a list box item.
	{
		assert(::IsWindow(m_hWnd));
		return SetItemData(nIndex, (DWORD)(DWORD_PTR)pData);
	}

	inline int CListBox::SetItemHeight(int nIndex, UINT cyItemHeight) const
	// Sets the height, in pixels, of an item or items in a list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0));
	}

	inline LCID CListBox::SetLocale(LCID nNewLocale) const
	// Sets the locale of a list box and returns the previous locale identifier.
	{
		assert(::IsWindow(m_hWnd));
		return (LCID)::SendMessage(m_hWnd, LB_SETLOCALE, (WPARAM)nNewLocale, 0);
	}

	inline BOOL CListBox::SetTabStops(int nTabStops, LPINT rgTabStops) const
	// Sets the tab stops to those specified in a specified array.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(LB_SETTABSTOPS, nTabStops, (LPARAM)rgTabStops);
	}

	inline void CListBox::SetTabStops() const
	// Sets the tab stops to those specified in a specified array.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(LB_SETTABSTOPS, 0, 0);
	}

	inline BOOL CListBox::SetTabStops(const int& cxEachStop) const
	// Sets the tab stops to those specified in a specified array.
	{
		assert(::IsWindow(m_hWnd));
		return (BOOL)SendMessage(LB_SETTABSTOPS, 1, (LPARAM)(LPINT)&cxEachStop);
	}

	inline int CListBox::SetTopIndex(int nIndex) const
	// Scrolls the list box so the specified item is at the top of the visible range.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETTOPINDEX, nIndex, 0);
	}

	inline int CListBox::GetCurSel() const
	// Returns the index of the currently selected item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETCURSEL, 0, 0);
	}

	inline int CListBox::SetCurSel(int nSelect) const
	// Selects a specified list box item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETCURSEL, nSelect, 0);
	}

	inline int CListBox::GetAnchorIndex() const
	// Returns the index of the item that the mouse last selected.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETANCHORINDEX, 0, 0);
	}

	inline int CListBox::GetCaretIndex() const
	// Returns the index of the item that has the focus rectangle.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETCARETINDEX, 0, 0L);
	}

	inline int CListBox::GetSelCount() const
	// Returns the number of selected items in a multiple-selection list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETSELCOUNT, 0, 0);
	}

	inline int CListBox::GetSelItems(int nMaxItems, LPINT rgIndex) const
	// Creates an array of the indexes of all selected items in a multiple-selection list box 
	//  and returns the total number of selected items.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_GETSELITEMS, nMaxItems, (LPARAM)rgIndex);
	}

	inline int CListBox::SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem) const
	// Selects a specified range of items in a list box.
	{
		assert(::IsWindow(m_hWnd));
		if (bSelect)
			return (int)SendMessage(LB_SELITEMRANGEEX, nFirstItem, nLastItem);
		else
			return (int)SendMessage(LB_SELITEMRANGEEX, nLastItem, nFirstItem);
	}

	inline void CListBox::SetAnchorIndex(int nIndex) const
	// Sets the item that the mouse last selected to a specified item.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(LB_SETANCHORINDEX, nIndex, 0);
	}

	inline int CListBox::SetCaretIndex(int nIndex, BOOL bScroll) const
	// Sets the focus rectangle to a specified list box item.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETCARETINDEX, nIndex, MAKELONG(bScroll, 0));
	}

	inline int CListBox::SetSel(int nIndex, BOOL bSelect) const
	// Selects an item in a multiple-selection list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_SETSEL, bSelect, nIndex);
	}

	inline int CListBox::AddString(LPCTSTR lpszItem) const
	// Adds a string to a list box and returns its index.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_ADDSTRING, 0, (LPARAM)lpszItem);
	}

	inline int CListBox::DeleteString(UINT nIndex) const
	// Removes a string from a list box and returns the number of strings remaining in the list.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_DELETESTRING, nIndex, 0);
	}

	inline int CListBox::Dir(UINT attr, LPCTSTR lpszWildCard) const
	// Adds a list of filenames to a list box and returns the index of the last filename added.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_DIR, attr, (LPARAM)lpszWildCard);
	}

	inline int CListBox::FindString(int nStartAfter, LPCTSTR lpszItem) const
	// Returns the index of the first string in the list box that begins with a specified string.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_FINDSTRING, nStartAfter, (LPARAM)lpszItem);
	}

	inline int CListBox::FindStringExact(int nIndexStart, LPCTSTR lpszFind) const
	// Returns the index of the string in the list box that is equal to a specified string.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind);
	}

	inline int CListBox::InsertString(int nIndex, LPCTSTR lpszItem) const
	// Inserts a string at a specified index in a list box.
	{
		assert(::IsWindow(m_hWnd));
		return (int)SendMessage(LB_INSERTSTRING, nIndex, (LPARAM)lpszItem);
	}

	inline void CListBox::ResetContent() const
	// Removes all items from a list box.
	{
		assert(::IsWindow(m_hWnd));
		SendMessage(LB_RESETCONTENT, 0, 0);
	}

	inline int CListBox::SelectString(int nStartAfter, LPCTSTR lpszItem) const
	// Selects the first string it finds that matches a specified prefix.
	{
		assert(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, LB_SELECTSTRING, nStartAfter, (LPARAM)lpszItem);
	}

	inline void CListBox::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  _T("ListBox");
	}


	////////////////////////////////////////
	// Definitions for the CStatic class
	//
	inline HBITMAP CStatic::GetBitmap() const
	// Returns the handle to the bitmap for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HBITMAP)SendMessage(STM_GETIMAGE, IMAGE_BITMAP, 0);
	}

	inline HCURSOR CStatic::GetCursor() const
	// Returns the handle to the icon for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HCURSOR)SendMessage(STM_GETIMAGE, IMAGE_CURSOR, 0);
	}

	inline HENHMETAFILE CStatic::GetEnhMetaFile() const
	// Returns the handle to the enhanced metafile for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HENHMETAFILE)SendMessage(STM_GETIMAGE, IMAGE_ENHMETAFILE, 0);
	}

	inline HICON CStatic::GetIcon() const
	// Returns the handle to the icon for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(STM_GETIMAGE, IMAGE_ICON, 0);
	}

	inline HBITMAP CStatic::SetBitmap(HBITMAP hBitmap) const
	// Sets the handle to the bitmap for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HBITMAP)SendMessage(STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
	}

	inline HCURSOR CStatic::SetCursor(HCURSOR hCursor) const
	// Sets the handle to the cursor for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HCURSOR)SendMessage(STM_SETIMAGE, IMAGE_CURSOR, (LPARAM)hCursor);
	}

	inline HENHMETAFILE CStatic::SetEnhMetaFile(HENHMETAFILE hMetaFile) const
	// Sets the handle to the enhanced metafile for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HENHMETAFILE)SendMessage(STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)hMetaFile);
	}

	inline HICON CStatic::SetIcon(HICON hIcon) const
	// Sets the handle to the icon for the static control
	{
		assert(::IsWindow(m_hWnd));
		return (HICON)SendMessage(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
	}

	inline void CStatic::PreRegisterClass(WNDCLASS &wc)
	{
		// Set the Window Class
		wc.lpszClassName =  _T("Static");
	}

}

#endif	// _WIN32XX_STDCONTROLS_H_

