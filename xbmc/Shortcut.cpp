// Shortcut.cpp: implementation of the CShortcut class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Shortcut.h"
#include "settings.h"
#include "tinyxml/tinyxml.h"
#include "util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShortcut::CShortcut()
{

}

CShortcut::~CShortcut()
{

}

bool CShortcut::Create(const CStdString& szPath)
{
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( szPath.c_str() ) )
    return FALSE;

	bool bPath = false;

  TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
	if ( strValue != "shortcut") 
    return false;
  const TiXmlNode *pChild = pRootElement->FirstChild();

	while (pChild>0)
	{
	    CStdString strValue=pChild->Value();
			if (strValue=="path")
			{
				m_strPath = pChild->FirstChild()->Value();
				bPath = true;
			}

			if (strValue=="video")
			{
				m_strVideo = pChild->FirstChild()->Value();
			}

			if (strValue=="parameters")
			{
				m_strParameters = pChild->FirstChild()->Value();
			}
		

		  pChild=pChild->NextSibling();
  
	}

	return bPath ? true : false;
}

bool CShortcut::Save(const CStdString& strFileName)
{
  if (g_stSettings.m_szShortcutDirectory[0] == 0) return false;

	CStdString strShort=strFileName;
	CUtil::ShortenFileName(strShort);
	CUtil::RemoveIllegalChars(strShort);
  CStdString strTotalPath=g_stSettings.m_szShortcutDirectory;
  strTotalPath+="\\";
  strTotalPath+=strShort;
  strTotalPath+=".cut";
	::DeleteFile(strTotalPath.c_str());
  FILE* fd=fopen(strTotalPath.c_str(), "w");
  if (!fd) return false;
  fprintf(fd,"<shortcut>\r\n");
  fprintf(fd,"  <path>%s</path>\r\n",m_strPath.c_str());
  fprintf(fd,"</shortcut>\r\n");
  fclose(fd);
  return true;
}