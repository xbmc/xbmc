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


// FileShoutcast.cpp: implementation of the CShoutcastFile class.
//
//////////////////////////////////////////////////////////////////////

#include "system.h"
#include "ShoutcastFile.h"
#include "guilib/GUIWindowManager.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "utils/HTMLUtil.h"
#include "utils/CharsetConverter.h"
#include "ApplicationMessenger.h"
#include "FileCache.h"
#include <climits>

using namespace XFILE;
using namespace MUSIC_INFO;

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
  StopThread();
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
  url2.SetProtocol("http");

  bool result = m_file.Open(url2);
  if (result)
  {
    m_tag.SetTitle(m_file.GetHttpHeader().GetValue("icy-name"));
    if (m_tag.GetTitle().empty())
      m_tag.SetTitle(m_file.GetHttpHeader().GetValue("ice-name")); // icecast
    m_tag.SetGenre(m_file.GetHttpHeader().GetValue("icy-genre"));
    if (m_tag.GetGenre().empty())
      m_tag.SetGenre(m_file.GetHttpHeader().GetValue("ice-genre")); // icecast
    m_tag.SetLoaded(true);
  }
  m_fileCharset = m_file.GetServerReportedCharset();
  m_metaint = atoi(m_file.GetHttpHeader().GetValue("icy-metaint").c_str());
  if (!m_metaint)
    m_metaint = -1;
  m_buffer = new char[16*255];
  m_tagPos = 1;
  m_tagChange.Set();
  Create();

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
    if (ExtractTagInfo(m_buffer)
        // this is here to workaround issues caused by application posting callbacks to itself (3cf882d9)
        // the callback will set an empty tag in the info manager item, while we think we have ours set
        || (m_file.GetPosition() < 10*m_metaint && !m_tagPos))
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
  reTitle.RegComp("StreamTitle=\'(.*?)\';");

  if (reTitle.RegFind(strBuffer.c_str()) != -1)
  {
    std::string newtitle(reTitle.GetMatch(1));
    result = (m_tag.GetTitle() != newtitle);
    m_tag.SetTitle(newtitle);
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
  if (control == IOCTRL_SET_CACHE)
    m_cacheReader = (CFileCache*)payload;

  return IFile::IoControl(control, payload);
}

void CShoutcastFile::Process()
{
  if (!m_cacheReader)
    return;

  while (!m_bStop)
  {
    if (m_tagChange.WaitMSec(500))
    {
      while (!m_bStop && m_cacheReader->GetPosition() < m_tagPos)
        Sleep(20);
      CApplicationMessenger::Get().SetCurrentSongTag(m_tag);
      m_tagPos = 0;
    }
  }
}
