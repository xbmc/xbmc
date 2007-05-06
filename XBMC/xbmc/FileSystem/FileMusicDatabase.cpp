#include "stdafx.h"
#include "FileMusicDatabase.h"
#include "../MusicDatabase.h"
#include <sys/stat.h>
#include "../Util.h"


using namespace XFILE;

CFileMusicDatabase::CFileMusicDatabase(void)
{
}

CFileMusicDatabase::~CFileMusicDatabase(void)
{
  Close();
}

bool CFileMusicDatabase::Open(const CURL& url, bool bBinary /*=true*/)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return false;

  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CUtil::GetFileName(strPath);

  CStdString strExtension;
  CUtil::GetExtension(strFileName, strExtension);
  CUtil::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName) || strExtension.IsEmpty())
    return false;

  long idSong=atol(strFileName.c_str());

  CSong song;
  if (!musicDatabase.GetSongById(idSong, song))
    return false;

  CStdString strExtensionFromDb;
  CUtil::GetExtension(song.strFileName, strExtensionFromDb);

  if (!strExtensionFromDb.Equals(strExtension))
    return false;

  return m_file.Open(song.strFileName);
}

bool CFileMusicDatabase::Exists(const CURL& url)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return false;

  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CUtil::GetFileName(strPath);

  CStdString strExtension;
  CUtil::GetExtension(strFileName, strExtension);
  CUtil::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName) || strExtension.IsEmpty())
    return false;

  long idSong=atol(strFileName.c_str());

  CSong song;
  if (!musicDatabase.GetSongById(idSong, song))
    return false;

  CStdString strExtensionFromDb;
  CUtil::GetExtension(song.strFileName, strExtensionFromDb);

  return strExtensionFromDb.Equals(strExtension);
}

int CFileMusicDatabase::Stat(const CURL& url, struct __stat64* buffer)
{
  CMusicDatabase musicDatabase;
  if (!musicDatabase.Open())
    return false;

  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CUtil::GetFileName(strPath);

  CStdString strExtension;
  CUtil::GetExtension(strFileName, strExtension);
  CUtil::RemoveExtension(strFileName);

  if (!StringUtils::IsNaturalNumber(strFileName) || strExtension.IsEmpty())
    return false;

  long idSong=atol(strFileName.c_str());

  CSong song;
  if (!musicDatabase.GetSongById(idSong, song))
    return false;

  CStdString strExtensionFromDb;
  CUtil::GetExtension(song.strFileName, strExtensionFromDb);

  if (!strExtensionFromDb.Equals(strExtension))
    return false;

 return m_file.Stat(song.strFileName, buffer);
}

unsigned int CFileMusicDatabase::Read(void* lpBuf, __int64 uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

__int64 CFileMusicDatabase::Seek(__int64 iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CFileMusicDatabase::Close()
{
  m_file.Close();
}

__int64 CFileMusicDatabase::GetPosition()
{
  return m_file.GetPosition();
}

__int64 CFileMusicDatabase::GetLength()
{
  return m_file.GetLength();
}
