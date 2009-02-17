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
#include "stdafx.h"
#ifdef __APPLE__
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifdef HAS_LCD 
#include "utils/LCDFactory.h" 
#endif

#include "Application.h"
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "xbox/IoSupport.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "FileSystem/HDDirectory.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/VirtualPathDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/SpecialProtocol.h"
#include "ThumbnailCache.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#ifdef HAS_UPNP
#include "FileSystem/UPnPDirectory.h"
#endif
#ifdef HAS_CREDITS
#include "Credits.h"
#endif
#include "Shortcut.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "utils/RegExp.h"
#include "utils/AlarmClock.h"
#include "utils/RssFeed.h"
#include "ButtonTranslator.h"
#include "Picture.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogVideoScan.h"
#include "utils/fstrcmp.h"
#include "MediaManager.h"
#include "utils/Network.h"
#include "GUIPassword.h"
#ifdef HAS_FTP_SERVER
#include "lib/libfilezilla/xbfilezilla.h"
#endif
#include "lib/libscrobbler/scrobbler.h"
#include "LastFmManager.h"
#include "MusicInfoLoader.h"
#include "XBVideoConfig.h"
#include "DirectXGraphics.h"
#include "lib/libGoAhead/XBMChttp.h"
#include "DNSNameCache.h"
#include "FileSystem/PluginDirectory.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#ifdef _WIN32PC
#include <shlobj.h>
#include "WIN32Util.h"
#endif
#include "GUIDialogYesNo.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/File.h"
#include "PlayList.h"
#include "Crc32.h"

using namespace std;
using namespace DIRECTORY;

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000;

HANDLE CUtil::m_hCurrentCpuUsage = NULL;

using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;

#ifndef HAS_SDL
static D3DGAMMARAMP oldramp, flashramp;
#elif defined(HAS_SDL_2D)
static Uint16 oldrampRed[256];
static Uint16 oldrampGreen[256];
static Uint16 oldrampBlue[256];
static Uint16 flashrampRed[256];
static Uint16 flashrampGreen[256];
static Uint16 flashrampBlue[256];
#endif

XBOXDETECTION v_xboxclients;

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

/* returns filename extension including period of filename */
const CStdString CUtil::GetExtension(const CStdString& strFileName)
{
  int period = strFileName.find_last_of('.');
  if(period >= 0)
  {
    if( strFileName.find_first_of('/', period+1) != string::npos ) return "";
    if( strFileName.find_first_of('\\', period+1) != string::npos ) return "";

    /* url options could be at the end of a url */
    const int options = strFileName.find_first_of('?', period+1);

    if(options >= 0)
      return strFileName.substr(period, options-period);
    else
      return strFileName.substr(period);
  }
  else
    return "";
}

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
  strExtension = GetExtension(strFile);
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const CStdString CUtil::GetFileName(const CStdString& strFileNameAndPath)
{
  /* find any slashes */
  const int slash1 = strFileNameAndPath.find_last_of('/');
  const int slash2 = strFileNameAndPath.find_last_of('\\');

  /* select the last one */
  int slash;
  if(slash2>slash1)
    slash = slash2;
  else
    slash = slash1;

  /* check if there is any options in the url */
  const int options = strFileNameAndPath.find_first_of('?', slash+1);
  if(options < 0)
    return strFileNameAndPath.substr(slash+1);
  else
    return strFileNameAndPath.substr(slash+1, options-(slash+1));
}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString path(strFileNameAndPath);
  RemoveSlashAtEnd(path);
  CStdString strFilename = GetFileName(path);
  
  CURL url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.GetProtocol() == "upnp")
    strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
#endif

  if (url.GetProtocol() == "rss")
  {
    CRssFeed feed;
    feed.Init(path);
    feed.ReadFeed();
    strFilename = feed.GetFeedTitle();
  }

  // LastFM
  if (url.GetProtocol() == "lastfm")
  {
    if (strFilename.IsEmpty()) 
      strFilename = g_localizeStrings.Get(15200); 
    else 
      strFilename = g_localizeStrings.Get(15200) + " - " + strFilename; 
  }

  // Shoutcast
  else if (url.GetProtocol() == "shout")
  {
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0) 
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.GetProtocol() == "smb" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20171);

  // XBMSP Network
  else if (url.GetProtocol() == "xbms" && strFilename.IsEmpty()) 
    strFilename = "XBMSP Network";

  // iTunes music share (DAAP)
  else if (url.GetProtocol() == "daap" && strFilename.IsEmpty()) 
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.GetProtocol() == "hdhomerun" && strFilename.IsEmpty()) 
    strFilename = "HDHomerun Devices";
  
  // ReplayTV Devices
  else if (url.GetProtocol() == "rtv") 
    strFilename = "ReplayTV Devices";

  // SAP Streams
  else if (url.GetProtocol() == "sap" && strFilename.IsEmpty()) 
    strFilename = "SAP Streams";

  // Music Playlists
  else if (path.Left(24).Equals("special://musicplaylists")) 
    strFilename = g_localizeStrings.Get(20011);

  // Video Playlists
  else if (path.Left(24).Equals("special://videoplaylists")) 
    strFilename = g_localizeStrings.Get(20012);

  // now remove the extension if needed
  if (g_guiSettings.GetBool("filelists.hideextensions") && !bIsFolder)
  {
    RemoveExtension(strFilename);
    return strFilename;
  }
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_advancedSettings.m_videoStackRegExps;

  CStdString strFileNameTemp = strFileName;
  CStdString strFileNameLower = strFileName;
  strFileNameLower.MakeLower();

  CStdString strVolume;
  CStdString strTestString;
  CRegExp reg;

//  CLog::Log(LOGDEBUG, "GetVolumeFromFileName:[%s]", strFileNameLower.c_str());
  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp:[%s]", regexps[i].c_str());
      continue;
    }
//    CLog::Log(LOGDEBUG, "Regexp:[%s]", regexps[i].c_str());

    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    {
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();

      /*
      reg.DumpOvector(LOGDEBUG);
      CLog::Log(LOGDEBUG, "Subcount=%i", iCount);
      for (int j = 0; j <= iCount; j++)
      {
        CStdString str = reg.GetMatch(j);
        CLog::Log(LOGDEBUG, "Sub(%i):[%s]", j, str.c_str());
      }
      */

      // simple regexp, only the volume is captured
      if (iCount == 1)
      {
        strVolumeNumber = reg.GetMatch(1);
        if (strVolumeNumber.IsEmpty()) return false;

        // Remove the extension (if any).  We do this on the base filename, as the regexp
        // match may include some of the extension (eg the "." in particular).
        // The extension will then be added back on at the end - there is no reason
        // to clean it off here. It will be cleaned off during the display routine, if
        // the settings to hide extensions are turned on.
        CStdString strFileNoExt = strFileNameTemp;
        RemoveExtension(strFileNoExt);
        CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
        CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
        strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;
        
        return true;
      }

      // advanced regexp with prefix (1), volume (2), and suffix (3)
      else if (iCount == 3)
      {
        // second subpatten contains the stacking volume
        strVolumeNumber = reg.GetMatch(2);
        if (strVolumeNumber.IsEmpty()) return false;

        // everything before the regexp match
        strFileTitle = strFileName.Left(iFoundToken);

        // first subpattern contains prefix
        strFileTitle += reg.GetMatch(1);

        // third subpattern contains suffix
        strFileTitle += reg.GetMatch(3);

        // everything after the regexp match
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);

        return true;
      }

      // unknown regexp format
      else
      {
        CLog::Log(LOGERROR, "Incorrect movie stacking regexp format:[%s]", regexps[i].c_str());
      }
    }
  }
  return false;
}

void CUtil::RemoveExtension(CStdString& strFileName)
{
  int iPos = strFileName.ReverseFind(".");
  // Extension found
  if (iPos > 0)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileName, strExtension);
    strExtension.ToLower();

    CStdString strFileMask;
    strFileMask = g_stSettings.m_pictureExtensions;
    strFileMask += g_stSettings.m_musicExtensions;
    strFileMask += g_stSettings.m_videoExtensions;
    strFileMask += ".py|.xml|.milk|.xpr|.cdg";

    // Only remove if its a valid media extension
    if (strFileMask.Find(strExtension.c_str()) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

void CUtil::CleanString(CStdString& strFileName, bool bIsFolder /* = false */)
{

  if (strFileName.Equals(".."))
   return;

  const CStdStringArray &regexps = g_advancedSettings.m_videoCleanRegExps;

  CRegExp reTags, reYear;
  CStdString strYear, strExtension;
  CStdString strFileNameTemp = strFileName;

  if (!bIsFolder)
  {
    GetExtension(strFileNameTemp, strExtension);
    RemoveExtension(strFileNameTemp);
  }

  reYear.RegComp("(.+[^ _\\,\\.\\(\\)\\[\\]\\-])[ _\\.\\(\\)\\[\\]\\-]+(19[0-9][0-9]|20[0-1][0-9])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  if (reYear.RegFind(strFileNameTemp.c_str()) >= 0)
  {
    strFileNameTemp = reYear.GetReplaceString("\\1");
    strYear = reYear.GetReplaceString("\\2");
  }

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strFileName.ToLower().c_str())) >= 0)
      strFileNameTemp = strFileNameTemp.Mid(0, j);
  }
  
  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  { 
    bool alreadyContainsSpace = (strFileNameTemp.Find(' ') >= 0); 
 
    for (int i = 0; i < (int)strFileNameTemp.size(); i++) 
    { 
      char c = strFileNameTemp.GetAt(i); 
      if ((c == '_') || ((!alreadyContainsSpace) && (c == '.'))) 
      { 
        strFileNameTemp.SetAt(i, ' '); 
      } 
    } 
  } 

  strFileName = strFileNameTemp.Trim();
  
  // append year
  if (!strYear.IsEmpty())
    strFileName = strFileName + " (" + strYear + ")";

  // restore extension if needed
  if (!g_guiSettings.GetBool("filelists.hideextensions") && !bIsFolder)
    strFileName += strExtension;

}

void CUtil::GetCommonPath(CStdString& strParent, const CStdString& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent = strParent.Left(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!CUtil::HasSlashAtEnd(strParent))
  {
    // currently GetDirectory() removes trailing slashes
    CUtil::GetDirectory(strParent.Mid(0), strParent);
    CUtil::AddSlashAtEnd(strParent);
  }
}

bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURL url(strPath);
  CStdString strFile = url.GetFileName();
  if ( ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) && strFile.IsEmpty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strPath,items);
    CUtil::GetDirectory(items[0]->m_strPath,items[0]->m_strDVDLabel);
    if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
      GetParentPath(items[0]->m_strDVDLabel, strParent);
    else
      strParent = items[0]->m_strDVDLabel;
    for( int i=1;i<items.Size();++i)
    {
      CUtil::GetDirectory(items[i]->m_strPath,items[i]->m_strDVDLabel);
      if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
        GetParentPath(items[i]->m_strDVDLabel, items[i]->m_strPath);
      else
        items[i]->m_strPath = items[i]->m_strDVDLabel;

      GetCommonPath(strParent,items[i]->m_strPath);
    }
    return true;
  }
  else if (url.GetProtocol() == "multipath")
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.GetProtocol() == "plugin")
  {
    if (!url.GetOptions().IsEmpty())
    {
      url.SetOptions("");
      url.GetURL(strParent);
      return true;
    }
    if (!url.GetFileName().IsEmpty())
    {
      url.SetFileName("");
      url.GetURL(strParent);
      return true;
    }
    if (!url.GetHostName().IsEmpty())
    {
      url.SetHostName("");
      url.GetURL(strParent);
      return true;
    }
    return true;  // already at root
  }
  else if (strFile.size() == 0)
  {
    if (url.GetHostName().size() > 0)
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      url.GetURL(strParent);
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile = strFile.Left(strFile.size() - 1);
  }

  int iPos = strFile.ReverseFind('/');
#ifndef _LINUX
  if (iPos < 0)
  {
    iPos = strFile.ReverseFind('\\');
  }
#endif
  if (iPos < 0)
  {
    url.SetFileName("");
    url.GetURL(strParent);
    return true;
  }

  strFile = strFile.Left(iPos);

  CUtil::AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  url.GetURL(strParent);
  return true;
}

