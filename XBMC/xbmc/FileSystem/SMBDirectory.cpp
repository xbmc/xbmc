/*
 * know bugs:
 * - doing smbc_stat on for example an IPC_SHARE leaves an open socket
 *   this happens when listing all shares from a pc ("smb://pc-name")
 */

#include "smbdirectory.h"
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
/*
	We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]

	unsigned int smbc_type; 
		 Type of entity.
	    SMBC_WORKGROUP=1,
	    SMBC_SERVER=2, 
	    SMBC_FILE_SHARE=3,
	    SMBC_PRINTER_SHARE=4,
	    SMBC_COMMS_SHARE=5,
	    SMBC_IPC_SHARE=6,
	    SMBC_DIR=7,
	    SMBC_FILE=8,
	    SMBC_LINK=9,
	unsigned int dirlen;
	unsigned int commentlen;
	char *comment;
	unsigned int namelen;
	char name[1];
}; dirent
*/

	CStdString strRoot = strPath;

	if (!CUtil::HasSlashAtEnd(strPath))
		strRoot+="/";

	smb.Init();

	smb.Lock();
	int fd = smbc_opendir(strPath);
	smb.Unlock();

	if (fd < 0) return false;
	else
	{
		struct smbc_dirent* dirEnt;
		smb.Lock();
		dirEnt = smbc_readdir(fd);
		smb.Unlock();
		while (dirEnt) 
		{
			if (dirEnt->name && strcmp(dirEnt->name,".") && strcmp(dirEnt->name,".."))
			{
				CStdString strFile=dirEnt->name;
				struct stat info;

				smb.Lock();
				smbc_stat(strRoot + strFile, &info);
				smb.Unlock();
				
				__int64 lTimeDate= info.st_ctime;
				
				FILETIME fileTime,localTime;
				LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000;
				fileTime.dwLowDateTime = (DWORD) ll;
				fileTime.dwHighDateTime = (DWORD)(ll >>32);

				FileTimeToLocalFileTime(&fileTime,&localTime); 

				if (info.st_mode & S_IFDIR)
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
						pItem->m_dwSize = info.st_size;
						FileTimeToSystemTime(&localTime, &pItem->m_stTime);
		        
						items.push_back(pItem);
					}
				}
			}
			smb.Lock();
			dirEnt = smbc_readdir(fd);
			smb.Unlock();
		}
		smb.Lock();
		smbc_closedir(fd);
		smb.Unlock();
	}
	return true;
}
