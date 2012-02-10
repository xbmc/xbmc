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


// FileShoutcast.cpp: implementation of the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#include "threads/SystemClock.h"
#include "system.h"
#include "Application.h"
#include "FileShoutcast.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "utils/HTMLUtil.h"
#include "utils/CharsetConverter.h"
#include "utils/TimeUtils.h"
#include "GUIInfoManager.h"
#include "utils/log.h"

using namespace XFILE;
using namespace MUSIC_INFO;

CFileShoutcast::CFileShoutcast()
{
  m_lastTime = XbmcThreads::SystemClockMillis();
  m_discarded = 0;
  m_currint = 0;
  m_buffer = NULL;
}

CFileShoutcast::~CFileShoutcast()
{
  Close();
}

int64_t CFileShoutcast::GetPosition()
{
  return m_file.GetPosition()-m_discarded;
}

int64_t CFileShoutcast::GetLength()
{
  return 0;
}

bool CFileShoutcast::Open(const CURL& url)
{
  CURL url2(url);
  url2.SetProtocolOptions("noshout=true&Icy-MetaData=1");
  url2.SetProtocol("http");

  bool result=false;
  if ((result=m_file.Open(url2.Get())))
  {
    m_tag.SetTitle(m_file.GetHttpHeader().GetValue("icy-name"));
    if (m_tag.GetTitle().IsEmpty())
      m_tag.SetTitle(m_file.GetHttpHeader().GetValue("ice-name")); // icecast
    m_tag.SetGenre(m_file.GetHttpHeader().GetValue("icy-genre"));
    if (m_tag.GetGenre().IsEmpty())
      m_tag.SetGenre(m_file.GetHttpHeader().GetValue("ice-genre")); // icecast
    m_tag.SetLoaded(true);
    g_infoManager.SetCurrentSongTag(m_tag);
  }
  m_metaint = atoi(m_file.GetHttpHeader().GetValue("icy-metaint").c_str());
  if (!m_metaint)
    m_metaint = -1;
  m_buffer = new char[16*255];

  return result;
}

unsigned int CFileShoutcast::Read(void* lpBuf, int64_t uiBufSize)
{
  if (m_currint >= m_metaint && m_metaint > 0)
  {
    unsigned char header;
    m_file.Read(&header,1);
    ReadTruncated(m_buffer, header*16);
    ExtractTagInfo(m_buffer);
    m_discarded += header*16+1;
    m_currint = 0;
  }
  if (XbmcThreads::SystemClockMillis() - m_lastTime > 500)
  {
    m_lastTime = XbmcThreads::SystemClockMillis();
    g_infoManager.SetCurrentSongTag(m_tag);
  }

  unsigned int toRead;
  if (m_metaint > 0)
    toRead = std::min((unsigned int)uiBufSize,(unsigned int)m_metaint-m_currint);
  else
    toRead = std::min((unsigned int)uiBufSize,(unsigned int)16*255);
  toRead = m_file.Read(lpBuf,toRead);
  m_currint += toRead;
  return toRead;
}

int64_t CFileShoutcast::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CFileShoutcast::Close()
{
  delete[] m_buffer;
  m_file.Close();
}

void CFileShoutcast::ExtractTagInfo(const char* buf)
{
  CStdString strBuffer = buf;
  g_charsetConverter.unknownToUTF8(strBuffer);

  CStdStringW wBuffer, wConverted;
  g_charsetConverter.utf8ToW(strBuffer, wBuffer, false);
  HTML::CHTMLUtil::ConvertHTMLToW(wBuffer, wConverted);
  g_charsetConverter.wToUTF8(wConverted, strBuffer);

  CRegExp reTitle(true);
  reTitle.RegComp("StreamTitle=\'(.*?)\';");

  if (reTitle.RegFind(strBuffer.c_str()) != -1)
    m_tag.SetTitle(reTitle.GetReplaceString("\\1"));
}

void CFileShoutcast::ReadTruncated(char* buf2, int size)
{
  char* buf = buf2;
  while (size > 0)
  {
    int read = m_file.Read(buf,size);
    size -= read;
    buf += read;
  }
}

