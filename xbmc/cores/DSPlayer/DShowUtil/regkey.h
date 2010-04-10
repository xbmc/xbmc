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

// RegKey.h  RegKey class header.
//
//////////////////////////////////////////////////////////////////////
#pragma warning (disable:4786)
#pragma warning (disable:4290)
#ifndef REGKEY_H
#define REGKEY_H
#include <winsock2.h>
#include <list>
#include <string>

#include "boost/shared_ptr.hpp"
#include <DshowUtil/SysError.h>

class RegKey
{
public:
  RegKey(HKEY base, const CStdString& pathname, bool bCreate = true) throw (SysError);
  virtual ~RegKey() {};
  std::list<CStdString> getSubkeys() const throw(SysError);
  void erase() throw(SysError);
  CStdString getValue(const CStdString& valueName) const throw(SysError);
  DWORD getDwordValue(const CStdString& valueName) const throw(SysError);
  RECT getRectValue(const CStdString& valueName) const throw(SysError);
  BYTE* getBinaryValue(const CStdString& valueName, BYTE* buf, DWORD dwBufSize) const throw(SysError);
  DWORD getBinarySize(const CStdString& valueName);
  void setValue(const CStdString& valueName, const CStdString& value) throw(SysError);
  void setValue(const CStdString& valueName, DWORD value) throw(SysError);
  void setValue(const CStdString& valueName, const BYTE* buf, DWORD dwBufSize) throw(SysError);
  void eraseValue(const CStdString& valueName) throw(SysError);
  std::map<CStdString,CStdString> getValues() const throw(SysError);
  RegKey createSubkey(const CStdString& name) throw (SysError);
  operator HKEY() const;
  bool hasValue(const CStdString& name) const throw(SysError);

private:
  class HKEYWrapper
  {
  public:
    HKEYWrapper(HKEY hkey, bool bClose=true):
        m_hkey(hkey), m_bClose(bClose) {}
    virtual ~HKEYWrapper()
    {
      if(m_bClose)
        RegCloseKey(m_hkey);
    }
    HKEY get() const { return m_hkey; }
  private:
    void operator= (const HKEYWrapper&); //not implem
    HKEYWrapper(const HKEYWrapper&); //not impl
    HKEY m_hkey;
    bool m_bClose;
  };
  boost::shared_ptr<HKEYWrapper> m_hkey;
  boost::shared_ptr<HKEYWrapper> m_hkeyfather;
  CStdString m_name;
  size_t readRawData(const CStdString& valueName, BYTE* buf, DWORD bufsize) const throw(SysError);
};

#endif//REGKEY_H
