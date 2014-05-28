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

#ifndef DVDSUBTITLES_DVDSUBTITLESTREAM_H_INCLUDED
#define DVDSUBTITLES_DVDSUBTITLESTREAM_H_INCLUDED
#include "DVDSubtitleStream.h"
#endif

#ifndef DVDSUBTITLES_DVDINPUTSTREAMS_DVDFACTORYINPUTSTREAM_H_INCLUDED
#define DVDSUBTITLES_DVDINPUTSTREAMS_DVDFACTORYINPUTSTREAM_H_INCLUDED
#include "DVDInputStreams/DVDFactoryInputStream.h"
#endif

#ifndef DVDSUBTITLES_DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#define DVDSUBTITLES_DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#include "DVDInputStreams/DVDInputStream.h"
#endif

#ifndef DVDSUBTITLES_UTILS_CHARSETCONVERTER_H_INCLUDED
#define DVDSUBTITLES_UTILS_CHARSETCONVERTER_H_INCLUDED
#include "utils/CharsetConverter.h"
#endif

#ifndef DVDSUBTITLES_UTILS_UTF8UTILS_H_INCLUDED
#define DVDSUBTITLES_UTILS_UTF8UTILS_H_INCLUDED
#include "utils/Utf8Utils.h"
#endif

#ifndef DVDSUBTITLES_UTILS_CHARSETDETECTION_H_INCLUDED
#define DVDSUBTITLES_UTILS_CHARSETDETECTION_H_INCLUDED
#include "utils/CharsetDetection.h"
#endif

#ifndef DVDSUBTITLES_FILESYSTEM_FILE_H_INCLUDED
#define DVDSUBTITLES_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif


using namespace std;
using XFILE::auto_buffer;

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
    static const size_t chunksize = 64 * 1024;
    auto_buffer buf;

    // read content
    size_t totalread = 0;
    int read;
    do
    {
      if (totalread == buf.size())
        buf.resize(buf.size() + chunksize);

      read = pInputStream->Read((uint8_t*)buf.get() + totalread, buf.size() - totalread);
      if (read > 0)
        totalread += read;
    } while (read > 0);

    delete pInputStream;
    if (!totalread)
      return false;

    std::string tmpStr(buf.get(), totalread);
    buf.clear();

    std::string enc(CCharsetDetection::GetBomEncoding(tmpStr));
    if (enc == "UTF-8" || (enc.empty() && CUtf8Utils::isValidUtf8(tmpStr)))
      m_stringstream << tmpStr;
    else if (!enc.empty())
    {
      std::string converted;
      g_charsetConverter.ToUtf8(enc, tmpStr, converted);
      if (converted.empty())
        return false;

      m_stringstream << converted;
    }
    else
    {
      std::string converted;
      g_charsetConverter.subtitleCharsetToUtf8(tmpStr, converted);
      if (converted.empty())
        return false;

      m_stringstream << converted;
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

