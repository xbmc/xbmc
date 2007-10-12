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
#include "MusicInfoTagLoaderSid.h"
#include "utils/RegExp.h"
#include "Util.h"
#include <cstring>

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
  CStdString strFileToLoad = strFileName;
  int iTrack = 0;
  CStdString strExtension;
  CUtil::GetExtension(strFileName,strExtension);
  strExtension.MakeLower();
  if (strExtension==".sidstream")
  {
    //  Extract the track to play
    CStdString strFile=CUtil::GetFileName(strFileName);
    int iStart=strFile.ReverseFind("-")+1;
    iTrack = atoi(strFile.substr(iStart, strFile.size()-iStart-10).c_str());
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFileName;
    CUtil::GetDirectory(strPath, strFileToLoad);
    CUtil::RemoveSlashAtEnd(strFileToLoad);   // we want the filename
  }
  CStdString strFileNameLower(strFileToLoad);
  strFileNameLower.MakeLower();
  int iHVSC = strFileNameLower.find("hvsc"); // need hvsc in path name since our lookupfile is based on hvsc paths
  if (iHVSC < 0)
  {
    iHVSC = strFileNameLower.find("c64music");
    if (iHVSC >= 0)
      iHVSC += 8;
  }
  else
    iHVSC += 4;
  if( iHVSC < 0 )
  {
    tag.SetLoaded(false);
    return( false );
  }

  CStdString strHVSCpath = strFileToLoad.substr(iHVSC,strFileToLoad.length()-1);

  strHVSCpath.Replace('\\','/'); // unix paths
  strHVSCpath.MakeLower();

  char temp[8192];
  CRegExp reg;
  if (!reg.RegComp("TITLE: ([^\r\n]*)\r?\n[^A]*ARTIST: ([^\r\n]*)\r?\n"))
  {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..): failed to compile regular expression");
    tag.SetLoaded(false);
    return( false );
  }

  sprintf(temp,"%s\\%s",g_settings.GetDatabaseFolder().c_str(),"stil.txt"); // changeme?
  std::ifstream f(temp);
  if( !f.good() ) {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..) unable to locate stil.txt");
    tag.SetLoaded(false);
    return( false );
  }

  const char* szStart = NULL;
  const char* szEnd = NULL;
  char temp2[8191];
  char* temp3 = temp2;
  while( !f.eof() && !szEnd )
  {
    f.read(temp,8191);
    CStdString strLower = temp;
    strLower.MakeLower();

    if (!szStart)
      szStart= (char *)strstr(strLower.c_str(),strHVSCpath.c_str());
    if (szStart)
    {
      szEnd = strstr(szStart+strHVSCpath.size(),".sid");
      if (szEnd)
      {
        memcpy(temp3,temp+(szStart-strLower.c_str()),szEnd-szStart);
        temp3 += szEnd-szStart;
      }
      else
      {
        memcpy(temp3,temp+(szStart-strLower.c_str()),strlen(szStart));
        szStart = NULL;
        temp3 += strlen(szStart);
      }
    }
  }
  f.close();

  if (!f.eof() && szEnd)
  {
    temp2[temp3-temp2] = '\0';
    temp3 = strstr(temp2,"TITLE:");
  }
  else
    temp3 = NULL;
  if (temp3)
  {
    for (int i=0;i<iTrack-1;++i) // skip tracks
    {
      int iStart = reg.RegFind(temp3);
      if (!iStart)
      {
        tag.SetLoaded(false);
        return false;
      }
      temp3 += iStart;
    }
    /*int iFind = */reg.RegFind(temp3);
    if( reg.RegFind(temp3) > -1 )
    {
      char* szTitle = reg.GetReplaceString("\\1");
      char* szArtist = reg.GetReplaceString("\\2");
      char* szMins = NULL;
      char* szSecs = NULL;
      CRegExp reg2;
      reg2.RegComp("(.*) \\(([0-9]*):([0-9]*)\\)");
      if (reg2.RegFind(szTitle) > -1)
      {
        szMins = reg2.GetReplaceString("\\2");
        szSecs = reg2.GetReplaceString("\\3");
        char* szTemp = reg2.GetReplaceString("\\1");
        free(szTitle);
        szTitle = szTemp;
      }
      tag.SetLoaded(true);
      tag.SetURL(strFileToLoad);
      tag.SetTrackNumber(iTrack);
      if (szMins && szSecs)
        tag.SetDuration(atoi(szMins)*60+atoi(szSecs));

      tag.SetTitle(szTitle);
      tag.SetArtist(szArtist);
      if( szTitle )
        free(szTitle);
      if( szArtist )
        free(szArtist);
      if( szMins )
        free(szMins);
      if( szSecs )
        free(szSecs);
    }
  }

  sprintf(temp,"%s\\%s",g_settings.GetDatabaseFolder().c_str(),"sidlist.csv"); // changeme?
  std::ifstream f2(temp);
  if( !f2.good() ) {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..) unable to locate sidlist.csv");
    tag.SetLoaded(false);
    return( false );
  }

  while( !f2.eof() ) {
    f2.getline(temp,8191);
    CStdString strTemp(temp);
    strTemp.MakeLower();
    unsigned int iFind = strTemp.find(strHVSCpath);
    if (iFind == string::npos)
      continue;

    char temp2[1024];
    char temp3[1024];
    strncpy(temp3,temp+iFind,strlen(strHVSCpath));
    temp3[strlen(strHVSCpath)] = '\0';
    sprintf(temp2,"\"%s\",\"[^\"]*\",\"[^\"]*\",\"([^\"]*)\",\"([^\"]*)\",\"([0-9]*)[^\"]*\",\"[0-9]*\",\"[0-9]*\",\"",temp3);
    for (int i=0;i<iTrack-1;++i)
      strcat(temp2,"[0-9]*:[0-9]* ");
    strcat(temp2,"([0-9]*):([0-9]*)");
    if( !reg.RegComp(temp2) )
    {
      CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..): failed to compile regular expression");
      tag.SetLoaded(false);
      return( false );
    }
    if( reg.RegFind(temp) >= 0 ) {
      char* szTitle = reg.GetReplaceString("\\1");
      char* szArtist = reg.GetReplaceString("\\2");
      char* szYear = reg.GetReplaceString("\\3");
      char* szMins = reg.GetReplaceString("\\4");
      char* szSecs = reg.GetReplaceString("\\5");
      tag.SetLoaded(true);
      tag.SetTrackNumber(iTrack);
      if (tag.GetDuration() == 0)
        tag.SetDuration(atoi(szMins)*60+atoi(szSecs));
      if (tag.GetTitle() == "")
        tag.SetTitle(szTitle);
      if (tag.GetArtist() == "")
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
      f2.close();
      return( true );
    }
  }

  f2.close();
  tag.SetLoaded(false);
  return( false );
}