const CStdString CUtil::GetMovieName(CFileItem* pItem, bool bUseFolderNames /* = false */)
{
  CStdString movieName;
  CStdString strArchivePath;
  movieName = pItem->m_strPath; 

  if (pItem->IsMultiPath())
    movieName = CMultiPathDirectory::GetFirstPath(pItem->m_strPath);

  if (IsStack(movieName))
    movieName = CStackDirectory::GetStackedTitlePath(movieName);

  if ((!pItem->m_bIsFolder || pItem->IsDVDFile(false, true) || IsInArchive(pItem->m_strPath)) && bUseFolderNames)
  {
    GetParentPath(pItem->m_strPath, movieName);
    if (IsInRAR(pItem->m_strPath) || IsInZIP(pItem->m_strPath) || movieName.Find( "VIDEO_TS" )  != -1)
    {
      GetParentPath(movieName, strArchivePath);
      movieName = strArchivePath;
    }
  }

  CUtil::RemoveSlashAtEnd(movieName); 
  movieName = CUtil::GetFileName(movieName); 

  if (!pItem->m_bIsFolder)
    CUtil::RemoveExtension(movieName);

  return movieName;
}

void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  //Make sure you have a full path in the filename, otherwise adds the base path before.
  CURL plItemUrl(strFilename);
  CURL plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if (plBaseUrl.IsLocal()) //Base in local directory
  {
    if (plItemUrl.IsLocal() ) //Filename is local or not qualified
    {
#ifdef _LINUX
      if (!( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') ) ) //Filename not fully qualified
#else
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
#endif
      {
        if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath))
        {
          strFilename = strBasePath + strFilename;
          strFilename.Replace('/', '\\');
        }
        else
        {
          strFilename = strBasePath + '\\' + strFilename;
          strFilename.Replace('/', '\\');
        }
      }
    }
    strFilename.Replace("\\.\\", "\\");
    while ((iDotDotLoc = strFilename.Find("\\..\\")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('\\') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }

    // This routine is only called from the playlist loaders,
    // where the filepath is in UTF-8 anyway, so we don't need
    // to do checking for FatX characters.
    //if (g_guiSettings.GetBool("servers.ftpautofatx") && (CUtil::IsHD(strFilename)))
    //  CUtil::GetFatXQualifiedPath(strFilename);
  }
  else //Base is remote
  {
    if (plItemUrl.IsLocal()) //Filename is local
    {
#ifdef _LINUX
      if ( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') )  //Filename not fully qualified
#else
      if (strFilename[1] == ':') // already fully qualified
#endif
        return;
      if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath)) //Begins with a slash.. not good.. but we try to make the best of it..

      {
        strFilename = strBasePath + strFilename;
        strFilename.Replace('\\', '/');
      }
      else
      {
        strFilename = strBasePath + '/' + strFilename;
        strFilename.Replace('\\', '/');
      }
    }
    strFilename.Replace("/./", "/");
    while ((iDotDotLoc = strFilename.Find("/../")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('/') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  strPath = getenv("XBMC_HOME");
  if (strPath != NULL && !strPath.IsEmpty())
  {
#ifdef _WIN32PC
    //expand potential relative path to full path
    if(GetFullPathName(strPath, 1024, szXBEFileName, 0) != 0)
    {
      strPath = szXBEFileName;
    }
#endif
  }
  else
  {
#ifdef _DEBUG
    printf("The XBMC_HOME environment variable is not set.\n");
#endif
#ifdef __APPLE__
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size = 2*MAXPATHLEN;

    result = _NSGetExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      // Assume local path inside application bundle.
      strcat(given_path, "../Resources/XBMC/");

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
      {
        strPath = real_path;
        return;
      }
    }
#endif
    char *szFileName = strrchr(szXBEFileName, PATH_SEPARATOR_CHAR);
    *szFileName = 0;
    strPath = szXBEFileName;
  }
}

/* WARNING, this function can easily fail on full urls, since they might have options at the end */
void CUtil::ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile)
{
  CStdString strExtension;
  GetExtension(strFile, strExtension);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
}

bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size() == 0) return false;
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

bool CUtil::IsRemote(const CStdString& strFile)
{
  CURL url(strFile);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();
  if (strProtocol == "cdda" || strProtocol == "iso9660" || strProtocol == "plugin") return false;
  if (strProtocol == "special") return IsRemote(CSpecialProtocol::TranslatePath(strFile));
  if (strProtocol.Left(3) == "mem") return false;   // memory cards
  if (strProtocol == "stack") return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));
  if (strProtocol == "virtualpath")
  { // virtual paths need to be checked separately
    CVirtualPathDirectory dir;
    vector<CStdString> paths;
    if (dir.GetPathes(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }
  if (strProtocol == "multipath")
  { // virtual paths need to be checked separately
    vector<CStdString> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }
  if ( !url.IsLocal() ) return true;
  return false;
}

bool CUtil::IsOnDVD(const CStdString& strFile)
{
#ifdef _WIN32PC
  if (strFile.Mid(1,1) == ":")
    return (GetDriveType(strFile.Left(2)) == DRIVE_CDROM);
#else
  if (strFile.Left(2).CompareNoCase("d:") == 0)
    return true;
#endif

  if (strFile.Left(4).CompareNoCase("dvd:") == 0)
    return true;

  if (strFile.Left(4).CompareNoCase("udf:") == 0)
    return true;

  if (strFile.Left(8).CompareNoCase("iso9660:") == 0)
    return true;

  if (strFile.Left(5).CompareNoCase("cdda:") == 0)
    return true;

  return false;
}

bool CUtil::IsOnLAN(const CStdString& strPath)
{
  if(IsMultiPath(strPath))
    return CUtil::IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath));
  if(IsStack(strPath))
    return CUtil::IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath));
  if(strPath.Left(8) == "special:")
    return CUtil::IsOnLAN(CSpecialProtocol::TranslatePath(strPath));
  if(IsDAAP(strPath))
    return true;
  if(IsTuxBox(strPath))
    return true;
  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if(IsInRAR(strPath) || IsInZIP(strPath))
    return CUtil::IsOnLAN(url.GetHostName());

  if(!IsRemote(strPath))
    return false;

  CStdString host = url.GetHostName();
  if(host.length() == 0)
    return false;


  unsigned long address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    CStdString ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address == INADDR_NONE)
  {
    // assume a hostname without dot's
    // is local (smb netbios hostnames)
    if(host.find('.') == string::npos)
      return true;
  }
  else
  {
    // check if we are on the local subnet
#if defined(HAS_LINUX_NETWORK) || defined(HAS_WIN32_NETWORK)
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;
    unsigned long subnet = ntohl(inet_addr(g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentNetmask()));
    unsigned long local  = ntohl(inet_addr(g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress()));
#else
    unsigned long subnet = ntohl(inet_addr(g_application.getNetwork().m_networkinfo.subnet));
    unsigned long local  = ntohl(inet_addr(g_application.getNetwork().m_networkinfo.ip));
#endif
    if( (address & subnet) == (local & subnet) )
      return true;
  }
  return false;
}

bool CUtil::IsMultiPath(const CStdString& strPath)
{
  if (strPath.Left(12).Equals("multipath://")) return true;
  return false;
}

bool CUtil::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile;
  strFileLow.MakeLower();
#if defined(_WIN32PC)
  if((GetDriveType(strFile.c_str()) == DRIVE_CDROM) || strFile.Left(6).Equals("dvd://"))
    return true;
#else
  if (strFileLow == "d:/"  || strFileLow == "d:\\"  || strFileLow == "d:" || strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool CUtil::IsVirtualPath(const CStdString& strFile)
{
  if (strFile.Left(14).Equals("virtualpath://")) return true;
  return false;
}

bool CUtil::IsStack(const CStdString& strFile)
{
  if (strFile.Left(8).Equals("stack://")) return true;
  return false;
}

bool CUtil::IsRAR(const CStdString& strFile)
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);

  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001"))
    return true;
  if (strExtension.CompareNoCase(".cbr") == 0)
    return true;
  if (strExtension.CompareNoCase(".rar") == 0)
    return true;

  return false;
}

bool CUtil::IsInArchive(const CStdString &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile);
}

bool CUtil::IsInZIP(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol() == "zip" && url.GetFileName() != "";
}

bool CUtil::IsInRAR(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol() == "rar" && url.GetFileName() != "";
}

bool CUtil::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  if (strExtension.CompareNoCase(".zip") == 0) return true;
  if (strExtension.CompareNoCase(".cbz") == 0) return true;
  return false;
}

bool CUtil::IsCDDA(const CStdString& strFile)
{
  return strFile.Left(5).Equals("cdda:");
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  return strFile.Left(8).Equals("iso9660:");
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  return strFile.Left(4).Equals("smb:");
}

bool CUtil::IsDAAP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("daap:");
}

bool CUtil::IsUPnP(const CStdString& strFile)
{
    return strFile.Left(5).Equals("upnp:");
}

bool CUtil::IsMemCard(const CStdString& strFile)
{
  return strFile.Left(3).Equals("mem");
}
bool CUtil::IsTuxBox(const CStdString& strFile)
{
  return strFile.Left(7).Equals("tuxbox:");
}

bool CUtil::IsMythTV(const CStdString& strFile)
{
  return strFile.Left(5).Equals("myth:");
}

bool CUtil::IsVTP(const CStdString& strFile)
{
  return strFile.Left(4).Equals("vtp:");
}

bool CUtil::IsTV(const CStdString& strFile)
{
  return IsMythTV(strFile) || IsTuxBox(strFile) || IsVTP(strFile);
}

bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps)
{
  if (strFileOrFolder.IsEmpty())
    return false;

  CStdString strExclude = strFileOrFolder;
  RemoveSlashAtEnd(strExclude);
  strExclude = GetFileName(strExclude);
  strExclude.MakeLower();
  
  CRegExp regExExcludes;

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strExclude) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strFileOrFolder.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!IsRemote(strURL)) return ;
  if (IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

void CUtil::UrlDecode(CStdString& strURLData)
//modified to be more accomodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  CStdString strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp,"%x",(unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  strURLData = strResult;
}

void CUtil::URLEncode(CStdString& strURLData)
{
  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    //if (kar == ' ') strResult += '+';
    if (isalnum(kar)) strResult += kar;
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  strURLData = strResult;
}

bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = CUtil::GetFileName(strFileName);
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  if (CUtil::HasSlashAtEnd(strDescription) )
  {
    strDescription = strDescription.Left(strDescription.size() - 1);
  }
  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

void CUtil::CreateShortcut(CFileItem* pItem)
{
}

void CUtil::ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl )
{
  strOutUrl = strProtocol;
  CStdString temp = strPath;
  temp.Replace( '\\', '/' );
  temp.Delete( 0, 3 );
  strOutUrl += temp;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !CDetectDVDMedia::IsDiscInDrive() )
  {
    strIcon = "defaultDVDEmpty.png";
    return ;
  }

  if ( IsDVD(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "defaultXBOXDVD.png";
      return ;
    }
    strIcon = "defaultDVDRom.png";
    return ;
  }

  if ( IsISO9660(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "defaultVCD.png";
      return ;
    }
    strIcon = "defaultDVDRom.png";
    return ;
  }

  if ( IsCDDA(strPath) )
  {
    strIcon = "defaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  WIN32_FIND_DATA wfd;

  CStdString strAlbumDir = CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), "*.tmp");
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(_P(strAlbumDir).c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      CFile::Delete(CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), wfd.cFileName));
  }
  while (FindNextFile(hFind, &wfd));
}

