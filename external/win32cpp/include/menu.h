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
// menu.h
//  Declaration of the CMenu class

// Notes
//  1) Owner-drawn menus send the WM_MEASUREITEM and WM_DRAWITEM messages
//     to the window that owns the menu. To manage owner drawing for menus,
//     handle these two messages in the CWnd's WndProc function.
//
//  2) The CMenu pointer returned by FromHandle might be a temporary pointer. It
//     should be used immediately, not saved for later use.
//
//  3) The CMenu pointers returned by FromHandle or GetSubMenu do not need
//     to be deleted. They are automatically deleted by the Win32++.
//
//  4) CMenu pointers returned by GetSubMenu are deleted when the parent CMenu is
//     detached, destroyed or deconstructed.
//
//  5) The HMENU that is attached to a CMenu object (using the attach function) is 
//     automatically deleted when the CMenu object goes out of scope. Detach the
//     HMENU to stop it being deleted when CMenu's destructor is called.
//
//  6) Pass CMenu objects by reference or by pointer when passing them as function 
//     arguments.
//
//  7) In those functions that use a MENUITEMINFO structure, its cbSize member is 
//     automatically set to the correct value.

//  Program sample
//  --------------
//	void CView::CreatePopup()
//	{
// 		CPoint pt = GetCursorPos();
// 	
// 		// Create the menu
// 		CMenu Popup;
// 		Popup.CreatePopupMenu();
// 
// 		// Add some menu items
// 		Popup.AppendMenu(MF_STRING, 101, _T("Menu Item &1"));
// 		Popup.AppendMenu(MF_STRING, 102, _T("Menu Item &2"));
// 		Popup.AppendMenu(MF_STRING, 103, _T("Menu Item &3"));
// 		Popup.AppendMenu(MF_SEPARATOR);
// 		Popup.AppendMenu(MF_STRING, 104, _T("Menu Item &4"));
// 
// 		// Set menu item states
// 		Popup.CheckMenuRadioItem(101, 101, 101, MF_BYCOMMAND);
// 		Popup.CheckMenuItem(102, MF_BYCOMMAND | MF_CHECKED);
// 		Popup.EnableMenuItem(103, MF_BYCOMMAND | MF_GRAYED);
// 		Popup.SetDefaultItem(104);
//	
// 		// Display the popup menu
// 		Popup.TrackPopupMenu(0, pt.x, pt.y, this); 
//	}



#if !defined(_WIN32XX_MENU_H_) && !defined(_WIN32_WCE)
#define _WIN32XX_MENU_H_


#include "wincore.h"
#include "gdi.h"


namespace Win32xx
{

	// Forward declarations
	class CBitmap;

	class CMenu
	{
		friend class CWinApp;

	public:
		//Construction
		CMenu() : m_hMenu(0), m_IsTmpMenu(FALSE) {}
		CMenu(UINT nID) : m_IsTmpMenu(FALSE) 
		{
			m_hMenu = ::LoadMenu(GetApp()->GetResourceHandle(), MAKEINTRESOURCE(nID));
		}
		~CMenu();

		//Initialization
		void Attach(HMENU hMenu);
		void CreateMenu();
		void CreatePopupMenu();
		void DestroyMenu();
		HMENU Detach();
		HMENU GetHandle() const;
		BOOL LoadMenu(LPCTSTR lpszResourceName);
		BOOL LoadMenu(UINT uIDResource);
		BOOL LoadMenuIndirect(const void* lpMenuTemplate);

		//Menu Operations
		BOOL TrackPopupMenu(UINT uFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = 0);
		BOOL TrackPopupMenuEx(UINT uFlags, int x, int y, CWnd* pWnd, LPTPMPARAMS lptpm);

		//Menu Item Operations
		BOOL AppendMenu(UINT uFlags, UINT_PTR uIDNewItem = 0, LPCTSTR lpszNewItem = NULL);
		BOOL AppendMenu(UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp);
		UINT CheckMenuItem(UINT uIDCheckItem, UINT uCheck);
		BOOL CheckMenuRadioItem(UINT uIDFirst, UINT uIDLast, UINT uIDItem, UINT uFlags);
		BOOL DeleteMenu(UINT uPosition, UINT uFlags);
		UINT EnableMenuItem(UINT uIDEnableItem, UINT uEnable);
		UINT GetDefaultItem(UINT gmdiFlags, BOOL fByPos = FALSE);
		DWORD GetMenuContextHelpId() const;

#if(WINVER >= 0x0500)	// Minimum OS required is Win2000
		BOOL GetMenuInfo(LPMENUINFO lpcmi) const;
		BOOL SetMenuInfo(LPCMENUINFO lpcmi);
#endif

