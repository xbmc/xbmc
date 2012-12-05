/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "FileURLProtocol.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "FileItem.h"

#define URL_RDONLY 1  /**< read-only */
#define URL_WRONLY 2  /**< write-only */
#define URL_RDWR   (URL_RDONLY|URL_WRONLY)  /**< read-write */

#define AVSEEK_SIZE  0x10000
#define AVSEEK_FORCE 0x20000

//========================================================================
int CFileURLProtocol::Open(AML_URLContext *h, const char *filename, int flags)
{
  if (flags != URL_RDONLY)
  {
    CLog::Log(LOGDEBUG, "CFileURLProtocol::Open: Only read-only is supported");
    return -EINVAL;
  }

  CStdString url = filename;
  if (url.Left(strlen("xb-http://")).Equals("xb-http://"))
  {
    url = url.Right(url.size() - strlen("xb-"));
  }
  else if (url.Left(strlen("xb-https://")).Equals("xb-https://"))
  {
    url = url.Right(url.size() - strlen("xb-"));
  }
  else if (url.Left(strlen("xb-ftp://")).Equals("xb-ftp://"))
  {
    url = url.Right(url.size() - strlen("xb-"));
  }
  else if (url.Left(strlen("xb-ftps://")).Equals("xb-ftps://"))
  {
    url = url.Right(url.size() - strlen("xb-"));
  }
  else if (url.Left(strlen("xb-hdhomerun://")).Equals("xb-hdhomerun://"))
  {
    url = url.Right(url.size() - strlen("xb-"));
  }
  CLog::Log(LOGDEBUG, "CFileURLProtocol::Open filename2(%s)", url.c_str());
  // open the file, always in read mode, calc bitrate
  unsigned int cflags = READ_BITRATE;
  XFILE::CFile *cfile = new XFILE::CFile();

  if (CFileItem(url, true).IsInternetStream())
    cflags |= READ_CACHED;

  // open file in binary mode
  if (!cfile->Open(url, cflags))
  {
    delete cfile;
    return -EIO;
  }

  h->priv_data = (void *)cfile;

  return 0;
}

//========================================================================
int CFileURLProtocol::Read(AML_URLContext *h, unsigned char *buf, int size)
{
  XFILE::CFile *cfile = (XFILE::CFile*)h->priv_data;

  int readsize = cfile->Read(buf, size);
  //CLog::Log(LOGDEBUG, "CFileURLProtocol::Read size(%d), readsize(%d)", size, readsize);

  return readsize;
}

//========================================================================
int CFileURLProtocol::Write(AML_URLContext *h, unsigned char *buf, int size)
{
  //CLog::Log(LOGDEBUG, "CFileURLProtocol::Write size(%d)", size);
  return 0;
}

//========================================================================
int64_t CFileURLProtocol::Seek(AML_URLContext *h, int64_t pos, int whence)
{
  //CLog::Log(LOGDEBUG, "CFileURLProtocol::Seek1 pos(%lld), whence(%d)", pos, whence);
  XFILE::CFile *cfile = (XFILE::CFile*)h->priv_data;
  whence &= ~AVSEEK_FORCE;

  // seek to the end of file
  if (pos == -1 || whence == AVSEEK_SIZE)
    pos = cfile->GetLength();
  else
    pos = cfile->Seek(pos, whence);

  //CLog::Log(LOGDEBUG, "CFileURLProtocol::Seek2 pos(%lld), whence(%d)", pos, whence);

  return pos;
}

//========================================================================
int64_t CFileURLProtocol::SeekEx(AML_URLContext *h, int64_t pos, int whence)
{
  //CLog::Log(LOGDEBUG, "CFileURLProtocol::SeekEx1 pos(%lld), whence(%d)", pos, whence);
  XFILE::CFile *cfile = (XFILE::CFile*)h->priv_data;
  whence &= ~AVSEEK_FORCE;

  // seek to the end of file
  if (pos == -1 || whence == AVSEEK_SIZE)
    pos = cfile->GetLength();
  else
    pos = cfile->Seek(pos, whence);

  //CLog::Log(LOGDEBUG, "CFileURLProtocol::SeekEx2 pos(%lld), whence(%d)", pos, whence);

  return pos;
}

//========================================================================
int CFileURLProtocol::Close(AML_URLContext *h)
{
  CLog::Log(LOGDEBUG, "CFileURLProtocol::Close");
  XFILE::CFile *cfile = (XFILE::CFile*)h->priv_data;
  cfile->Close();
  delete cfile;

  return 0;
}
