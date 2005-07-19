// RssReader.cpp: implementation of the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "RssReader.h"
#include "Http.h"
#include "../utils/HTMLUtil.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRssReader::CRssReader() : CThread()
{
  m_pObserver = NULL;
  m_iLeadingSpaces = 0;
  m_bIsRunning = false;
}

CRssReader::~CRssReader()
{
	if (m_bIsRunning)
  {
    StopThread();
    m_bIsRunning = false;
  }
}

void CRssReader::Create(IRssObserver* aObserver, const vector<wstring>& aUrls, INT iLeadingSpaces)
{
  m_pObserver = aObserver;
  m_iLeadingSpaces = iLeadingSpaces;
  m_vecUrls = aUrls;
  m_strFeed.resize(aUrls.size());
  m_strColors.resize(aUrls.size());
}

void CRssReader::AddToQueue(int iAdd)
{
  if (iAdd < (int)m_vecUrls.size())
    m_vecQueue.push_back(iAdd);
  if (!m_bIsRunning)
  {
    m_bIsRunning = true;
    StopThread();
    CThread::Create();
  }
}

void CRssReader::OnExit()
{
  m_bIsRunning = false;
}

void CRssReader::Process()
{
  int tempLeading = m_iLeadingSpaces;
  CStdString strFeed;
  LPBYTE pbColors = NULL;
  while (m_vecQueue.size())
  {
    int iFeed = m_vecQueue.front();
    m_vecQueue.erase(m_vecQueue.begin());

    m_strFeed[iFeed] = "";
    m_strColors[iFeed] = "";

    if (iFeed > 0)
    {
      m_iLeadingSpaces = tempLeading / 2;
    }
    else
    {
      m_iLeadingSpaces = tempLeading;
    }

    CHTTP http;
    CStdString strXML;
    CStdString strUrl = m_vecUrls[iFeed];

    int nRetries = 3;

    while ( (!m_bStop) && (nRetries > 0) )
    {
      nRetries--;

      if (http.Get(strUrl, strXML))
      {
        CLog::Log(LOGDEBUG, "Got rss feed: %s", strUrl.c_str());
        break;
      }
    }

    bool forceUTF8(false);
    if (strXML[0] == (char)0xff && strXML[1] == (char)0xfe)  // byte order marker -> unicode
    { // convert to utf8
      strXML = strXML.Mid(2);
      CStdString strUTF8;
      g_charsetConverter.UTF16toUTF8((WCHAR *)strXML.c_str(), strUTF8);
      strXML = strUTF8;
      forceUTF8 = true;
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

      // erase any <content:encoded> tags (also unsupported by tinyxml)
      iStart = strXML.Find("<content:encoded>");
      iEnd = 0;
      while (iStart > 0)
      {
        // get <content:encoded> end position
        iEnd = strXML.Find("</content:encoded>", iStart) + 18;

        // erase the section
        strXML = strXML.erase(iStart, iEnd - iStart);

        iStart = strXML.Find("<content:encoded>");
      }

      if (Parse((LPSTR)strXML.c_str(),iFeed, forceUTF8))
      {
        CLog::Log(LOGDEBUG, "Parsed rss feed: %s", strUrl.c_str());
      }
    }
  }
  getFeed(strFeed,pbColors);
  if (strFeed.size() > 0)
  {
    g_graphicsContext.Lock();
    m_pObserver->OnFeedUpdate(strFeed, pbColors);
    g_graphicsContext.Unlock();
  }
  m_iLeadingSpaces = tempLeading;
}

void CRssReader::getFeed(CStdString& strText, LPBYTE& pbColors)
{
  strText.Empty();
  for (unsigned int i=0;i<m_strFeed.size();++i)
    strText += m_strFeed[i];
  int nTextLength = strText.GetLength();
  pbColors = new BYTE[nTextLength];
  int k=0;
  for (unsigned int j=0;j<m_strColors.size();++j)
    for (unsigned int i = 0; i < m_strColors[j].size(); i++)
    {
      pbColors[k++] = m_strColors[j][i] - 48;
    }
}

