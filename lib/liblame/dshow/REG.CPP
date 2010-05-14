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

#include <Windows.h>

#include "reg.h"

namespace Lame
{

CRegKey::CRegKey(void)
{
    m_hRootKey = NULL;
    m_name[0] = 0;
    m_hKey = NULL;
}


CRegKey::CRegKey(HKEY rt, PTSTR pName)
{
    m_hRootKey = rt;
    m_hKey = NULL;
    if(pName)
    {
        lstrcpy(m_name, pName);
        Open(m_hRootKey, m_name);
    }
    else
        m_name[0] = 0;
}


CRegKey::~CRegKey(void)
{
    Close();
}



BOOL    CRegKey::Open(HKEY rootKey, PTSTR pName)
{
    if(m_hKey)
        Close();

    m_hRootKey = rootKey;
    if(pName) 
    {
        lstrcpy(m_name, pName);
        if(RegOpenKeyEx(m_hRootKey, m_name, 0, KEY_ALL_ACCESS, &m_hKey) != ERROR_SUCCESS) 
        {
            m_hKey = NULL;
            return FALSE;
        }
    }
    else 
    {
        m_name[0] = 0;
        m_hKey = m_hRootKey;
    }

    return TRUE;
}


BOOL    CRegKey::Create(HKEY rootKey, PTSTR pName)
{
    if(m_hKey)
        Close();

    m_hRootKey = rootKey;
    if(pName) 
    {
        lstrcpy(m_name, pName);
        if(RegCreateKeyEx(m_hRootKey, pName, NULL,
                TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &m_hKey, NULL) != ERROR_SUCCESS) 
        {

            m_hKey = NULL;
            return FALSE;
        }
    }
    else 
    {
        m_name[0] = 0;
    }
    m_hRootKey = m_hKey;

    return TRUE;
}


BOOL    CRegKey::Open(PTSTR an)
{
    TCHAR achName[MAX_PATH];

    if(m_hKey)
        Close();

    lstrcpy(achName, m_name);
    if(an)
        lstrcat(achName, an);

    if(RegOpenKeyEx(m_hRootKey, achName, 0, KEY_ALL_ACCESS, &m_hKey) != ERROR_SUCCESS) 
    {
        m_hKey = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOL    CRegKey::Create(PTSTR an)
{
    TCHAR achName[MAX_PATH];

    if(m_hKey)
        Close();

    lstrcpy(achName, m_name);
    if(an)
        lstrcat(achName, an);

    if(RegCreateKeyEx(m_hRootKey, achName, NULL,
            TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &m_hKey, NULL) != ERROR_SUCCESS) 
    {

        m_hKey = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOL    CRegKey::Close(void)
{
    if(m_hKey && m_hKey != m_hRootKey)
        RegCloseKey(m_hKey);

    m_hKey = 0;
    return TRUE;
}

BOOL    CRegKey::getFlag(PTSTR valuename, BOOL bDefault)
{
    if(!m_hKey)
        return bDefault;

    DWORD cbData;
    DWORD   dwData;
    DWORD   dwType;

    cbData = sizeof(dwData);
    if(RegQueryValueEx(m_hKey, valuename, NULL, &dwType, (PBYTE)&dwData, &cbData) == ERROR_SUCCESS) 
    {
        if(dwType == REG_DWORD)
            return (dwData) ? TRUE : FALSE;
    }
    return bDefault;
}


void    CRegKey::setFlag(PTSTR valuename, BOOL bValue, BOOL bDefault)
{
    if(getFlag(valuename, bDefault) == bValue )
        return;

    RegSetValueEx(m_hKey, valuename, 0, REG_DWORD, (PBYTE)&bValue, sizeof(bValue));
}


void    CRegKey::setFlag(PTSTR valuename, BOOL bValue)
{
    RegSetValueEx(m_hKey, valuename, 0, REG_DWORD, (PBYTE)&bValue, sizeof(bValue));
}


DWORD   CRegKey::getDWORD(PTSTR valuename, DWORD bDefault)
{
    DWORD dwData;
    DWORD cbData;
    DWORD   dwType;

    if(!m_hKey)
        return bDefault;

    cbData = sizeof(dwData);
    if(RegQueryValueEx(m_hKey, valuename, NULL, &dwType, (PBYTE)&dwData, &cbData) == ERROR_SUCCESS) {
        if(dwType == REG_DWORD) 
        {
            return (UINT)dwData;
        }
    }

    return bDefault;
}


void    CRegKey::setDWORD(PTSTR valuename, DWORD dwValue, DWORD dwDefault)
{
    DWORD dwData = dwValue;

    if(getDWORD(valuename, dwDefault) == dwValue)
        return;

    RegSetValueEx(m_hKey, valuename, 0, REG_DWORD, (PBYTE)&dwData, sizeof(dwData));
}


void    CRegKey::setDWORD(PTSTR valuename, DWORD dwValue)
{
    DWORD dwData = dwValue;
    RegSetValueEx(m_hKey, valuename, 0, REG_DWORD, (PBYTE)&dwData, sizeof(dwData));
}


DWORD CRegKey::getString(PTSTR valuename, PTSTR pDefault, PTSTR pResult, int cbSize)
{
    DWORD dwType;

    cbSize *= sizeof(TCHAR);    // for unicode strings

    if(m_hKey) 
    {
        if(RegQueryValueEx(m_hKey, valuename, NULL, &dwType, (LPBYTE)pResult, (LPDWORD)&cbSize) == ERROR_SUCCESS) 
        {
            if(dwType == REG_SZ) 
            {
                return(cbSize - 1);
            }
        }
    }
    lstrcpy(pResult, pDefault);
    return lstrlen(pDefault);
}


void    CRegKey::setString(PTSTR valuename, PTSTR pData)
{
    RegSetValueEx(m_hKey, valuename, 0, REG_SZ, (LPBYTE)pData, (lstrlen(pData) + 1)*sizeof(TCHAR));
}


DWORD CRegKey::getBinary(PTSTR valuename, PVOID pDefault, PVOID pResult, int cbSize)
{
    DWORD dwType;

    if(RegQueryValueEx(m_hKey, valuename, NULL, &dwType, (LPBYTE)pResult, (LPDWORD)&cbSize) == ERROR_SUCCESS) 
    {
        if(dwType == REG_BINARY) 
        {
            return cbSize;
        }
    }

    memmove(pResult, pDefault, cbSize);
    return cbSize;
}


DWORD CRegKey::setBinary(PTSTR valuename, PVOID pData, int cbSize)
{
    RegSetValueEx(m_hKey, valuename, 0, REG_BINARY, (LPBYTE)pData, cbSize);
    return cbSize;
}

} // namespace Lame
