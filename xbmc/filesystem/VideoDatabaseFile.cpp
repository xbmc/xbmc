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

#include "VideoDatabaseFile.h"
#include "video/VideoDatabase.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <sys/stat.h>

using namespace XFILE;

CVideoDatabaseFile::CVideoDatabaseFile(void)
{
}

CVideoDatabaseFile::~CVideoDatabaseFile(void)
{
  Close();
}

CStdString CVideoDatabaseFile::TranslateUrl(const CURL& url)
{
  CStdString strFileName=URIUtils::GetFileName(url.Get());
  if (strFileName.IsEmpty())
	  return "";

  CStdString strPath;
  URIUtils::GetDirectory(url.Get(), strPath);
  if (strPath.IsEmpty())
	  return "";

  CStdString strExtension;
  URIUtils::GetExtension(strFileName, strExtension);
  URIUtils::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName))
    return "";
  long idDb=atol(strFileName.c_str());

  CStdStringArray pathElem;
  StringUtils::SplitString(strPath, "/", pathElem);
  if (pathElem.size() == 0)
    return "";

  CStdString itemType = pathElem.at(2);
  VIDEODB_CONTENT_TYPE type;
  if (itemType.Equals("movies") || itemType.Equals("recentlyaddedmovies"))
    type = VIDEODB_CONTENT_MOVIES;
  else if (itemType.Equals("episodes") || itemType.Equals("recentlyaddedepisodes"))
    type = VIDEODB_CONTENT_EPISODES;
  else if (itemType.Equals("musicvideos") || itemType.Equals("recentlyaddedmusicvideos"))
    type = VIDEODB_CONTENT_MUSICVIDEOS;
  else
    return "";

  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
    return "";

  CStdString realFilename;
  videoDatabase.GetFilePathById(idDb, realFilename, type);

  return realFilename;
}

bool CVideoDatabaseFile::Open(const CURL& url)
{
  return m_file.Open(TranslateUrl(url));
}

bool CVideoDatabaseFile::Exists(const CURL& url)
{
  return !TranslateUrl(url).IsEmpty();
}

int CVideoDatabaseFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return m_file.Stat(TranslateUrl(url), buffer);
}

unsigned int CVideoDatabaseFile::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CVideoDatabaseFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CVideoDatabaseFile::Close()
{
  m_file.Close();
}

int64_t CVideoDatabaseFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CVideoDatabaseFile::GetLength()
{
  return m_file.GetLength();
}

