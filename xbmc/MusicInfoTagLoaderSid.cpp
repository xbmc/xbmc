#include "stdafx.h"
#include "musicinfotagloadersid.h"
#include "utils/RegExp.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderSid::CMusicInfoTagLoaderSid(void)
{
}

CMusicInfoTagLoaderSid::~CMusicInfoTagLoaderSid()
{
}

bool CMusicInfoTagLoaderSid::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	tag.SetURL(strFileName);
	CStdString strFileNameLower(strFileName);
	strFileNameLower.MakeLower();
	int iHVSC = strFileNameLower.find("hvsc"); // need hvsc in path name since our lookupfile is based on hvsc paths
	if( iHVSC < 0 ) {
		tag.SetLoaded(false);
		return( false );
	}
	
	CStdString strHVSCpath = strFileName.substr(iHVSC+4,strFileName.length()-1);

	strHVSCpath.Replace('\\','/'); // unix paths
	
	char temp[8192];
	CRegExp reg;
	sprintf(temp,"\"%s\",\"[^\"]*\",\"[^\"]*\",\"([^\"]*)\",\"([^\"]*)\",\"([0-9]*)[^\"]*\",\"[0-9]*\",\"[0-9]*\",\"([0-9]*):([0-9]*)",strHVSCpath.c_str());
	if( !reg.RegComp(temp) ) {
		CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..): failed to compile regular expression");
		tag.SetLoaded(false);
		return( false );
	}

	sprintf(temp,"%s\\%s",g_stSettings.m_szAlbumDirectory,"sidlist.csv"); // changeme?
	std::ifstream f(temp); 
	if( !f.good() ) {
		CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..) unable to locate sidlist.csv");
		tag.SetLoaded(false);
		return( false );
	}
	
	while( !f.eof() ) {
		f.getline(temp,8191);
		if( reg.RegFind(temp) >= 0 ) {
			char* szTitle = reg.GetReplaceString("\\1");
			char* szArtist = reg.GetReplaceString("\\2");
			char* szYear = reg.GetReplaceString("\\3");
			char* szMins = reg.GetReplaceString("\\4");
			char* szSecs = reg.GetReplaceString("\\5");
			tag.SetLoaded(true);
			tag.SetTrackNumber(0);
			tag.SetDuration(atoi(szMins)*60+atoi(szSecs));
			tag.SetTitle(szTitle);
			tag.SetArtist(szArtist);
			SYSTEMTIME dateTime;
			dateTime.wYear = atoi(szYear);
			tag.SetReleaseDate(dateTime);
			if( szTitle )
				free(szTitle);
			if( szArtist )
				free(szArtist);
			if( szYear )
				free(szYear);
			if( szMins )
				free(szMins);
			if( szSecs )
				free(szSecs);
			f.close();
			return( true );
		}
	}

	f.close();
	tag.SetLoaded(false);
	return( false );
}