/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//  RegKey.cpp:  implementation of RegKey class
//
//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif //_DEBUG

#include <DshowUtil/RegKey.h>
#include <sstream>
#include <memory>
#ifdef _DEBUG
#include <iostream>
#endif

#ifdef _DEBUG
// Include CRTDBG.H after all other headers
#include <stdlib.h>
#include <crtdbg.h>
#define NEW_INLINE_WORKAROUND new ( _NORMAL_BLOCK ,\
                                    __FILE__ , __LINE__ )
#define new NEW_INLINE_WORKAROUND
#endif

using namespace std;

RegKey::RegKey(HKEY base, const CStdString& pathname, bool bCreate) throw(SysError)
{
  HKEY hKey;
  int n;
  if(bCreate)
  {
    DWORD dwDummy;
    n = RegCreateKeyEx(base, pathname.c_str(), 0, "zello", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, &hKey, &dwDummy);
  }
  else
  {
    n = RegOpenKeyEx(base, pathname.c_str(), 0, KEY_ALL_ACCESS, &hKey);
  }
  if(n != ERROR_SUCCESS)
    throw SysError(n);
  boost::shared_ptr<HKEYWrapper> shared(new HKEYWrapper(hKey));
  m_hkey = shared;
  size_t lastslash = pathname.rfind('\\');
  if(lastslash != CStdString::npos)
  {
    m_name = pathname.substr(lastslash + 1);
    n = RegOpenKeyEx(base, pathname.substr(0, lastslash).c_str(), 0, KEY_SET_VALUE, &hKey);
    if(n != ERROR_SUCCESS)
      throw SysError(n);

    boost::shared_ptr<HKEYWrapper> fatherShared(new HKEYWrapper(hKey));
    m_hkeyfather = fatherShared;
  }
  else
  {
    m_name = pathname;
    boost::shared_ptr<HKEYWrapper> fatherShared(new HKEYWrapper(base, false));
    m_hkeyfather = fatherShared;
  }

}

std::list<CStdString> RegKey::getSubkeys() const throw(SysError)
{
  int n; 
  int c = 0; 
  list<CStdString> lst; 
  FILETIME ft; 
  do
  {
    ULONG size = MAX_PATH; 
    char buf[MAX_PATH]; 
    HKEY hk = m_hkey->get(); 
    n = RegEnumKeyEx(m_hkey->get(), c++, buf, &size, NULL, NULL, NULL, &ft); 
    switch(n)
    {
    case ERROR_SUCCESS:
      {
        lst.push_back(buf); 
        break; 
      }
    case ERROR_NO_MORE_ITEMS:
      break; 
    default:
      throw SysError(n); 
    }
  }
  while(n != ERROR_NO_MORE_ITEMS); 
  return lst; 
}

void RegKey::erase()throw(SysError)
{
  list<CStdString> subs = getSubkeys(); 
  for(list<CStdString>::iterator it = subs.begin(); it != subs.end(); ++it)
  {
    RegKey(m_hkey->get(), *it, false).erase(); 
  }
  int n = RegDeleteKey(m_hkeyfather->get(), m_name.c_str()); 
  if(n != ERROR_SUCCESS)
    throw SysError(n); 
}

size_t RegKey::readRawData(const CStdString& valueName, BYTE* buf, DWORD bufsize) const throw(SysError)
{
  DWORD dwDummy; 
  int n = RegQueryValueEx(m_hkey->get(), (valueName == "" ? NULL : valueName.c_str()), NULL, 
              &dwDummy, buf, &bufsize); 
  if(n != ERROR_SUCCESS)
    throw SysError(n); 

  return bufsize; 
}
CStdString RegKey::getValue(const CStdString& valueName) const throw(SysError)
{
  size_t size = readRawData(valueName, NULL, 0); 
  if(size)
  {
    std::auto_ptr<char> buf = std::auto_ptr<char>(new char[size+1]); 
    readRawData(valueName, (BYTE*)(buf.get()), size); 
    return (buf.get()); 
  }
  else 
    return ""; 
}

DWORD RegKey::getDwordValue(const CStdString& valueName) const throw(SysError)
{
  DWORD dwRet; 
  readRawData(valueName, (BYTE*)&dwRet, sizeof(DWORD)); 
  return dwRet; 
}

