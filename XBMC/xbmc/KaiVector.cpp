// KaiVector.cpp: implementation of the CKaiVector class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KaiVector.h"
#include "settings.h"
#include "tinyxml/tinyxml.h"
#include "util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKaiVector::CKaiVector()
{
	InitializeCriticalSection(&m_critical);
	m_bDirty = false;
}

CKaiVector::~CKaiVector()
{
	DeleteCriticalSection(&m_critical);
}

void CKaiVector::Save(const CStdString& strPath)
{
	if (m_bDirty)
	{
		//	Remove old file
		::DeleteFile(strPath.c_str());

		// <titles>
		//	  <title>
		//	    <id>0x11223344</path>
		//	    <vector>Arena/Platform/Genre/Game</vector>
		//	  </title>
		// </titles>

		TiXmlDocument xmlDoc;
		TiXmlElement xmlRootElement("titles");
		TiXmlNode* pRootNode = xmlDoc.InsertEndChild(xmlRootElement);

		EnterCriticalSection(&m_critical);

		for (TITLEVECTORMAP::iterator it = m_mapTitles.begin(); it!=m_mapTitles.end(); it++)
		{
			DWORD dwTitleId = it->first;
			CStdString strTitleId;
			strTitleId.Format("0x%x",dwTitleId);
			strTitleId.ToUpper();
			CStdString strVector  = it->second;

			TiXmlElement xmlTitleElement("title");
			xmlTitleElement.SetAttribute("id",strTitleId);
			TiXmlNode* pTitleNode = pRootNode->InsertEndChild(xmlTitleElement);

			TiXmlElement xmlVectorElement("vector");
			TiXmlNode* pVectorNode = pTitleNode->InsertEndChild(xmlVectorElement);
			TiXmlText vectorValue(strVector);
			pVectorNode->InsertEndChild(vectorValue);
		}

		LeaveCriticalSection(&m_critical);

		xmlDoc.SaveFile(strPath);
		m_bDirty = false;
	}
}

void CKaiVector::Load(const CStdString& strPath)
{
	TiXmlDocument xmlDoc;
	if (xmlDoc.LoadFile(strPath.c_str()))
	{
		TiXmlElement* pRootElement = xmlDoc.RootElement();
		const TiXmlElement* pTitleElement = pRootElement->FirstChildElement("title");

		EnterCriticalSection(&m_critical);

		while (pTitleElement)
		{
			CStdString strTitleId = pTitleElement->Attribute("id");
			CStdString strVector;

			const TiXmlNode* pChildNode = pTitleElement->FirstChild();
			while(pChildNode)
			{
				CStdString strValue = pChildNode->Value();

				if (strValue=="vector" && pChildNode->FirstChild())
				{
					strVector = pChildNode->FirstChild()->Value();
				}

				pChildNode = pChildNode->NextSibling();
			}

			DWORD dwTitleId = strtoul(strTitleId.c_str(),NULL,16);
			m_mapTitles[dwTitleId] = strVector;

			pTitleElement = pTitleElement->NextSiblingElement();
  		}

		LeaveCriticalSection(&m_critical);
	}
}

void CKaiVector::AddTitle(DWORD aTitleId, CStdString& aVector)
{
	EnterCriticalSection(&m_critical);
	m_mapTitles[aTitleId] = aVector;
	LeaveCriticalSection(&m_critical);
	m_bDirty = true;
}

bool CKaiVector::GetTitle(DWORD aTitleId, CStdString& aVector)
{
	EnterCriticalSection(&m_critical);
	
	TITLEVECTORMAP::iterator it = m_mapTitles.find(aTitleId);
	bool bContainsTitle = ( it != m_mapTitles.end() );
	if (bContainsTitle)
	{
		aVector = m_mapTitles[aTitleId];
	}

	LeaveCriticalSection(&m_critical);
	return bContainsTitle;
}

bool CKaiVector::ContainsTitle(DWORD aTitleId)
{
	EnterCriticalSection(&m_critical);

	bool bContainsTitle = ( m_mapTitles.find(aTitleId) != m_mapTitles.end() );
	
	LeaveCriticalSection(&m_critical);
	return bContainsTitle;
}