void CUtil::DeleteGUISettings()
{
  // Load in master code first to ensure it's setting isn't reset
  TiXmlDocument doc;
  if (doc.LoadFile(g_settings.GetSettingsFile()))
  {
    g_guiSettings.LoadMasterLock(doc.RootElement());
  }
  // delete the settings file only
  CLog::Log(LOGINFO, "  DeleteFile(%s)", g_settings.GetSettingsFile().c_str());
  CFile::Delete(g_settings.GetSettingsFile());
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  CURL url(_P(strFileName));
  return url.IsLocal();
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  WIN32_FIND_DATA wfd;
#ifndef _LINUX
  CAutoPtrFind hFind ( FindFirstFile(_P("special://temp/*.*"), &wfd));
#else
  CAutoPtrFind hFind ( FindFirstFile(_P("special://temp/*"), &wfd));
#endif
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strFile;
          strFile.Format("special://temp/%s", wfd.cFileName);
          if (strFile.Find("subtitle") >= 0 )
          {
            if (strFile.Find(".keep") != (signed int) strFile.size()-5) // do not remove files ending with .keep
              CFile::Delete(strFile);
          }
          else if (strFile.Find("vobsub_queue") >= 0 )
          {
            CFile::Delete(strFile);
          }
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

static const char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", NULL};

void CUtil::CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback )
{
  DWORD startTimer = timeGetTime();
  CLog::Log(LOGDEBUG,"%s: START", __FUNCTION__);

  // new array for commons sub dirs
  const char * common_sub_dirs[] = {"subs",
                              "Subs",
                              "subtitles",
                              "Subtitles",
                              "vobsubs",
                              "Vobsubs",
                              "sub",
                              "Sub",
                              "vobsub",
                              "Vobsub",
                              "subtitle",
                              "Subtitle",
                              NULL};

  vector<CStdString> vecExtensionsCached;
  strExtensionCached = "";

  ClearSubtitles();

  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return ;
  if (item.IsPlayList()) return ;
  if (!item.IsVideo()) return ;

  vector<CStdString> strLookInPaths;

  CStdString strFileName;
  CStdString strFileNameNoExt;
  CStdString strPath;

  CUtil::Split(strMovie, strPath, strFileName);
  ReplaceExtension(strFileName, "", strFileNameNoExt);
  strLookInPaths.push_back(strPath);

  if (!g_stSettings.iAdditionalSubtitleDirectoryChecked && !g_guiSettings.GetString("subtitles.custompath").IsEmpty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !IsHD(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      g_stSettings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }
    else if (!CDirectory::Exists(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      g_stSettings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }

    g_stSettings.iAdditionalSubtitleDirectoryChecked = 1;
  }

  if (strMovie.substr(0,6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    CStdString strArchive = url.GetHostName();
    CUtil::Split(strArchive, strPath, strFileName);
    strLookInPaths.push_back(strPath);
  }

  // checking if any of the common subdirs exist ..
  CLog::Log(LOGDEBUG,"%s: Checking for common subirs...", __FUNCTION__);
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    CStdString strParent;
    CUtil::GetParentPath(strLookInPaths[i],strParent);
    if (CURL(strParent).GetFileName() == "")
      strParent = "";
    for (int j=0; common_sub_dirs[j]; j+=2)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j],strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
      else
      {
        CURL url(strLookInPaths[i]);
        if (url.GetProtocol() == "smb" || url.GetProtocol() == "xbms")
        {
          CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j+1],strPath2);
          if (CDirectory::Exists(strPath2))
            strLookInPaths.push_back(strPath2);
        }
      }

      // ../common dirs aswell
      if (strParent != "")
      {
        CUtil::AddFileToFolder(strParent,common_sub_dirs[j],strPath2);
        if (CDirectory::Exists(strPath2))
          strLookInPaths.push_back(strPath2);
        else
        {
          CURL url(strParent);

          if (url.GetProtocol() == "smb" || url.GetProtocol() == "xbms")
          {
            CUtil::AddFileToFolder(strParent,common_sub_dirs[j+1],strPath2);
            if (CDirectory::Exists(strPath2))
              strLookInPaths.push_back(strPath2);
          }
        }
      }
    }
  }
  // .. done checking for common subdirs

  // check if there any cd-directories in the paths we have added so far
  char temp[6];
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    sprintf(temp,"cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],temp,strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs

  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (g_stSettings.iAdditionalSubtitleDirectoryChecked == 1)
  {
    strPath = g_guiSettings.GetString("subtitles.custompath");
    if (!HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }

  DWORD nextTimer = timeGetTime();
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(nextTimer - startTimer));

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;

  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",false);
      int fnl = strFileNameNoExt.size();

      CStdString strFileNameNoExtNoCase(strFileNameNoExt);
      strFileNameNoExtNoCase.MakeLower();
      for (int j = 0; j < (int)items.Size(); j++)
      {
        Split(items[j]->m_strPath, strPath, strItem);

        // is this a rar-file ..
        if ((CUtil::IsRAR(strItem) || CUtil::IsZIP(strItem)) && g_guiSettings.GetBool("subtitles.searchrars"))
        {
          CStdString strRar, strItemWithPath;
          CUtil::AddFileToFolder(strLookInPaths[step],strFileNameNoExt+CUtil::GetExtension(strItem),strRar);
          CUtil::AddFileToFolder(strLookInPaths[step],strItem,strItemWithPath);

          unsigned int iPos = strMovie.substr(0,6)=="rar://"?1:0;
          iPos = strMovie.substr(0,6)=="zip://"?1:0;
          if ((step != iPos) || (strFileNameNoExtNoCase+".rar").Equals(strItem) || (strFileNameNoExtNoCase+".zip").Equals(strItem))
            CacheRarSubtitles( vecExtensionsCached, items[j]->m_strPath, strFileNameNoExtNoCase);
        }
        else
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            int l = strlen(sub_exts[i]);

            //Cache any alternate subtitles.
            if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
            {
              strLExt = strItem.Right(strItem.GetLength() - 9);
              strDest.Format("special://temp/subtitle.alt-%s", strLExt);
              if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
              {
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                strExtensionCached = strLExt;
              }
            }

            //Cache subtitle with same name as movie
            if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
            {
              strLExt = strItem.Right(strItem.size() - fnl - 1); //Disregard separator char
              strDest.Format("special://temp/subtitle.%s", strLExt);
              if (find(vecExtensionsCached.begin(),vecExtensionsCached.end(),strLExt) == vecExtensionsCached.end())
              {
                if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
                {
                  vecExtensionsCached.push_back(strLExt);
                  CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                }
              }
            }
          }
        }
      }

      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - nextTimer));

  // rename any keep subtitles
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items,".keep");
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      CFile::Delete(items[i]->m_strPath.Left(items[i]->m_strPath.size()-5));
      CFile::Rename(items[i]->m_strPath,items[i]->m_strPath.Left(items[i]->m_strPath.size()-5));
    }
  }

  // construct string of added exts?
  for (vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+" ";

  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(timeGetTime() - startTimer));
}

bool CUtil::CacheRarSubtitles(vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt)
{
  bool bFoundSubs = false;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (CUtil::GetExtension(strRarPath).Equals(".zip"))
  {
    CStdString strZipPath;
    CUtil::CreateArchivePath(strZipPath,"zip",strRarPath,"");
    if (!CDirectory::GetDirectory(strZipPath,ItemList,"",false))
      return false;
  }
  else
  {
    // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
    // so now we dont have to find any subdirs anymore, all files in the rar is checked.
    if( !g_RarManager.GetFilesInRar(ItemList, strRarPath, false, "") )
      return false;
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->m_strPath;
    CLog::Log(LOGDEBUG, "CacheRarSubs:: Found file %s", strPathInRar.c_str());
    // always check any embedded rar archives
    // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
    if (CUtil::IsRAR(strPathInRar) || CUtil::IsZIP(strPathInRar))
    {
      CStdString strExtAdded;
      CStdString strRarInRar;
      if (CUtil::GetExtension(strPathInRar).Equals(".rar"))
        CUtil::CreateArchivePath(strRarInRar, "rar", strRarPath, strPathInRar);
      else
        CUtil::CreateArchivePath(strRarInRar, "zip", strRarPath, strPathInRar);
      CacheRarSubtitles(vecExtensionsCached,strRarInRar,strCompare, strExtExt);
    }
    // done checking if this is a rar-in-rar

    int iPos=0;
    CStdString strExt = CUtil::GetExtension(strPathInRar);
    CStdString strFileName = CUtil::GetFileName(strPathInRar);
    CStdString strFileNameNoCase(strFileName);
    strFileNameNoCase.MakeLower();
    if (strFileNameNoCase.Find(strCompare) >= 0)
      while (sub_exts[iPos])
      {
        if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
        {
          CStdString strSourceUrl, strDestUrl;
          if (CUtil::GetExtension(strRarPath).Equals(".rar"))
            CUtil::CreateArchivePath(strSourceUrl, "rar", strRarPath, strPathInRar);
          else
            strSourceUrl = strPathInRar;

          CStdString strDestFile;
          strDestFile.Format("special://temp/subtitle%s%s", sub_exts[iPos],strExtExt.c_str());

          if (CFile::Cache(strSourceUrl,strDestFile))
          {
            vecExtensionsCached.push_back(CStdString(sub_exts[iPos]));
            CLog::Log(LOGINFO, " cached subtitle %s->%s", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
            break;
          }
        }

        iPos++;
      }
  }
  return bFoundSubs;
}

void CUtil::PrepareSubtitleFonts()
{
  CStdString strFontPath = "special://xbmc/system/players/mplayer/font";

  if( IsUsingTTFSubtitles()
    || g_guiSettings.GetInt("subtitles.height") == 0
    || g_guiSettings.GetString("subtitles.font").size() == 0)
  {
    /* delete all files in the font dir, so mplayer doesn't try to load them */

    CStdString strSearchMask = strFontPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(_P(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if(wfd.cFileName[0] == 0) continue;
        if( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
          CFile::Delete(CUtil::AddFileToFolder(strFontPath, wfd.cFileName));
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
  else
  {
    CStdString strPath;
    strPath.Format("%s\\%s\\%i",
                  strFontPath.c_str(),
                  g_guiSettings.GetString("Subtitles.Font").c_str(),
                  g_guiSettings.GetInt("Subtitles.Height"));

    CStdString strSearchMask = strPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(_P(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if (wfd.cFileName[0] == 0) continue;
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strSource = CUtil::AddFileToFolder(strPath, wfd.cFileName);
          CStdString strDest = CUtil::AddFileToFolder(strFontPath, wfd.cFileName);
          CFile::Cache(strSource, strDest);
        }
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n = dwHigh;
  n <<= 32;
  n += dwLow;
  return n;
}

bool CUtil::IsDOSPath(const CStdString &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  return false;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  strResult = strFolder;
  // remove the stack:// as it screws up the logic below
  // FIXME: this appears to be plain wrong.
  if (IsStack(strFolder))
    strResult = strResult.Mid(8);

  // Add a slash to the end of the path if necessary
  bool unixPath = !IsDOSPath(strResult);
  if (!CUtil::HasSlashAtEnd(strResult))
  {
    if (unixPath)
      strResult += '/';
    else
      strResult += '\\';
  }
  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.Mid(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (unixPath)
    strResult.Replace('\\', '/');
  else
    strResult.Replace('/', '\\');

  // re-add the stack:// protocol
  if (IsStack(strFolder))
    strResult = "stack://" + strResult;
}

void CUtil::AddSlashAtEnd(CStdString& strFolder)
{
  // correct check for base url like smb://
  if (strFolder.Right(3).Equals("://"))
    return;

  if (!CUtil::HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

void CUtil::RemoveSlashAtEnd(CStdString& strFolder)
{
  // correct check for base url like smb://
  if (strFolder.Right(3).Equals("://"))
    return;

  if (CUtil::HasSlashAtEnd(strFolder))
    strFolder.Delete(strFolder.size() - 1);
}

void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end

  int iPos1 = strFilePath.ReverseFind('/');
  int iPos2 = strFilePath.ReverseFind('\\');

  if (iPos2 > iPos1)
  {
    iPos1 = iPos2;
  }

  if (iPos1 > 0)
  {
    strDirectoryPath = strFilePath.Left(iPos1 + 1); // include the slash
  }
}

void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

void CUtil::CreateArchivePath(CStdString& strUrlPath, const CStdString& strType,
                              const CStdString& strArchivePath,
                              const CStdString& strFilePathInArchive,
                              const CStdString& strPwd)
{
  CStdString strBuffer;

  strUrlPath = strType+"://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CUtil::URLEncode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }

  strBuffer = strArchivePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInArchive;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
  return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
  CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
  CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
  return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD()
{
  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");
  CFileItem item("dvd://1", false);
  item.SetLabel(CDetectDVDMedia::GetDVDLabel());
  g_application.PlayFile(item);
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(fn_template.c_str(), i);

    WIN32_FIND_DATA wfd;
    HANDLE hFind;
    memset(&wfd, 0, sizeof(wfd));
    if ((hFind = FindFirstFile(_P(name).c_str(), &wfd)) != INVALID_HANDLE_VALUE)
      FindClose(hFind);
    else
    {
      // FindFirstFile didn't find the file 'szName', return it
      return name;
    }
  }
  return "";
}

void CUtil::InitGamma()
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldramp);
#elif defined(HAS_SDL_2D)
  SDL_GetGammaRamp(oldrampRed, oldrampGreen, oldrampBlue);
#endif
}
void CUtil::RestoreBrightnessContrastGamma()
{
  g_graphicsContext.Lock();
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &oldramp);
#elif defined(HAS_SDL_2D)
  SDL_SetGammaRamp(oldrampRed, oldrampGreen, oldrampGreen);
#endif
  g_graphicsContext.Unlock();
}

void CUtil::SetBrightnessContrastGammaPercent(int iBrightNess, int iContrast, int iGamma, bool bImmediate)
{
  if (iBrightNess < 0) iBrightNess = 0;
  if (iBrightNess > 100) iBrightNess = 100;
  if (iContrast < 0) iContrast = 0;
  if (iContrast > 100) iContrast = 100;
  if (iGamma < 0) iGamma = 0;
  if (iGamma > 100) iGamma = 100;

  float fBrightNess = (((float)iBrightNess) / 50.0f) - 1.0f; // -1..1 Default: 0
  float fContrast = (((float)iContrast) / 50.0f);      // 0..2  Default: 1
  float fGamma = (((float)iGamma) / 40.0f) + 0.5f;      // 0.5..3.0 Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, bImmediate);
}

void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
  // calculate ramp
#ifndef HAS_SDL
  D3DGAMMARAMP ramp;
#elif defined(HAS_SDL_2D)
  Uint16 rampRed[256];
  Uint16 rampGreen[256];
  Uint16 rampBlue[256];
#endif

  Gamma = 1.0f / Gamma;
#ifndef HAS_SDL
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }
#elif defined(HAS_SDL_2D)
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    rampBlue[i] = rampGreen[i] = rampRed[i] = clamp(f);
  }
#endif

  // set ramp next v sync
  g_graphicsContext.Lock();
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &ramp);
#elif defined(HAS_SDL_2D)
  SDL_SetGammaRamp(rampRed, rampGreen, rampBlue);
#endif
  g_graphicsContext.Unlock();
}


void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}


void CUtil::FlashScreen(bool bImmediate, bool bOn)
{
  static bool bInFlash = false;

  if (bInFlash == bOn)
    return ;
  bInFlash = bOn;
  g_graphicsContext.Lock();
  if (bOn)
  {
#ifndef HAS_SDL
    g_graphicsContext.Get3DDevice()->GetGammaRamp(&flashramp);
#elif defined(HAS_SDL_2D)
    SDL_GetGammaRamp(flashrampRed, flashrampGreen, flashrampBlue);
#endif
    SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
  }
  else
#ifndef HAS_SDL
    g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &flashramp);
#elif defined(HAS_SDL_2D)
    SDL_SetGammaRamp(flashrampRed, flashrampGreen, flashrampBlue);
#endif
  g_graphicsContext.Unlock();
}

