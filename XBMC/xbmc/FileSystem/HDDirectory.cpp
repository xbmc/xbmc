#include "hddirectory.h"
#include "../url.h"
#include "../util.h"
#include "../xbox/iosupport.h"
#include "../autoptrhandle.h"
using namespace AUTOPTR;
CHDDirectory::CHDDirectory(void)
{
}

CHDDirectory::~CHDDirectory(void)
{
}


bool  CHDDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	WIN32_FIND_DATA wfd;
	
	CStdString strRoot=strPath;
	CURL url(strPath);

	memset(&wfd,0,sizeof(wfd));
	if (!CUtil::HasSlashAtEnd(strPath) )
		strRoot+="\\";
#if 0
	if ( CUtil::IsDVD(strRoot) )
  {
    CIoSupport helper;
    helper.Remount("D:","Cdrom0");
  }
#endif
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
  	
          items.push_back(pItem);      
        }
      }
      else
      {
				if ( IsAllowed( wfd.cFileName) )
				{
					CFileItem *pItem = new CFileItem(wfd.cFileName);
					pItem->m_strPath=strRoot;
					pItem->m_strPath+=wfd.cFileName;

					pItem->m_bIsFolder=false;
					pItem->m_dwSize=wfd.nFileSizeLow;
					FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);
					items.push_back(pItem);
				}
      }
    }
  } while (FindNextFile((HANDLE)hFind, &wfd));

  return true;
}