		UINT GetMenuItemCount() const;
		UINT GetMenuItemID(int nPos) const;
		BOOL GetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
		UINT GetMenuState(UINT uID, UINT uFlags) const;
		int GetMenuString(UINT uIDItem, LPTSTR lpString, int nMaxCount, UINT uFlags) const;
		int GetMenuString(UINT uIDItem, CString& rString, UINT uFlags) const;
		CMenu* GetSubMenu(int nPos);
		BOOL InsertMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem = 0, LPCTSTR lpszNewItem = NULL);
		BOOL InsertMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp);
		BOOL InsertMenuItem(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);
		BOOL ModifyMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem = 0, LPCTSTR lpszNewItem = NULL);
		BOOL ModifyMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp);
		BOOL RemoveMenu(UINT uPosition, UINT uFlags);
		BOOL SetDefaultItem(UINT uItem, BOOL fByPos = FALSE);
		BOOL SetMenuContextHelpId(DWORD dwContextHelpId);
		BOOL SetMenuItemBitmaps(UINT uPosition, UINT uFlags, const CBitmap* pBmpUnchecked, const CBitmap* pBmpChecked);
		BOOL SetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos = FALSE);

		//Operators
		BOOL operator != (const CMenu& menu) const;
		BOOL operator == (const CMenu& menu) const;
		operator HMENU () const;

	private:
		CMenu(const CMenu&);				// Disable copy construction
		CMenu& operator = (const CMenu&);	// Disable assignment operator
		void AddToMap();
		BOOL RemoveFromMap();
		std::vector<MenuPtr> m_vSubMenus;	// A vector of smart pointers to CMenu
		HMENU m_hMenu;
		BOOL m_IsTmpMenu;
	};

	inline CMenu::~CMenu()
	{
		if (m_hMenu)
		{	
			if (!m_IsTmpMenu)
			{
				::DestroyMenu(m_hMenu);
			}
			
			RemoveFromMap();
		}

		m_vSubMenus.clear();
	}

	inline void CMenu::AddToMap()
	// Store the HMENU and CMenu pointer in the HMENU map
	{
		assert( GetApp() );
		assert(m_hMenu);
		
		GetApp()->m_csMapLock.Lock();
		GetApp()->m_mapHMENU.insert(std::make_pair(m_hMenu, this));
		GetApp()->m_csMapLock.Release();
	}

	inline BOOL CMenu::RemoveFromMap()
	{
		BOOL Success = FALSE;

		if (GetApp())
		{
			// Allocate an iterator for our HDC map
			std::map<HMENU, CMenu*, CompareHMENU>::iterator m;

			CWinApp* pApp = GetApp();
			if (pApp)
			{
				// Erase the CDC pointer entry from the map
				pApp->m_csMapLock.Lock();
				for (m = pApp->m_mapHMENU.begin(); m != pApp->m_mapHMENU.end(); ++m)
				{
					if (this == m->second)
					{
						pApp->m_mapHMENU.erase(m);
						Success = TRUE;
						break;
					}
				}

				pApp->m_csMapLock.Release();
			}
		}

		return Success;
	}
	
	
	inline BOOL CMenu::AppendMenu(UINT uFlags, UINT_PTR uIDNewItem /*= 0*/, LPCTSTR lpszNewItem /*= NULL*/)
	// Appends a new item to the end of the specified menu bar, drop-down menu, submenu, or shortcut menu.
	{
		assert(IsMenu(m_hMenu));
		return ::AppendMenu(m_hMenu, uFlags, uIDNewItem, lpszNewItem);
	}

	inline BOOL CMenu::AppendMenu(UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp)
	// Appends a new item to the end of the specified menu bar, drop-down menu, submenu, or shortcut menu.
	{
		assert(IsMenu(m_hMenu));
		assert(pBmp);
		return ::AppendMenu(m_hMenu, uFlags, uIDNewItem, (LPCTSTR)pBmp->GetHandle());
	}

	inline void CMenu::Attach(HMENU hMenu)
	// Attaches an existing menu to this CMenu
	{
		if (m_hMenu != NULL && m_hMenu != hMenu)
		{
			::DestroyMenu(Detach());
		}

		if (hMenu)
		{
			m_hMenu = hMenu;
			AddToMap();
		}
	}
	
	inline UINT CMenu::CheckMenuItem(UINT uIDCheckItem, UINT uCheck)
	// Sets the state of the specified menu item's check-mark attribute to either selected or clear.
	{
		assert(IsMenu(m_hMenu));
		return ::CheckMenuItem(m_hMenu, uIDCheckItem, uCheck);
	}

	inline BOOL CMenu::CheckMenuRadioItem(UINT uIDFirst, UINT uIDLast, UINT uIDItem, UINT uFlags)
	// Checks a specified menu item and makes it a radio item. At the same time, the function clears 
	//  all other menu items in the associated group and clears the radio-item type flag for those items.
	{
		assert(IsMenu(m_hMenu));
		return ::CheckMenuRadioItem(m_hMenu, uIDFirst, uIDLast, uIDItem, uFlags);
	}	

	inline void CMenu::CreateMenu()
	// Creates an empty menu.
	{
		assert(NULL == m_hMenu);
		m_hMenu = ::CreateMenu();
		AddToMap();
	}

	inline void CMenu::CreatePopupMenu()
	// Creates a drop-down menu, submenu, or shortcut menu. The menu is initially empty.
	{
		assert(NULL == m_hMenu);
		m_hMenu = ::CreatePopupMenu();
		AddToMap();
	}
	
	inline BOOL CMenu::DeleteMenu(UINT uPosition, UINT uFlags)
	// Deletes an item from the specified menu.
	{
		assert(IsMenu(m_hMenu));
		return ::DeleteMenu(m_hMenu, uPosition, uFlags);
	}	

	inline void CMenu::DestroyMenu()
	// Destroys the menu and frees any memory that the menu occupies.
	{
		if (::IsMenu(m_hMenu)) 
			::DestroyMenu(m_hMenu);
		
		m_hMenu = 0;
		RemoveFromMap();
		m_vSubMenus.clear();
	}

	inline HMENU CMenu::Detach()
	// Detaches the HMENU from this CMenu. If the HMENU is not detached it will be 
	// destroyed when this CMenu is deconstructed.
	{
		assert(IsMenu(m_hMenu));
		HMENU hMenu = m_hMenu;
		m_hMenu = 0;
		RemoveFromMap();
		m_vSubMenus.clear();
		return hMenu;
	}

	inline HMENU CMenu::GetHandle() const
	// Returns the HMENU assigned to this CMenu
	{
		return m_hMenu;
	}

	inline UINT CMenu::EnableMenuItem(UINT uIDEnableItem, UINT uEnable)
	// Enables, disables, or grays the specified menu item.
	// The uEnable parameter must be a combination of either MF_BYCOMMAND or MF_BYPOSITION
	// and MF_ENABLED, MF_DISABLED, or MF_GRAYED.
	{
		assert(IsMenu(m_hMenu));
		return ::EnableMenuItem(m_hMenu, uIDEnableItem, uEnable);
	}
	
	inline UINT CMenu::GetDefaultItem(UINT gmdiFlags, BOOL fByPos /*= FALSE*/)
	// Determines the default menu item.
	// The gmdiFlags parameter specifies how the function searches for menu items. 
	// This parameter can be zero or more of the following values: GMDI_GOINTOPOPUPS; GMDI_USEDISABLED.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuDefaultItem(m_hMenu, fByPos, gmdiFlags);
	}

	inline DWORD CMenu::GetMenuContextHelpId() const
	// Retrieves the Help context identifier associated with the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuContextHelpId(m_hMenu);
	}

