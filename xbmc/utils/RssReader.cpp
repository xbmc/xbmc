// RssReader.cpp: implementation of the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "graphicContext.h"
#include "RssReader.h"
#include "Http.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRssReader::CRssReader() : CThread()
{
	m_pObserver = NULL;
	m_iLeadingSpaces = 0;
}

CRssReader::~CRssReader()
{

}

void CRssReader::Create(IRssObserver* aObserver, CStdString& aUrl, INT iLeadingSpaces)
{
	m_pObserver		 = aObserver;
	m_strUrl		 = aUrl;
	m_iLeadingSpaces = iLeadingSpaces;

	CThread::Create(false);
}

void CRssReader::Process() 
{
	CHTTP http;
	CStdString strXML;
	CStdString strFeed;
	LPBYTE	   pbColors = NULL;

	int nRetries = 3;

	http.SetHTTPVer(0);

	while ( (!m_bStop) && (nRetries>0) )
	{	
		nRetries--;

		if (http.Get(m_strUrl, strXML))
		{
			break;
		}
	}

	if ((!strXML.IsEmpty()) && m_pObserver)
	{
		if (Parse((LPSTR)strXML.c_str()))
		{
			getFeed(strFeed, pbColors);
			g_graphicsContext.Lock();
			m_pObserver->OnFeedUpdate(strFeed, pbColors);
			g_graphicsContext.Unlock();
		}
	}
}



void CRssReader::getFeed(CStdString& strText, LPBYTE& pbColors)
{
	strText = m_strFeed;
	int nTextLength = m_strFeed.GetLength();
	pbColors = new BYTE[nTextLength];

	for(int i=0; i<nTextLength; i++)
	{
		pbColors[i] = m_strColors[i] - 48;
	}
}

void CRssReader::AddString(CStdString aString, int aColour)
{
	m_strFeed += aString;

	int nStringLength = aString.GetLength();

	for(int i=0;i<nStringLength;i++)
	{
		aString[i]= (CHAR) (48+aColour);
	}
	
	m_strColors += aString;
}

void CRssReader::GetNewsItems(TiXmlElement* channelXmlNode)
{
	TiXmlElement * itemNode = channelXmlNode->FirstChildElement("item");
	while (itemNode>0)
	{
		TiXmlNode* childNode = itemNode->FirstChild();
		while (childNode>0)
		{
			CStdString strName = childNode->Value();

			if (strName.Equals("title"))
			{
				CStdString title = childNode->FirstChild()->Value();		
				AddString(title, RSS_COLOR_HEADLINE);
				title = " - ";		
				AddString(title, RSS_COLOR_BODY);
			}
/*
			if (strName.Equals("description"))
			{
				CStdString description;
				description.Format("%s     ", childNode->FirstChild()->Value());
				
				if (description.GetLength()>127)
				{
					description = description.Mid(0,127);
				}

				AddString(description, RSS_COLOR_BODY);
			}
*/
			childNode = childNode->NextSibling();
		}
		

		itemNode = itemNode->NextSiblingElement("item");
	}
}

bool CRssReader::Load(CStdString& aFile)
{
	m_strFeed="";
	m_strColors="";

	if ( !m_xml.LoadFile( "sample.xml" ) ) 
	{
		// Unable to load/parse XML
		return false;
	}

	return Parse();
}

bool CRssReader::Parse(LPSTR szBuffer)
{
	m_strFeed="";
	m_strColors="";

	m_xml.Parse((LPCSTR)szBuffer);

	return Parse();
}

bool CRssReader::Parse()
{
	CStdString strLeadingSpace = " ";
	for (int i=0; i<m_iLeadingSpaces; i++)
	{
		AddString(strLeadingSpace,0);
	}

	TiXmlElement* rootXmlNode = m_xml.RootElement();

	TiXmlElement* rssXmlNode = NULL;
	
	CStdString strValue = rootXmlNode->Value();
	if (( strValue.Find("rss")>=0 ) || ( strValue.Find("rdf")>=0 )) 
	{
		rssXmlNode = rootXmlNode;
	}
	else
	{
		// Unable to find root <rss> or <rdf> node
		return false;
	}

	TiXmlElement* channelXmlNode = rssXmlNode->FirstChildElement("channel");
	if (channelXmlNode)
	{
		TiXmlElement* titleNode = channelXmlNode->FirstChildElement("title");
		if (titleNode)
		{
			CStdString strChannel;
			strChannel.Format("%s RSS News: ", titleNode->FirstChild()->Value());
			AddString(strChannel, RSS_COLOR_CHANNEL);
		}

		GetNewsItems(channelXmlNode);
	}

	GetNewsItems(rssXmlNode);
	return true;
}