// RssReader.h: interface for the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
#define AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StdString.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <vector>
#include <list>

#include "utils/XBMCTinyXML.h"

#include "system.h"


#define RSS_COLOR_BODY  0
#define RSS_COLOR_HEADLINE 1
#define RSS_COLOR_CHANNEL 2

typedef uint32_t character_t;
typedef std::vector<character_t> vecText;

class IRssObserver;

class CRssReader : public CThread,
                   public CCriticalSection
{
public:
  CRssReader();
  virtual ~CRssReader();

  void Create(IRssObserver* aObserver, const std::vector<std::string>& aUrl, const std::vector<int>& times, int spacesBetweenFeeds, bool rtl);
  bool Parse(LPSTR szBuffer, int iFeed);
  void getFeed(vecText &text);
  void AddTag(const CStdString &addTag);
  void AddToQueue(int iAdd);
  void UpdateObserver();
  void SetObserver(IRssObserver* observer);
  void CheckForUpdates();
  void requestRefresh();
  unsigned int m_SavedScrollPos;

private:
  void fromRSSToUTF16(const CStdStringA& strSource, CStdStringW& strDest);
  void Process();
  bool Parse(int iFeed);
  void GetNewsItems(TiXmlElement* channelXmlNode, int iFeed);
  void AddString(CStdStringW aString, int aColour, int iFeed);
  void UpdateFeed();
  virtual void OnExit();
  int GetQueueSize();

  IRssObserver* m_pObserver;

  std::vector<CStdStringW> m_strFeed;
  std::vector<CStdStringW> m_strColors;
  std::vector<SYSTEMTIME *> m_vecTimeStamps;
  std::vector<int> m_vecUpdateTimes;
  int m_spacesBetweenFeeds;
  CXBMCTinyXML m_xml;
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

  void Start();
  void Stop();
  void Reset();
  bool IsActive() { return m_bActive; }

  bool GetReader(int controlID, int windowID, IRssObserver* observer, CRssReader *&reader);

private:
  struct READERCONTROL
  {
    int controlID;
    int windowID;
    CRssReader *reader;
  };

  std::vector<READERCONTROL> m_readers;
  bool m_bActive;
};

extern CRssManager g_rssManager;

#endif // !defined(AFX_RSSREADER_H__157FED93_0CDE_4295_A9AF_75BEF4E81761__INCLUDED_)