#if(WINVER >= 0x0500)
// minimum OS required : Win2000

	inline BOOL CMenu::GetMenuInfo(LPMENUINFO lpcmi) const
	// Retrieves the menu information.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuInfo(m_hMenu, lpcmi);
	}

	inline BOOL CMenu::SetMenuInfo(LPCMENUINFO lpcmi)
	// Sets the menu information from the specified MENUINFO structure.
	{
		assert(IsMenu(m_hMenu));
		return ::SetMenuInfo(m_hMenu, lpcmi);
	}

#endif

	inline UINT CMenu::GetMenuItemCount() const
	// Retrieves the number of menu items.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuItemCount(m_hMenu);
	}

	inline UINT CMenu::GetMenuItemID(int nPos) const
	// Retrieves the menu item identifier of a menu item located at the specified position
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuItemID(m_hMenu, nPos);
	}

	inline BOOL CMenu::GetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos /*= FALSE*/)
	// retrieves information about the specified menu item.
	{
		assert(IsMenu(m_hMenu));
		assert(lpMenuItemInfo);
		lpMenuItemInfo->cbSize = GetSizeofMenuItemInfo();
		return ::GetMenuItemInfo(m_hMenu, uItem, fByPos, lpMenuItemInfo);
	}

	inline UINT CMenu::GetMenuState(UINT uID, UINT uFlags) const
	// Retrieves the menu flags associated with the specified menu item.
	// Possible values for uFlags are: MF_BYCOMMAND (default) or MF_BYPOSITION.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuState(m_hMenu, uID, uFlags);
	}

	inline int CMenu::GetMenuString(UINT uIDItem, LPTSTR lpString, int nMaxCount, UINT uFlags) const
	// Copies the text string of the specified menu item into the specified buffer.
	{
		assert(IsMenu(m_hMenu));
		assert(lpString);
		return ::GetMenuString(m_hMenu, uIDItem, lpString, nMaxCount, uFlags);
	}

	inline int CMenu::GetMenuString(UINT uIDItem, CString& rString, UINT uFlags) const
	// Copies the text string of the specified menu item into the specified buffer.
	{
		assert(IsMenu(m_hMenu));
		return ::GetMenuString(m_hMenu, uIDItem, (LPTSTR)rString.c_str(), rString.GetLength(), uFlags);
	}

	inline CMenu* CMenu::GetSubMenu(int nPos)
	// Retrieves the CMenu object of a pop-up menu.
	{
		assert(IsMenu(m_hMenu));
		CMenu* pMenu = new CMenu;
		pMenu->m_hMenu = ::GetSubMenu(m_hMenu, nPos);
		pMenu->m_IsTmpMenu = TRUE;
		m_vSubMenus.push_back(pMenu);
		return pMenu;
	}

	inline BOOL CMenu::InsertMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem /*= 0*/, LPCTSTR lpszNewItem /*= NULL*/)
	// Inserts a new menu item into a menu, moving other items down the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, uPosition, uFlags, uIDNewItem, lpszNewItem);
	}

	inline BOOL CMenu::InsertMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp)
	// Inserts a new menu item into a menu, moving other items down the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, uPosition, uFlags, uIDNewItem, (LPCTSTR)pBmp->GetHandle());
	}

	inline BOOL CMenu::InsertMenuItem(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos /*= FALSE*/)
	// Inserts a new menu item at the specified position in a menu.
	{
		assert(IsMenu(m_hMenu));
		assert(lpMenuItemInfo);
		lpMenuItemInfo->cbSize = GetSizeofMenuItemInfo();
		return ::InsertMenuItem(m_hMenu, uItem, fByPos, lpMenuItemInfo);
	}
	
	inline BOOL CMenu::LoadMenu(LPCTSTR lpszResourceName)
	// Loads the menu from the specified windows resource.
	{
		assert(NULL == m_hMenu);
		assert(lpszResourceName);
		m_hMenu = ::LoadMenu(GetApp()->GetResourceHandle(), lpszResourceName);
		if (m_hMenu) AddToMap();
		return NULL != m_hMenu;
	}

	inline BOOL CMenu::LoadMenu(UINT uIDResource)
	// Loads the menu from the specified windows resource.
	{
		assert(NULL == m_hMenu);
		m_hMenu = ::LoadMenu(GetApp()->GetResourceHandle(), MAKEINTRESOURCE(uIDResource));
		if (m_hMenu) AddToMap();
		return NULL != m_hMenu;
	}

	inline BOOL CMenu::LoadMenuIndirect(const void* lpMenuTemplate)
	// Loads the specified menu template and assigns it to this CMenu.
	{
		assert(NULL == m_hMenu);
		assert(lpMenuTemplate);
		m_hMenu = ::LoadMenuIndirect(lpMenuTemplate);
		if (m_hMenu) AddToMap();
		return NULL != m_hMenu;
	}	

	inline BOOL CMenu::ModifyMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem /*= 0*/, LPCTSTR lpszNewItem /*= NULL*/)
	// Changes an existing menu item. This function is used to specify the content, appearance, and behavior of the menu item.
	{
		assert(IsMenu(m_hMenu));
		return ::ModifyMenu(m_hMenu, uPosition, uFlags, uIDNewItem, lpszNewItem);
	}

	inline BOOL CMenu::ModifyMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, const CBitmap* pBmp)
	// Changes an existing menu item. This function is used to specify the content, appearance, and behavior of the menu item.
	{
		assert(IsMenu(m_hMenu));
		assert(pBmp);
		return ::ModifyMenu(m_hMenu, uPosition, uFlags, uIDNewItem, (LPCTSTR)pBmp->GetHandle());
	}

	inline BOOL CMenu::RemoveMenu(UINT uPosition, UINT uFlags)
	// Deletes a menu item or detaches a submenu from the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::RemoveMenu(m_hMenu, uPosition, uFlags);
	}

	inline BOOL CMenu::SetDefaultItem(UINT uItem, BOOL fByPos /*= FALSE*/)
	//  sets the default menu item for the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::SetMenuDefaultItem(m_hMenu, uItem, fByPos);
	}

	inline BOOL CMenu::SetMenuContextHelpId(DWORD dwContextHelpId)
	// Associates a Help context identifier with the menu.
	{
		assert(IsMenu(m_hMenu));
		return ::SetMenuContextHelpId(m_hMenu, dwContextHelpId);
	}

	inline BOOL CMenu::SetMenuItemBitmaps(UINT uPosition, UINT uFlags, const CBitmap* pBmpUnchecked, const CBitmap* pBmpChecked)
	// Associates the specified bitmap with a menu item.
	{
		assert(IsMenu(m_hMenu));
		return ::SetMenuItemBitmaps(m_hMenu, uPosition, uFlags, *pBmpUnchecked, *pBmpChecked);
	}

	inline BOOL CMenu::SetMenuItemInfo(UINT uItem, LPMENUITEMINFO lpMenuItemInfo, BOOL fByPos /*= FALSE*/)
	// Changes information about a menu item.
	{
		assert(IsMenu(m_hMenu));
		assert(lpMenuItemInfo);
		lpMenuItemInfo->cbSize = GetSizeofMenuItemInfo();
		return ::SetMenuItemInfo(m_hMenu, uItem, fByPos, lpMenuItemInfo);
	}
		
	inline BOOL CMenu::TrackPopupMenu(UINT uFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect /*= 0*/)
	// Displays a shortcut menu at the specified location and tracks the selection of items on the menu.
	{
		assert(IsMenu(m_hMenu));
		HWND hWnd = pWnd? pWnd->GetHwnd() : 0;
		return ::TrackPopupMenu(m_hMenu, uFlags, x, y, 0, hWnd, lpRect);
	}

	inline BOOL CMenu::TrackPopupMenuEx(UINT uFlags, int x, int y, CWnd* pWnd, LPTPMPARAMS lptpm)
	// Displays a shortcut menu at the specified location and tracks the selection of items on the shortcut menu.
	{
		assert(IsMenu(m_hMenu));
		HWND hWnd = pWnd? pWnd->GetHwnd() : 0;
		return ::TrackPopupMenuEx(m_hMenu, uFlags, x, y, hWnd, lptpm);
	}

	inline BOOL CMenu::operator != (const CMenu& menu) const
	// Returns TRUE if the two menu objects are not equal.
	{
		return menu.m_hMenu != m_hMenu;
	}

	inline BOOL CMenu::operator == (const CMenu& menu) const
	// Returns TRUE of the two menu object are equal
	{
		return menu.m_hMenu == m_hMenu;
	}

	inline CMenu::operator HMENU () const
	// Retrieves the menu's handle.
	{
		return m_hMenu;
	}
	
	
	///////////////////////////////////////
	// Global functions
	//
	
	inline CMenu* FromHandle(HMENU hMenu)
	// Returns the CMenu object associated with the menu handle (HMENU).
	{
		assert( GetApp() );
		CMenu* pMenu = GetApp()->GetCMenuFromMap(hMenu);
		if (::IsMenu(hMenu) && pMenu == 0)
		{
			GetApp()->AddTmpMenu(hMenu);
			pMenu = GetApp()->GetCMenuFromMap(hMenu);
		}
		return pMenu;
	}

}	// namespace Win32xx

#endif	// _WIN32XX_MENU_H_

