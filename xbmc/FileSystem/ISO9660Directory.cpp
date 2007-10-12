
#include "stdafx.h"
#include "ISO9660Directory.h"
#include "../xbox/IoSupport.h"
#include "iso9660.h"
#include "../Util.h"

using namespace DIRECTORY;

CISO9660Directory::CISO9660Directory(void)
{}

CISO9660Directory::~CISO9660Directory(void)
{}

bool CISO9660Directory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (!CUtil::HasSlashAtEnd(strPath) )
    strRoot += "/";

  // Scan active disc if not done before
  if (!m_isoReader.IsScanned())
    m_isoReader.Scan();

  CURL url(strPath);

  WIN32_FIND_DATA wfd;
  HANDLE hFind;

  memset(&wfd, 0, sizeof(wfd));

  CStdString strSearchMask;
  CStdString strDirectory = url.GetFileName();
  if (strDirectory != "")
  {
    strSearchMask.Format("\\%s", strDirectory.c_str());
  }
  else
  {
    strSearchMask = "\\";
  }
  for (int i = 0; i < (int)strSearchMask.size(); ++i )
  {
    if (strSearchMask[i] == '/') strSearchMask[i] = '\\';
  }

  hFind = m_isoReader.FindFirstFile((char*)strSearchMask.c_str(), &wfd);
  if (hFind == NULL)
    return false;

  do
  {
    if (wfd.cFileName[0] != 0)
    {
      if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        CStdString strDir = wfd.cFileName;
        if (strDir != "." && strDir != "..")
        {
          CFileItem *pItem = new CFileItem(wfd.cFileName);
          pItem->m_strPath = strRoot;
          pItem->m_strPath += wfd.cFileName;
          pItem->m_bIsFolder = true;
          CUtil::AddSlashAtEnd(pItem->m_strPath);
          FILETIME localTime;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
          items.Add(pItem);
        }
      }
      else
      {
        if ( IsAllowed( wfd.cFileName) )
        {
          CFileItem *pItem = new CFileItem(wfd.cFileName);
          pItem->m_strPath = strRoot;
          pItem->m_strPath += wfd.cFileName;
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FILETIME localTime;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
          items.Add(pItem);
        }
      }
    }
  }
  while (m_isoReader.FindNextFile(hFind, &wfd));
  m_isoReader.FindClose(hFind);

  return true;
}

bool CISO9660Directory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
