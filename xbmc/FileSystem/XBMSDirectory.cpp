#include "xbmsdirectory.h"
#include "../url.h"
#include "../util.h"
#include "../sectionloader.h"

extern "C" {
	#include "../lib/libxbms/ccincludes.h"
	#include "../lib/libxbms/ccbuffer.h"
	#include "../lib/libxbms/ccxclient.h"
	#include "../lib/libxbms/ccxmltrans.h"
}

CXBMSDirectory::CXBMSDirectory(void)
{
	CSectionLoader::Load("LIBXBMS");
}

CXBMSDirectory::~CXBMSDirectory(void)
{
	CSectionLoader::Unload("LIBXBMS");
}



bool  CXBMSDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	unsigned long handle;
	char *filename, *fileinfo;
	bool rv=false;
	CURL url(strPath);
  
	CStdString strRoot=strPath;
	if (!CUtil::HasSlashAtEnd(strPath) )
		strRoot+="/";

	

	CcXstreamServerConnection conn = NULL;

	if (cc_xstream_client_connect(url.GetHostName().c_str(),
																(url.HasPort()) ? url.GetPort(): 1400, &conn) != CC_XSTREAM_CLIENT_OK)
	{
		if (conn != NULL) cc_xstream_client_disconnect(conn);	
		
		return false;
	}

	if (cc_xstream_client_version_handshake(conn) != CC_XSTREAM_CLIENT_OK)
	{
		if (conn != NULL) cc_xstream_client_disconnect(conn);	
		
		return false;
	}

	// Authenticate here!  
	CStdString strPassword=url.GetPassWord();
	CStdString strUserName=url.GetUserName();
	if (strPassword.size() && strUserName.size())
	{
		// We don't check the return value here.  If authentication
		// step fails, let's try if server lets us log in without
		// authentication.
		cc_xstream_client_password_authenticate(conn,
																						strUserName.c_str(),
																						strPassword.c_str() );
	}
	CStdString strFileName=url.GetFileName();
	CStdString strDir, strFile;
	strDir="";
	if (cc_xstream_client_setcwd(conn, "/") == CC_XSTREAM_CLIENT_OK)
	{
		strFile=url.GetFileName();
		for (int i=0; i < (int)strFile.size(); ++i)
		{
			if (strFile[i]=='/' || strFile[i]=='\\')
			{
				if (strDir!="")
				{
					if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
					{
						if (conn != NULL) cc_xstream_client_disconnect(conn);	
						return false;
					}
				}
				strDir="";
			}
			else
			{
				strDir += strFile[i];
			}
		}
	}
	else
	{
		if (conn != NULL) cc_xstream_client_disconnect(conn);	
		return false;
	}
	if (strDir.size() > 0)
	{
		if (cc_xstream_client_setcwd(conn, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
		{
			if (conn != NULL) cc_xstream_client_disconnect(conn);	
			
			return false;
		}
	}

	if (cc_xstream_client_dir_open(conn, &handle) != CC_XSTREAM_CLIENT_OK)
	{
		if (conn != NULL) cc_xstream_client_disconnect(conn);	
		
		return false;
	}

	while (cc_xstream_client_dir_read(conn, handle, &filename, &fileinfo) == CC_XSTREAM_CLIENT_OK)
	{
		if (*filename == '\0')
		{
			free(filename);
			free(fileinfo);
			break;
		}
		bool bIsDirectory=false;

		if (strstr(fileinfo, "<ATTRIB>directory</ATTRIB>"))
				bIsDirectory=true;

		if ( bIsDirectory || IsAllowed( filename) )
		{

			CFileItem* pItem = new CFileItem(filename);

			char* pstrSizeStart=strstr(fileinfo, "<SIZE>");
			char* pstrSizeEnd =strstr(fileinfo, "</SIZE>");
			if (pstrSizeStart && pstrSizeEnd)
			{
				char szSize[128];
				pstrSizeStart+=strlen("<SIZE>");
				strncpy(szSize,pstrSizeStart, pstrSizeEnd-pstrSizeStart);
				szSize[pstrSizeEnd-pstrSizeStart]=0;
				pItem->m_dwSize = atol(szSize);
			}

			char* pstrAccessStart=strstr(fileinfo, "<ACCESS>");
			char* pstrAccessEnd  =strstr(fileinfo, "</ACCESS>");
			if (pstrAccessStart && pstrAccessEnd)
			{
				char szAccess[128];
				pstrAccessStart+=strlen("<ACCESS>");
				strncpy(szAccess,pstrAccessStart, pstrAccessEnd-pstrAccessStart);
				szAccess[pstrAccessEnd-pstrAccessStart]=0;
				__int64 lTimeDate= _atoi64(szAccess);
		
				FILETIME fileTime,localTime;
				LONGLONG ll = Int32x32To64(lTimeDate, 10000000) + 116444736000000000;
				fileTime.dwLowDateTime = (DWORD) ll;
				fileTime.dwHighDateTime = (DWORD)(ll >>32);

				FileTimeToLocalFileTime(&fileTime,&localTime);
				FileTimeToSystemTime(&localTime, &pItem->m_stTime);

			}

			
			pItem->m_strPath=strRoot;
      pItem->m_strPath+=filename;
			pItem->m_bIsFolder=bIsDirectory;
			items.push_back(pItem);
		}
		free(filename);
		free(fileinfo);
	}
	cc_xstream_client_close_all(conn);
	rv = true;

	if (conn != NULL)
		cc_xstream_client_disconnect(conn);
	
	

	return true;
}
