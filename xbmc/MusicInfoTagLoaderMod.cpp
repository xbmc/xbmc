#include "stdafx.h"
#include "MusicInfoTagLoaderMod.h"
#include "lib/mikxbox/mikmod.h"
#include "Util.h"

#include <fstream>

using namespace MUSIC_INFO;

CMusicInfoTagLoaderMod::CMusicInfoTagLoaderMod(void)
{
}

CMusicInfoTagLoaderMod::~CMusicInfoTagLoaderMod()
{
}

bool CMusicInfoTagLoaderMod::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	tag.SetURL(strFileName);
	// first, does the module have a .mdz?
	CStdString strMDZ;
	CUtil::ReplaceExtension(strFileName,".mdz",strMDZ);
	if( CFile::Exists(strMDZ) ) {
		if( !getFile(strMDZ,strMDZ) ) {
			tag.SetLoaded(false);
			return( false );
		}
		std::ifstream inMDZ(strMDZ.c_str());
		char temp[8192];
		char temp2[8192];
		
		while( !inMDZ.eof() ) {
			inMDZ.getline(temp,8191);
			if( strstr(temp,"COMPOSER") ) {
				strcpy(temp2,temp+strlen("COMPOSER "));
				tag.SetArtist(temp2);
			}
			else if( strstr(temp,"TITLE") ) {
				strcpy(temp2,temp+strlen("TITLE "));
				tag.SetTitle(temp2);
				tag.SetLoaded(true);
			} else if( strstr(temp,"PLAYTIME") ) {
				char* temp3 = strtok(temp+strlen("PLAYTIME "),":");
				int iSecs = atoi(temp3)*60;
				temp3 = strtok(NULL,":");
				iSecs += atoi(temp3);
				tag.SetDuration(iSecs);
			} else if( strstr(temp,"STYLE") ) {
				strcpy(temp2,temp+strlen("STYLE "));
				tag.SetGenre(temp2);
			}
		}
		return( tag.Loaded() );
	} else {
		// no, then try to atleast fetch the title
		CStdString strMod;
		if( !getFile(strMod,strFileName) ) {
			tag.SetLoaded(false);
			return( false );
		}
		char* szTitle = Mod_Player_LoadTitle(reinterpret_cast<CHAR*>(const_cast<char*>(strMod.c_str())));

		if( szTitle ) {
			if( !CStdString(szTitle).empty() ) {
				tag.SetTitle(szTitle);
				free(szTitle);
				tag.SetLoaded(true);
				return( true );
			}
		}
	}
	tag.SetLoaded(false);
	return( false );
}

bool CMusicInfoTagLoaderMod::getFile(CStdString& strFile, const CStdString& strSource)
{
	if( !CUtil::IsHD(strSource) ) {
		if (!CFile::Cache(strSource.c_str(), "Z:\\cachedmod", NULL, NULL)) {
			::DeleteFile("Z:\\cachedmod");
			CLog::Log(LOGERROR, "ModTagLoader: Unable to cache file %s\n", strSource.c_str());
			strFile = "";
			return( false );
		}
		strFile = "Z:\\cachedmod";
	}
	else
		strFile = strSource;
	
	return( true );
}