// Shortcut.h: interface for the CShortcut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShortcut 
{
public:
	CShortcut();
	virtual ~CShortcut();
	
	bool Create(const CStdString& szPath);
  bool Save(const CStdString& strFileName);

	CStdString m_strPath;
	CStdString m_strVideo;
	CStdString m_strParameters;
};

#endif // !defined(AFX_SHORTCUT_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
