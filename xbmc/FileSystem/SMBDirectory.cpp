#include "smbdirectory.h"
#include "../lib/libsmb/smb++.h"
#include "../url.h"
#include "../settings.h"
#include "../util.h"
#include "../sectionLoader.h"


CSMBDirectory::CSMBDirectory(void)
{
	
} 

CSMBDirectory::~CSMBDirectory(void)
{
	
}


bool  CSMBDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
  bool bResult(true);
	CURL url(strPath);

	CStdString strRoot=strPath;
	if (!CUtil::HasSlashAtEnd(strPath) )
		strRoot+="/";
	{

			SMB* psmb =smbFile.GetSMB();
			smbFile.Lock();
			CStdString strPassword=url.GetPassWord();
			CStdString strUserName=url.GetUserName();
			CStdString strDirectory=url.GetFileName();

			char szUserName[256];
			char szPassWd[256];
			if (strUserName.size() )
				strcpy(szUserName, strUserName.c_str());
			else
				strcpy(szUserName, "");

			if (strPassword.size() )
				strcpy(szPassWd, strPassword.c_str());
			else
				strcpy(szPassWd, "");

//			psmb->setNBNSAddress(g_stSettings.m_strNameServer);
	//		psmb->setPasswordCallback(&cb);
			smbFile.SetLogin(szUserName,szPassWd);
			SMBdirent* dirEnt;
			CStdString strFile = strPath.Right( strPath.size() -  strlen("smb://") );
			int fd = psmb->opendir( strFile.c_str()  );
			if (fd < 0) 
			{
				bResult=false;
			}
			else
			{
				while ((dirEnt = psmb->readdir(fd))) 
				{
	    		
	    		
					if (dirEnt->d_name && strcmp(dirEnt->d_name,".") && strcmp(dirEnt->d_name,".."))
					{
						__int64 lTimeDate= dirEnt->st_ctime;
						CStdString strFile=dirEnt->d_name;

						FILETIME fileTime,localTime;
						LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000;
						fileTime.dwLowDateTime = (DWORD) ll;
						fileTime.dwHighDateTime = (DWORD)(ll >>32);

						FileTimeToLocalFileTime(&fileTime,&localTime);
	          

						if (dirEnt->st_mode & S_IFDIR)
						{
							CFileItem *pItem = new CFileItem(strFile);
							pItem->m_strPath=strRoot;
							pItem->m_strPath+=strFile;
							pItem->m_bIsFolder=true;
							FileTimeToSystemTime(&localTime, &pItem->m_stTime);  
							items.push_back(pItem);
						}
						else
						{
							if ( IsAllowed( strFile) )
							{
								CFileItem *pItem = new CFileItem(strFile);
								pItem->m_strPath=strRoot;
								pItem->m_strPath+=strFile;
								pItem->m_bIsFolder=false;
								pItem->m_dwSize=dirEnt->st_size;
								FileTimeToSystemTime(&localTime, &pItem->m_stTime);
		            
								items.push_back(pItem);
							}
						}
					}

			    
				}
				psmb->closedir(fd);

  		}
			smbFile.Unlock();
	}
	return bResult;
}
