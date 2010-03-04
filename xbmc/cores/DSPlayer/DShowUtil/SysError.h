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