void CUtil::TakeScreenshot(const char* fn, bool flashScreen)
{
#ifndef HAS_SDL
    LPDIRECT3DSURFACE8 lpSurface = NULL;

    g_graphicsContext.Lock();
    if (g_application.IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, sizeof(OVERSCAN));
      g_graphicsContext.ResetOverscan(g_graphicsContext.GetVideoResolution(), g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan);
      g_application.Render();
      memcpy(&g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
    g_application.RenderNoPresent();
    if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
    {
      if (FAILED(XGWriteSurfaceToFile(lpSurface, fn)))
      {
        CLog::Log(LOGERROR, "Failed to Generate Screenshot");
      }
      else
      {
        CLog::Log(LOGINFO, "Screen shot saved as %s", fn);
      }
      lpSurface->Release();
    }
    g_graphicsContext.Unlock();
    if (flashScreen)
    {
      FlashScreen(true, true);
      Sleep(10);
      FlashScreen(true, false);
    }
#else

#endif

#ifdef HAS_SDL_OPENGL

    g_graphicsContext.BeginPaint();
    if (g_application.IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    g_application.RenderNoPresent();

    GLint viewport[4];
    void *pixels = NULL;
    glReadBuffer(GL_BACK);
    glGetIntegerv(GL_VIEWPORT, viewport);
    pixels = malloc(viewport[2] * viewport[3] * 4);
    if (pixels)
    {
      glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, pixels);
      XGWriteSurfaceToFile(pixels, viewport[2], viewport[3], fn);
      free(pixels);
    }
    g_graphicsContext.EndPaint();

#endif

}

void CUtil::TakeScreenshot()
{
  static bool savingScreenshots = false;
  static vector<CStdString> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("pictures.screenshotpath", false);
  if (strDir.IsEmpty())
  {
    strDir = "special://temp/";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  CUtil::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(CUtil::AddFileToFolder(strDir, "screenshot%03d.bmp"), 999);

    if (!file.IsEmpty())
    {
      TakeScreenshot(file.c_str(), true);
      if (savingScreenshots)
        screenShots.push_back(file);
      if (promptUser)
      { // grab the real directory
        CStdString newDir = g_guiSettings.GetString("pictures.screenshotpath");
        if (!newDir.IsEmpty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            CStdString file = CUtil::GetNextFilename(CUtil::AddFileToFolder(newDir, "screenshot%03d.bmp"), 999);
            CFile::Cache(screenShots[i], file);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}

void CUtil::ClearCache()
{
  for (int i = 0; i < 16; i++)
  {
    CStdString strHex, folder;
    strHex.Format("%x", i);
    CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), strHex, folder);
    g_directoryCache.ClearDirectory(folder);
  }
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (__int64)stat->st_size;

#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::Stat64ToStat(struct stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX) 
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)stat->st_atime;
  result->st_mtime = (time_t)stat->st_mtime;
  result->st_ctime = (time_t)stat->st_ctime;
}

bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.
  vector<string> strArray;
  CURL url(strPath);
  string path = url.GetFileName().c_str();
  int iSize = path.size();
  char cSep = CUtil::GetDirectorySeperator(strPath);
  if (path.at(iSize - 1) == cSep) path.erase(iSize - 1, iSize - 1); // remove slash at end
  CStdString strTemp;

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // split strPath up into an array
  // music\album\ will result in
  // music
  // music\album
  //

  int i = 0;
  CFileItem item(strPath,true);
  if (item.IsHD())
  {
    // remove the root drive from the filename
    if (CUtil::IsDOSPath(item.m_strPath))
      i = 2;
  }
  else if (!item.IsSmb())
  {
    CLog::Log(LOGERROR,"CUtil::CreateDirectoryEx called with an unsupported path: %s",strPath.c_str());
    return false;
  }

  int s = i;
  while (i < iSize)
  {
    i = path.find(cSep, i + 1);
    if (i < 0) i = iSize; // get remaining chars
    strArray.push_back(path.substr(s, i - s));
  }

  // create the directories
  url.GetURLWithoutFilename(strTemp);
  for (unsigned int i = 0; i < strArray.size(); i++)
  {
    CStdString strTemp1;
    CUtil::AddFileToFolder(strTemp,strArray[i],strTemp1);
    CDirectory::Create(strTemp1);
  }
  strArray.clear();

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}

CStdString CUtil::MakeLegalFileName(const CStdString &strFile)
{
  CStdString strPath;
  GetDirectory(strFile,strPath);
  CStdString result = GetFileName(strFile);

  // just filter out some illegal characters on windows
  result.Remove(':');
  result.Remove('*');
  result.Remove('?');
  result.Remove('\"');
  result.Remove('<');
  result.Remove('>');
  result.Remove('|');

  result = strPath+result;

  return result;
}

void CUtil::AddDirectorySeperator(CStdString& strPath)
{
  strPath += GetDirectorySeperator(strPath);
}

char CUtil::GetDirectorySeperator(const CStdString &strFilename)
{
  CURL url(strFilename);
  return url.GetDirectorySeparator();
}

bool CUtil::IsUsingTTFSubtitles()
{
  return CUtil::GetExtension(g_guiSettings.GetString("subtitles.font")).Equals(".ttf");
}

typedef struct
{
  char command[32];
  bool needsParameters;
  char description[128];
} BUILT_IN;

const BUILT_IN commands[] = {
  { "Help",                       false,  "This help message" },
  { "Reboot",                     false,  "Reboot the xbox (power cycle)" },
  { "Restart",                    false,  "Restart the xbox (power cycle)" },
  { "ShutDown",                   false,  "Shutdown the xbox" },
  { "Powerdown",                  false,  "Powerdown system" },
  { "Quit",                       false,  "Quit XBMC" },
  { "Hibernate",                  false,  "Hibernates the system" },
  { "Suspend",                    false,  "Suspends the system" },
  { "RestartApp",                 false,  "Restart XBMC" },
  { "Minimize",                   false,  "Minimize XBMC" },
  { "Credits",                    false,  "Run XBMCs Credits" },
  { "Reset",                      false,  "Reset the xbox (warm reboot)" },
  { "Mastermode",                 false,  "Control master mode" },
  { "ActivateWindow",             true,   "Activate the specified window" },
  { "ReplaceWindow",              true,   "Replaces the current window with the new one" },
  { "TakeScreenshot",             false,  "Takes a Screenshot" },
  { "RunScript",                  true,   "Run the specified script" },
  { "RunPlugin",                  true,   "Run the specified plugin" },
  { "Extract",                    true,   "Extracts the specified archive" },
  { "PlayMedia",                  true,   "Play the specified media file (or playlist)" },
  { "SlideShow",                  true,   "Run a slideshow from the specified directory" },
  { "RecursiveSlideShow",         true,   "Run a slideshow from the specified directory, including all subdirs" },
  { "ReloadSkin",                 false,  "Reload XBMC's skin" },
  { "PlayerControl",              true,   "Control the music or video player" },
  { "Playlist.PlayOffset",        true,   "Start playing from a particular offset in the playlist" },
  { "EjectTray",                  false,  "Close or open the DVD tray" },
  { "AlarmClock",                 true,   "Prompt for a length of time and start an alarm clock" },
  { "CancelAlarm",                true,   "Cancels an alarm" },
  { "Action",                     true,   "Executes an action for the active window (same as in keymap)" },
  { "Notification",               true,   "Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon." },
  { "PlayDVD",                    false,  "Plays the inserted CD or DVD media from the DVD-ROM Drive!" },
  { "Skin.ToggleSetting",         true,   "Toggles a skin setting on or off" },
  { "Skin.SetString",             true,   "Prompts and sets skin string" },
  { "Skin.SetNumeric",            true,   "Prompts and sets numeric input" },
  { "Skin.SetPath",               true,   "Prompts and sets a skin path" },
  { "Skin.Theme",                 true,   "Control skin theme" },
  { "Skin.SetImage",              true,   "Prompts and sets a skin image" },
  { "Skin.SetLargeImage",         true,   "Prompts and sets a large skin images" },
  { "Skin.SetFile",               true,   "Prompts and sets a file" },
  { "Skin.SetBool",               true,   "Sets a skin setting on" },
  { "Skin.Reset",                 true,   "Resets a skin setting to default" },
  { "Skin.ResetSettings",         false,  "Resets all skin settings" },
  { "Mute",                       false,  "Mute the player" },
  { "SetVolume",                  true,   "Set the current volume" },
  { "Dialog.Close",               true,   "Close a dialog" },
  { "System.LogOff",              false,  "Log off current user" },
  { "System.Exec",                true,   "Execute shell commands" },
#ifdef _WIN32PC
  { "System.ExecWait",            true,   "Execute shell commands and freezes XBMC until shell is closed" },
#endif
  { "Resolution",                 true,   "Change XBMC's Resolution" },
  { "SetFocus",                   true,   "Change current focus to a different control id" },
  { "UpdateLibrary",              true,   "Update the selected library (music or video)" },
  { "PageDown",                   true,   "Send a page down event to the pagecontrol with given id" },
  { "PageUp",                     true,   "Send a page up event to the pagecontrol with given id" },
  { "LastFM.Love",                false,  "Add the current playing last.fm radio track to the last.fm loved tracks" },
  { "LastFM.Ban",                 false,  "Ban the current playing last.fm radio track" },
  { "Container.Refresh",          false,  "Refresh current listing" },
  { "Container.NextViewMode",     false,  "Move to the next view type (and refresh the listing)" },
  { "Container.PreviousViewMode", false,  "Move to the previous view type (and refresh the listing)" },
  { "Container.SetViewMode",      true,   "Move to the view with the given id" },
  { "Container.NextSortMethod",   false,  "Change to the next sort method" },
  { "Container.PreviousSortMethod",false, "Change to the previous sort method" },
  { "Container.SetSortMethod",    true,   "Change to the specified sort method" },
  { "Container.SortDirection",    false,  "Toggle the sort direction" },
  { "Control.Move",               true,   "Tells the specified control to 'move' to another entry specified by offset" },
  { "SendClick",                  true,   "Send a click message from the given control to the given window" },
  { "LoadProfile",                true,   "Load the specified profile (note; if locks are active it won't work)" },
  { "SetProperty",                true,   "Sets a window property for the current window (key,value)" },
#ifdef HAS_LIRC
  { "LIRC.Stop",                  false,  "Removes XBMC as LIRC client" },
  { "LIRC.Start",                 false,  "Adds XBMC as LIRC client" },
#endif
#ifdef HAS_LCD
  { "LCD.Suspend",                false,  "Suspends LCDproc" },
  { "LCD.Resume",                 false,  "Resumes LCDproc" },
#endif
};

bool CUtil::IsBuiltIn(const CStdString& execString)
{
  CStdString function, param;
  SplitExecFunction(execString, function, param);
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    if (function.CompareNoCase(commands[i].command) == 0 && (!commands[i].needsParameters || !param.IsEmpty()))
      return true;
  }
  return false;
}

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam)
{
  strParam = "";

  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    strParam = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    strFunction = execString.Left(iPos);
  }
  else
    strFunction = execString;

  //xbmc is the standard prefix.. so allways remove this
  //all other commands with go through in full
  if( strFunction.Left(5).Equals("xbmc.", false) )
    strFunction.Delete(0, 5);
}