RECT RegKey::getRectValue(const CStdString& valueName) const throw(SysError)
{
  RECT rectRet; 
  readRawData(valueName, (BYTE*)&rectRet, sizeof(RECT)); 
  return rectRet; 
}

BYTE* RegKey::getBinaryValue(const CStdString& valueName, BYTE* buf, DWORD dwBufSize) const throw(SysError)
{
  readRawData(valueName, buf, dwBufSize); 
  return buf; 
}

DWORD RegKey::getBinarySize(const CStdString& valueName)
{
  return readRawData(valueName, NULL, 0); 
}
void RegKey::setValue(const CStdString& valueName, const CStdString& value) throw(SysError)
{
  size_t s = value.size(); 
  int n = RegSetValueEx(m_hkey->get(), (valueName.c_str() == "" ? NULL : valueName.c_str()), 0,
    REG_SZ, (UCHAR*)value.c_str(), value.size()); 

  if(n != ERROR_SUCCESS)
    throw SysError(n); 
}
void RegKey::setValue(const CStdString& valueName, DWORD value)throw(SysError)
{
  int n = RegSetValueEx(m_hkey->get(), (valueName.c_str() == "" ? NULL : valueName.c_str()), 0,
    REG_DWORD, (UCHAR*)&value, sizeof(value));

  if(n != ERROR_SUCCESS)
    throw SysError(n); 
}

void RegKey::setValue(const CStdString& valueName, const BYTE* buf, DWORD dwBufSize) throw(SysError)
{
  int n = RegSetValueEx(m_hkey->get(), (valueName.c_str() == "" ? NULL : valueName.c_str()), 0,
    REG_BINARY, buf, dwBufSize);

  if(n != ERROR_SUCCESS)
    throw SysError(n); 
}
void RegKey::eraseValue(const CStdString& valueName) throw(SysError)
{
  int n = RegDeleteValue(m_hkey->get(), valueName.c_str());

  if(n != ERROR_SUCCESS)
    throw SysError(n); 
}
std::map<CStdString, CStdString> RegKey::getValues() const throw(SysError)
{
  std::map<CStdString, CStdString> retlist; 
  char buf[MAX_PATH]; 
  char buf2[MAX_PATH]; 
  ULONG size, size2; 
  DWORD dwType; 
  int n; 
  int c = 0; 
  do
  {
    size = size2 = MAX_PATH; 
    n = RegEnumValue(m_hkey->get(), c++, buf, &size, NULL, &dwType, (UCHAR*)buf2, &size2); 
    switch(n)
    {
      case ERROR_SUCCESS:
      {
        switch(dwType)
        {
        case REG_SZ:
          retlist.insert(make_pair<CStdString, CStdString>(buf, buf2)); 
          break; 
        case REG_DWORD:
        {
          ostringstream oss; 
          oss << *(int*)buf[2]; 
          retlist.insert(make_pair<CStdString, CStdString>(buf, oss.str())); 
          break; 
        }
        case REG_EXPAND_SZ:
        {
          ExpandEnvironmentStrings(buf2, buf2, MAX_PATH); 
          retlist.insert(make_pair<CStdString, CStdString>(buf, buf2)); 
          break; 
        }
        default:
          retlist.insert(make_pair<CStdString, CStdString>(buf, "[Unrecognized regvalue type]")); 
        }
      }
      case ERROR_NO_MORE_ITEMS:
        break; 
      default:
        throw SysError(n); 
    }
  }
  while(n != ERROR_NO_MORE_ITEMS); 
  return retlist; 
}

RegKey RegKey::createSubkey(const CStdString& name) throw (SysError)
{
  return RegKey(m_hkey->get(), name); 
}

RegKey::operator HKEY() const
{
  return m_hkey->get(); 
}

bool RegKey::hasValue(const CStdString& name) const throw(SysError)
{
  bool b = true; 
  try
  {
    readRawData(name, NULL, 0); 
  }
  catch(const SysError& err)
  {
    switch(err.code())
    {
    case ERROR_FILE_NOT_FOUND:
      b = false; 
    case ERROR_MORE_DATA:
      break; 
    default:
      throw; 
    }
  }
  return b; 
}
