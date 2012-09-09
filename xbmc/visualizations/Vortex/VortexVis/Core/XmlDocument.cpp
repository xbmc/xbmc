/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// XmlDocument.cpp: implementation of the CXmlDocument class.
//
//////////////////////////////////////////////////////////////////////
#include "XmlDocument.h"
#include <stdio.h>  

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXmlDocument::CXmlDocument()
{
	m_doc	= 0;
	m_size	= 0;
	m_nodes	= 0;

	m_szTag[0]	= 0;
	m_szText[0]	= 0;
}

void CXmlDocument::Create(char* szString)
{
	m_size = strlen(szString);
	m_doc = new char[m_size+1];
	memcpy(m_doc, szString, m_size+1);
}

CXmlDocument::~CXmlDocument()
{
	if(m_doc) {
		delete[] m_doc;
		m_doc = NULL;
	}
}		



//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_load_doc
//	Opens an XML document and loads it into memory.
//
int CXmlDocument::Load(char* szFile)
{
	FILE* hFile;
	
	hFile  = fopen(szFile,"rb");
	if (hFile==NULL)
	{
		return -1;
	}

	fseek(hFile,0,SEEK_END);
	m_size = ftell(hFile);

	fseek(hFile,0,SEEK_SET);
	
	m_doc = new char[m_size];
	if (!m_doc)
	{
		m_size = 0;
		fclose(hFile);
		return -2;
	}

	if (fread(m_doc, m_size, 1, hFile)<=0)
	{
		delete[] m_doc;
		m_doc  = 0;
		m_size = 0;
		fclose(hFile);
		return -3;
	}

	fclose(hFile);
	return 0;
}








//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_close_doc
//	Closes XML document freeing up resources.
//
void CXmlDocument::Close()
{
	if (m_doc!=NULL)
	{
		delete[] m_doc;
		m_doc  =0;
	}

	m_size =0;
	m_nodes	= 0;
	m_szTag[0]	= 0;
	m_szText[0]	= 0;
}




int CXmlDocument::GetNodeCount(char* szTag)
{
	m_nodes = 0;

	char* szCurrentTag;
	XmlNode node;
	
	node = GetNextNode(XML_ROOT_NODE);
	while (node>0)
	{
		szCurrentTag = GetNodeTag(node);
		if ( !strcmpi(szCurrentTag,szTag) )
			m_nodes++;

		node = GetNextNode(node);
	}

	return m_nodes;
}



//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_next_tag
//	Moves the current position to the next tag.
//
XmlNode CXmlDocument::GetNextNode(XmlNode node)
{
	int  openBracket = -1;
	int  closeBracket = -1;
	int  i;
	char c;

	for (i=node; i<m_size; i++)
	{
		c=m_doc[i];
		
		if (openBracket<0)
		{
			if (c=='<')
				openBracket=i;
			continue;
		}

		if (closeBracket<0)
		{
			if (c=='>')
			{
				closeBracket=i;
				break;
			}
		}
	}

	if ((openBracket>=0) && (closeBracket>=0))
	{
		return openBracket+1;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_get_tag_name
//	Gets the tag name at the current position (max 32 chars!).
//
char* CXmlDocument::GetNodeTag(XmlNode node)
{
	int  i;
	char c;

	for (i=node; i<m_size; i++)
	{
		c=m_doc[i];

		if ( (c==' ') || (c=='\n') || (c=='\r') || (c=='\t') || (c=='>') )
		{
			memcpy(m_szTag,&m_doc[node],i-node);
			m_szTag[i-node]=0;
			return m_szTag;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_get_child_tag
//	Gets the position of the child tag.
//
XmlNode CXmlDocument::GetChildNode(XmlNode node, char* szTag)
{
	char szCurrentTag[32];
	char* szChildTag;

	// get parent node tag
	strcpy(szCurrentTag,GetNodeTag(node));

	// get child node
	node = GetNextNode(node);
	while (node>0)
	{
		// get child node tag
		szChildTag = GetNodeTag(node);

		// does the child's tag match the one we're looking for
		if ( !strcmpi(szChildTag,szTag) )
			return node;

		// is this actually the parent's closing tag?
		else if ( !strcmpi(&szChildTag[1],szCurrentTag) )
			return 0;

		node = GetNextNode(node);
	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////////////
//  Function: xml_get_tag_text
//	Gets the text of a given tag (max limit 128 chars!!).
//
char* CXmlDocument::GetNodeText(XmlNode node)
{
	int i,text=0;
	int opens=1;
	int elements=0;
	char c;
	for (i=node;i<(m_size-1);i++)
	{
		c = m_doc[i];

		switch (c)
		{
			case '<':
				opens++;
				if (m_doc[i+1]!='/')
					elements++;
				else
					elements--;
				break;
			case '>' :
				opens--;
				break;
			case ' ' :
			case '\n':
			case '\r':
			case '\t':
				break;
			default:
				if ((opens==0) && (elements==0))
					text = i;
				break;
		}

		if (text)
			break;
	}

	if (!text)
		return 0;

	for (i=text;i<m_size;i++)
	{
		c = m_doc[i];
		if (c=='<')
		{
			memcpy(m_szText,&m_doc[text],i-text);
			m_szText[i-text]=0;
			return m_szText;
		}
	}

	m_szText[0]=0;
	return m_szText;
}




void CXmlDocument::EnumerateNodes(char* szTag, XmlNodeCallback pFunc)
{
	char* szCurrentTag;
	XmlNode node;
	
	node = GetNextNode(XML_ROOT_NODE);
	while (node>0)
	{
		szCurrentTag = GetNodeTag(node);
		if ( !strcmpi(szCurrentTag,szTag) )
			pFunc(szTag,node);

		node = GetNextNode(node);
	}
}
