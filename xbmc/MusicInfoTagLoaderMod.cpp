/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "MusicInfoTagLoaderMod.h"
#include "Util.h"
#include "MusicInfoTag.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"

#include <fstream>

using namespace XFILE;
using namespace MUSIC_INFO;
using namespace std;

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
  CStdString strMDZ(CUtil::ReplaceExtension(strFileName,".mdz"));
  if (CFile::Exists(strMDZ))
  {
    if (!getFile(strMDZ,strMDZ))
    {
      tag.SetLoaded(false);
      return( false );
    }
    ifstream inMDZ(_P(strMDZ.c_str()));
    char temp[8192];
    char temp2[8192];

    while (!inMDZ.eof())
    {
      inMDZ.getline(temp,8191);
      if (strstr(temp,"COMPOSER"))
      {
        strcpy(temp2,temp+strlen("COMPOSER "));
        tag.SetArtist(temp2);
      }
      else if (strstr(temp,"TITLE"))
      {
        strcpy(temp2,temp+strlen("TITLE "));
        tag.SetTitle(temp2);
        tag.SetLoaded(true);
      }
      else if (strstr(temp,"PLAYTIME"))
      {
        char* temp3 = strtok(temp+strlen("PLAYTIME "),":");
        int iSecs = atoi(temp3)*60;
        temp3 = strtok(NULL,":");
        iSecs += atoi(temp3);
        tag.SetDuration(iSecs);
      }
      else if (strstr(temp,"STYLE"))
      {
        strcpy(temp2,temp+strlen("STYLE "));
        tag.SetGenre(temp2);
      }
    }
    return( tag.Loaded() );
  }
  else
  {
    // TODO: no, then try to atleast fetch the title
  }

  return tag.Loaded();
}

bool CMusicInfoTagLoaderMod::getFile(CStdString& strFile, const CStdString& strSource)
{
  if (!CUtil::IsHD(strSource))
  {
    if (!CFile::Cache(strSource.c_str(), "special://temp/cachedmod", NULL, NULL))
    {
      CFile::Delete("special://temp/cachedmod");
      CLog::Log(LOGERROR, "ModTagLoader: Unable to cache file %s\n", strSource.c_str());
      strFile = "";
      return( false );
    }
    strFile = "special://temp/cachedmod";
  }
  else
    strFile = strSource;

  return( true );
}

