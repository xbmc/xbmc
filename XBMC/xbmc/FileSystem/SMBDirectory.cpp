
#include "stdafx.h"
/*
 * know bugs:
 * - when opening a server for the first time with ip adres and the second time
 *   with server name, access to the server is denied.
 * - when browsing entire network, user can't go back one step
 *   share = smb://, user selects a workgroup, user selects a server.
 *   doing ".." will go back to smb:// (entire network) and not to workgroup list.
 *
 * debugging is set to a max of 10 for release builds (see local.h)
 */

#include "smbdirectory.h"
#include "../settings.h"
#include "../util.h"
#include "../utils/log.h"
#include "../sectionLoader.h"
#include "../url.h"
#include "../applicationmessenger.h"
#include "../GUIWindowManager.h"
#include "../GUIDialogOk.h"
#include "directorycache.h"

CSMBDirectory::CSMBDirectory(void)
{
} 

CSMBDirectory::~CSMBDirectory(void)
{
}

bool  CSMBDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	// We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]
	VECFILEITEMS vecCacheItems;
    g_directoryCache.ClearDirectory(strPath);

    // note, samba uses UTF8 strings internal,
	// that's why we have to convert strings and wstrings to UTF8.
	size_t strLen;

	//Separate roots for the authentication and the containing items to allow browsing to work correctly
	CStdString strRoot = strPath, strAuth = strPath;
	if (!CUtil::HasSlashAtEnd(strPath))
	{
		strRoot+="/"; 
		strAuth+="/";
	}

	{
		CURL url(strAuth);	//Use deafult credentials if none is specified.
		if(url.GetUserName().length() == 0 && url.GetHostName().length() > 0) 
		{
			url.SetUserName(g_stSettings.m_strSambaDefaultUserName);
			url.SetPassword(g_stSettings.m_strSambaDefaultPassword);
			url.GetURL(strAuth);
		}
	}
		
	smb.Init();
	smb.Lock();
	int fd = smbc_opendir(strAuth);
	smb.Unlock();

	if (fd < 0)
	{
		int nt_error;
		if (errno == ENODEV) nt_error = NT_STATUS_INVALID_COMPUTER_NAME;
		else nt_error = map_nt_error_from_unix(errno);

		// write error to logfile
		CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
			strPath.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));

		// is we have an 'invalid handle' error we don't display the error
		// because most of the time this means there is no cdrom in the server's
		// cdrom drive.
		if (nt_error != 0xc0000008)
		{
			const char* cError = get_friendly_nt_error_msg(nt_error);
			
			CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			pDialog->SetHeading(257);
			pDialog->SetLine(0,cError);
			pDialog->SetLine(1,L"");
			pDialog->SetLine(2,L"");

			ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
			g_applicationMessenger.SendMessage(tMsg, false);
		}
		return false;
	}
	else
	{
		struct smbc_dirent* dirEnt;
		wchar_t wStrFile[1024]; // buffer for converting strings
		CStdString strFile;

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
				strLen = convert_string(CH_UTF8, CH_UCS2, dirEnt->name, dirEnt->namelen, wStrFile, 1024, false);
				wStrFile[strLen] = 0;

				CUtil::Unicode2Ansi(wStrFile, strFile);

				// doing stat on one of these types of shares leaves an open session
				// so just skip them and only stat real dirs / files.
				if( dirEnt->smbc_type != SMBC_IPC_SHARE &&
						dirEnt->smbc_type != SMBC_FILE_SHARE &&
						dirEnt->smbc_type != SMBC_PRINTER_SHARE &&
						dirEnt->smbc_type != SMBC_COMMS_SHARE &&
						dirEnt->smbc_type != SMBC_WORKGROUP &&
						dirEnt->smbc_type != SMBC_SERVER)
				{
					struct __stat64 info;
					CStdString strFullName = strAuth + dirEnt->name; //Make sure we use the authenticated path wich contains any default username

					smb.Lock();
					smbc_stat(strFullName, &info);
					smb.Unlock();

					bIsDir = (info.st_mode & S_IFDIR) ? true : false;
					lTimeDate = info.st_ctime;
					iSize = info.st_size;
				}
				
				FILETIME fileTime,localTime;
				LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000;
				fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
				fileTime.dwHighDateTime = (DWORD)(ll >>32);
				FileTimeToLocalFileTime(&fileTime,&localTime); 

				if (bIsDir)
				{
					CFileItem *pItem = new CFileItem(strFile);
					pItem->m_strPath = strRoot;

					// needed for network / workgroup browsing
					// skip if root has already a valid domain and type is not a server
					if ((strRoot.find(';') == -1) &&
							(dirEnt->smbc_type == SMBC_SERVER)) 
							/*&& (strRoot.find('@') == -1))*/ //Removed to allow browsing even if a user is specified
					{
						// lenght > 6, which means a workgroup name is specified and we need to
						// remove it. Domain without user is not allowed
						int strLength = strRoot.length();
						if (strLength > 6)
						{
							if(CUtil::HasSlashAtEnd(strRoot))
								pItem->m_strPath = "smb://";
						}
					}
					pItem->m_strPath += dirEnt->name;
					if(!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';
					pItem->m_bIsFolder = true;
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);  
					vecCacheItems.push_back(pItem);
					items.push_back(new CFileItem(*pItem));
				}
				else
				{
					CFileItem *pItem = new CFileItem(strFile);
					pItem->m_strPath = strRoot + dirEnt->name;
					pItem->m_bIsFolder = false;
					pItem->m_dwSize = iSize;
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);
		        
					vecCacheItems.push_back(pItem);

					if (IsAllowed(dirEnt->name)) items.push_back(new CFileItem(*pItem));
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
  g_directoryCache.SetDirectory(strPath,vecCacheItems);
	smb.PurgeEx(CURL(strPath));
	return true;
}
