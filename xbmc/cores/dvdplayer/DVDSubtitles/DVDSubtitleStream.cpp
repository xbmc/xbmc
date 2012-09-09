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

#include "DVDSubtitleStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/CharsetConverter.h"

using namespace std;

CDVDSubtitleStream::CDVDSubtitleStream()
{
}

CDVDSubtitleStream::~CDVDSubtitleStream()
{
}

bool CDVDSubtitleStream::Open(const string& strFile)
{
  CDVDInputStream* pInputStream;
  pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strFile, "");
  if (pInputStream && pInputStream->Open(strFile.c_str(), ""))
  {
    unsigned char buffer[16384];
    int size_read = 0;
    size_read = pInputStream->Read(buffer,3);
    bool isUTF8 = false;
    bool isUTF16 = false;
    if (buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
      isUTF8 = true;
    else if (buffer[0] == 0xFF && buffer[1] == 0xFE)
    {
      isUTF16 = true;
      pInputStream->Seek(2, SEEK_SET);
    }
    else
      pInputStream->Seek(0, SEEK_SET);

    if (isUTF16)
    {
      std::wstringstream wstringstream;
      while( (size_read = pInputStream->Read(buffer, sizeof(buffer)-2) ) > 0 )
      {
        buffer[size_read] = buffer[size_read + 1] = '\0';
        CStdStringW temp; 
        g_charsetConverter.utf16LEtoW(CStdString16((uint16_t*)buffer),temp); 
        wstringstream << temp; 
      }
      delete pInputStream;

      CStdString strUTF8;
      g_charsetConverter.wToUTF8(CStdStringW(wstringstream.str()),strUTF8);
      m_stringstream.str("");
      m_stringstream << strUTF8;
    }
    else
    {
      while( (size_read = pInputStream->Read(buffer, sizeof(buffer)-1) ) > 0 )
      {
        buffer[size_read] = '\0';
        m_stringstream << buffer;
      }
      delete pInputStream;

      if (!isUTF8)
        isUTF8 = g_charsetConverter.isValidUtf8(m_stringstream.str());

      if (!isUTF8)
      {
        CStdStringW strUTF16;
        CStdString strUTF8;
        g_charsetConverter.subtitleCharsetToW(m_stringstream.str(), strUTF16);
        g_charsetConverter.wToUTF8(strUTF16,strUTF8);
        m_stringstream.str("");
        m_stringstream << strUTF8;
      }
    }
    return true;
  }

  delete pInputStream;
  return false;
}

int CDVDSubtitleStream::Read(char* buf, int buf_size)
{
  return (int)m_stringstream.readsome(buf, buf_size);
}

long CDVDSubtitleStream::Seek(long offset, int whence)
{
  switch (whence)
  {
    case SEEK_CUR:
    {
      m_stringstream.seekg(offset, ios::cur);
      break;
    }
    case SEEK_END:
    {
      m_stringstream.seekg(offset, ios::end);
      break;
    }
    case SEEK_SET:
    {
      m_stringstream.seekg(offset, ios::beg);
      break;
    }
  }
  return (int)m_stringstream.tellg();
}

char* CDVDSubtitleStream::ReadLine(char* buf, int iLen)
{
  if (m_stringstream.getline(buf, iLen))
    return buf;
  else
    return NULL;
}

