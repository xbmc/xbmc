#include "smbdirectory.h"
#include "../lib/libsmb/smb++.h"
#include "../url.h"
#include "../settings.h"
#include "../util.h"
#include "../sectionLoader.h"

static char szUserName[256];
static char szPassWd[256];

CSMBDirectory::CSMBDirectory(void)
{
	CSectionLoader::Load("LIBSMB");
} 

CSMBDirectory::~CSMBDirectory(void)
{
	CSectionLoader::Unload("LIBSMB");
}


class MyCallback : public SmbAnswerCallback
{
protected:
	// Warning: don't use a fixed size buffer in a real application.
	// This is a security hazard.
	char buf[200];
public:
	char *getAnswer(int type, const char *optmessage) {
		switch (type) {
			case ANSWER_USER_NAME:
				strcpy(buf, szUserName);
				break;
			case ANSWER_USER_PASSWORD:
//				cout<<"Password for user "<<optmessage<<": ";
//				cin>>buf;
				strcpy(buf, szPassWd);
				break;
			case ANSWER_SERVICE_PASSWORD:
				strcpy(buf, szPassWd);
				break;
		}
		return buf;
	}
} cb;


bool  CSMBDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
  bool bResult(true);
	CURL url(strPath);

	CStdString strRoot=strPath;
	if (!CUtil::HasSlashAtEnd(strPath) )
		strRoot+="/";

	{
		
	  
		
		{

			SMB smb ;

			CStdString strPassword=url.GetPassWord();
			CStdString strUserName=url.GetUserName();
			CStdString strDirectory=url.GetFileName();

			if (strUserName.size() )
				strcpy(szUserName, strUserName.c_str());
			else
				strcpy(szUserName, "");

			if (strPassword.size() )
				strcpy(szPassWd, strPassword.c_str());
			else
				strcpy(szPassWd, "");

			SMBdirent* dirEnt;
			smb.setNBNSAddress(g_stSettings.m_strNameServer);
			smb.setPasswordCallback(&cb);

			CStdString strFile = strPath.Right( strPath.size() -  strlen("smb://") );
			int fd = smb.opendir( strFile.c_str()  );
			if (fd < 0) 
			{
				bResult=false;
			}
			else
			{
				while ((dirEnt = smb.readdir(fd))) 
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
		  
				smb.closedir(fd);
  		}

		}
		
	}
	return bResult;
}
