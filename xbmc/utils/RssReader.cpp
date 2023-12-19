/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RssReader.h"

#include "CharsetConverter.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "guilib/GUIRSSControl.h"
#include "guilib/LocalizeStrings.h"
#include "log.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/HTMLUtil.h"
#include "utils/XTimeUtils.h"

#include <mutex>

#define RSS_COLOR_BODY      0
#define RSS_COLOR_HEADLINE  1
#define RSS_COLOR_CHANNEL   2

using namespace XFILE;
using namespace std::chrono_literals;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRssReader::CRssReader() : CThread("RSSReader")
{
  m_pObserver = NULL;
  m_spacesBetweenFeeds = 0;
  m_bIsRunning = false;
  m_savedScrollPixelPos = 0;
  m_rtlText = false;
  m_requestRefresh = false;
}

CRssReader::~CRssReader()
{
  if (m_pObserver)
    m_pObserver->OnFeedRelease();
  StopThread();
  for (unsigned int i = 0; i < m_vecTimeStamps.size(); i++)
    delete m_vecTimeStamps[i];
}

void CRssReader::Create(IRssObserver* aObserver, const std::vector<std::string>& aUrls, const std::vector<int> &times, int spacesBetweenFeeds, bool rtl)
{
  std::unique_lock<CCriticalSection> lock(m_critical);

  m_pObserver = aObserver;
  m_spacesBetweenFeeds = spacesBetweenFeeds;
  m_vecUrls = aUrls;
  m_strFeed.resize(aUrls.size());
  m_strColors.resize(aUrls.size());
  // set update times
  m_vecUpdateTimes = times;
  m_rtlText = rtl;
  m_requestRefresh = false;

  // update each feed on creation
  for (unsigned int i = 0; i < m_vecUpdateTimes.size(); ++i)
  {
    AddToQueue(i);
    KODI::TIME::SystemTime* time = new KODI::TIME::SystemTime;
    KODI::TIME::GetLocalTime(time);
    m_vecTimeStamps.push_back(time);
  }
}

void CRssReader::requestRefresh()
{
  m_requestRefresh = true;
}

void CRssReader::AddToQueue(int iAdd)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  if (iAdd < (int)m_vecUrls.size())
    m_vecQueue.push_back(iAdd);
  if (!m_bIsRunning)
  {
    StopThread();
    m_bIsRunning = true;
    CThread::Create(false);
  }
}

void CRssReader::OnExit()
{
  m_bIsRunning = false;
}

int CRssReader::GetQueueSize()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  return m_vecQueue.size();
}

void CRssReader::Process()
{
  while (GetQueueSize())
  {
    std::unique_lock<CCriticalSection> lock(m_critical);

    int iFeed = m_vecQueue.front();
    m_vecQueue.erase(m_vecQueue.begin());

    m_strFeed[iFeed].clear();
    m_strColors[iFeed].clear();

    CCurlFile http;
    http.SetUserAgent(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_userAgent);
    http.SetTimeout(2);
    std::string strXML;
    std::string strUrl = m_vecUrls[iFeed];
    lock.unlock();

    int nRetries = 3;
    CURL url(strUrl);

    // we wait for the network to come up
    if ((url.IsProtocol("http") || url.IsProtocol("https")) &&
        !CServiceBroker::GetNetwork().IsAvailable())
    {
      CLog::Log(LOGWARNING, "RSS: No network connection");
      strXML = "<rss><item><title>"+g_localizeStrings.Get(15301)+"</title></item></rss>";
    }
    else
    {
      XbmcThreads::EndTime<> timeout(15s);
      while (!m_bStop && nRetries > 0)
      {
        if (timeout.IsTimePast())
        {
          CLog::Log(LOGERROR, "Timeout while retrieving rss feed: {}", strUrl);
          break;
        }
        nRetries--;

        if (!url.IsProtocol("http") && !url.IsProtocol("https"))
        {
          CFile file;
          std::vector<uint8_t> buffer;
          if (file.LoadFile(strUrl, buffer) > 0)
          {
            strXML.assign(reinterpret_cast<char*>(buffer.data()), buffer.size());
            break;
          }
        }
        else
        {
          if (http.Get(strUrl, strXML))
          {
            CLog::Log(LOGDEBUG, "Got rss feed: {}", strUrl);
            break;
          }
          else if (nRetries > 0)
            CThread::Sleep(5000ms); // Network problems? Retry, but not immediately.
          else
            CLog::Log(LOGERROR, "Unable to obtain rss feed: {}", strUrl);
        }
      }
      http.Cancel();
    }

    if (!strXML.empty() && m_pObserver && Parse(strXML, iFeed))
      CLog::Log(LOGDEBUG, "Parsed rss feed: {}", strUrl);
  }
  UpdateObserver();
}

void CRssReader::getFeed(vecText &text)
{
  text.clear();
  // double the spaces at the start of the set
  for (int j = 0; j < m_spacesBetweenFeeds; j++)
    text.push_back(L' ');
  for (unsigned int i = 0; i < m_strFeed.size(); i++)
  {
    for (int j = 0; j < m_spacesBetweenFeeds; j++)
      text.push_back(L' ');

    for (unsigned int j = 0; j < m_strFeed[i].size(); j++)
    {
      character_t letter = m_strFeed[i][j] | ((m_strColors[i][j] - 48) << 16);
      text.push_back(letter);
    }
  }
}

void CRssReader::AddTag(const std::string &aString)
{
  m_tagSet.push_back(aString);
}

