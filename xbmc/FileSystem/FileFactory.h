// FileFactory1.h: interface for the CFileFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
#define AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

namespace XFILE
{
class CFileFactory
{
public:
  CFileFactory();
  virtual ~CFileFactory();
  static IFile* CreateLoader(const CStdString& strFileName);
  static IFile* CreateLoader(const CURL& url);
};
}
#endif // !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
