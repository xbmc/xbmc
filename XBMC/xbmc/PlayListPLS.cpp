#include "playlistpls.h"
#include "util.h"
#include "filesystem/file.h"

#define START_PLAYLIST_MARKER "[playlist]"
#define PLAYLIST_NAME					"PlaylistName"
using namespace PLAYLIST;
/*----------------------------------------------------------------------
[playlist]
PlaylistName=Playlist 001
File1=E:\Program Files\Winamp3\demo.mp3
Title1=demo
Length1=5
File2=E:\Program Files\Winamp3\demo.mp3
Title2=demo
Length2=5
NumberOfEntries=2
Version=2
----------------------------------------------------------------------*/
CPlayListPLS::CPlayListPLS(void)
{
}

CPlayListPLS::~CPlayListPLS(void)
{
}


bool CPlayListPLS::Load(const CStdString& strFileName)
{
Clear();
	m_strPlayListName=CUtil::GetFileName(strFileName);
	CFile file;
	if (!file.Open(strFileName,false) ) 
	{
		file.Close();
		return false;
	}

	char szLine[4096];
	if ( !file.ReadString(szLine, sizeof(szLine) ) )
	{
		file.Close();
		return false;
	}
	CStdString strLine=szLine;
	CUtil::RemoveCRLF(strLine);
	if (strLine != START_PLAYLIST_MARKER)
	{
		file.Close();
		return false;
	}
	CStdString strFilename="";
	CStdString strInfo="";
	CStdString strDuration="";
	while (file.ReadString(szLine,sizeof(szLine) ) )
	{
		strLine=szLine;
		CUtil::RemoveCRLF(strLine);
		int iPosEqual=strLine.Find("=");
		if (iPosEqual>0)
		{
			CStdString strLeft =strLine.Left(iPosEqual);
			iPosEqual++;
			CStdString strValue=strLine.Right(strLine.size()-iPosEqual);
			strLeft.ToLower();
			if (strLeft.Left( (int)strlen("FILE") ) == "file")
			{	
				strFilename=strValue;
			}
			if (strLeft.Left( (int)strlen("Title") ) == "title")
			{	
				strInfo=strValue;
			}
			if (strLeft.Left( (int)strlen("Length") ) == "length")
			{	
				strDuration=strValue;
			}
			if (strLeft=="playlistname")
			{
				m_strPlayListName=strValue;
			}

			if (strDuration.size() && strInfo.size() && strFilename.size()) 
			{
				long lDuration=atol(strDuration.c_str());
				lDuration*=1000;
				CPlayListItem newItem(strInfo,strFilename,lDuration);
				Add(newItem);
				strFilename="";
				strInfo="";
				strDuration="";
			}
		}		
	}
	file.Close();


	return true;
}

void CPlayListPLS::Save(const CStdString& strFileName) const
{
	if (!m_vecItems.size()) return;
	FILE *fd = fopen(strFileName.c_str(),"w+");
	if (!fd) return;
	fprintf(fd,"%s\n",START_PLAYLIST_MARKER);	
	fprintf(fd,"PlaylistName=%s\n",m_strPlayListName.c_str() );	

	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		const CPlayListItem& item = m_vecItems[i];
		fprintf(fd,"File%i=%s\n",i+1, item.GetFileName().c_str() );
		fprintf(fd,"Title%i=%s\n",i+1, item.GetDescription().c_str() );
		fprintf(fd,"Length%i=%i\n",i+1, item.GetDuration()/1000 );
	}
	
	fprintf(fd,"NumberOfEntries=%i\n",m_vecItems.size());
	fprintf(fd,"Version=2\n");
	fclose(fd);
}
