#include "../stdafx.h"
#include "sndtrkdirectory.h"
#include "../util.h"
#include "../xbox/iosupport.h"

SOUNDTRACK datastorage; //created a vector of the XSOUNDTRACK_DATA class to keep track of each album

CSndtrkDirectory::CSndtrkDirectory(void)
{	 
}

CSndtrkDirectory::~CSndtrkDirectory(void)
{
}


bool  CSndtrkDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	CStdString strRoot=strPath;
	if(IsAlone(strRoot))
	{
		// Add each user provided soundtrack to the soundtrack vector
		// Took code from samples in XDK
		XSOUNDTRACK_DATA stData;
	    HANDLE hSoundtrack = XFindFirstSoundtrack( &stData );
	    if( INVALID_HANDLE_VALUE != hSoundtrack )
		{
			do
			{
				// Ignore empty soundtracks && parent directories
				if( stData.uSongCount > 0 && strRoot != "..")
				{
					CSoundtrack stInfo;
					stInfo.uSoundtrackId = stData.uSoundtrackId;
					stInfo.uSongCount    = stData.uSongCount;
					wcscpy(stInfo.strName, stData.szName );
					datastorage.push_back(stInfo);
					CFileItem *pItem = new CFileItem(stData.szName);
					pItem->m_strPath=strRoot;
					char tmpvar[4];
					itoa (stData.uSoundtrackId,tmpvar,10);
					pItem->m_strPath+=tmpvar;
					pItem->m_bIsFolder=true;
					items.push_back(pItem);
				}
			} while( XFindNextSoundtrack( hSoundtrack, &stData ) );
		}
	}
	else
	{
		CFileItem *pItem2 = new CFileItem("..");
		pItem2->m_strPath="soundtrack://";
		pItem2->m_bIsFolder=true;
		pItem2->SetLabel("..");
		items.push_back(pItem2);
//		char *ptr=NULL;
		char *ptr = strstr(strRoot,"//");
		ptr+=2;
		int m_iconvert=atoi(ptr); //convert from char back to int to compare to data
		CSoundtrack stInfo;
		ISOUNDTRACK tmpdata;
		tmpdata = datastorage.begin();
		while(tmpdata!=datastorage.end())
		{
			stInfo = *tmpdata;
			if(stInfo.uSoundtrackId == m_iconvert)
				break;
			tmpdata++;
		}
		for( UINT i = 0; i < stInfo.uSongCount; ++i )
		{
			DWORD dwSongId;
			DWORD dwSongLength;
			WCHAR strSong[64];
			if( XGetSoundtrackSongInfo( stInfo.uSoundtrackId, i, &dwSongId, 
                       &dwSongLength, strSong, MAX_SONG_NAME ) )
			{
            // Add it to the list
			CFileItem *pItem = new CFileItem(strSong);
			pItem->m_strPath="E:\\TDATA\\fffe0000\\music\\";
			char tmpvar[16];
			*tmpvar=NULL;
			sprintf(tmpvar,"%04x",stInfo.uSoundtrackId);
			pItem->m_strPath+=tmpvar;
			pItem->m_strPath+="\\";
			*tmpvar=NULL;
			sprintf(tmpvar,"%08x",dwSongId);
			pItem->m_strPath+=tmpvar;
			pItem->m_strPath+=".wma";
			pItem->m_bIsFolder=false;
			items.push_back(pItem);
			}
		}
	}
  return true;
}

bool  CSndtrkDirectory::IsAlone(const CStdString& strPath)
{
	char tmpvar[33] = "soundtrack://";
	if(strcmp(tmpvar,strPath) != 0)
		return false;
	return true;
}

bool CSndtrkDirectory::FindTrackName(const CStdString& strPath, char *NameOfSong)
{
	char* ptr = strstr(strPath.c_str(),"E:\\TDATA\\fffe0000\\music\\");
	if(ptr == NULL)	return false;
	ptr+=strlen("E:\\TDATA\\fffe0000\\music\\");
	char album[5];
	int x = 0;
	for(x=0;x<4;x++)
	{
		album[x]=*ptr;
		ptr+=1;
	}
	album[4]='\0';
	ptr+=1;
	char trackno[9];
	for (x=0;x<8;x++)
	{
		trackno[x] = *ptr;
		ptr+=1;
	}
	trackno[8]='\0';
	int AlbumID,SongID;
	sscanf(album,"%x",&AlbumID);
	sscanf(trackno,"%x",&SongID);
	x=0;
	bool test=true;
	while(test == true)
	{
		DWORD dwSongId;
		DWORD dwSongLength; 
		WCHAR Songname[64];
		if( XGetSoundtrackSongInfo( AlbumID, x, &dwSongId, &dwSongLength, Songname, MAX_SONG_NAME ) == false)
			test = false;
		if(dwSongId==SongID)
		{
			wcstombs(NameOfSong,Songname,64);
			return true;
		}
		x++;
	}
	return false;
}