void CUtil::GetBuiltInHelp(CStdString &help)
{
  help.Empty();
  for (unsigned int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
    help += commands[i].description;
    help += "\n";
  }
}

int CUtil::ExecBuiltIn(const CStdString& execString)
{
  // Get the text after the "XBMC."
  CStdString execute, parameter;
  SplitExecFunction(execString, execute, parameter);
  CStdString strParameterCaseIntact = parameter;
  parameter.ToLower();
  execute.ToLower();

  if (execute.Equals("reboot") || execute.Equals("restart"))  //Will reboot the xbox, aka cold reboot
  {
    g_application.getApplicationMessenger().Restart();
  }
  else if (execute.Equals("shutdown"))
  {
    g_application.getApplicationMessenger().Shutdown();
  }
  else if (execute.Equals("powerdown"))
  {
    g_application.getApplicationMessenger().Powerdown();
  }
  else if (execute.Equals("restartapp"))
  {
    g_application.getApplicationMessenger().RestartApp();
  }
  else if (execute.Equals("hibernate"))
  {
    g_application.getApplicationMessenger().Hibernate();
  }
  else if (execute.Equals("suspend"))
  {
    g_application.getApplicationMessenger().Suspend();
  }
  else if (execute.Equals("quit"))
  {
    g_application.getApplicationMessenger().Quit();
  }
  else if (execute.Equals("minimize"))
  {
    g_application.getApplicationMessenger().Minimize();
  }
  else if (execute.Equals("loadprofile") && g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
  {
    for (unsigned int i=0;i<g_settings.m_vecProfiles.size();++i )
    {
      if (g_settings.m_vecProfiles[i].getName().Equals(strParameterCaseIntact))
      {
        g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
        g_settings.LoadProfile(i);
        g_application.StartEventServer(); // event server could be needed in some situations
      }
    }
  }
  else if (execute.Equals("mastermode"))
  {
    if (g_passwordManager.bMasterUser)
    {
      g_passwordManager.bMasterUser = false;
      g_passwordManager.LockSources(true);
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
    }
    else if (g_passwordManager.IsMasterLockUnlocked(true))
    {
      g_passwordManager.LockSources(false);
      g_passwordManager.bMasterUser = true;
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
    }

    DeleteVideoDatabaseDirectoryCache();
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_graphicsContext.SendMessage(msg);
  }
  else if (execute.Equals("takescreenshot"))
  {
    TakeScreenshot();
  }
  else if (execute.Equals("credits"))
  {
#ifdef HAS_CREDITS
    RunCredits();
#endif
  }
  else if (execute.Equals("reset")) //Will reset the xbox, aka soft reset
  {
    g_application.getApplicationMessenger().Reset();
  }
  else if (execute.Equals("activatewindow") || execute.Equals("replacewindow"))
  {
    // get the parameters
    CStdString strWindow;
    CStdString strPath;

    // split the parameter on first comma
    int iPos = parameter.Find(",");
    if (iPos == 0)
    {
      // error condition missing path
      // XMBC.ActivateWindow(1,)
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid parameter: %s", parameter.c_str());
      return -7;
    }
    else if (iPos < 0)
    {
      // no path parameter
      // XBMC.ActivateWindow(5001)
      strWindow = strParameterCaseIntact;
    }
    else
    {
      // path parameter included
      // XBMC.ActivateWindow(5001,F:\Music\)
      strWindow = strParameterCaseIntact.Left(iPos);
      strPath = strParameterCaseIntact.Mid(iPos + 1);
    }
    // confirm the window destination is actually a number
    // before switching
    int iWindow = g_buttonTranslator.TranslateWindowString(strWindow.c_str());
    if (iWindow != WINDOW_INVALID)
    {
      // disable the screensaver
      g_application.ResetScreenSaverWindow();
      if (execute.Equals("activatewindow"))
        m_gWindowManager.ActivateWindow(iWindow, strPath);
      else  // ReplaceWindow
        m_gWindowManager.ChangeActiveWindow(iWindow, strPath);
    }
    else
    {
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if (execute.Equals("setfocus"))
  {
    // see whether we have more than one param
    vector<CStdString> params;
    StringUtils::SplitString(parameter,",",params);
    if (params.size())
    {
      int controlID = atol(params[0].c_str());
      int subItem = (params.size() > 1) ? atol(params[1].c_str())+1 : 0;
      CGUIMessage msg(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), controlID, subItem);
      g_graphicsContext.SendMessage(msg);
    }
  }
#ifdef HAS_PYTHON
  else if (execute.Equals("runscript"))
  {
    vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() > 0)  // we need to construct arguments to pass to python
    {
      unsigned int argc = params.size();
      char ** argv = new char*[argc];

      vector<CStdString> path;
      //split the path up to find the filename
      StringUtils::SplitString(params[0],"\\",path);
      argv[0] = path.size() > 0 ? (char*)path[path.size() - 1].c_str() : (char*)params[0].c_str();

      for(unsigned int i = 1; i < argc; i++)
        argv[i] = (char*)params[i].c_str();

      g_pythonParser.evalFile(params[0].c_str(), argc, (const char**)argv);
      delete [] argv;
    }
    else
      g_pythonParser.evalFile(strParameterCaseIntact.c_str());
  }
#endif
#if defined(_LINUX) && !defined(__APPLE__)
  else if (execute.Equals("system.exec"))
  {
    system(strParameterCaseIntact.c_str()); 
  }
#elif defined(_WIN32PC)
  else if (execute.Equals("system.exec"))
  {
    CWIN32Util::XBMCShellExecute(strParameterCaseIntact, false);
  }
  else if (execute.Equals("system.execwait"))
  {
    CWIN32Util::XBMCShellExecute(strParameterCaseIntact, true);
  }
#endif
  else if (execute.Equals("resolution"))
  {
    RESOLUTION res = PAL_4x3;
    if (parameter.Equals("pal")) res = PAL_4x3;
    else if (parameter.Equals("pal16x9")) res = PAL_16x9;
    else if (parameter.Equals("ntsc")) res = NTSC_4x3;
    else if (parameter.Equals("ntsc16x9")) res = NTSC_16x9;
    else if (parameter.Equals("720p")) res = HDTV_720p;
    else if (parameter.Equals("1080i")) res = HDTV_1080i;
    if (g_videoConfig.IsValidResolution(res))
    {
      g_guiSettings.SetInt("videoscreen.resolution", res);
      //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
      g_graphicsContext.SetVideoResolution(res, TRUE);
      //set our lookandfeelres to the resolution set in graphiccontext
      g_guiSettings.m_LookAndFeelResolution = res;
      g_application.ReloadSkin();
    }
  }
  else if (execute.Equals("extract"))
  {
    // Detects if file is zip or zip then extracts
    CStdString strDestDirect = "";
    vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() < 2)
      CUtil::GetDirectory(params[0],strDestDirect);
    else
      strDestDirect = params[1];

    CUtil::AddSlashAtEnd(strDestDirect);

    if (params.size() < 1)
      return -1; // No File Selected

    if (CUtil::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);
    else if (CUtil::IsRAR(params[0]))
      g_RarManager.ExtractArchive(params[0],strDestDirect);
    else
      CLog::Log(LOGERROR, "CUtil::ExecuteBuiltin: No archive given");
  }
  else if (execute.Equals("runplugin"))
  {
    if (!strParameterCaseIntact.IsEmpty())
    {
      CFileItem item(strParameterCaseIntact);
      if (!item.m_bIsFolder)
      {
        item.m_strPath = strParameterCaseIntact;
        CPluginDirectory::RunScriptWithParams(item.m_strPath);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CUtil::ExecBuiltIn, runplugin called with no arguments.");
    }
  }
  else if (execute.Equals("playmedia"))
  {
    if (strParameterCaseIntact.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia called with empty parameter");
      return -3;
    }

    vector<CStdString> params2;
    StringUtils::SplitString(strParameterCaseIntact,",",params2);

    CFileItem item(params2[0], false);
    if (item.IsVideoDb())
    {
      CVideoDatabase database;
      database.Open();
      DIRECTORY::VIDEODATABASEDIRECTORY::CQueryParams params;
      DIRECTORY::CVideoDatabaseDirectory::GetQueryParams(item.m_strPath,params);
      if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
        database.GetMovieInfo("",*item.GetVideoInfoTag(),params.GetMovieId());
      if (params.GetContentType() == VIDEODB_CONTENT_TVSHOWS)
        database.GetEpisodeInfo("",*item.GetVideoInfoTag(),params.GetEpisodeId());
      if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
        database.GetMusicVideoInfo("",*item.GetVideoInfoTag(),params.GetMVideoId());
      item.m_strPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
    }

    // restore to previous window if needed
    if( m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
        m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
        m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION )
        m_gWindowManager.PreviousWindow();

    // reset screensaver
    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();

    // set fullscreen or windowed
    if (params2.size() == 2 && params2[1] == "1")
      g_stSettings.m_bStartVideoWindowed = true;

    // play media
    if (!g_application.PlayMedia(item, item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia could not play media: %s", strParameterCaseIntact.c_str());
      return false;
    }
  }
  else if (execute.Equals("slideShow") || execute.Equals("recursiveslideShow"))
  {
    if (strParameterCaseIntact.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.SlideShow called with empty parameter");
      return -2;
    }
    bool bRecursive = false;
    bool bRandom = false;
    bool bNotRandom = false;
    CStdString strDir = strParameterCaseIntact;

    // leave RecursiveSlideShow command as-is
    if (execute.Equals("RecursiveSlideShow"))
      bRecursive = true;

    // SlideShow([recursive],[[not]random],dir)
    // additional parameters must at the beginning as the dir may contain commas
    else
    {
      vector<CStdString> results;
      StringUtils::SplitString(strParameterCaseIntact, ",", results, 2); // max of two split pieces
      while (results.size() > 0)
      {
        // pop off the first item in the vector
        CStdString strTest = results.front();
        results.erase(results.begin());

        if (strTest.Equals("recursive"))
          bRecursive = true;
        else if (strTest.Equals("random"))
          bRandom = true;
        else if (strTest.Equals("notrandom"))
          bNotRandom = true;
        else
        {
          // not a known parameter, so it must be the directory 
          // add the test string back to the remainder of the result array
          // (this means the directory contained a comma)
          strDir = strTest;
          if (results.size() > 0)
            strDir += "," + results.front();
          break;
        }
        // it was a parameter, so we need to split again
        if (results.size() > 0)
        {
          strTest = results.front();
          StringUtils::SplitString(strTest, ",", results, 2);
        }
      }
    }

    // encode parameters
    unsigned int iParams = 0;
    if (bRecursive)
      iParams += 1;
    if (bRandom)
      iParams += 2;
    if (bNotRandom)
      iParams += 4;

    CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, iParams);
    msg.SetStringParam(strDir);
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute.Equals("reloadskin"))
  {
    //  Reload the skin
    g_application.ReloadSkin();
  }
  else if (execute.Equals("playercontrol"))
  {
    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.PlayerControl called with empty parameter");
      return -3;
    }
    if (parameter.Equals("play"))
    { // play/pause
      // either resume playing, or pause
      if (g_application.IsPlaying())
      {
        if (g_application.GetPlaySpeed() != 1)
          g_application.SetPlaySpeed(1);
        else
          g_application.m_pPlayer->Pause();
      }
    }
    else if (parameter.Equals("stop"))
    {
      g_application.StopPlaying();
    }
    else if (parameter.Equals("rewind") || parameter.Equals("forward"))
    {
      if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
      {
        int iPlaySpeed = g_application.GetPlaySpeed();
        if (parameter.Equals("rewind") && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (parameter.Equals("rewind") && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (parameter.Equals("forward") && iPlaySpeed < 1) //goes up a notch if you're RWing
        {
          iPlaySpeed /= 2;
          if (iPlaySpeed == -1) iPlaySpeed = 1;
        }
        else
          iPlaySpeed *= 2;

        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        g_application.SetPlaySpeed(iPlaySpeed);
      }
    }
    else if (parameter.Equals("next"))
    {
      CAction action;
      action.wID = ACTION_NEXT_ITEM;
      g_application.OnAction(action);
    }
    else if (parameter.Equals("previous"))
    {
      CAction action;
      action.wID = ACTION_PREV_ITEM;
      g_application.OnAction(action);
    }
    else if (parameter.Equals("bigskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, true);
    }
    else if (parameter.Equals("bigskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, true);
    }
    else if (parameter.Equals("smallskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, false);
    }
    else if (parameter.Equals("smallskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, false);
    }
    else if( parameter.Equals("showvideomenu") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer )
      {
        CAction action;
        action.fAmount1 = action.fAmount2 = action.fRepeat = 0.0;
        action.m_dwButtonCode = 0;
        action.wID = ACTION_SHOW_VIDEOMENU;
        g_application.m_pPlayer->OnAction(action);
      }
    }
    else if( parameter.Equals("record") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer && g_application.m_pPlayer->CanRecord())
      {
        if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
          g_application.getApplicationMessenger().HttpApi(g_application.m_pPlayer->IsRecording()?"broadcastlevel; RecordStopping;1":"broadcastlevel; RecordStarting;1");
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
      }
    }
    else if (parameter.Left(9).Equals("partymode"))
    {
      CStdString strXspPath = "";
      //empty param=music, "music"=music, "video"=video, else xsp path
      PartyModeContext context = PARTYMODECONTEXT_MUSIC;
      if (parameter.size() > 9)
      {
        if (parameter.Mid(10).Equals("video)"))
          context = PARTYMODECONTEXT_VIDEO;
        else if (!parameter.Mid(10).Equals("music)"))
        {
          strXspPath = parameter.Mid(10).TrimRight(")");
          context = PARTYMODECONTEXT_UNKNOWN;
        }
      }
      if (g_partyModeManager.IsEnabled())
        g_partyModeManager.Disable();
      else
        g_partyModeManager.Enable(context, strXspPath);
    }
    else if (parameter.Equals("random"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();

      // reverse the current setting
      g_playlistPlayer.SetShuffle(iPlaylist, !(g_playlistPlayer.IsShuffled(iPlaylist)));

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_stSettings.m_bMyMusicPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_stSettings.m_bMyVideoPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
      }

      // send message
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, g_playlistPlayer.IsShuffled(iPlaylist));
      m_gWindowManager.SendThreadMessage(msg);

    }
    else if (parameter.Left(6).Equals("repeat"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);

      if (parameter.Equals("repeatall"))
        state = PLAYLIST::REPEAT_ALL;
      else if (parameter.Equals("repeatone"))
        state = PLAYLIST::REPEAT_ONE;
      else if (parameter.Equals("repeatoff"))
        state = PLAYLIST::REPEAT_NONE;
      else if (state == PLAYLIST::REPEAT_NONE)
        state = PLAYLIST::REPEAT_ALL;
      else if (state == PLAYLIST::REPEAT_ALL)
        state = PLAYLIST::REPEAT_ONE;
      else
        state = PLAYLIST::REPEAT_NONE;

      g_playlistPlayer.SetRepeat(iPlaylist, state);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_stSettings.m_bMyMusicPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_stSettings.m_bMyVideoPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
      }

      // send messages so now playing window can get updated
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
      m_gWindowManager.SendThreadMessage(msg);
    }
  }
  else if (execute.Equals("mute"))
  {
    g_application.Mute();
  }
  else if (execute.Equals("setvolume"))
  {
    g_application.SetVolume(atoi(parameter.c_str()));
  }
  else if (execute.Left(19).Equals("playlist.playoffset"))
  {
    // get current playlist
    int pos = atol(parameter.c_str());
    g_playlistPlayer.PlayNext(pos);
  }
  else if (execute.Equals("ejecttray"))
  {
    if (CIoSupport::GetTrayState() == TRAY_OPEN)
      CIoSupport::CloseTray();
    else
      CIoSupport::EjectTray();
  }
  else if( execute.Equals("alarmclock") )
  {
    float fSecs = -1.f;
    CStdString strCommand;
    CStdString strName;
    if (!parameter.IsEmpty())
    {
      CRegExp reg;
      if (!reg.RegComp("([^,]*),([^\\(,]*)(\\([^\\)]*\\)?)?,?(.*)?$"))
        return -1; // whatever
      if (reg.RegFind(strParameterCaseIntact.c_str()) > -1)
      {
        char* szParam = reg.GetReplaceString("\\2\\3");
        if (szParam)
        {
          strCommand = szParam;
          free(szParam);
        }
        szParam = reg.GetReplaceString("\\4");
        if (szParam)
        {
          if (strlen(szParam))
            fSecs = static_cast<float>(atoi(szParam)*60);
          free(szParam);
        }
        szParam = reg.GetReplaceString("\\1");
        if (szParam)
        {
          strName = szParam;
          free(szParam);
        }
      }
    }

    if (fSecs == -1.f)
    {
      CStdString strTime;
      CStdString strHeading;
      if (strCommand.Equals("xbmc.shutdown") || strCommand.Equals("xbmc.shutdown()"))
        strHeading = g_localizeStrings.Get(20145);
      else
        strHeading = g_localizeStrings.Get(13209);
      if( CGUIDialogNumeric::ShowAndGetNumber(strTime, strHeading) )
        fSecs = static_cast<float>(atoi(strTime.c_str())*60);
      else
        return -4;
    }
    if( g_alarmClock.isRunning() )
      g_alarmClock.stop(strName);

    g_alarmClock.start(strName,fSecs,strCommand);
  }
  else if (execute.Equals("notification"))
  {
    vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() < 2)
      return -1;
    if (params.size() == 4)
      g_application.m_guiDialogKaiToast.QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
    else if (params.size() == 3)
      g_application.m_guiDialogKaiToast.QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
    else
      g_application.m_guiDialogKaiToast.QueueNotification(params[0],params[1]);
  }
  else if (execute.Equals("cancelalarm"))
  {
    g_alarmClock.stop(parameter);
  }
  else if (execute.Equals("playdvd"))
  {
    CAutorun::PlayDisc();
  }
  else if (execute.Equals("skin.togglesetting"))
  {
    int setting = g_settings.TranslateSkinBool(parameter);
    g_settings.SetSkinBool(setting, !g_settings.GetSkinBool(setting));
    g_settings.Save();
  }
  else if (execute.Equals("skin.setbool"))
  {
    int pos = parameter.Find(",");
    if (pos >= 0)
    {
      int string = g_settings.TranslateSkinBool(parameter.Left(pos));
      g_settings.SetSkinBool(string, parameter.Mid(pos+1).Equals("true"));
      g_settings.Save();
      return 0;
    }
    // default is to set it to true
    int setting = g_settings.TranslateSkinBool(parameter);
    g_settings.SetSkinBool(setting, true);
    g_settings.Save();
  }
  else if (execute.Equals("skin.reset"))
  {
    g_settings.ResetSkinSetting(parameter);
    g_settings.Save();
  }
  else if (execute.Equals("skin.resetsettings"))
  {
    g_settings.ResetSkinSettings();
    g_settings.Save();
  }
  else if (execute.Equals("skin.theme"))
  {
    // enumerate themes
    vector<CStdString> vecTheme;
    GetSkinThemes(vecTheme);

    int iTheme = -1;

    CStdString strTmpTheme;
    // find current theme
    if (!g_guiSettings.GetString("lookandfeel.skintheme").Equals("skindefault"))
      for (unsigned int i=0;i<vecTheme.size();++i)
      {
        strTmpTheme = g_guiSettings.GetString("lookandfeel.skintheme");
        RemoveExtension(strTmpTheme);
        if (vecTheme[i].Equals(strTmpTheme))
        {
          iTheme=i;
          break;
        }
      }

    int iParam = atoi(parameter.c_str());
    if (iParam == 0 || iParam == 1)
      iTheme++;
    else if (iParam == -1)
      iTheme--;
    if (iTheme > (int)vecTheme.size()-1)
      iTheme = -1;
    if (iTheme < -1)
      iTheme = vecTheme.size()-1;

    CStdString strSkinTheme;
    if (iTheme==-1)
      g_guiSettings.SetString("lookandfeel.skintheme","skindefault");
    else
    {
      strSkinTheme.Format("%s.xpr",vecTheme[iTheme]);
      g_guiSettings.SetString("lookandfeel.skintheme",strSkinTheme);
    }
    // also set the default color theme
    CStdString colorTheme(strSkinTheme);
    ReplaceExtension(colorTheme, ".xml", colorTheme);

    g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);

    g_application.DelayLoadSkin();
  }
  else if (execute.Equals("skin.setstring") || execute.Equals("skin.setimage") || execute.Equals("skin.setfile") ||
           execute.Equals("skin.setpath") || execute.Equals("skin.setnumeric") || execute.Equals("skin.setlargeimage"))
  {
    // break the parameter up if necessary
    // only search for the first "," and use that to break the string up
    int pos = strParameterCaseIntact.Find(",");
    int string;
    if (pos >= 0)
    {
      string = g_settings.TranslateSkinString(strParameterCaseIntact.Left(pos));
      if (execute.Equals("skin.setstring"))
      {
        g_settings.SetSkinString(string, strParameterCaseIntact.Mid(pos+1));
        g_settings.Save();
        return 0;
      }
    }
    else
      string = g_settings.TranslateSkinString(strParameterCaseIntact);
    CStdString value = g_settings.GetSkinString(string);
    VECSOURCES localShares;
    g_mediaManager.GetLocalDrives(localShares);
    if (execute.Equals("skin.setstring"))
    {
      if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(1029), true))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setnumeric"))
    {
      if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(611)))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setimage"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, g_localizeStrings.Get(1030), value))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setlargeimage"))
    {
      VECSOURCES *shares = g_settings.GetSourcesFromType("pictures");
      if (!shares) shares = &localShares;
      if (CGUIDialogFileBrowser::ShowAndGetImage(*shares, g_localizeStrings.Get(1030), value))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setfile"))
    {
      CStdString strMask;
      if (pos > -1)
        strMask = strParameterCaseIntact.Mid(pos+1);

      int iEnd=strMask.Find(",");
      if (iEnd > -1)
      {
        value = strMask.Mid(iEnd+1);
        CUtil::AddSlashAtEnd(value);
        bool bIsSource;
        if (GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
        {
          CMediaSource share;
          share.strName = g_localizeStrings.Get(13278);
          share.strPath = value;
          localShares.push_back(share);
        }
        CStdString strTemp = strMask;
        strMask = strTemp.Left(iEnd);
      }
      else
        iEnd = strMask.size();
      if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, g_localizeStrings.Get(1033), value))
        g_settings.SetSkinString(string, value);
    }
    else // execute.Equals("skin.setpath"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, g_localizeStrings.Get(1031), value))
        g_settings.SetSkinString(string, value);
    }
    g_settings.Save();
  }
  else if (execute.Equals("dialog.close"))
  {
    CStdStringArray arSplit;
    StringUtils::SplitString(parameter,",", arSplit);
    bool bForce = false;
    if (arSplit.size() > 1)
      if (arSplit[1].Equals("true"))
        bForce = true;
    if (arSplit[0].Equals("all"))
    {
      m_gWindowManager.CloseDialogs(bForce);
    }
    else
    {
      DWORD id = g_buttonTranslator.TranslateWindowString(arSplit[0]);
      CGUIWindow *window = (CGUIWindow *)m_gWindowManager.GetWindow(id);
      if (window && window->IsDialog())
        ((CGUIDialog *)window)->Close(bForce);
    }
  }
  else if (execute.Equals("system.logoff"))
  {
    if (m_gWindowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN || !g_settings.bUseLoginScreen)
      return -1;

    g_settings.m_iLastUsedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
    g_application.StopPlaying();
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && musicScan->IsScanning())
      musicScan->StopScanning();

    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_settings.LoadProfile(0); // login screen always runs as default user
    g_passwordManager.m_mapSMBPasswordCache.clear();
    g_passwordManager.bMasterUser = false;
    m_gWindowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
    g_application.StartEventServer(); // event server could be needed in some situations      
  }
  else if (execute.Equals("pagedown"))
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_DOWN, m_gWindowManager.GetFocusedWindow(), id);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("pageup"))
  {
    int id = atoi(parameter.c_str());
    CGUIMessage message(GUI_MSG_PAGE_UP, m_gWindowManager.GetFocusedWindow(), id);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("updatelibrary"))
  {
    if (parameter.Equals("music"))
    {
      CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (scanner)
      {
        if (scanner->IsScanning())
          scanner->StopScanning();
        else
          scanner->StartScanning("");
      }
    }
    if (parameter.Equals("video"))
    {
      CGUIDialogVideoScan *scanner = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      SScraperInfo info;
      VIDEO::SScanSettings settings;
      if (scanner)
      {
        if (scanner->IsScanning())
          scanner->StopScanning();
        else
          CGUIWindowVideoBase::OnScan("",info,settings);
      }
    }
  }
  else if (execute.Equals("lastfm.love"))
  {
    CLastFmManager::GetInstance()->Love(parameter.Equals("false") ? false : true);
  }
  else if (execute.Equals("lastfm.ban"))
  {
    CLastFmManager::GetInstance()->Ban(parameter.Equals("false") ? false : true);
  }
  else if (execute.Equals("control.move"))
  {
    CStdStringArray arSplit;
    StringUtils::SplitString(parameter,",", arSplit);
    if (arSplit.size() < 2)
      return -1;
    CGUIMessage message(GUI_MSG_MOVE_OFFSET, m_gWindowManager.GetFocusedWindow(), atoi(arSplit[0].c_str()), atoi(arSplit[1].c_str()));
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.refresh"))
  { // NOTE: These messages require a media window, thus they're sent to the current activewindow.
    //       This shouldn't stop a dialog intercepting it though.
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, m_gWindowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE);
    message.SetStringParam(strParameterCaseIntact);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.nextviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, m_gWindowManager.GetActiveWindow(), 0, 0, 1);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.previousviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, m_gWindowManager.GetActiveWindow(), 0, 0, -1);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.setviewmode"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_VIEW_MODE, m_gWindowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.nextsortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, m_gWindowManager.GetActiveWindow(), 0, 0, 1);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.previoussortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, m_gWindowManager.GetActiveWindow(), 0, 0, -1);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.setsortmethod"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, m_gWindowManager.GetActiveWindow(), 0, atoi(parameter.c_str()));
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("container.sortdirection"))
  {
    CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, m_gWindowManager.GetActiveWindow(), 0, 0);
    g_graphicsContext.SendMessage(message);
  }
  else if (execute.Equals("sendclick"))
  {
    CStdStringArray params;
    StringUtils::SplitString(parameter, ",", params);
    if (params.size() == 2)
    {
      // have a window - convert it
      int windowID = g_buttonTranslator.TranslateWindowString(params[0].c_str());
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[1].c_str()), windowID);
      g_graphicsContext.SendMessage(message);
    }
    else if (params.size() == 1)
    { // single param - assume you meant the active window
      CGUIMessage message(GUI_MSG_CLICKED, atoi(params[0].c_str()), m_gWindowManager.GetActiveWindow());
      g_graphicsContext.SendMessage(message);
    }
  }
  else if (execute.Equals("action"))
  {
    CStdStringArray params;
    StringUtils::SplitString(parameter, ",", params);
    if (params.size())
    {
      // try translating the action from our ButtonTranslator
      WORD actionID;
      if (g_buttonTranslator.TranslateActionString(params[0].c_str(), actionID))
      {
        CAction action;
        action.wID = actionID;
        action.fAmount1 = 1.0f;
        if (params.size() == 2)
        { // have a window - convert it and send to it.
          int windowID = g_buttonTranslator.TranslateWindowString(params[1].c_str());
          CGUIWindow *window = m_gWindowManager.GetWindow(windowID);
          if (window)
            window->OnAction(action);
        }
        else // send to our app
          g_application.OnAction(action);
      }
    }
  }
  else if (execute.Equals("setproperty"))
  {
    CStdStringArray params;
    StringUtils::SplitString(parameter, ",", params);
    if (params.size() == 2)
    {
      CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (window)
        window->SetProperty(params[0],params[1]);
    }
  }
