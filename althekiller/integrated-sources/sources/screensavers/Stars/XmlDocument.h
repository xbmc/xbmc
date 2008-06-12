// XmlDocument.h: interface for the CXmlDocument class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_)
#define AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <xtl.h>
#define XML_ROOT_NODE 0
#define XML_MAX_TAGNAME_SIZE	32
#define XML_MAX_INNERTEXT_SIZE	1024

typedef int    XmlNode;
typedef void (*XmlNodeCallback) (char* szTag, XmlNode node);      



class CXmlDocument  
{
public:
	CXmlDocument();
	virtual ~CXmlDocument();

	void	Create(char* szString);
	int     Load(char* szFile);
	void    Close();

	int		GetNodeCount(char* tag);

	void    EnumerateNodes(char* szTag, XmlNodeCallback pFunc);

	XmlNode GetChildNode(XmlNode node, char* szTag);
	XmlNode GetNextNode(XmlNode node);
	char*   GetNodeText(XmlNode node);
	char*   GetNodeTag(XmlNode node);

private:

	char*	m_doc;
	int		m_size;
	int		m_nodes;
	char	m_szTag[XML_MAX_TAGNAME_SIZE];
	char	m_szText[XML_MAX_INNERTEXT_SIZE];
};

#endif // !defined(AFX_XMLDOCUMENT_H__D68461F7_E0CE_4FA0_B1C9_0541610164E9__INCLUDED_)
