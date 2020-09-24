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
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/JSONVariantParser.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/UrlOptions.h"

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
    if (icyTitle == "This is my server name") // Handle badly set up servers
      icyTitle.clear();

    std::string icyGenre = m_file.GetHttpHeader().GetValue("icy-genre");
    if (icyGenre.empty())
      icyGenre = m_file.GetHttpHeader().GetValue("ice-genre"); // icecast

    {
      CSingleLock lock(m_tagSection);
      m_tag.SetStationName(icyTitle);
      m_tag.SetGenre(icyGenre);
      m_tag.SetLoaded(true);
    }
  }
  m_fileCharset = m_file.GetProperty(XFILE::FILE_PROPERTY_CONTENT_CHARSET);
  m_metaint = atoi(m_file.GetHttpHeader().GetValue("icy-metaint").c_str());
  if (!m_metaint)
    m_metaint = -1;
  m_buffer = new char[16*255];
  m_tagPos = 1;
  m_tagChange.Set();

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
  m_title.clear();

  {
    CSingleLock lock(m_tagSection);
    m_tag.Clear();
    m_tagChange.Set();
  }
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
  reTitle.RegComp("StreamTitle=\'(.*?)\';");

  if (reTitle.RegFind(strBuffer.c_str()) != -1)
  {
    const std::string newtitle = reTitle.GetMatch(1);

    result = (m_title != newtitle);
    if (result) // track has changed
    {
      m_title = newtitle;

      std::string title;
      std::string artistInfo;
      std::string coverURL;

      CRegExp reURL(true);
      reURL.RegComp("StreamUrl=\'(.*?)\';");
      bool haveStreamUrlData =
          (reURL.RegFind(strBuffer.c_str()) != -1) && !reURL.GetMatch(1).empty();

      if (haveStreamUrlData) // track has changed and extra metadata might be available
      {
        const std::string streamUrlData = reURL.GetMatch(1);
        if (StringUtils::StartsWithNoCase(streamUrlData, "http://") ||
            StringUtils::StartsWithNoCase(streamUrlData, "https://"))
        {
          // Bauer Media Radio listenapi null event to erase current data
          if (!StringUtils::EndsWithNoCase(streamUrlData, "eventdata/-1"))
          {
            const CURL dataURL(streamUrlData);
            XFILE::CCurlFile http;
            std::string extData;

            if (http.Get(dataURL.Get(), extData))
            {
              const std::string contentType = http.GetHttpHeader().GetMimeType();
              if (StringUtils::EqualsNoCase(contentType, "application/json"))
              {
                CVariant json;
                if (CJSONVariantParser::Parse(extData, json))
                {
                  // Check for Bauer Media Radio listenapi meta data.
                  // Example: StreamUrl='https://listenapi.bauerradio.com/api9/eventdata/58431417'
                  artistInfo = json["eventSongArtist"].asString();
                  title = json["eventSongTitle"].asString();
                  coverURL = json["eventImageUrl"].asString();
                }
              }
            }
          }
        }
        else if (StringUtils::StartsWithNoCase(streamUrlData, "&"))
        {
          // Check for SAM Cast meta data.
          // Example: StreamUrl='&artist=RECLAM&title=BOLORDURAN%2017&album=&duration=17894&songtype=S&overlay=no&buycd=&website=&picture='

          CUrlOptions urlOptions(streamUrlData);
          const CUrlOptions::UrlOptions& options = urlOptions.GetOptions();

          auto it = options.find("artist");
          if (it != options.end())
            artistInfo = (*it).second.asString();

          it = options.find("title");
          if (it != options.end())
            title = (*it).second.asString();
        }
      }

      if (artistInfo.empty() || title.empty())
      {
        // Most stations supply StreamTitle in format "artist - songtitle"
        const std::vector<std::string> tokens = StringUtils::Split(newtitle, " - ");
        if (tokens.size() == 2)
        {
          if (artistInfo.empty())
            artistInfo = tokens[0];

          if (title.empty())
            title = tokens[1];
        }
        else
        {
          if (title.empty())
          {
            // Do not display Bauer Media Radio SteamTitle values to mark start/stop of ad breaks.
            if (!StringUtils::StartsWithNoCase(newtitle, "START ADBREAK ") &&
                !StringUtils::StartsWithNoCase(newtitle, "STOP ADBREAK "))
              title = newtitle;
          }
        }
      }

      if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bShoutcastArt)
        coverURL.clear();

      CSingleLock lock(m_tagSection);
      m_tag.SetArtist(artistInfo);
      m_tag.SetTitle(title);
      m_tag.SetStationArt(coverURL);
    }
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
