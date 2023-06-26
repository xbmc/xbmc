/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/IRssObserver.h"
#include "utils/XBMCTinyXML2.h"

#include <list>
#include <string>
#include <vector>

namespace KODI::TIME
{
struct SystemTime;
}

class CRssReader : public CThread
{
public:
  CRssReader();
  ~CRssReader() override;

  void Create(IRssObserver* aObserver, const std::vector<std::string>& aUrl, const std::vector<int>& times, int spacesBetweenFeeds, bool rtl);
  bool Parse(const std::string& data, int iFeed);
  void getFeed(vecText &text);
  void AddTag(const std::string &addTag);
  void AddToQueue(int iAdd);
  void UpdateObserver();
  void SetObserver(IRssObserver* observer);
  void CheckForUpdates();
  void requestRefresh();
  float m_savedScrollPixelPos;

private:
  void Process() override;
  bool Parse(int iFeed);
  void GetNewsItems(tinyxml2::XMLNode* channelXmlNode, int iFeed);
  void AddString(std::wstring aString, int aColour, int iFeed);
  void UpdateFeed();
  void OnExit() override;
  int GetQueueSize();

  IRssObserver* m_pObserver;

  std::vector<std::wstring> m_strFeed;
  std::vector<std::wstring> m_strColors;
  std::vector<KODI::TIME::SystemTime*> m_vecTimeStamps;
  std::vector<int> m_vecUpdateTimes;
  int m_spacesBetweenFeeds;
  CXBMCTinyXML2 m_xml;
  std::list<std::string> m_tagSet;
  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecQueue;
  bool m_bIsRunning;
  bool m_rtlText;
  bool m_requestRefresh;

  CCriticalSection m_critical;
};
