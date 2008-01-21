/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MusicInfoTagLoaderMod.h"
#ifdef HAS_MIKMOD
#include "lib/mikxbox/mikmod.h"
#endif
#include "Util.h"

#include <fstream>

using namespace XFILE;
using namespace MUSIC_INFO;

CMusicInfoTagLoaderMod::CMusicInfoTagLoaderMod(void)
{
  CSectionLoader::Load("MOD_RX");
  CSectionLoader::Load("MOD_RW");
#ifdef HAS_MIKMOD
  MikMod_RegisterAllLoaders();
#endif
}

CMusicInfoTagLoaderMod::~CMusicInfoTagLoaderMod()
{
  CSectionLoader::Unload("MOD_RW");
	CSectionLoader::Unload("MOD_RX");
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
    tag.SetLoaded(false);
    if( getFile(strMod,strFileName) ) 
    {
#ifdef HAS_MIKMOD
      char* szTitle = Mod_Player_LoadTitle(reinterpret_cast<CHAR*>(const_cast<char*>(strMod.c_str())));
      
      if( szTitle ) {
        if( !CStdString(szTitle).empty() ) {
          tag.SetTitle(szTitle);
          free(szTitle);
          tag.SetLoaded(true);
        }
      }
#endif
    }
	}
    
  return tag.Loaded();
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

