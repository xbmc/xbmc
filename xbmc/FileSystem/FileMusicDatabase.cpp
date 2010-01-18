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

#include "FileMusicDatabase.h"
#include "MusicDatabase.h"
#include "Util.h"
#include "URL.h"
#include "StringUtils.h"

#include <sys/stat.h>

using namespace XFILE;

CFileMusicDatabase::CFileMusicDatabase(void)
{
}

CFileMusicDatabase::~CFileMusicDatabase(void)
{
  Close();
}

CStdString CFileMusicDatabase::TranslateUrl(const CURL& url)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return "";

  CStdString strFileName=CUtil::GetFileName(url.Get());
  CStdString strExtension;
  CUtil::GetExtension(strFileName, strExtension);
  CUtil::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName))
    return "";

  long idSong=atol(strFileName.c_str());

  CSong song;
  if (!musicDatabase.GetSongById(idSong, song))
    return "";

  CStdString strExtensionFromDb;
  CUtil::GetExtension(song.strFileName, strExtensionFromDb);

  if (!strExtensionFromDb.Equals(strExtension))
    return "";

  return song.strFileName;
}

bool CFileMusicDatabase::Open(const CURL& url)
{
  return m_file.Open(TranslateUrl(url));
}

bool CFileMusicDatabase::Exists(const CURL& url)
{
  return !TranslateUrl(url).IsEmpty();
}

int CFileMusicDatabase::Stat(const CURL& url, struct __stat64* buffer)
{
  return m_file.Stat(TranslateUrl(url), buffer);
}

unsigned int CFileMusicDatabase::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CFileMusicDatabase::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CFileMusicDatabase::Close()
{
  m_file.Close();
}

int64_t CFileMusicDatabase::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CFileMusicDatabase::GetLength()
{
  return m_file.GetLength();
}

