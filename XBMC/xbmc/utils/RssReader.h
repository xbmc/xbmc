// RssReader.h: interface for the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"

#define RSS_COLOR_BODY		0
#define RSS_COLOR_HEADLINE	1
#define RSS_COLOR_CHANNEL	2

class IRssObserver
{
public:
	virtual void OnFeedUpdate(CStdString& aFeed, LPBYTE aColorArray)=0;
};

class CRssReader : CThread
{
public:
	CRssReader();
	virtual ~CRssReader();

	void	Create(IRssObserver* aObserver, CStdString& aUrl, INT iLeadingSpaces);
	bool	Load(CStdString& aFile);
	bool	Parse(LPSTR szBuffer);
	void	getFeed(CStdString& strText, LPBYTE& pbColors);
	void	AddTag(const CStdString addTag);

private:
	void Process();
	bool Parse();
	void GetNewsItems(TiXmlElement* channelXmlNode);
	void AddString(CStdString aString, int aColour);

	IRssObserver*			m_pObserver;
	CStdString				m_strUrl;
	CStdString				m_strFeed;
	CStdString				m_strColors;
	INT								m_iLeadingSpaces;
	TiXmlDocument			m_xml;
	list <CStdString>	m_tagSet;
};

#endif // !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
