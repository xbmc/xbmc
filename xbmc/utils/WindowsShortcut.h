// WindowsShortcut.h: interface for the CWindowsShortcut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_)
#define AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;
class CWindowsShortcut
{
public:
  CWindowsShortcut();
  virtual ~CWindowsShortcut();
  static bool IsShortcut(const string& strFileName);
  bool GetShortcut(const string& strFileName, string& strFileOrDir);
};

#endif // !defined(AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_)
