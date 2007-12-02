// RssReader.h: interface for the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Thread.h"
#ifndef _LINUX
#include "../lib/libiconv/iconv.h"
#else
#include <iconv.h>
#endif

#define RSS_COLOR_BODY  0
#define RSS_COLOR_HEADLINE 1
#define RSS_COLOR_CHANNEL 2

class IRssObserver
{
public:
  virtual void OnFeedUpdate(const vector<DWORD> &feed) = 0;
  virtual ~IRssObserver() {}
};

class CRssReader : public CThread
{
public:
  CRssReader();
  virtual ~CRssReader();

  void Create(IRssObserver* aObserver, const vector<string>& aUrl, const vector<int>& times, int spacesBetweenFeeds);
  bool Parse(LPSTR szBuffer, int iFeed);
  void getFeed(vector<DWORD> &text);
  void AddTag(const CStdString addTag);
  void AddToQueue(int iAdd);
  void UpdateObserver();
  void SetObserver(IRssObserver* observer);
  void CheckForUpdates();

private:
  void fromRSSToUTF16(const CStdStringA& strSource, CStdStringW& strDest);
  void Process();
  bool Parse(int iFeed);
  void GetNewsItems(TiXmlElement* channelXmlNode, int iFeed);
  void AddString(CStdStringW aString, int aColour, int iFeed);
  void UpdateFeed();
  virtual void OnExit();

  IRssObserver* m_pObserver;
  
  std::vector<CStdStringW> m_strFeed;
  std::vector<CStdStringW> m_strColors;
  std::vector<SYSTEMTIME *> m_vecTimeStamps;
  std::vector<int> m_vecUpdateTimes;
  int m_spacesBetweenFeeds;
  TiXmlDocument m_xml;
  std::list<CStdString> m_tagSet;
  std::vector<string> m_vecUrls;
  std::vector<int> m_vecQueue;
  bool m_bIsRunning;
  iconv_t m_iconv;
  bool m_shouldFlip;
  CStdString m_encoding;
};

class CRssManager
{
public:
  CRssManager();
  ~CRssManager();

  bool GetReader(DWORD controlID, DWORD windowID, IRssObserver* observer, CRssReader *&reader);

private:
  struct READERCONTROL
  {
    DWORD controlID;
    DWORD windowID;
    CRssReader *reader;
  };

  std::vector<READERCONTROL> m_readers;
};

extern CRssManager g_rssManager;

#endif // !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
