
#include "../stdafx.h"
#include "hddirectory.h"
#include "../util.h"
#include "../xbox/iosupport.h"
#include "directorycache.h"
#include "iso9660.h"

CHDDirectory::CHDDirectory(void)
{
}

CHDDirectory::~CHDDirectory(void)
{
}

bool CHDDirectory::GetDirectory(const CStdString& strPath,CFileItemList &items)
{
	WIN32_FIND_DATA wfd;

	CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath);

	CStdString strRoot=strPath;
	CURL url(strPath);

	memset(&wfd,0,sizeof(wfd));
	if (!CUtil::HasSlashAtEnd(strPath) )
		strRoot+="\\";
	strRoot.Replace("/", "\\");
	if (CUtil::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
		//	Reset iso reader and remount or
		//	we can't access the dvd-rom
		m_isoReader.Reset();
		CIoSupport helper;
		helper.Remount("D:","Cdrom0");
  }

  CStdString strSearchMask=strRoot;
  strSearchMask+="*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(),&wfd));
	if (!hFind.isValid())
		return false;
	do
	{
		if (wfd.cFileName[0]!=0)
		{
			if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        CStdString strDir=wfd.cFileName;
        if (strDir != "." && strDir != "..")
        {
          CFileItem *pItem = new CFileItem(wfd.cFileName);
          pItem->m_strPath=strRoot;
          pItem->m_strPath+=wfd.cFileName;
          pItem->m_bIsFolder=true;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
  	
					vecCacheItems.Add(pItem);
          items.Add(new CFileItem(*pItem));   
        }
      }
      else
      {
				CFileItem *pItem = new CFileItem(wfd.cFileName);
				pItem->m_strPath=strRoot;
				pItem->m_strPath+=wfd.cFileName;

				pItem->m_bIsFolder=false;
				pItem->m_dwSize=CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
				FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
				FileTimeToSystemTime(&localTime, &pItem->m_stTime);
				if ( IsAllowed( wfd.cFileName) )
				{
					vecCacheItems.Add(pItem);
					items.Add(new CFileItem(*pItem));
				}
				else
 					vecCacheItems.Add(pItem);
     }
    }
  } while (FindNextFile((HANDLE)hFind, &wfd));

  g_directoryCache.SetDirectory(strPath,vecCacheItems);

  return true;
}

bool CHDDirectory::Create(const char* strPath)
{
  return ::CreateDirectory(strPath, NULL) ? true : false;
}

bool CHDDirectory::Remove(const char* strPath)
{
  return ::RemoveDirectory(strPath) ? true : false;
}

bool CHDDirectory::Exists(const char* strPath)
{
	DWORD attributes = GetFileAttributes(strPath);
	if (FILE_ATTRIBUTE_DIRECTORY == attributes) return true;
	return false;
}