#ifdef HAS_LIRC
  else if (execute.Equals("lirc.stop"))
  {
    g_RemoteControl.Disconnect(); 
  }
  else if (execute.Equals("lirc.start"))
  {
    g_RemoteControl.Initialize(); 
  }
#endif
#ifdef HAS_LCD
  else if (execute.Equals("lcd.suspend"))
  {
    g_lcd->Suspend(); 
  }
  else if (execute.Equals("lcd.resume"))
  {
    g_lcd->Resume(); 
  }
#endif
  else
    return -1;
  return 0;
}
int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.IsEmpty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();
  if (checkURL.GetProtocol() == "lastfm")
    return 1;
  if (checkURL.GetProtocol() == "tuxbox")
    return 1;
  if (checkURL.GetProtocol() == "plugin")
    return 1;
  if (checkURL.GetProtocol() == "multipath")
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing for matching name [%s]", strPath.c_str());
  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (IsOnDVD(share.strPath))
    {
      if (IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing name [%s]", strName.c_str());
    if (strPath.Equals(strName))
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  CStdString strDest;
  urlDest.GetURLWithoutUserDetails(strDest);
  ForceForwardSlashes(strDest);
  if (!HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }

    // doesnt match a name, so try the source path
    vector<CStdString> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      CStdString strShare;
      urlShare.GetURLWithoutUserDetails(strShare);
      ForceForwardSlashes(strShare);
      if (!HasSlashAtEnd(strShare))
        strShare += "/";
      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching source at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGWARNING,"CUtil::GetMatchingSource... no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialSource(const CStdString &strSpecial)
{
  CStdString strReturn=strSpecial;
  if (!strSpecial.IsEmpty() && strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      CUtil::AddFileToFolder("special://home/", strSpecial.Mid(5), strReturn);
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      CUtil::AddFileToFolder("special://subtitles/", strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      CUtil::AddFileToFolder("special://userdata/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      CUtil::AddFileToFolder("special://database/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      CUtil::AddFileToFolder("special://thumbnails/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      CUtil::AddFileToFolder("special://recordings/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      CUtil::AddFileToFolder("special://screenshots/", strSpecial.Mid(12), strReturn);
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
      CUtil::AddFileToFolder("special://musicplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
      CUtil::AddFileToFolder("special://videoplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      CUtil::AddFileToFolder("special://cdrips/", strSpecial.Mid(7), strReturn);
    // this one will be removed post 2.0
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath",false), strSpecial.Mid(10), strReturn);
  }
  return strReturn;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strReturn);
  vec.push_back(strReturn);
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return DIRECTORY::CMultiPathDirectory::ConstructMultiPath(vec);;
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "video", strReturn);
  vec.push_back(strReturn);
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return DIRECTORY::CMultiPathDirectory::ConstructMultiPath(vec);;
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb");
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb");
}

void CUtil::DeleteDirectoryCache(const CStdString strType /* = ""*/)
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CStdString strFile = "special://temp/";
  if (!strType.IsEmpty())
  {
    strFile += strType;
    if (!strFile.Right(1).Equals("-"))
      strFile += "-";
  }
  strFile += "*.fi";
  CAutoPtrFind hFind(FindFirstFile(_P(strFile).c_str(), &wfd));
  if (!hFind.isValid())
    return;
  do
  {
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      CFile::Delete(CUtil::AddFileToFolder("special://temp/", wfd.cFileName));
  }
  while (FindNextFile(hFind, &wfd));
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  TIME_ZONE_INFORMATION tziNew;
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  int iRescBiases, iHourUTC;
  int iMinuteNew;

  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations
  float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time

  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
  CLog::Log(LOGDEBUG, "-          Bias: %lu minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %lu",tziNew.DaylightBias);
  CLog::Log(LOGDEBUG, "-  StandardBias: %lu",tziNew.StandardBias);

  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default:
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");

  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }

  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear     = (WORD)iYear;
  NewTime.wMonth    = (WORD)iMonth;
  NewTime.wDay      = (WORD)iDay;
  NewTime.wHour     = (WORD)iHourUTC;
  NewTime.wMinute   = (WORD)iMinute;

  FILETIME stNewTime, stCurTime;
  SystemTimeToFileTime(&NewTime, &stNewTime);
  SystemTimeToFileTime(&CurTime, &stCurTime);
  return false;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;

  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  {
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);

    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}

bool CUtil::AutoDetection()
{
  bool bReturn=false;
  if (g_guiSettings.GetBool("autodetect.onoff"))
  {
    static DWORD pingTimer = 0;
    if( timeGetTime() - pingTimer < (DWORD)g_advancedSettings.m_autoDetectPingTime * 1000)
      return false;
    pingTimer = timeGetTime();

    // send ping and request new client info
    if ( CUtil::AutoDetectionPing(
      g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("servers.ftpserveruser"):"anonymous",
      g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("servers.ftpserverpassword"):"anonymous",
      g_guiSettings.GetString("autodetect.nickname"),21 /*Our FTP Port! TODO: Extract FTP from FTP Server settings!*/) )
    {
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit;
      // do we have clients in our list ?
      for(unsigned int i=0; i < v_xboxclients.client_ip.size(); i++)
      {
        // extract client informations
        StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
        if ((int)arSplit.size() > 1 && !v_xboxclients.client_informed[i])
        {
          //extract client info and build the ftp link!
          strNickName     = arSplit[0].c_str();
          strFtpUserName  = arSplit[1].c_str();
          strFtpPassword  = arSplit[2].c_str();
          strFtpPort      = arSplit[3].c_str();
          strBoosMode     = arSplit[4].c_str();
          strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i],strFtpPort.c_str());

          //Do Notification for this Client
          CStdString strtemplbl;
          strtemplbl.Format("%s %s",strNickName, v_xboxclients.client_ip[i]);
          g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(1251), strtemplbl);

          //Debug Log
          CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", g_localizeStrings.Get(1251).c_str(), strNickName.c_str(), strFTPPath.c_str());

          //set the client_informed to TRUE, to prevent loop Notification
          v_xboxclients.client_informed[i]=true;

          //YES NO PopUP: ask for connecting to the detected client via Filemanger!
          if (g_guiSettings.GetBool("autodetect.popupinfo") && CGUIDialogYesNo::ShowAndGetInput(1251, 0, 1257, 0))
          {
            m_gWindowManager.ActivateWindow(WINDOW_FILES, strFTPPath); //Open in MyFiles
          }
          bReturn = true;
        }
      }
    }
  }
  return bReturn;
}

