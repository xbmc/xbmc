/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


// FileShoutcast.cpp: implementation of the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "ShoutcastFile.h"

#include "FileCache.h"
#include "FileItem.h"
#include "URL.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/RegExp.h"

#include <climits>

using namespace XFILE;
using namespace MUSIC_INFO;
using namespace KODI::MESSAGING;

CShoutcastFile::CShoutcastFile() :
  IFile(), CThread("ShoutcastFile")
{
  m_discarded = 0;
  m_currint = 0;
  m_buffer = NULL;
  m_cacheReader = NULL;
  m_tagPos = 0;
  m_metaint = 0;
}

CShoutcastFile::~CShoutcastFile()
{
  Close();
}

int64_t CShoutcastFile::GetPosition()
{
  return m_file.GetPosition()-m_discarded;
}

int64_t CShoutcastFile::GetLength()
{
  return 0;
}

bool CShoutcastFile::Open(const CURL& url)
{
  CURL url2(url);
  url2.SetProtocolOptions(url2.GetProtocolOptions()+"&noshout=true&Icy-MetaData=1");
  if (url.GetProtocol() == "shouts")
    url2.SetProtocol("https");
  else if (url.GetProtocol() == "shout")
    url2.SetProtocol("http");

  bool result = m_file.Open(url2);
  if (result)
  {
    std::string icyTitle;
    icyTitle = m_file.GetHttpHeader().GetValue("icy-name");
    if (icyTitle.empty())
      icyTitle = m_file.GetHttpHeader().GetValue("ice-name"); // icecast
    m_tag.SetGenre(m_file.GetHttpHeader().GetValue("icy-genre"));
    if (m_tag.GetGenre().empty())
      m_tag.SetGenre(m_file.GetHttpHeader().GetValue("ice-genre")); // icecast
    // Handle no title or badly set up servers where the default icy-name header hasn't been changed
    if (icyTitle.empty() || (icyTitle == "This is my server name"))
      icyTitle = "shoutcast"; // Dummy value used by CMusicGUIInfo.cpp to indicate no station name
                              // set from headers
    m_tag.SetStationName(icyTitle);
    m_tag.SetLoaded(true);
  }
  m_fileCharset = m_file.GetProperty(XFILE::FILE_PROPERTY_CONTENT_CHARSET);
  m_metaint = atoi(m_file.GetHttpHeader().GetValue("icy-metaint").c_str());
  if (!m_metaint)
    m_metaint = -1;
  m_buffer = new char[16*255];
  m_tagPos = 1;

  return result;
}

ssize_t CShoutcastFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  if (m_currint >= m_metaint && m_metaint > 0)
  {
    unsigned char header;
    m_file.Read(&header,1);
    ReadTruncated(m_buffer, header*16);
    if ((ExtractTagInfo(m_buffer)
        // this is here to workaround issues caused by application posting callbacks to itself (3cf882d9)
        // the callback will set an empty tag in the info manager item, while we think we have ours set
        || m_file.GetPosition() < 10*m_metaint) && !m_tagPos)
    {
      m_tagPos = m_file.GetPosition();
      m_tagChange.Set();
    }
    m_discarded += header*16+1;
    m_currint = 0;
  }

  ssize_t toRead;
  if (m_metaint > 0)
    toRead = std::min<size_t>(uiBufSize,m_metaint-m_currint);
  else
    toRead = std::min<size_t>(uiBufSize,16*255);
  toRead = m_file.Read(lpBuf,toRead);
  if (toRead > 0)
    m_currint += toRead;
  return toRead;
}

int64_t CShoutcastFile::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CShoutcastFile::Close()
{
  StopThread();
  delete[] m_buffer;
  m_buffer = NULL;
  m_file.Close();
}

