/*
 * know bugs:
 * - doing smbc_stat on for example an IPC_SHARE leaves an open socket
 *   this happens when listing all shares from a pc ("smb://pc-name")
 *
 * debugging is off for release builds (see local.h)
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
*/

	// note, samba uses UTF8 strings internal, that's why we have to convert strings
	// and wstrings to UTF8.
	char strUtfPath[1024];
	size_t strLen;
	CStdString strRoot = strPath;

	if (!CUtil::HasSlashAtEnd(strPath))
		strRoot+="/";

	smb.Init();

	// convert from string to UTF8
	strLen = convert_string(CH_DOS, CH_UTF8, strPath, strPath.length(), strUtfPath, 1024);
	strUtfPath[strLen] = 0;

	smb.Lock();
	int fd = smbc_opendir(strUtfPath);
	smb.Unlock();

	if (fd < 0) return false;
	else
	{
		struct smbc_dirent* dirEnt;
		wchar_t wStrFile[1024];
		char strUtfFile[1024];

		smb.Lock();
		dirEnt = smbc_readdir(fd);
		smb.Unlock();

		while (dirEnt) 
		{
			if (dirEnt->name && strcmp(dirEnt->name,".") && strcmp(dirEnt->name,"..") &&
				 (dirEnt->name[dirEnt->namelen - 2] != '$'))
			{
				unsigned __int64 iSize = 0;
				bool bIsDir = true;
				__int64 lTimeDate = 0;

				
				// convert from UTF8 to wide string
				strLen = convert_string(CH_UTF8, CH_UCS2, dirEnt->name, dirEnt->namelen, wStrFile, 1024);
				wStrFile[strLen] = 0;

				// doing stat on one of these types of shares leaves an open session
				// so just skip them and only stat real dirs / files.
				if( dirEnt->smbc_type != SMBC_FILE_SHARE &&
						dirEnt->smbc_type != SMBC_PRINTER_SHARE &&
						dirEnt->smbc_type != SMBC_COMMS_SHARE &&
						dirEnt->smbc_type != SMBC_WORKGROUP &&
						dirEnt->smbc_type != SMBC_SERVER)
				{
					struct __stat64 info;
					CStdString strFile = strRoot + wStrFile;

					// convert from string to UTF8
					strLen = convert_string(CH_DOS, CH_UTF8, strFile, strFile.length(), strUtfFile, 1024);
					strUtfFile[strLen] = 0;

					smb.Lock();
					// it's safe to cast to a struct stat* here
					smbc_stat(strUtfFile, &info);
					smb.Unlock();

					bIsDir = (info.st_mode & S_IFDIR) ? true : false;
					lTimeDate = info.st_ctime;
					iSize = info.st_size;
				}
				
				FILETIME fileTime,localTime;
				LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000;
				fileTime.dwLowDateTime = (DWORD) ll;
				fileTime.dwHighDateTime = (DWORD)(ll >>32);

				FileTimeToLocalFileTime(&fileTime,&localTime); 

				if (bIsDir)
				{
					CFileItem *pItem = new CFileItem(wStrFile);
					pItem->m_strPath=strRoot;
					pItem->m_strPath+=wStrFile;
					pItem->m_bIsFolder=true;
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);  
					items.push_back(pItem);
				}
				else
				{
					if (IsAllowed(wStrFile))
					{
						CFileItem *pItem = new CFileItem(wStrFile);
						pItem->m_strPath=strRoot;
						pItem->m_strPath+=wStrFile;
						pItem->m_bIsFolder=false;
						pItem->m_dwSize = iSize;
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
