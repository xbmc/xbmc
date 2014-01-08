#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <list>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/IRssObserver.h"
#include "utils/XBMCTinyXML.h"

class CRssReader : public CThread
{
public:
  CRssReader();
  virtual ~CRssReader();

  void Create(IRssObserver* aObserver, const std::vector<std::string>& aUrl, const std::vector<int>& times, int spacesBetweenFeeds, bool rtl);
  bool Parse(const std::string& data, int iFeed, const std::string& charset);
  void getFeed(vecText &text);
  void AddTag(const std::string &addTag);
  void AddToQueue(int iAdd);
  void UpdateObserver();
  void SetObserver(IRssObserver* observer);
  void CheckForUpdates();
  void requestRefresh();
  float m_savedScrollPixelPos;

private:
  void Process();
  bool Parse(int iFeed);
  void GetNewsItems(TiXmlElement* channelXmlNode, int iFeed);
  void AddString(std::wstring aString, int aColour, int iFeed);
  void UpdateFeed();
  virtual void OnExit();
  int GetQueueSize();

  IRssObserver* m_pObserver;

  std::vector<std::wstring> m_strFeed;
  std::vector<std::wstring> m_strColors;
  std::vector<SYSTEMTIME *> m_vecTimeStamps;
  std::vector<int> m_vecUpdateTimes;
  int m_spacesBetweenFeeds;
  CXBMCTinyXML m_xml;
  std::list<std::string> m_tagSet;
  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecQueue;
  bool m_bIsRunning;
  bool m_rtlText;
  bool m_requestRefresh;

  CCriticalSection m_critical;
};
