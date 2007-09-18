// RssReader.cpp: implementation of the CRssReader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RssReader.h"
#include "HTTP.h"
#include "../utils/HTMLUtil.h"
#include "../Util.h"
#include "../xbox/Network.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRssReader::CRssReader() : CThread()
{
  m_pObserver = NULL;
  m_spacesBetweenFeeds = 0;
  m_bIsRunning = false;
  m_iconv = (iconv_t) -1;
  m_shouldFlip = false;
}

CRssReader::~CRssReader()
{
	if (m_bIsRunning)
  {
    StopThread();
    m_bIsRunning = false;
  }
  for (unsigned int i = 0; i < m_vecTimeStamps.size(); i++)
    delete m_vecTimeStamps[i];
}

void CRssReader::Create(IRssObserver* aObserver, const vector<string>& aUrls, const vector<int> &times, int spacesBetweenFeeds)
{
  m_pObserver = aObserver;
  m_spacesBetweenFeeds = spacesBetweenFeeds; 
  m_vecUrls = aUrls;
  m_strFeed.resize(aUrls.size());
  m_strColors.resize(aUrls.size());
  // set update times
  m_vecUpdateTimes = times;
  // update each feed on creation
  for (unsigned int i=0;i<m_vecUpdateTimes.size();++i )
  {
    AddToQueue(i);
    SYSTEMTIME* time = new SYSTEMTIME;
    GetLocalTime(time);
    m_vecTimeStamps.push_back(time);
  }
}

void CRssReader::AddToQueue(int iAdd)
{  
  if (iAdd < (int)m_vecUrls.size())
    m_vecQueue.push_back(iAdd);
  if (!m_bIsRunning)
  {
    StopThread();
    m_bIsRunning = true;
    CThread::Create(false, THREAD_MINSTACKSIZE);
  }
}

void CRssReader::OnExit()
{
  m_bIsRunning = false;
}

void CRssReader::Process()
{
  while (m_vecQueue.size())
  {
    int iFeed = m_vecQueue.front();
    m_vecQueue.erase(m_vecQueue.begin());

    m_strFeed[iFeed] = "";
    m_strColors[iFeed] = "";

    CHTTP http;
    http.SetUserAgent("XBMC/pre-2.1 (http://www.xboxmediacenter.com)");
    CStdString strXML;
    CStdString strUrl = m_vecUrls[iFeed];

    int nRetries = 3;
    CURL url(strUrl);

    if ((url.GetProtocol() == "http" || url.GetProtocol() == "https") && (!g_guiSettings.GetBool("network.enableinternet") || !g_network.IsAvailable()))
      strXML = "<rss><item><title>"+g_localizeStrings.Get(15301)+"</title></item></rss>";
    else
    {
      while ( (!m_bStop) && (nRetries > 0) )
      {
        nRetries--;

        if (url.GetProtocol() != "http" && url.GetProtocol() != "https")
        {
          CFile file;
          if (file.Open(strUrl))
          {
            char *yo = new char[(int)file.GetLength()+1];
            file.Read(yo,file.GetLength());
            yo[file.GetLength()] = '\0';
            strXML = yo;
            delete[] yo;
            break;
          }
        }
        else
          if (http.Get(strUrl, strXML))
          {
            CLog::Log(LOGDEBUG, "Got rss feed: %s", strUrl.c_str());
            break;
          }
      }
    }
    if ((!strXML.IsEmpty()) && m_pObserver)
    {
      // erase any <content:encoded> tags (also unsupported by tinyxml)
      int iStart = strXML.Find("<content:encoded>");
      int iEnd = 0;
      while (iStart > 0)
      {
        // get <content:encoded> end position
        iEnd = strXML.Find("</content:encoded>", iStart) + 18;

        // erase the section
        strXML = strXML.erase(iStart, iEnd - iStart);

        iStart = strXML.Find("<content:encoded>");
      }

      if (Parse((LPSTR)strXML.c_str(),iFeed))
      {
        CLog::Log(LOGDEBUG, "Parsed rss feed: %s", strUrl.c_str());
      }
    }
  }
  UpdateObserver();
}

