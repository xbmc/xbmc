/*
 *  LAME MP3 encoder for DirectShow
 *  Registry calls handling class
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef	__REG__
#define	__REG__

namespace Lame
{
class	CRegKey 
{
protected:
	TCHAR	m_name[MAX_PATH];
	HKEY	m_hKey;
	HKEY	m_hRootKey;
public:
	CRegKey(void);
	CRegKey(HKEY rootKey, PTSTR pName);
	~CRegKey(void);

	BOOL	Open(HKEY rootKey, PTSTR pName);
	BOOL	Create(HKEY rootKey, PTSTR pName);
	BOOL	Open(PTSTR an = NULL);
	BOOL	Create(PTSTR an = NULL);
	BOOL	Close(void);

	operator HKEY () const { return m_hKey; };

	BOOL	getFlag(PTSTR valuename, BOOL bDefault);
	void	setFlag(PTSTR valuename, BOOL bValue, BOOL bDefault);
	void	setFlag(PTSTR valuename, BOOL bValue);
	DWORD	getDWORD(PTSTR valuename, DWORD bDefault);
	void	setDWORD(PTSTR valuename, DWORD dwValue);
	void	setDWORD(PTSTR valuename, DWORD dwValue, DWORD dwDefault);
	DWORD	getString(PTSTR valuename, PTSTR pDefault, PTSTR pResult, int cbSize);
	void	setString(PTSTR valuename, PTSTR pData);
	DWORD	getBinary(PTSTR valuename, PVOID pDefault, PVOID pResult, int cbSize);
	DWORD	setBinary(PTSTR valuename, PVOID pData, int cbSize);
};

class CRegEnumKey 
{
public:
	CRegEnumKey(HKEY hKey)
	{
		m_hKey = hKey;
		m_dwIndex = 0;
	}

	~CRegEnumKey()
	{
	}

	LONG Next(LPTSTR pszStr, DWORD cbName)
	{
		FILETIME	ftLastWriteTime;
		LONG lRet =  RegEnumKeyEx(m_hKey, m_dwIndex, pszStr, 
						&cbName, NULL, NULL, NULL, &ftLastWriteTime); 
 
		m_dwIndex++;
		return lRet;
	}


	void Reset(void)
	{
		m_dwIndex = 0;
	}
protected: 
	HKEY	m_hKey;
	DWORD	m_dwIndex;
};
}
#endif // __REG__
