// RssReader.cpp: implementation of the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "graphicContext.h"
#include "RssReader.h"
#include "Http.h"
#include "../utils/HTMLUtil.h"
#include <map>

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
		// remove CDATA sections from our buffer (timyXML is not able to parse these)
		CStdString strCDATAElement;
		int iStart = strXML.Find("<![CDATA[");
		int iEnd = 0;
		while (iStart > 0)
		{
			// get CDATA end position
			iEnd = strXML.Find("]]>", iStart) + 3;

			// get data in CDATA section
			strCDATAElement = strXML.substr(iStart + 9, iEnd - iStart - 12);

			// replace CDATA section with new string
			strXML = strXML.erase(iStart, iEnd - iStart).insert(iStart, strCDATAElement);

			iStart = strXML.Find("<![CDATA[");
		}

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

void CRssReader::AddTag(const CStdString aString) 
{
	m_tagSet.push_back(aString);
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
	map <CStdString, CStdString> mTagElements;
	typedef pair <CStdString, CStdString> StrPair;
	list <CStdString>::iterator i;

	// Add the title tag in if we didn't pass any tags in at all
	// Represents default behaviour before configurability

	if(m_tagSet.empty())
		AddTag("title");

 	while (itemNode>0)
 	{
 		TiXmlNode* childNode = itemNode->FirstChild();
		mTagElements.clear();
 		while (childNode>0)
		{	
 			CStdString strName = childNode->Value();
 
			for(i=m_tagSet.begin(); i!= m_tagSet.end(); i++)
			{
				if(i->Equals(strName))
				{
					mTagElements.insert(StrPair(*i,childNode->FirstChild()->Value()));
				}
			}
 			childNode = childNode->NextSibling();
 		}

		int rsscolour = RSS_COLOR_HEADLINE;
		for (i=m_tagSet.begin();i!=m_tagSet.end();i++)
		{
			CStdString text;
			HTML::CHTMLUtil html;
			map <CStdString, CStdString>::iterator j = mTagElements.find(*i);
 		
			if(j == mTagElements.end())
				continue;
 
			html.ConvertHTMLToAnsi(j->second, text);
			
			AddString(text, rsscolour);
			rsscolour=RSS_COLOR_BODY;
			text = " - ";
			AddString(text, rsscolour);
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

	if (!rootXmlNode)
		return false;

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