void CRssReader::getFeed(CStdStringW& strText, LPBYTE& pbColors)
{
  strText.Empty();
  // double the spaces at the start of the set
  strText.append(m_spacesBetweenFeeds, L' ');
  for (unsigned int i=0;i<m_strFeed.size();++i)
  {
    strText.append(m_spacesBetweenFeeds, L' ');
    strText += m_strFeed[i];
  }
  int nTextLength = strText.GetLength();
  pbColors = new BYTE[nTextLength];
  int k=0;
  // double the spaces at the start of the set
  for (int i = 0; i < m_spacesBetweenFeeds; i++)
    pbColors[k++] = 0;
  for (unsigned int j=0;j<m_strColors.size();++j)
  {
    for (int i = 0; i < m_spacesBetweenFeeds; i++)
      pbColors[k++] = 0;
    for (unsigned int i = 0; i < m_strColors[j].size(); i++)
      pbColors[k++] = m_strColors[j][i] - 48;
  }
}

void CRssReader::AddTag(const CStdString aString)
{
  m_tagSet.push_back(aString);
}

void CRssReader::AddString(CStdStringW aString, int aColour, int iFeed)
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
  HTML::CHTMLUtil html;

  TiXmlElement * itemNode = channelXmlNode->FirstChildElement("item");
  map <CStdString, CStdStringW> mTagElements;
  typedef pair <CStdString, CStdStringW> StrPair;
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
          CStdString htmlText = childNode->FirstChild()->Value();

          // This usually happens in right-to-left languages where they want to
          // specify in the RSS body that the text should be RTL.
          // <title>
          //		<div dir="RTL">��� ����: ���� �� �����</div> 
          // </title>
          if (htmlText.Equals("div") || htmlText.Equals("span"))
          {
	          m_shouldFlip = true;
	          htmlText = childNode->FirstChild()->FirstChild()->Value();
          }

          CStdString text;
          CStdStringW unicodeText;

          html.ConvertHTMLToAnsi(htmlText, text);
          fromRSSToUTF16(text, unicodeText);

          mTagElements.insert(StrPair(*i, unicodeText));
        }
      }
      childNode = childNode->NextSibling();
    }

    int rsscolour = RSS_COLOR_HEADLINE;
    for (i = m_tagSet.begin();i != m_tagSet.end();i++)
    {
      map <CStdString, CStdStringW>::iterator j = mTagElements.find(*i);

      if (j == mTagElements.end())
        continue;

      CStdStringW& text = j->second;
      AddString(text, rsscolour, iFeed);
      rsscolour = RSS_COLOR_BODY;
      text = " - ";
      AddString(text, rsscolour, iFeed);
    }
    itemNode = itemNode->NextSiblingElement("item");
  }
}

void CRssReader::fromRSSToUTF16(const CStdStringA& strSource, CStdStringW& strDest)
{
	CStdString flippedStrSource;

	if (m_shouldFlip)
	{
		g_charsetConverter.logicalToVisualBiDi(strSource, flippedStrSource, m_encoding, FRIBIDI_TYPE_RTL);
	}
	else
	{
		flippedStrSource = strSource;
	}
	
	if (m_iconv != (iconv_t) - 1)
    {
		const char* src = flippedStrSource.c_str();
		size_t inBytes = flippedStrSource.length() + 1;

		wchar_t outBuf[1024];
		char* dst = (char*) &outBuf[0];
		size_t outBytes=1024;
		size_t originalOutBytes = outBytes;

		iconv(m_iconv, NULL, &inBytes, NULL, &outBytes);

#ifdef _LINUX
		if (iconv(m_iconv, (char**) &src, &inBytes, &dst, &outBytes) == -1)
#else
		if (iconv(m_iconv, &src, &inBytes, &dst, &outBytes) == -1)
#endif
		{
			// For some reason it failed (maybe wrong charset?). Nothing to do but
			// return the original..
			strDest = flippedStrSource;
			return;
		}

		outBuf[(originalOutBytes - outBytes) / 2] = '\0';
		strDest = outBuf;
	}
	else
	{
		strDest = flippedStrSource;
		return;
	}
}

