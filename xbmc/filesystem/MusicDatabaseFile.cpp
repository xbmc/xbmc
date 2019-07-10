/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicDatabaseFile.h"

#include "URL.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CMusicDatabaseFile::CMusicDatabaseFile(void) = default;

CMusicDatabaseFile::~CMusicDatabaseFile(void)
{
  Close();
}

std::string CMusicDatabaseFile::TranslateUrl(const CURL& url)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return "";

  std::string strFileName=URIUtils::GetFileName(url.Get());
  std::string strExtension = URIUtils::GetExtension(strFileName);
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

ssize_t CMusicDatabaseFile::Read(void* lpBuf, size_t uiBufSize)
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