void CRssReader::AddTag(const CStdString aString)
{
  m_tagSet.push_back(aString);
}

void CRssReader::AddString(CStdString aString, int aColour, int iFeed)
{
  m_strFeed[iFeed] += aString;

  int nStringLength = aString.GetLength();

  for (int i = 0;i < nStringLength;i++)
  {
    aString[i] = (CHAR) (48 + aColour);
  }

  m_strColors[iFeed] += aString;
}

void CRssReader::GetNewsItems(TiXmlElement* channelXmlNode, int iFeed)
{
  TiXmlElement * itemNode = channelXmlNode->FirstChildElement("item");
  map <CStdString, CStdString> mTagElements;
  typedef pair <CStdString, CStdString> StrPair;
  list <CStdString>::iterator i;

  bool bEmpty=true;

  // Add the title tag in if we didn't pass any tags in at all
  // Represents default behaviour before configurability

  if (m_tagSet.empty())
    AddTag("title");

  while (itemNode > 0)
  {
    bEmpty = false;
    TiXmlNode* childNode = itemNode->FirstChild();
    mTagElements.clear();
    while (childNode > 0)
    {
      CStdString strName = childNode->Value();

      for (i = m_tagSet.begin(); i != m_tagSet.end(); i++)
      {
        if (!childNode->NoChildren() && i->Equals(strName))
        {
          mTagElements.insert(StrPair(*i, childNode->FirstChild()->Value()));
        }
      }
      childNode = childNode->NextSibling();
    }

    int rsscolour = RSS_COLOR_HEADLINE;
    for (i = m_tagSet.begin();i != m_tagSet.end();i++)
    {
      CStdString text;
      HTML::CHTMLUtil html;
      map <CStdString, CStdString>::iterator j = mTagElements.find(*i);

      if (j == mTagElements.end())
        continue;

      html.ConvertHTMLToAnsi(j->second, text);

      AddString(text, rsscolour, iFeed);
      rsscolour = RSS_COLOR_BODY;
      text = " - ";
      AddString(text, rsscolour, iFeed);
    }
    itemNode = itemNode->NextSiblingElement("item");
  }
  // spiff - avoid trailing ' - '
  if( bEmpty )
  {
    m_strFeed[iFeed].erase(m_strFeed[iFeed].length()-3);
    m_strColors[iFeed].erase(m_strColors[iFeed].length()-3);
  }
}

bool CRssReader::Parse(LPSTR szBuffer, int iFeed, bool forceUTF8)
{
  m_xml.Clear();
  m_xml.Parse((LPCSTR)szBuffer, 0, forceUTF8 ? TIXML_ENCODING_UTF8 : TIXML_DEFAULT_ENCODING);
  return Parse(iFeed);
}

bool CRssReader::Parse(int iFeed)
{
  CStdString strLeadingSpace = " ";
  for (int i = 0; i < m_iLeadingSpaces; i++)
  {
    AddString(strLeadingSpace, 0, iFeed);
  }

  TiXmlElement* rootXmlNode = m_xml.RootElement();

  if (!rootXmlNode)
    return false;

  TiXmlElement* rssXmlNode = NULL;

  CStdString strValue = rootXmlNode->Value();
  if (( strValue.Find("rss") >= 0 ) || ( strValue.Find("rdf") >= 0 ))
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
    if (titleNode && !titleNode->NoChildren())
    {
      CStdString strChannel;
      strChannel.Format("%s: ", titleNode->FirstChild()->Value());
      AddString(strChannel, RSS_COLOR_CHANNEL, iFeed);
    }

    GetNewsItems(channelXmlNode,iFeed);
  }

  GetNewsItems(rssXmlNode,iFeed);
  return true;
}