void CRssReader::AddString(std::wstring aString, int aColour, int iFeed)
{
  if (m_rtlText)
    m_strFeed[iFeed] = aString + m_strFeed[iFeed];
  else
    m_strFeed[iFeed] += aString;

  size_t nStringLength = aString.size();

  for (size_t i = 0;i < nStringLength;i++)
    aString[i] = static_cast<char>(48 + aColour);

  if (m_rtlText)
    m_strColors[iFeed] = aString + m_strColors[iFeed];
  else
    m_strColors[iFeed] += aString;
}

void CRssReader::GetNewsItems(tinyxml2::XMLNode* channelXmlNode, int iFeed)
{
  HTML::CHTMLUtil html;

  auto* itemNode = channelXmlNode->FirstChildElement("item");
  std::map<std::string, std::wstring> mTagElements;
  typedef std::pair<std::string, std::wstring> StrPair;
  std::list<std::string>::iterator i;

  // Add the title tag in if we didn't pass any tags in at all
  // Represents default behaviour before configurability

  if (m_tagSet.empty())
    AddTag("title");

  while (itemNode)
  {
    auto* childNode = itemNode->FirstChild();
    mTagElements.clear();
    while (childNode)
    {
      std::string strName = childNode->Value();

      for (i = m_tagSet.begin(); i != m_tagSet.end(); ++i)
      {
        if (!childNode->NoChildren() && *i == strName)
        {
          std::string htmlText = childNode->FirstChild()->Value();

          // This usually happens in right-to-left languages where they want to
          // specify in the RSS body that the text should be RTL.
          // <title>
          //  <div dir="RTL">��� ����: ���� �� �����</div>
          // </title>
          if (htmlText == "div" || htmlText == "span")
            htmlText = childNode->FirstChild()->FirstChild()->Value();

          std::wstring unicodeText, unicodeText2;

          g_charsetConverter.utf8ToW(htmlText, unicodeText2, m_rtlText);
          html.ConvertHTMLToW(unicodeText2, unicodeText);

          mTagElements.insert(StrPair(*i, unicodeText));
        }
      }
      childNode = childNode->NextSibling();
    }

    int rsscolour = RSS_COLOR_HEADLINE;
    for (i = m_tagSet.begin(); i != m_tagSet.end(); ++i)
    {
      std::map<std::string, std::wstring>::iterator j = mTagElements.find(*i);

      if (j == mTagElements.end())
        continue;

      std::wstring& text = j->second;
      AddString(text, rsscolour, iFeed);
      rsscolour = RSS_COLOR_BODY;
      text = L" - ";
      AddString(text, rsscolour, iFeed);
    }
    itemNode = itemNode->NextSiblingElement("item");
  }
}

bool CRssReader::Parse(const std::string& data, int iFeed)
{
  m_xml.Clear();
  m_xml.Parse(data);

  return Parse(iFeed);
}

bool CRssReader::Parse(int iFeed)
{
  auto* rootXmlNode = m_xml.RootElement();

  if (!rootXmlNode)
    return false;

  std::string strValue = rootXmlNode->Value();
  if (strValue.find("rss") == std::string::npos && strValue.find("rdf") == std::string::npos)
  {
    // Unable to find root <rss> or <rdf> node
    return false;
  }

  auto* channelXmlNode = rootXmlNode->FirstChildElement("channel");
  if (channelXmlNode)
  {
    auto* titleNode = channelXmlNode->FirstChildElement("title");
    if (titleNode && !titleNode->NoChildren())
    {
      std::string strChannel = titleNode->FirstChild()->Value();
      std::wstring strChannelUnicode;
      g_charsetConverter.utf8ToW(strChannel, strChannelUnicode, m_rtlText);
      AddString(strChannelUnicode, RSS_COLOR_CHANNEL, iFeed);

      AddString(L":", RSS_COLOR_CHANNEL, iFeed);
      AddString(L" ", RSS_COLOR_CHANNEL, iFeed);
    }

    GetNewsItems(channelXmlNode,iFeed);
  }

  GetNewsItems(rootXmlNode, iFeed);

  // avoid trailing ' - '
  if (m_strFeed[iFeed].size() > 3 && m_strFeed[iFeed].substr(m_strFeed[iFeed].size() - 3) == L" - ")
  {
    if (m_rtlText)
    {
      m_strFeed[iFeed].erase(0, 3);
      m_strColors[iFeed].erase(0, 3);
    }
    else
    {
      m_strFeed[iFeed].erase(m_strFeed[iFeed].length() - 3);
      m_strColors[iFeed].erase(m_strColors[iFeed].length() - 3);
    }
  }
  return true;
}

void CRssReader::SetObserver(IRssObserver *observer)
{
  m_pObserver = observer;
}

void CRssReader::UpdateObserver()
{
  if (!m_pObserver)
    return;

  vecText feed;
  getFeed(feed);
  if (!feed.empty())
  {
    std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
    if (m_pObserver) // need to check again when locked to make sure observer wasnt removed
      m_pObserver->OnFeedUpdate(feed);
  }
}

void CRssReader::CheckForUpdates()
{
  KODI::TIME::SystemTime time;
  KODI::TIME::GetLocalTime(&time);

  for (unsigned int i = 0;i < m_vecUpdateTimes.size(); ++i )
  {
    if (m_requestRefresh || ((time.day * 24 * 60) + (time.hour * 60) + time.minute) -
                                    ((m_vecTimeStamps[i]->day * 24 * 60) +
                                     (m_vecTimeStamps[i]->hour * 60) + m_vecTimeStamps[i]->minute) >
                                m_vecUpdateTimes[i])
    {
      CLog::Log(LOGDEBUG, "Updating RSS");
      KODI::TIME::GetLocalTime(m_vecTimeStamps[i]);
      AddToQueue(i);
    }
  }

  m_requestRefresh = false;
}