bool CShoutcastFile::ExtractTagInfo(const char* buf)
{
  std::string strBuffer = buf;

  if (!m_fileCharset.empty())
  {
    std::string converted;
    g_charsetConverter.ToUtf8(m_fileCharset, strBuffer, converted);
    strBuffer = converted;
  }
  else
    g_charsetConverter.unknownToUTF8(strBuffer);

  bool result=false;

  std::wstring wBuffer, wConverted;
  g_charsetConverter.utf8ToW(strBuffer, wBuffer, false);
  HTML::CHTMLUtil::ConvertHTMLToW(wBuffer, wConverted);
  g_charsetConverter.wToUTF8(wConverted, strBuffer);

  CRegExp reTitle(true);
  CRegExp otherData(true);
  reTitle.RegComp("StreamTitle=\'(.*?)\';");
  otherData.RegComp("StreamUrl=\'(.*?)\';");

  /* Example of data contained in the metadata buffer (v1 streams only contain the StreamTitle)
     StreamTitle='Tuesday's Gone - Lynyrd Skynyrd';
     StreamUrl='https://listenapi.planetradio.co.uk/api9/eventdata/58431417';
  */

  if (otherData.RegFind(strBuffer.c_str()) != -1)
  {
    std::string getOtherData(otherData.GetMatch(1));
    if (!getOtherData.empty())
      haveExtraData = true;
  }
  if (reTitle.RegFind(strBuffer.c_str()) != -1)
  {
    std::string newtitle(reTitle.GetMatch(1));
    CSingleLock lock(m_tagSection);
    result = (oldTitle != newtitle);

    if (result && !haveExtraData)
      m_tag.SetTitle(newtitle);
    oldTitle = newtitle;

    if (result && haveExtraData) // track has changed and extra metadata might be available
    {
      std::string serverUrl(otherData.GetMatch(1));
      // if the url ends with -1 then there is no extra data to fetch
      if ((serverUrl.find("http") != std::string::npos) &&
          (serverUrl.find("eventdata/-1") == std::string::npos))
      {
        XFILE::CCurlFile http;
        std::string extData;
        CURL dataUrl(serverUrl);
        http.Get(dataUrl.Get(), extData);

      /* Example of data returned from the server
         {"eventId":58431417,"eventStart":"2020-09-15 10:03:23","eventFinish":"2020-09-15 10:10:43","eventDuration":438,"eventType":"Song","eventSongTitle":"Tuesday's Gone",
         "eventSongArtist":"Lynyrd Skynyrd",
         "eventImageUrl":"https://assets.planetradio.co.uk/artist/1-1/320x320/753.jpg?ver=1465083598",
         "eventImageUrlSmall":"https://assets.planetradio.co.uk/artist/1-1/160x160/753.jpg?ver=1465083598",
         "eventAppleMusicUrl":"https://geo.itunes.apple.com/dk/album/287661543?i=287661795"}
      */

        std::size_t titleStart = extData.find("\"eventSongTitle\":");
        std::size_t artistStart = extData.find("\"eventSongArtist\":");
        std::size_t imageStart = extData.find("\"eventImageUrl\":");
        std::size_t imageEnd = extData.find("ImageUrlSmall");
        // ensure we found some valid data
        if (artistStart != std::string::npos && titleStart != std::string::npos)
        {
          titleStart += 18;
          artistStart += 19;
          int getArtist = imageStart - artistStart - 2;
          int getTitle = artistStart - titleStart - 21;
          int getCover = imageEnd - imageStart - 25;

          std::string artistInfo = extData.substr(artistStart, getArtist);
          std::string trackData = extData.substr(titleStart, getTitle);
          std::string coverUrl = extData.substr(imageStart + 17, getCover);

          m_tag.SetArtist(artistInfo);
          m_tag.SetTitle(trackData);
          m_tag.SetShoutcastCover(coverUrl);
        }
      }
      else
      { // blank the tags and cover if adverts etc playing or no info available
        m_tag.SetArtist("");
        m_tag.SetTitle("");
        m_tag.SetShoutcastCover("");
      }
    }
    if (result)
      m_tagChange.Set();
  }

  return result;
}

void CShoutcastFile::ReadTruncated(char* buf2, int size)
{
  char* buf = buf2;
  while (size > 0)
  {
    int read = m_file.Read(buf,size);
    size -= read;
    buf += read;
  }
}

int CShoutcastFile::IoControl(EIoControl control, void* payload)
{
  if (control == IOCTRL_SET_CACHE && m_cacheReader == nullptr)
  {
    m_cacheReader = (CFileCache*)payload;
    Create();
  }

  return IFile::IoControl(control, payload);
}

void CShoutcastFile::Process()
{
  while (!m_bStop)
  {
    if (m_tagChange.WaitMSec(500))
    {
      while (!m_bStop && m_cacheReader->GetPosition() < m_tagPos)
        CThread::Sleep(20);
      CSingleLock lock(m_tagSection);
      CApplicationMessenger::GetInstance().PostMsg(TMSG_UPDATE_CURRENT_ITEM, 1,-1, static_cast<void*>(new CFileItem(m_tag)));
      m_tagPos = 0;
    }
  }
}
