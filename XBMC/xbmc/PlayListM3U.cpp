
#include "stdafx.h"
#include "playlistm3u.h"
#include "filesystem/file.h"
#include "util.h"

using namespace PLAYLIST;
using namespace XFILE;

#define M3U_START_MARKER	"#EXTM3U"
#define M3U_INFO_MARKER		"#EXTINF"

// example m3u file:
//			#EXTM3U
//			#EXTINF:5,demo
//			E:\Program Files\Winamp3\demo.mp3
//			#EXTINF:5,demo
//			E:\Program Files\Winamp3\demo.mp3




CPlayListM3U::CPlayListM3U(void)
{
}

CPlayListM3U::~CPlayListM3U(void)
{
}


bool CPlayListM3U::Load(const CStdString& strFileName)
{
	char		szLine[4096];
	CStdString	strLine;
	CStdString	strInfo = "";
	long		lDuration = 0;
	CStdString strBasePath;

	Clear();

	m_strPlayListName = CUtil::GetFileName(strFileName);
	CUtil::GetParentPath(strFileName,strBasePath);

	CFile file;
	if (!file.Open(strFileName,false) ) 
	{
		file.Close();
		return false;
	}

	while (file.ReadString(szLine,1024))
	{
		strLine = szLine;
		CUtil::RemoveCRLF(strLine);

		if (strLine.Left( (int)strlen(M3U_INFO_MARKER) ) == M3U_INFO_MARKER)
		{
			// start of info 
			int iColon = (int)strLine.find(":");
			int iComma = (int)strLine.find(",");
			if (iColon >=0 && iComma >= 0 && iComma > iColon)
			{
				// Read the info and duration
				iColon++;
				CStdString strLength = strLine.Mid(iColon, iComma-iColon);
				lDuration = atoi(strLength.c_str());
				iComma++;
				strInfo = strLine.Right((int)strLine.size()-iComma);

			}
		}
		else if (strLine != M3U_START_MARKER)
		{
			CStdString strFileName = strLine;

			if (strFileName.length() > 0)
			{
				// If no info was read from from the extended tag information, use the file name
				if (strInfo.length() == 0)
				{
					strInfo = CUtil::GetFileName(strFileName);
				}

				// If this is a samba base path we need to translate the file name to UTF-8 since
				// this is a samba requirement
				if (CUtil::IsSmb(strBasePath))
				{
					CStdString strFileNameUtf8;
					g_charsetConverter.stringCharsetToUtf8(strFileName, strFileNameUtf8);
					strFileName = strFileNameUtf8;
				}

				// Get the full path file name and add it to the the play list
				CUtil::GetQualifiedFilename(strBasePath, strFileName);
				CPlayListItem newItem(strInfo, strFileName, lDuration);
				Add(newItem);

				// Reset the values just in case there part of the file have the extended marker
				// and part don't
				strInfo = "";
				lDuration = 0;
			}
		}
	}

	file.Close();
	return true;
}

void CPlayListM3U::Save(const CStdString& strFileName) const
{
	if (!m_vecItems.size()) return;
	FILE *fd = fopen(strFileName.c_str(),"w+");
	if (!fd) return;
	fprintf(fd,"%s\n",M3U_START_MARKER);
	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		const CPlayListItem& item = m_vecItems[i];
		fprintf(fd,"%s:%i,%s\n",M3U_INFO_MARKER, item.GetDuration()/1000, item.GetDescription().c_str() );
		fprintf(fd,"%s\n",item.GetFileName().c_str() );
	}
	fclose(fd);
}
