// FileFactory1.h: interface for the CFileFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
#define AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ifile.h"
using namespace XFILE;

namespace XFILE
{
	class CFileFactory  
	{
	public:
		CFileFactory();
		virtual ~CFileFactory();
		IFile* CreateLoader(const CStdString& strFileName);
	};
};
#endif // !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
