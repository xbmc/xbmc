// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNfoFile
{
public:
	CNfoFile();
	virtual ~CNfoFile();
	
	HRESULT Create(LPSTR szPath);

	string m_strImDbUrl;
private:
	HRESULT Load(char* szFile);
	void	Close();
private:
	char*	m_doc;
	int		m_size;
};

#endif // !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
