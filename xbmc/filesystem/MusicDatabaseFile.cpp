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

#ifndef FILESYSTEM_MUSICDATABASEFILE_H_INCLUDED
#define FILESYSTEM_MUSICDATABASEFILE_H_INCLUDED
#include "MusicDatabaseFile.h"
#endif

#ifndef FILESYSTEM_MUSIC_MUSICDATABASE_H_INCLUDED
#define FILESYSTEM_MUSIC_MUSICDATABASE_H_INCLUDED
#include "music/MusicDatabase.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif


#include <sys/stat.h>

using namespace XFILE;

CMusicDatabaseFile::CMusicDatabaseFile(void)
{
}

CMusicDatabaseFile::~CMusicDatabaseFile(void)
{
  Close();
}

CStdString CMusicDatabaseFile::TranslateUrl(const CURL& url)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return "";

  CStdString strFileName=URIUtils::GetFileName(url.Get());
  CStdString strExtension = URIUtils::GetExtension(strFileName);
  URIUtils::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName))
    return "";

  long idSong=atol(strFileName.c_str());

  CSong song;
  if (!musicDatabase.GetSong(idSong, song))
    return "";

  StringUtils::ToLower(strExtension);
  if (!URIUtils::HasExtension(song.strFileName, strExtension))
    return "";

  return song.strFileName;
}

bool CMusicDatabaseFile::Open(const CURL& url)
{
  return m_file.Open(TranslateUrl(url));
}

bool CMusicDatabaseFile::Exists(const CURL& url)
{
  return !TranslateUrl(url).empty();
}

int CMusicDatabaseFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return m_file.Stat(TranslateUrl(url), buffer);
}

unsigned int CMusicDatabaseFile::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CMusicDatabaseFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CMusicDatabaseFile::Close()
{
  m_file.Close();
}

int64_t CMusicDatabaseFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CMusicDatabaseFile::GetLength()
{
  return m_file.GetLength();
}

