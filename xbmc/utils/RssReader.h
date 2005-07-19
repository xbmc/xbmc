// RssReader.h: interface for the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"

#define RSS_COLOR_BODY  0
#define RSS_COLOR_HEADLINE 1
#define RSS_COLOR_CHANNEL 2

class IRssObserver
{
public:
  virtual void OnFeedUpdate(CStdString& aFeed, LPBYTE aColorArray) = 0;
};

class CRssReader : public CThread
{
public:
  CRssReader();
  virtual ~CRssReader();

  void Create(IRssObserver* aObserver, const vector<wstring>& aUrl, INT iLeadingSpaces);
  bool Parse(LPSTR szBuffer, int iFeed, bool forceUTF8 = false);
  void getFeed(CStdString& strText, LPBYTE& pbColors);
  void AddTag(const CStdString addTag);
  void AddToQueue(int iAdd);
private:
  void Process();
  bool Parse(int iFeed);
  void GetNewsItems(TiXmlElement* channelXmlNode, int iFeed);
  void AddString(CStdString aString, int aColour, int iFeed);
  void UpdateFeed();
  virtual void OnExit();

  IRssObserver* m_pObserver;
  
  std::vector<CStdString> m_strFeed;
  std::vector<CStdString> m_strColors;
  INT m_iLeadingSpaces;
  TiXmlDocument m_xml;
  list <CStdString> m_tagSet;
  vector<wstring> m_vecUrls;
  vector<int> m_vecQueue;
  bool m_bIsRunning;
};

#endif // !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