bool CUtil::AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort)
{
  bool bFoundNewClient= false;
  CStdString strLocalIP;
  CStdString strSendMessage = "ping\0";
  CStdString strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char sztmp[512];

  static int udp_server_socket, inited=0;
#ifndef _LINUX
  int cliLen;
#else
  socklen_t cliLen;
#endif
  int t1,t2,t3,t4, life=0;

  struct sockaddr_in server;
  struct sockaddr_in cliAddr;
  struct timeval timeout={0,500};
  fd_set readfds;
  char hostname[255];
#ifndef _LINUX
    WORD wVer;
    WSADATA wData;
    PHOSTENT hostinfo;
    wVer = MAKEWORD( 2, 0 );
    if (WSAStartup(wVer,&wData) == 0)
    {
#else
    struct hostent * hostinfo;
#endif
      if(gethostname(hostname,sizeof(hostname)) == 0)
      {
        if((hostinfo = gethostbyname(hostname)) != NULL)
        {
          strLocalIP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
          strNickName.Format("%s",hostname);
        }
      }
#ifndef _LINUX
      WSACleanup();
    }
#endif
  // get IP address
  sscanf( (char *)strLocalIP.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
  if( !t1 ) return false;
  cliLen = sizeof( cliAddr);
  // setup UDP socket
  if( !inited )
  {
    int tUDPsocket  = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char value      = 1;
    setsockopt( tUDPsocket, SOL_SOCKET, SO_BROADCAST, &value, value );
    struct sockaddr_in addr;
    memset(&(addr),0,sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = INADDR_ANY;
    addr.sin_port         = htons(iUDPPort);
    bind(tUDPsocket,(struct sockaddr *)(&addr),sizeof(addr));
    udp_server_socket = tUDPsocket;
    inited = 1;
  }
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );
  if (life == SOCKET_ERROR )
    return false;
  memset(&(server),0,sizeof(server));
  server.sin_family = AF_INET;
#ifndef _LINUX
  server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
  server.sin_addr.s_addr = INADDR_BROADCAST;
#endif
  server.sin_port = htons(iUDPPort);
  sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );

  unsigned int iLookUpCountMax = 2;
  unsigned int i=0;
  bool bUpdateShares=false;

  // Ping able clients? 0:false
  if (life == 0 )
  {
    if(v_xboxclients.client_ip.size() > 0)
    {
      // clients in list without life signal!
      // calculate iLookUpCountMax value counter dependence on clients size!
      if(v_xboxclients.client_ip.size() > iLookUpCountMax)
        iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

      for (i=0; i<v_xboxclients.client_ip.size(); i++)
      {
        bUpdateShares=false;
        //only 1 client, clear our list
        if(v_xboxclients.client_lookup_count[i] >= iLookUpCountMax && v_xboxclients.client_ip.size() == 1 )
        {
          v_xboxclients.client_ip.clear();
          v_xboxclients.client_info.clear();
          v_xboxclients.client_lookup_count.clear();
          v_xboxclients.client_informed.clear();

          // debug log, clients removed from our list
          CLog::Log(LOGDEBUG,"Autodetection: all Clients Removed! (mode LIFE 0)");
          bUpdateShares = true;
        }
        else
        {
          // check client lookup counter! Not reached the CountMax, Add +1!
          if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
            v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
          else
          {
            // client lookup counter REACHED CountMax, remove this client
            v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
            v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
            v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
            v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

            // debug log, clients removed from our list
            CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 0)",i );
            bUpdateShares = true;
          }
        }
        if(bUpdateShares)
        {
          // a client is removed from our list, update our shares
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
          m_gWindowManager.SendThreadMessage(msg);
        }
      }
    }
  }
  // life !=0 we are online and ready to receive and send
  while( life )
  {
    bFoundNewClient = false;
    bUpdateShares = false;
    // Receive ping request or Info
    int iSockRet = recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen);
    if (iSockRet != SOCKET_ERROR)
    {
      CStdString strTmp;
      // do we received a new Client info or just a "ping" request
      if(strReceiveMessage.Equals(sztmp))
      {
        // we received a "ping" request, sending our informations
        strTmp.Format("%s;%s;%s;%d;%d\r\n\0",
          strNickName.c_str(),  // Our Nick-, Device Name!
          strFTPUserName.c_str(), // User Name for our FTP Server
          strFTPPass.c_str(), // Password for our FTP Server
          iFTPPort, // FTP PORT Adress for our FTP Server
          0 ); // BOOSMODE, for our FTP Server!
        sendto(udp_server_socket,(char *)strTmp.c_str(),strlen((char *)strTmp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      }
      else
      {
        //We received new client information, extracting information
        CStdString strInfo, strIP;
        strInfo.Format("%s",sztmp); //this is the client info
        strIP.Format("%d.%d.%d.%d",
#ifndef _LINUX
          cliAddr.sin_addr.S_un.S_un_b.s_b1,
          cliAddr.sin_addr.S_un.S_un_b.s_b2,
          cliAddr.sin_addr.S_un.S_un_b.s_b3,
          cliAddr.sin_addr.S_un.S_un_b.s_b4
#else
          (int)((char *)(cliAddr.sin_addr.s_addr))[0],
          (int)((char *)(cliAddr.sin_addr.s_addr))[1],
          (int)((char *)(cliAddr.sin_addr.s_addr))[2],
          (int)((char *)(cliAddr.sin_addr.s_addr))[3]
#endif
        ); //this is the client IP

        //Is this our Local IP ?
        if ( !strIP.Equals(strLocalIP) )
        {
          //is our list empty?
          if(v_xboxclients.client_ip.size() <= 0 )
          {
            // the list is empty, add. this client to the list!
            v_xboxclients.client_ip.push_back(strIP);
            v_xboxclients.client_info.push_back(strInfo);
            v_xboxclients.client_lookup_count.push_back(0);
            v_xboxclients.client_informed.push_back(false);
            bFoundNewClient = true;
            bUpdateShares = true;
          }
          // our list is not empty, check if we allready have this client in our list!
          else
          {
            // this should be a new client or?
            // check list
            bFoundNewClient = true;
            for (i=0; i<v_xboxclients.client_ip.size(); i++)
            {
              if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                bFoundNewClient=false;
            }
            if(bFoundNewClient)
            {
              // bFoundNewClient is still true, the client is not in our list!
              // add. this client to our list!
              v_xboxclients.client_ip.push_back(strIP);
              v_xboxclients.client_info.push_back(strInfo);
              v_xboxclients.client_lookup_count.push_back(0);
              v_xboxclients.client_informed.push_back(false);
              bUpdateShares = true;
            }
            else // this is a existing client! check for LIFE & lookup counter
            {
              // calculate iLookUpCountMax value counter dependence on clients size!
              if(v_xboxclients.client_ip.size() > iLookUpCountMax)
                iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                {
                  // found client in list, reset looup_Count and the client_info
                  v_xboxclients.client_info[i]=strInfo;
                  v_xboxclients.client_lookup_count[i] = 0;
                }
                else
                {
                  // check client lookup counter! Not reached the CountMax, Add +1!
                  if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
                    v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
                  else
                  {
                    // client lookup counter REACHED CountMax, remove this client
                    v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
                    v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
                    v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
                    v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

                    // debug log, clients removed from our list
                    CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 1)",i );

                    // client is removed from our list, update our shares
                    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
                    m_gWindowManager.SendThreadMessage(msg);
                  }
                }
              }
              // here comes our list for debug log
              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] (mode LIFE=1)",i );
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
                CLog::Log(LOGDEBUG,"IP:%s Info:%s LookUpCount:%i Informed:%s",
                  v_xboxclients.client_ip[i].c_str(),
                  v_xboxclients.client_info[i].c_str(),
                  v_xboxclients.client_lookup_count[i],
                  v_xboxclients.client_informed[i] ? "true":"false");
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
              }
            }
          }
          if(bUpdateShares)
          {
            // a client is add or removed from our list, update our shares
            CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
            m_gWindowManager.SendThreadMessage(msg);
          }
        }
      }
    }
    else
    {
       CLog::Log(LOGDEBUG, "Autodetection: Socket error %u", WSAGetLastError());
    }
    timeout.tv_sec=0;
    timeout.tv_usec = 5000;
    FD_ZERO(&readfds);
    FD_SET(udp_server_socket, &readfds);
    life = select( 0,&readfds, NULL, NULL, &timeout );
  }
  return bFoundNewClient;
}

