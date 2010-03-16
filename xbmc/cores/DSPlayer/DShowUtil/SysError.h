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

// SysError.h:  SysError class header.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIN32ERROR_H__4A48AD09_8EC6_48D2_BC29_986C25F03772__INCLUDED_)
#define AFX_WIN32ERROR_H__4A48AD09_8EC6_48D2_BC29_986C25F03772__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include <stdexcept>
#include <string>

///Incapsulates a O.S. specific error
class SysError:public std::runtime_error
{
public:
  ///default ctor, encapsulates last error
  SysError();
  SysError(const std::string& msg);
  ///encapsulates a specific error
  /**
  \param code is the error to be encapsulated
  */
  SysError(int code);
  ///dtor
  virtual ~SysError()throw();
  ///return O.S. specific error code
  /**
  \return O.S. peculiar error code
  */
  int code()const{return m_code;}
private:
  std::string errorDesc(int code);
  int m_code;
};

#endif // !defined(AFX_WIN32ERROR_H__4A48AD09_8EC6_48D2_BC29_986C25F03772__INCLUDED_)
