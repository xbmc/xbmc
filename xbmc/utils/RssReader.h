// RssReader.h: interface for the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include "Thread.h"

#include <vector>
#include <list>

#include "tinyXML/tinyxml.h"

#define RSS_COLOR_BODY  0
#define RSS_COLOR_HEADLINE 1
#define RSS_COLOR_CHANNEL 2

class IRssObserver
{
public:
  virtual void OnFeedUpdate(const std::vector<DWORD> &feed) = 0;
  virtual ~IRssObserver() {}
};

class CRssReader : public CThread
{
public:
  CRssReader();
  virtual ~CRssReader();

  void Create(IRssObserver* aObserver, const std::vector<std::string>& aUrl, const std::vector<int>& times, int spacesBetweenFeeds);
  bool Parse(LPSTR szBuffer, int iFeed);
  void getFeed(std::vector<DWORD> &text);
  void AddTag(const CStdString addTag);
  void AddToQueue(int iAdd);
  void UpdateObserver();
  void SetObserver(IRssObserver* observer);
  void CheckForUpdates();
  void requestRefresh();

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
  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecQueue;
  bool m_bIsRunning;
  CStdString m_encoding;
  bool m_rtlText;
  bool m_requestRefresh;
};

class CRssManager
{
public:
  CRssManager();
  ~CRssManager();

  void Stop();
  void Reset();

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
