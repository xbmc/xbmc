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
	CStdString strBasePath;
	Clear();
	m_strPlayListName=CUtil::GetFileName(strFileName);
	CUtil::GetParentPath(strFileName,strBasePath);
	CFile file;
	if (!file.Open(strFileName,false) ) 
	{
		file.Close();
		return false;
	}

	char szLine[4096];
	if ( !file.ReadString(szLine, 1024 ) )
	{
		file.Close();
		return false;
	}
	CStdString strLine=szLine;
	CUtil::RemoveCRLF(strLine);
	if (strLine != M3U_START_MARKER)
	{
		file.Close();
		return false;
	}

	while (file.ReadString(szLine,1024 ) )
	{
		strLine=szLine;
		CUtil::RemoveCRLF(strLine);
		if (strLine.Left( (int)strlen(M3U_INFO_MARKER) ) == M3U_INFO_MARKER)
		{
			// start of info 
			int iColon=(int)strLine.find(":");
			int iComma=(int)strLine.find(",");
			if (iColon >=0 && iComma >= 0 && iComma > iColon)
			{
				iColon++;
				CStdString strLength=strLine.Mid(iColon, iComma-iColon);
				iComma++;
				CStdString strInfo=strLine.Right((int)strLine.size()-iComma);
				long lDuration=atoi(strLength.c_str());
				//lDuration*=1000;

				if (file.ReadString(szLine,1024 ) )
				{
					CStdString strFileName=szLine;
					CUtil::RemoveCRLF(strFileName);
					CUtil::GetQualifiedFilename(strBasePath,strFileName);
					CPlayListItem newItem(strInfo,strFileName,lDuration);
					Add(newItem);
				}
				else
				{
					// eof
					break;
				}
			}
		}
		else
		{
			CStdString strFileName=szLine;
			CUtil::RemoveCRLF(strFileName);
			CUtil::GetQualifiedFilename(strBasePath,strFileName);
			CPlayListItem newItem(strFileName, strFileName, 0);
			Add(newItem);
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
