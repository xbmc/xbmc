#include "stdafx.h"
#include "ftpdirectory.h"
#include "ftpparse.h"
#include "../url.h"
#include "../util.h"
#include "DirectoryCache.h"

extern int ftpparse(struct ftpparse *fp,char *buf,int len);
CFTPDirectory::CFTPDirectory(void){}
CFTPDirectory::~CFTPDirectory(void){}

void UxTimeToFileTime(time_t ut, LPFILETIME pft)
{
	LONGLONG ll;
	ll = Int32x32To64(ut, 10000000) + 116444736000000000;
	pft->dwLowDateTime  = (DWORD)&ll;
	pft->dwHighDateTime = (DWORD)(ll >> 32);
}
bool CFTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  FILETIME ft;
  struct ftpparse lp;
	CStdString scPath=strPath;
  CFileItemList vecCacheItems;
  char buff[MAX_BUFFSIZE], tbuf[MAX_PATH];
  
  if (!CUtil::HasSlashAtEnd(scPath)) scPath += '/';
  g_directoryCache.ClearDirectory(scPath);
  if (!FTPUtil.GetFTPList(scPath,buff)) return false;

  char *chrs, *cLine=buff; int num = 0;
  //bool bCon = false; char *cLine; CStdString szBuffer; int iline = 0;
	do
  {
    //bCon = true; // We are Connected!
    //cLine = 0;
    //if (!FTPUtil.GetDataLine(scPath, bCon, iline, szBuffer)) break;
    //cLine = (char*)szBuffer.c_str();
    //num = 0;
		cLine += num;
		chrs = strchr(cLine, '\r');
		num = int(chrs - cLine + 2);
		if (chrs != NULL) 
		{
			if (ftpparse(&lp, cLine, num-2) == 1)
			{
				strncpy(tbuf, lp.name, min(lp.namelen, MAX_PATH));
				tbuf[min(lp.namelen, MAX_PATH)]=0;
				if (lp.namelen > 0)
				{
					if (tbuf[0] != '.')
					{
						CFileItem* pItem = new CFileItem(tbuf);
						pItem->m_strPath = scPath + tbuf;
						pItem->m_bIsFolder = (bool)(lp.flagtrycwd != 0);
						pItem->m_dwSize = lp.size;
						UxTimeToFileTime(lp.mtime, &ft);
						FileTimeToSystemTime(&ft,&pItem->m_stTime);
						if ((!pItem->m_bIsFolder) && (IsAllowed(tbuf)))
						{
							vecCacheItems.Add(pItem);
							items.Add(new CFileItem(*pItem));
						}
						else
						{
							if (pItem->m_bIsFolder)
							{
								vecCacheItems.Add(pItem);
								items.Add(new CFileItem(*pItem));
							}
						}
					}
				}
			}
		}
    //iline = iline + 1;
	}
  while (chrs!=NULL);
  g_directoryCache.SetDirectory(scPath, vecCacheItems);
	return true;
}