bool CRssReader::Parse(LPSTR szBuffer, int iFeed)
{
  m_xml.Clear();
  m_xml.Parse((LPCSTR)szBuffer, 0, TIXML_ENCODING_LEGACY);

  if (m_iconv != (iconv_t) -1)
  {
	  iconv_close(m_iconv);
      m_iconv = (iconv_t) -1;
	  m_shouldFlip = false;
  }

  m_encoding = "UTF-8";
  if (m_xml.RootElement())
  {
	TiXmlDeclaration *tiXmlDeclaration = m_xml.RootElement()->Parent()->FirstChild()->ToDeclaration();
	if (tiXmlDeclaration != NULL && strlen(tiXmlDeclaration->Encoding()) > 0)
	{
		m_encoding = tiXmlDeclaration->Encoding();
	}
  }

  CLog::Log(LOGDEBUG, "RSS feed encoding: %s", m_encoding.c_str());
#ifndef _LINUX  
  m_iconv = iconv_open("UTF-16LE", m_encoding.c_str());
#else
  m_iconv = iconv_open("WCHAR_T", m_encoding.c_str());
#endif  

  if (g_charsetConverter.isBidiCharset(m_encoding))
  {
	  m_shouldFlip = true;
  }

  return Parse(iFeed);
}

bool CRssReader::Parse(int iFeed)
{
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
      CStdString strChannel = titleNode->FirstChild()->Value();
	  CStdStringW strChannelUnicode;
	  fromRSSToUTF16(strChannel, strChannelUnicode);
	  AddString(strChannelUnicode, RSS_COLOR_CHANNEL, iFeed);

	  AddString(": ", RSS_COLOR_CHANNEL, iFeed);
    }

    GetNewsItems(channelXmlNode,iFeed);
  }

  GetNewsItems(rssXmlNode,iFeed);

  // avoid trailing ' - '
  if( m_strFeed[iFeed].size() > 3 && m_strFeed[iFeed].Mid(m_strFeed[iFeed].size()-3) == L" - ")
  {
    m_strFeed[iFeed].erase(m_strFeed[iFeed].length()-3);
    m_strColors[iFeed].erase(m_strColors[iFeed].length()-3);
  }
  return true;
}

void CRssReader::SetObserver(IRssObserver *observer)
{
  m_pObserver = observer;
}

void CRssReader::UpdateObserver()
{
  if (!m_pObserver) return;
  CStdStringW strFeed;
  LPBYTE pbColors = NULL;
  getFeed(strFeed,pbColors);
  
  // lock so make sure m_pObserver does not get deleted
  g_graphicsContext.Lock();
  if (strFeed.size() > 0 && m_pObserver)
  {
    m_pObserver->OnFeedUpdate(strFeed, pbColors);
  }
  g_graphicsContext.Unlock();

}

void CRssReader::CheckForUpdates()
{
  SYSTEMTIME time;
  GetLocalTime(&time);
  for (unsigned int i = 0;i < m_vecUpdateTimes.size(); ++i )
  {
    if (((time.wDay * 24 * 60) + (time.wHour * 60) + time.wMinute) - ((m_vecTimeStamps[i]->wDay * 24 * 60) + (m_vecTimeStamps[i]->wHour * 60) + m_vecTimeStamps[i]->wMinute) > m_vecUpdateTimes[i] )
    {
      CLog::Log(LOGDEBUG, "Updating RSS");
      GetLocalTime(m_vecTimeStamps[i]);
      AddToQueue(i);
    }
  }
}

CRssManager g_rssManager;

CRssManager::CRssManager()
{
}

CRssManager::~CRssManager()
{
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].reader)
    {
      m_readers[i].reader->StopThread();
      delete m_readers[i].reader;
    }
  }
  m_readers.clear();
}

// returns true if the reader doesn't need creating, false otherwise
bool CRssManager::GetReader(DWORD controlID, DWORD windowID, IRssObserver* observer, CRssReader *&reader)
{
  // check to see if we've already created this reader
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].controlID == controlID && m_readers[i].windowID == windowID)
    {
      reader = m_readers[i].reader;
      reader->SetObserver(observer);
      reader->UpdateObserver();
      return true;
    }
  }
  // need to create a new one
  READERCONTROL readerControl;
  readerControl.controlID = controlID;
  readerControl.windowID = windowID;
  reader = readerControl.reader = new CRssReader;
  m_readers.push_back(readerControl);
  return false;
}