void CUtil::AutoDetectionGetSource(VECSOURCES &shares)
{
  if(v_xboxclients.client_ip.size() > 0)
  {
    // client list is not empty, add to shares
    CMediaSource share;
    for (unsigned int i=0; i< v_xboxclients.client_ip.size(); i++)
    {
      //extract client info string: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit;
      StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i].c_str(),strFtpPort.c_str());

        strNickName.TrimRight(' ');
        share.strName.Format("FTP XBMC_PC (%s)", strNickName.c_str());
        share.strPath.Format("%s",strFTPPath.c_str());
        shares.push_back(share);
      }
    }
  }
}
bool CUtil::IsFTP(const CStdString& strFile)
{
  if( strFile.Left(6).Equals("ftp://") ) return true;
  else if( strFile.Left(7).Equals("ftpx://") ) return true;
  else if( strFile.Left(7).Equals("ftps://") ) return true;
  else return false;
}

bool CUtil::GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax )
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  class CXFUser* m_pUser;
  vector<CXFUser*> users;
  g_application.m_pFileZilla->GetAllUsers(users);
  iUserMax = users.size();
  if (iUserMax > 0)
  {
    //for (int i = 1 ; i < iUserSize; i++){ delete users[i]; }
    m_pUser = users[iFTPUserID];
    strFtpUser1 = m_pUser->GetName();
    if (strFtpUser1.size() != 0) return true;
    else return false;
  }
  else
#endif
    return false;
}
bool CUtil::SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword)
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  CStdString strTempUserName;
  class CXFUser* p_ftpUser;
  vector<CXFUser*> v_ftpusers;
  bool bFoundUser = false;
  g_application.m_pFileZilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
  if (iUserSize > 0)
  {
    int i = 1 ;
    while( i <= iUserSize)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName.Equals(strFtpUserName.c_str()) )
      {
        if (p_ftpUser->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          g_guiSettings.SetString("servers.ftpserverpassword",strFtpUserPassword.c_str());
          return true;
        }
        break;
      }
      i++;
    }
  }
#endif
  return false;
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,bUseFileDirectories);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,items,strMask,bUseFileDirectories);
    else if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",false);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->m_strPath.Equals(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->m_strPath,item);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

CStdString CUtil::SubstitutePath(const CStdString& strFileName)
{
  //CLog::Log(LOGDEBUG,"%s checking source filename:[%s]", __FUNCTION__, strFileName.c_str());
  // substitutes paths to correct issues with remote playlists containing full paths
  for (unsigned int i = 0; i < g_advancedSettings.m_pathSubstitutions.size(); i++)
  {
    vector<CStdString> vecSplit;
    StringUtils::SplitString(g_advancedSettings.m_pathSubstitutions[i], " , ", vecSplit);

    // something is wrong, go to next substitution
    if (vecSplit.size() != 2)
      continue;

    CStdString strSearch = vecSplit[0];
    CStdString strReplace = vecSplit[1];
    strSearch.Replace(",,",",");
    strReplace.Replace(",,",",");

    if (!CUtil::HasSlashAtEnd(strSearch))
      CUtil::AddSlashAtEnd(strSearch);
    if (!CUtil::HasSlashAtEnd(strReplace))
      CUtil::AddSlashAtEnd(strReplace);

    // if left most characters match the search, replace them
    //CLog::Log(LOGDEBUG,"%s testing for path:[%s]", __FUNCTION__, strSearch.c_str());
    int iLen = strSearch.size();
    if (strFileName.Left(iLen).Equals(strSearch))
    {
      // fix slashes
      CStdString strTemp = strFileName.Mid(iLen);
      strTemp.Replace("\\", strReplace.Right(1));
      CStdString strFileNameNew = strReplace + strTemp;
      //CLog::Log(LOGDEBUG,"%s new filename:[%s]", __FUNCTION__, strFileNameNew.c_str());
      return strFileNameNew;
    }
  }
  // nothing matches, return original string
  //CLog::Log(LOGDEBUG,"%s no matches", __FUNCTION__);
  return strFileName;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size();
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize))
    return false;

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos != CStdString::npos )
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos != CStdString::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos != CStdString::npos )
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    if ( nPos == CStdString::npos ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsFileOperations(const CStdString& strPath)
{
  // currently only hd and smb support delete and rename
  if (IsHD(strPath))
    return true;
  if (IsSmb(strPath))
    return true;
  if (IsMythTV(strPath))
  {
    CURL url(strPath);
    return url.GetFileName().Left(11).Equals("recordings/") && url.GetFileName().length() > 11;
  }
  if (IsStack(strPath))
  {
    CStackDirectory dir;
    return SupportsFileOperations(dir.GetFirstStackedFile(strPath));
  }
  if (IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsFileOperations(strPath);

  return false;
}

CStdString CUtil::GetCachedAlbumThumb(const CStdString& album, const CStdString& artist)
{
  if (album.IsEmpty())
    return GetCachedMusicThumb("unknown"+artist);
  if (artist.IsEmpty())
    return GetCachedMusicThumb(album+"unknown");
  return GetCachedMusicThumb(album+artist);
}

CStdString CUtil::GetCachedMusicThumb(const CStdString& path)
{
  Crc32 crc;
  CStdString noSlashPath(path);
  RemoveSlashAtEnd(noSlashPath);
  crc.ComputeFromLowerCase(noSlashPath);
  CStdString hex;
  hex.Format("%08x", (unsigned __int32) crc);
  CStdString thumb;
  thumb.Format("%c/%s.tbn", hex[0], hex.c_str());
  return CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), thumb);
}

void CUtil::GetSkinThemes(vector<CStdString>& vecTheme)
{
  CStdString strPath;
  CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(),"media",strPath);
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr"))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::WipeDir(const CStdString& strPath) // DANGEROUS!!!!
{
  CFileItemList items;
  CUtil::GetRecursiveListing(strPath,items,"");
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->m_strPath);
  }
  items.Clear();
  CUtil::GetRecursiveDirsListing(strPath,items);
  for (int i=items.Size()-1;i>-1;--i) // need to wipe them backwards
  {
    CUtil::AddSlashAtEnd(items[i]->m_strPath);
    CDirectory::Remove(items[i]->m_strPath);
  }

  CStdString tmpPath = strPath;
  AddSlashAtEnd(tmpPath);
  CDirectory::Remove(tmpPath);
}

void CUtil::ClearFileItemCache()
{
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/", items, ".fi", false);
  for (int i = 0; i < items.Size(); ++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->m_strPath);
  }
}

#ifdef _LINUX

//
// FIXME, this should be merged with the function below.
//
bool CUtil::Command(const CStdStringArray& arrArgs)
{
#ifdef _DEBUG
  printf("Executing: ");
  for (size_t i=0; i<arrArgs.size(); i++)
    printf("%s ", arrArgs[i].c_str());
  printf("\n");
#endif

  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0);
    close(1);
    close(2);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      for (size_t i=0; i<arrArgs.size(); i++)
        args[i] = (char *)arrArgs[i].c_str();
      execvp(args[0], args);
    }
  }
  else
  {
    waitpid(child, &n, 0);
  }

  return WEXITSTATUS(n) == 0;
}

bool CUtil::SudoCommand(const CStdString &strCommand)
{
  CLog::Log(LOGDEBUG, "Executing sudo command: <%s>", strCommand.c_str());
  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0); // close stdin to avoid sudo request password
    close(1);
    close(2);
    CStdStringArray arrArgs;
    StringUtils::SplitString(strCommand, " ", arrArgs);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      args[0] = (char *)"/usr/bin/sudo";
      args[1] = (char *)"-S";
      for (size_t i=0; i<arrArgs.size(); i++)
      {
        args[i+2] = (char *)arrArgs[i].c_str();
      }
      execvp("/usr/bin/sudo", args);
    }
  }
  else
    waitpid(child, &n, 0);

  return WEXITSTATUS(n) == 0;
}
#endif
