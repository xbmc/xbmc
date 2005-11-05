#include "stdafx.h"
#include "application.h"
#include "util.h"
#include "xbox/iosupport.h"
#include "xbox/xbeheader.h"
#include "xbox/undocumented.h"
#include "xbresource.h"
#include "DetectDVDType.h"
#include "ThumbnailCache.h"
#include "filesystem/hddirectory.h"
#include "filesystem/DirectoryCache.h"
#include "Credits.h"
#include "shortcut.h"
#include "playlistplayer.h"
#include "lib/libPython/XBPython.h"
#include "utils/RegExp.h"
#include "utils/AlarmClock.h" 
#include "cores/VideoRenderers/RenderManager.h"
#include "ButtonTranslator.h"
#include "Picture.h"
#include "GUIDialogNumeric.h"
#include "autorun.h"
#include "utils/fstrcmp.h"

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS = 10000000;
struct SSortByLabel
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CGUIListItem& rpStart = *pStart;
    CGUIListItem& rpEnd = *pEnd;

    CStdString strLabel1 = rpStart.GetLabel();
    strLabel1.ToLower();

    CStdString strLabel2 = rpEnd.GetLabel();
    strLabel2.ToLower();

    if (m_bSortAscending)
      return (strcmp(strLabel1.c_str(), strLabel2.c_str()) < 0);
    else
      return (strcmp(strLabel1.c_str(), strLabel2.c_str()) >= 0);
  }

  static bool m_bSortAscending;
};
bool CUtil::m_bNetworkUp = false;
bool SSortByLabel::m_bSortAscending;
char g_szTitleIP[32];
CStdString strHasClientIP="",strHasClientInfo="",strNewClientIP,strNewClientInfo; 
using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
static D3DGAMMARAMP oldramp, flashramp;

extern "C"
{
	extern bool WINAPI NtSetSystemTime(LPFILETIME SystemTime , LPFILETIME PreviousTime );
};
CUtil::CUtil(void)
{
  memset(g_szTitleIP, 0, sizeof(g_szTitleIP));
}

CUtil::~CUtil(void)
{}

char* CUtil::GetExtension(const CStdString& strFileName)
{
  CURL url(strFileName);
  const char* extension;
  extension = strFileName.c_str()+strFileName.rfind(".");
  return (char*)extension ;
}

char* CUtil::GetFileName(const CStdString& strFileNameAndPath)
{
  CURL url(strFileNameAndPath);
  const char* extension;
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip"))
    extension = strFileNameAndPath.c_str()+strFileNameAndPath.rfind("\\");
  else
    extension = strrchr(strFileNameAndPath.c_str(), '\\');
  if (!extension)
  {
    extension = strrchr(strFileNameAndPath.c_str(), '/');
    if (!extension) return (char*)strFileNameAndPath.c_str();
  }
  extension++;
  return (char*)extension;
}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath)
{
  // use above to get the filename
  CStdString strFilename = GetFileName(strFileNameAndPath);
  // now remove the extension if needed
  if (IsSmb(strFileNameAndPath))
  {
    CStdString strTempFilename;
    g_charsetConverter.utf8ToStringCharset(strFilename, strTempFilename);
    strFilename = strTempFilename;
  }
  if (g_guiSettings.GetBool("FileLists.HideExtensions"))
  {
    RemoveExtension(strFilename);
    return strFilename;
  }
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_settings.m_MyVideoStackRegExps;

  CStdString strFileNameTemp = strFileName;
  CStdString strFileNameLower = strFileName;
  strFileNameLower.MakeLower();

  CStdString strVolume;
  CStdString strTestString;
  CRegExp reg;

  //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 1 : " + strFileName);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp: %s.  Check XBoxMediaCenter.xml", regexps[i].c_str());
      continue;
    }
    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    { // found this token
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();
      //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 2 : " + strFileName + " : " + strRegExp + " : iRegLength=%i : iCount=%i", iRegLength, iCount);
      if( 1 == iCount )
      {
        char *pReplace = reg.GetReplaceString("\\1");

        if (pReplace)
        {
          strVolumeNumber = pReplace;
          free(pReplace);

          // remove the extension (if any).  We do this on the base filename, as the regexp
          // match may include some of the extension (eg the "." in particular).

          // the extension will then be added back on at the end - there is no reason 
          // to clean it off here. It will be cleaned off during the display routine, if 
          // the settings to hide extensions are turned on.
          CStdString strFileNoExt = strFileNameTemp;
          RemoveExtension(strFileNoExt);
          CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
          CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
          strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;
          //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 3 : " + strFileName + " : " + strVolumeNumber + " : " + strFileTitle + " : " + strFileExt + " : " + strFileRight + " : " + strFileTitle);
          return true;
        }

      }
      else if( iCount > 1 )
      {        
        //Second Sub value contains the stacking
        strVolumeNumber = strFileName.Mid(iFoundToken + reg.GetSubStart(2), reg.GetSubLenght(2));

        strFileTitle = strFileName.Left(iFoundToken);

        //First Sub value contains prefix
        strFileTitle += strFileName.Mid(iFoundToken + reg.GetSubStart(1), reg.GetSubLenght(1));

        //Third Sub value contains suffix
        strFileTitle += strFileName.Mid(iFoundToken + reg.GetSubStart(3), reg.GetSubLenght(3));
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);
        //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 4 : " + strFileName + " : " + strVolumeNumber + " : " + strFileTitle);
        return true;
      }

    }
  }
  //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 5 : " + strFileName);
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
    CUtil::Lower(strExtension);

    CStdString strFileMask;
    strFileMask = g_stSettings.m_szMyMusicExtensions;
    strFileMask += g_stSettings.m_szMyPicturesExtensions;
    strFileMask += g_stSettings.m_szMyVideoExtensions;
    strFileMask += ".py|.xml|.milk";

    // Only remove if its a valid media extension
    if (strFileMask.Find(strExtension.c_str()) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

void CUtil::CleanFileName(CStdString& strFileName)
{
  bool result = false;

  //CLog::Log(LOGNOTICE, "CleanFileName : 1 : " + strFileName);

  // remove volume indicator from stacked files
  CStdString strFileTitle;
  CStdString strVolumeNumber;
  if (GetVolumeFromFileName(strFileName, strFileTitle, strVolumeNumber))
  {
    //CLog::Log(LOGNOTICE, "CleanFileName : 2 : " + strFileName + " : " + strFileTitle + " : " + strVolumeNumber);
    //If we have same extension as before (ie GetVolumeFromFileName didn't remove it). remove it now
    if(g_guiSettings.GetBool("FileLists.HideExtensions") 
    	&& (strcmp(GetExtension(strFileName.c_str()), GetExtension(strFileTitle.c_str()) ) == 0))
    {
      RemoveExtension(strFileTitle);
    }

    strFileName = strFileTitle;
  }
  else if (g_guiSettings.GetBool("FileLists.HideExtensions"))
  {
    RemoveExtension(strFileName);
  }

  //CLog::Log(LOGNOTICE, "CleanFileName : 3 : " + strFileName);

  // remove known tokens:      { "divx", "xvid", "3ivx", "ac3", "ac351", "mp3", "wma", "m4a", "mp4", "ogg", "SCR", "TS", "sharereactor" }
  // including any separators: { ' ', '-', '_', '.', '[', ']', '(', ')' }
  // logic is as follows:
  //   - multiple tokens can be listed, separated by any combination of separators
  //   - first token must follow a '-' token, potentially in addition to other separator tokens
  //   - thus, something like "video_XviD_AC3" will not be parsed, but something like "video_-_XviD_AC3" will be parsed

  // special logic - if a known token is found, try to group it with any
  // other tokens up to the dash separating the token group from the title.
  // thus, something like "The Godfather-DivX_503_2p_VBR-HQ_480x640_16x9" should
  // be fully cleaned up to "The Godfather"
  // the problem with this logic is that it may clean out things we still
  // want to see, such as language codes.

  {
    //m_szMyVideoCleanSeparatorsString = " -_.[]()+";

    //m_szMyVideoCleanTokensArray = "divx|xvid|3ivx|ac3|ac351|mp3|wma|m4a|mp4|ogg|scr|ts|sharereactor";

    const CStdString & separatorsString = g_settings.m_szMyVideoCleanSeparatorsString;
    const CStdStringArray & tokens = g_settings.m_szMyVideoCleanTokensArray;

    CStdString strFileNameTempLower = strFileName;
    strFileNameTempLower.MakeLower();

    int maxPos = 0;
    bool tokenFoundWithSeparator = false;

    while ((maxPos < (int)strFileName.size()) && (!tokenFoundWithSeparator))
    {
      bool tokenFound = false;

      for (int i = 0; i < (int)tokens.size(); i++)
      {
        CStdString token = tokens[i];
        int pos = strFileNameTempLower.Find(token, maxPos);
        if (pos >= maxPos && pos > 0)
        {
          tokenFound = tokenFound | true;
          char separator = strFileName.GetAt(pos - 1);
          char buffer[10];
          itoa(pos, buffer, 10);
          char buffer2[10];
          itoa(maxPos, buffer2, 10);
          //CLog::Log(LOGNOTICE, "CleanFileName : 4 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2 + " : " + separatorsString);
          if (separatorsString.Find(separator) > -1)
          {
            // token has some separator before it - now look for the
            // specific '-' separator, and trim any additional separators.

            int pos2 = pos;
            while (pos2 > 0)
            {
              separator = strFileName.GetAt(pos2 - 1);
              if (separator == '-')
                tokenFoundWithSeparator = true;
              else if (separatorsString.Find(separator) == -1)
                break;
              pos2--;
            }
            if (tokenFoundWithSeparator)
              pos = pos2;
            //if (tokenFoundWithSeparator)
              //CLog::Log(LOGNOTICE, "CleanFileName : 5 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
            //else
              //CLog::Log(LOGNOTICE, "CleanFileName : 6 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
          }

          if (tokenFoundWithSeparator)
          {
            if (pos > 0)
              strFileName = strFileName.Left(pos);
            break;
          }
          else
          {
            maxPos = max(maxPos, pos + 1);
          }
        }
      }

      if (!tokenFound)
        break;

      maxPos++;
    }

  }


  // TODO: would be nice if we could remove years (i.e. "(1999)") from the
  // title, and put the year in a separate column instead

  // TODO: would also be nice if we could do something with
  // languages (i.e. "[ITA]") - need to consider files with
  // multiple audio tracks


  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  // if the extension is shown, the '.' before the extension should be 
  // left as is.

  strFileName = strFileName.Trim();
  //CLog::Log(LOGNOTICE, "CleanFileName : 7 : " + strFileName);

  int extPos = (int)strFileName.size();
  if (!g_guiSettings.GetBool("FileLists.HideExtensions"))
  {
    CStdString strFileNameTemp = strFileName;
    RemoveExtension(strFileNameTemp);
    //CLog::Log(LOGNOTICE, "CleanFileName : 8 : " + strFileName + " : " + strFileNameTemp);
    extPos = strFileNameTemp.size();
  }

  {
    bool alreadyContainsSpace = (strFileName.Find(' ') >= 0);

    for (int i = 0; i < extPos; i++)
    {
      char c = strFileName.GetAt(i);
      if ((c == '_') || ((!alreadyContainsSpace) && (c == '.')))
      {
        strFileName.SetAt(i, ' ');
      }
    }
  }

  strFileName = strFileName.Trim();
  //CLog::Log(LOGNOTICE, "CleanFileName : 9 : " + strFileName);
}

bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURL url(strPath);
  CStdString strFile = url.GetFileName();
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip"))
  { 
    if (url.GetFileName().size() == 0)
    {
      CUtil::GetDirectory(url.GetHostName(),strParent);
      return true;
    }
    else
    {
      CStdString strParentPath;
      bool bOkay = GetParentPath("D:\\"+url.GetFileName(),strParentPath);
      if (bOkay) 
      {
        if (strParentPath.size() > 3)
          strParent = strPath.substr(0,strPath.size()-url.GetFileName().size())+strParentPath.substr(3)+"\\";
        else
          strParent = strPath.substr(0,strPath.size()-url.GetFileName().size());
        return true;
      }
      else
        return false;
    }
  }
  else if (strFile.size() == 0)
  {
    if (url.GetProtocol() == "smb" && (url.GetHostName().size() > 0))
    {
      // we have an smb share with only server or workgroup name
      // set hostname to "" and return true.
      url.SetHostName("");
      url.GetURL(strParent);
      return true;
    }
    else if (url.GetProtocol() == "xbms" && (url.GetHostName().size() > 0))
    {
      // we have an xbms share with only server name
      // set hostname to "" and return true.
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
  if (iPos < 0)
  {
    iPos = strFile.ReverseFind('\\');
  }
  if (iPos < 0)
  {
    url.SetFileName("");
    url.GetURL(strParent);
    return true;
  }

  strFile = strFile.Left(iPos);
  url.SetFileName(strFile);
  url.GetURL(strParent);
  return true;
}


void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  //Make sure you have a full path in the filename, otherwise adds the base path before.
  CURL plItemUrl(strFilename);
  CURL plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if (plBaseUrl.GetProtocol().length() == 0) //Base in local directory
  {
    if (plItemUrl.GetProtocol().length() == 0 ) //Filename is local or not qualified
    {
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
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
    if (g_guiSettings.GetBool("Servers.FTPAutoFatX") && (CUtil::IsHD(strFilename)))
      CUtil::GetFatXQualifiedPath(strFilename);
  }
  else //Base is remote
  {
    if (plItemUrl.GetProtocol().length() == 0 ) //Filename is local
    {
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

bool CUtil::PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video)
{
  BYTE	*Kernel=(BYTE *)0x80010000;
  DWORD	i, j = 0;
  DWORD	*CountryPtr;
  BYTE	CountryValues[4]={0, 1, 2, 4};
  BYTE	VideoTyValues[5]={0, 1, 2, 3, 3};
  BYTE	VideoFrValues[5]={0x00, 0x40, 0x40, 0x80, 0x40};

  // No Country or Video specified, do nothing.
  if ((Country==0) && (Video==0))
    return( false );

  // Video specified with no Country - select default Country for this Video Mode.
  if (Country==0)
  {
    Country=COUNTRY_EUR;
    if (Video==VIDEO_NTSCM)	
      Country=COUNTRY_USA;
    else if (Video==VIDEO_NTSCJ)	
      Country=COUNTRY_JAP;
  }

  // Country specified with no Video - select default Video Mode for this Country.
  if (Video==0)
  {
    Video=VIDEO_PAL50;
    if(Country==COUNTRY_USA)
	    Video=VIDEO_NTSCM;
    if(Country==COUNTRY_JAP)
	    Video=VIDEO_NTSCJ;
  }

  // Search for the original code in the Kernel.
  // Searching from 0x80011000 to 0x80024000 in order that this will work on as many Kernels
  // as possible.

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=OriginalData[0])	
	    continue;

    for(j=0; j<57; j++)
    {
	    if(Kernel[i+j]!=OriginalData[j])	
		    break;
    }
    if(j==57)	
	    break;
  }

  if(j==57)
  {
    // Ok, found the code to patch. Get pointer to original Country setting.
    // This may not be strictly neccessary, but lets do it anyway for completeness.

    j=(Kernel[i+57])+(Kernel[i+58]<<8)+(Kernel[i+59]<<16)+(Kernel[i+60]<<24);
    CountryPtr=(DWORD *)j;
  }
  else
  {
    // Did not find code in the Kernel. Check if my patch is already there.

    for(i=0x1000; i<0x14000; i++)
    {
	    if(Kernel[i]!=PatchData[0])	
		    continue;

	    for(j=0; j<25; j++)
	    {
		    if(Kernel[i+j]!=PatchData[j])	
			    break;
	    }
	    if(j==25)	
		    break;
    }

    if(j==25)
    {
	    // Ok, found my patch. Get pointer to original Country setting.
	    // This may not be strictly neccessary, but lets do it anyway for completeness.

	    j=(Kernel[i+66])+(Kernel[i+67]<<8)+(Kernel[i+68]<<16)+(Kernel[i+69]<<24);
	    CountryPtr=(DWORD *)j;
    }
    else
    {
	    // Did not find my patch - so I can't work with this BIOS. Exit.
	    return( false );
    }
  }

  // Patch in new code.

  j=MmQueryAddressProtect(&Kernel[i]);
  MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);

  memcpy(&Kernel[i], &PatchData[0], 70);

  // Patch Success. Fix up values.

  *CountryPtr=(DWORD)CountryValues[Country];
  Kernel[i+0x1f]=CountryValues[Country];
  Kernel[i+0x19]=VideoTyValues[Video];
  Kernel[i+0x1a]=VideoFrValues[Video];

  j=(DWORD)CountryPtr;
  Kernel[i+66]=(BYTE)(j&0xff);
  Kernel[i+67]=(BYTE)((j>>8)&0xff);
  Kernel[i+68]=(BYTE)((j>>16)&0xff);
  Kernel[i+69]=(BYTE)((j>>24)&0xff);

  MmSetAddressProtect(&Kernel[i], 70, j);

  // All Done!
  return( true );
} 


void CUtil::RunXBE(const char* szPath1, char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry) 
{
  /// \brief Runs an executable file
  /// \param szPath1 Path of executeable to run
  /// \param szParameters Any parameters to pass to the executeable being run
  g_application.PrintXBEToLCD(szPath1); //write to LCD
  Sleep(600);        //and wait a little bit to execute

  char szDevicePath[1024];
  char szPath[1024];
  char szXbePath[1024];
  strcpy(szPath, szPath1);
  if (strncmp(szPath1, "Q:", 2) == 0)
  { // mayaswell support the virtual drive as well...
    CStdString strPath;
    if (strlen(g_stSettings.szHomeDir) > 1)
    { // home dir is defined
      strPath = g_stSettings.szHomeDir;
    }
    else
    { // home dir is xbe dir
      GetHomePath(strPath);
    }
    if (!HasSlashAtEnd(strPath))
      strPath += "\\";
    if (szPath1[2] == '\\')
      strPath += szPath1 + 3;
    else
      strPath += szPath1 + 2;
    strcpy(szPath, strPath.c_str());
  }
  char* szBackslash = strrchr(szPath, '\\');
  if (szBackslash)
  {
    *szBackslash = 0x00;
    char* szXbe = &szBackslash[1];

    char* szColon = strrchr(szPath, ':');
    if (szColon)
    {
      *szColon = 0x00;
      char* szDrive = szPath;
      char* szDirectory = &szColon[1];

      CIoSupport helper;
      helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

      strcat(szDevicePath, szDirectory);
      wsprintf(szXbePath, "d:\\%s", szXbe);

      g_application.Stop();

      CUtil::LaunchXbe(szDevicePath, szXbePath, szParameters, ForceVideo, ForceCountry);
    }
  }
  
  CLog::Log(LOGERROR, "Unable to run xbe : %s", szPath);
}

void CUtil::LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry)
{
  CLog::Log(LOGINFO, "launch xbe:%s %s", szPath, szXbe);
  CLog::Log(LOGINFO, " mount %s as D:", szPath);

  CIoSupport helper;
  helper.Unmount("D:");
  helper.Mount("D:", const_cast<char*>(szPath));

  CLog::Log(LOGINFO, "launch xbe:%s", szXbe);

  if (ForceVideo != VIDEO_NULL)
  {
    if (!ForceCountry)
      if (ForceVideo == VIDEO_NTSCM)
        ForceCountry = COUNTRY_USA;
      if (ForceVideo == VIDEO_NTSCJ)
        ForceCountry = COUNTRY_JAP;
      if (ForceVideo == VIDEO_PAL50)
        ForceCountry = COUNTRY_EUR;
    
      CLog::Log(LOGDEBUG,"forcing video mode: %i",ForceVideo);
      
      bool bSuccessful = PatchCountryVideo(ForceCountry, ForceVideo);
      if( !bSuccessful )
        CLog::Log(LOGINFO,"AutoSwitch: Failed to set mode");
  }

  if (szParameters == NULL)
  {
    XLaunchNewImage(szXbe, NULL );
  }
  else
  {
    LAUNCH_DATA LaunchData;
    strcpy((char*)LaunchData.Data, szParameters);

    XLaunchNewImage(szXbe, &LaunchData );
  }
}

void CUtil::GetThumbnail(const CStdString& strFileName, CStdString& strThumb)
{
  strThumb = "";
  CFileItem item(strFileName, false);

  CStdString strFile;
  CUtil::ReplaceExtension(strFileName, ".tbn", strFile);
  if (CFile::Exists(strFile))
  {
    strThumb = strFile;
    return ;
  }

  if (item.IsXBE())
  {
    if (CUtil::GetXBEIcon(strFileName, strThumb) ) return ;
    strThumb = "defaultProgamIcon.png";
    return ;
  }

  if (item.IsShortCut() )
  {
    CShortcut shortcut;
    if ( shortcut.Create( strFileName ) )
    {
      CStdString strFile = shortcut.m_strPath;

      GetThumbnail(strFile, strThumb);
      return ;
    }
  }

  /*
  Crc32 crc;
  crc.ComputeFromLowerCase(strFileName);
  strThumb.Format("%s\\%x.tbn", g_stSettings.szThumbnailsDirectory, crc);
  */

  GetCachedThumbnail(strFileName, strThumb);
}

void CUtil::GetCachedThumbnail(const CStdString& strFileName, CStdString& strCachedThumb)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(strFileName);
  CStdString strHex;
  strHex.Format("%08x",crc);
  strCachedThumb.Format("%s\\%s\\%s.tbn", g_stSettings.szThumbnailsDirectory, strHex.Left(1).c_str(), strHex.c_str());
}

void CUtil::GetDate(SYSTEMTIME stTime, CStdString& strDateTime)
{
  char szTmp[128];
  sprintf(szTmp, "%i-%i-%i %02.2i:%02.2i",
          stTime.wDay, stTime.wMonth, stTime.wYear,
          stTime.wHour, stTime.wMinute);
  strDateTime = szTmp;
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport helper;
  helper.GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName, '\\');
  *szFileName = 0;
  strPath = szXBEFileName;
}

bool CUtil::IsEthernetConnected()
{
  if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
    return false;

  return true;
}

void CUtil::GetTitleIP(CStdString& ip)
{
  ip = g_szTitleIP;
}

bool CUtil::InitializeNetwork(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{
  if (!IsEthernetConnected())
  {
    CLog::Log(LOGWARNING, "network cable unplugged");
    return false;
  }

  struct network_info networkinfo ;
  memset(&networkinfo , 0, sizeof(networkinfo ));
  bool bSetup(false);
  if (iAssignment == NETWORK_DHCP)
  {
    bSetup = true;
    CLog::Log(LOGNOTICE, "use DHCP");
    networkinfo.DHCP = true;
  }
  else if (iAssignment == NETWORK_STATIC)
  {
    bSetup = true;
    CLog::Log(LOGNOTICE, "use static ip");
    networkinfo.DHCP = false;
    strcpy(networkinfo.ip, szLocalAddress);
    strcpy(networkinfo.subnet, szLocalSubnet);
    strcpy(networkinfo.gateway, szLocalGateway);
    strcpy(networkinfo.DNS1, szNameServer);
  }
  else
  {
    CLog::Log(LOGWARNING, "Not initializing network, using settings as they are setup by dashboard");
  }

  if (bSetup)
  {
    CLog::Log(LOGINFO, "setting up network...");
    int iCount = 0;
    while (CUtil::SetUpNetwork( false, networkinfo ) == 1 && iCount < 100)
    {
      Sleep(50);
      iCount++;
    }
  }
  else
  {
    CLog::Log(LOGINFO, "init network");
    XNetStartupParams xnsp;
    memset(&xnsp, 0, sizeof(xnsp));
    xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

    // Bypass security so that we may connect to 'untrusted' hosts
    xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    // create more memory for networking
    xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
    xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
    xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
    xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
    xnsp.cfgSockMaxSockets = 64; // default = 64
    xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
    xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
    INT err = XNetStartup(&xnsp);
  }

  CLog::Log(LOGINFO, "get local ip address:");
  XNADDR xna;
  DWORD dwState;
  do
  {
    dwState = XNetGetTitleXnAddr(&xna);
    Sleep(50);
  }
  while (dwState == XNET_GET_XNADDR_PENDING);

  XNetInAddrToString(xna.ina, g_szTitleIP, 32);

  CLog::Log(LOGINFO, "ip adres:%s", g_szTitleIP);
  WSADATA WsaData;
  int err = WSAStartup( MAKEWORD(2, 2), &WsaData );

  if (err == NO_ERROR)
  {
    m_bNetworkUp = true;
    return true;
  }
  return false;
}
void CUtil::ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime)
{
  __int64 l64Result = ((__int64)sec + SECS_BETWEEN_EPOCHS) + SECS_TO_100NS + (nsec / 100);
  ftTime.dwLowDateTime = (DWORD)l64Result;
  ftTime.dwHighDateTime = (DWORD)(l64Result >> 32);
}

__int64 CUtil::CompareSystemTime(const SYSTEMTIME *a, const SYSTEMTIME *b)
{
  ULARGE_INTEGER ula, ulb;
  SystemTimeToFileTime(a, (FILETIME*) &ula);
  SystemTimeToFileTime(b, (FILETIME*) &ulb);
  return ulb.QuadPart -ula.QuadPart;
}

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

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
  int iPos = strFile.ReverseFind(".");
  if (iPos < 0)
  {
    strExtension = "";
    return ;
  }
  strExtension = strFile.Right( strFile.size() - iPos);
}

void CUtil::Lower(CStdString& strText)
{
  char szText[1024];
  strcpy(szText, strText.c_str());
  //for multi-byte language, the strText.size() is not correct
  for (unsigned int i = 0; i < strlen(szText);++i)
    szText[i] = tolower(szText[i]);
  strText = szText;
};

void CUtil::Unicode2Ansi(const wstring& wstrText, CStdString& strName)
{
  strName = "";
  char *pstr = (char*)wstrText.c_str();
  for (int i = 0; i < (int)wstrText.size();++i )
  {
    strName += pstr[i * 2];
  }
}

bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size() == 0) return false;
  char kar = strFile.c_str()[strFile.size() - 1];
  if (kar == '/' || kar == '\\') return true;
  return false;
}

bool CUtil::IsRemote(const CStdString& strFile)
{
  CURL url(strFile);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();
  if (strProtocol == "cdda" || strProtocol == "iso9660") return false;
  if ( url.GetProtocol().size() ) return true;
  return false;
}

bool CUtil::IsOnDVD(const CStdString& strFile)
{
  if (strFile.Left(4) == "DVD:" || strFile.Left(4) == "dvd:")
    return true;

  if (strFile.Left(2) == "D:" || strFile.Left(2) == "d:")
    return true;

  if (strFile.Left(4) == "UDF:" || strFile.Left(4) == "udf:")
    return true;

  if (strFile.Left(9) == "ISO9660:" || strFile.Left(9) == "iso9660:")
    return true;

  return false;
}

bool CUtil::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile; strFileLow.MakeLower();
  if (strFileLow == "d:\\" || strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;

  return false;
}

bool CUtil::IsVirualPath(const CStdString& strFile)
{ 
  if (strFile.Left(13).Equals("virtualpath://")) return true;
  return false;
}

bool CUtil::IsRAR(const CStdString& strFile) // also checks for comic books
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001")) return true;
  if (strExtension.CompareNoCase(".cbr") == 0) return true;
  if (strExtension.CompareNoCase(".rar") == 0)
  {
    std::vector<CStdString> tokens;
    CUtil::Tokenize(strFile,tokens,".");
    if (tokens.size() < 2)
      return false;
    CStdString token = tokens[tokens.size()-2];
    if (token.Left(4).CompareNoCase("part") == 0) // only list '.part01.rar'
    {
      if (atoi(token.Right(4).c_str()) == 1)
        return true;
    }
    else
      return true;
  }
  return false;
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
  CURL url(strFile);
  if (url.GetProtocol() == "cdda")
    return true;
  return false;
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  CStdString strLeft = strFile.Left(8);
  strLeft.ToLower();
  if (strLeft == "iso9660:")
    return true;
  return false;
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  CStdString strLeft = strFile.Left(4);
  return (strLeft.CompareNoCase("smb:") == 0);
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!IsRemote(strURL)) return ;
  if (IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

void CUtil::RemoveCRLF(CStdString& strLine)
{
  while ( strLine.size() && (strLine.Right(1) == "\n" || strLine.Right(1) == "\r") )
  {
    strLine = strLine.Left((int)strLine.size() - 1);
  }

}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

void CUtil::UrlDecode(CStdString& strURLData)
{
  CStdString strResult;
  for (unsigned int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num;
        sscanf(strTmp,"%x",&dec_num);
        strResult += (char)dec_num;
        i += 2;
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
  for (int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == ' ') strResult += '+';
    else if (isalnum(kar)) strResult += kar;
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  strURLData = strResult;
}

void CUtil::SaveString(const CStdString &strTxt, FILE *fd)
{
  int iSize = strTxt.size();
  fwrite(&iSize, 1, sizeof(int), fd);
  if (iSize > 0)
  {
    fwrite(&strTxt.c_str()[0], 1, iSize, fd);
  }
}
int CUtil::LoadString(CStdString &strTxt, byte* pBuffer)
{
  strTxt = "";
  int iSize;
  int iCount = sizeof(int);
  memcpy(&iSize, pBuffer, sizeof(int));
  if (iSize == 0) return iCount;
  char *szTmp = new char [iSize + 2];
  memcpy(szTmp , &pBuffer[iCount], iSize);
  szTmp[iSize] = 0;
  iCount += iSize;
  strTxt = szTmp;
  delete [] szTmp;
  return iCount;
}
bool CUtil::LoadString(string &strTxt, FILE *fd)
{
  strTxt = "";
  int iSize;
  int iRead = fread(&iSize, 1, sizeof(int), fd);
  if (iRead != sizeof(int) ) return false;
  if (feof(fd)) return false;
  if (iSize == 0) return true;
  if (iSize > 0 && iSize < 16384)
  {
    char *szTmp = new char [iSize + 2];
    iRead = fread(szTmp, 1, iSize, fd);
    if (iRead != iSize)
    {
      delete [] szTmp;
      return false;
    }
    szTmp[iSize] = 0;
    strTxt = szTmp;
    delete [] szTmp;
    return true;
  }
  return false;
}

void CUtil::SaveInt(int iValue, FILE *fd)
{
  fwrite(&iValue, 1, sizeof(int), fd);
}

int CUtil::LoadInt( FILE *fd)
{
  int iValue;
  fread(&iValue, 1, sizeof(int), fd);
  return iValue;
}

void CUtil::LoadDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
  fread(&dateTime, 1, sizeof(dateTime), fd);
}

void CUtil::SaveDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
  fwrite(&dateTime, 1, sizeof(dateTime), fd);
}


void CUtil::GetSongInfo(const CStdString& strFileName, CStdString& strSongCacheName)
{
  Crc32 crc;
  crc.Compute(strFileName);
  strSongCacheName.Format("%s\\songinfo\\%x.si", g_stSettings.m_szAlbumDirectory, crc);
}

void CUtil::GetAlbumFolderThumb(const CStdString& strFileName, CStdString& strThumb, bool bTempDir /*=false*/)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(strFileName);
  if (bTempDir)
    strThumb.Format("%s\\thumbs\\temp\\%x.tbn", g_stSettings.m_szAlbumDirectory, crc);
  else
    strThumb.Format("%s\\thumbs\\%x.tbn", g_stSettings.m_szAlbumDirectory, crc);
}

void CUtil::GetAlbumThumb(const CStdString& strAlbumName, const CStdString& strFileName, CStdString& strThumb, bool bTempDir /*=false*/)
{
  CStdString str;
  if (strAlbumName.IsEmpty())
  {
    str = "unknown" + strFileName;
  }
  else
  {
    str = strAlbumName + strFileName;
  }
  GetAlbumFolderThumb(str, strThumb, bTempDir);
}


bool CUtil::GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon)
{
  // check if thumbnail already exists
  if (CUtil::IsOnDVD(strFilePath) && !CDetectDVDMedia::IsDiscInDrive() )
  {
    strIcon = "defaultDVDEmpty.png";
    return true;
  }


  if (CUtil::IsOnDVD(strFilePath) || g_guiSettings.GetBool("MyPrograms.CacheProgramThumbs") )  // create CRC for DVD as we can't store default.tbn on DVD
  {
    /*
    Crc32 crc;
    crc.Compute(strFilePath);
    strIcon.Format("%s\\%x.tbn", g_stSettings.szThumbnailsDirectory, crc);
    */
    GetCachedThumbnail(strFilePath, strIcon);
  }
  else
  {
    CStdString strPath = "";
    CStdString strFileName = "";
    CStdString defaultTbn;
    CUtil::Split(strFilePath, strPath, strFileName);
    CUtil::ReplaceExtension(strFileName, ".tbn", defaultTbn);
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    strIcon.Format("%s\\%s", strPath.c_str(), defaultTbn.c_str());
  }

  if (CFile::Exists(strIcon) && !CUtil::IsOnDVD(strFilePath))   // always create thumbnail for DVD.
  {
    //yes, just return
    return true;
  }

  // no, then create a new thumb
  // Locate file ID and get TitleImage.xbx E:\UDATA\<ID>\TitleImage.xbx

  bool bFoundThumbnail = false;
  CStdString szFileName;
  szFileName.Format("E:\\UDATA\\%08x\\TitleImage.xbx", GetXbeID( strFilePath ) );
  if (!CFile::Exists(szFileName))
  {
    // extract icon from .xbe
    CXBE xbeReader;
    ::DeleteFile("T:\\1.xpr");
    if ( !xbeReader.ExtractIcon(strFilePath, "T:\\1.xpr"))
    {
      return false;
    }
    szFileName = "T:\\1.xpr";
  }

  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if ( SUCCEEDED( pPackedResource->Create( szFileName.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture;
    LPDIRECT3DTEXTURE8 m_pTexture;
    D3DSURFACE_DESC descSurface;

    pTexture = pPackedResource->GetTexture((DWORD)0);

    if ( pTexture )
    {
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight = descSurface.Height;
        int iWidth = descSurface.Width;
        DWORD dwFormat = descSurface.Format;
        g_graphicsContext.Get3DDevice()->CreateTexture( 128,
            128,
            1,
            0,
            D3DFMT_LIN_A8R8G8B8,
            0,
            &m_pTexture);
        LPDIRECT3DSURFACE8 pSrcSurface = NULL;
        LPDIRECT3DSURFACE8 pDestSurface = NULL;

        pTexture->GetSurfaceLevel( 0, &pSrcSurface );
        m_pTexture->GetSurfaceLevel( 0, &pDestSurface );

        D3DXLoadSurfaceFromSurface( pDestSurface, NULL, NULL,
                                    pSrcSurface, NULL, NULL,
                                    D3DX_DEFAULT, D3DCOLOR( 0 ) );
        D3DLOCKED_RECT rectLocked;
        if ( D3D_OK == m_pTexture->LockRect(0, &rectLocked, NULL, 0L ) )
        {
          BYTE *pBuff = (BYTE*)rectLocked.pBits;
          if (pBuff)
          {
            DWORD strideScreen = rectLocked.Pitch;
            //mp_msg(0,0," strideScreen=%i\n", strideScreen);
            CPicture pic;
            if (pic.CreateThumbnailFromSurface(pBuff, iHeight, iWidth, strideScreen, strIcon.c_str()))
              bFoundThumbnail = true;
          }
          m_pTexture->UnlockRect(0);
        }
        pSrcSurface->Release();
        pDestSurface->Release();
        m_pTexture->Release();
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  if (bFoundThumbnail) CUtil::ClearCache();
  return bFoundThumbnail;
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
    strDescription = strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)    
    strDescription = strFName;
  return true;
}
bool CUtil::GetXBEDescription(const CStdString& strFileName, CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "rb");
  if (!hFile)
  {
    strDescription = CUtil::GetFileName(strFileName);
    return false;
  }
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fclose(hFile);

  CHAR TitleName[40];
  WideCharToMultiByte(CP_ACP, 0, HC.TitleName, -1, TitleName, 40, NULL, NULL);
  if (strlen(TitleName) > 0)
  {
    strDescription = TitleName;
    return true;
  }
  strDescription = CUtil::GetFileName(strFileName);
  return false;
}

bool CUtil::SetXBEDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "r+b");
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fseek(hFile,HS.XbeHeaderSize, SEEK_SET);

  MultiByteToWideChar(CP_ACP,0,strDescription.c_str(),-1,HC.TitleName,40);
  fwrite(&HC,1,sizeof(HC),hFile);
  fclose(hFile);
  
  return true;
}

DWORD CUtil::GetXbeID( const CStdString& strFilePath)
{
  DWORD dwReturn = 0;

  DWORD dwCertificateLocation;
  DWORD dwLoadAddress;
  DWORD dwRead;
  //  WCHAR wcTitle[41];

  CAutoPtrHandle hFile( CreateFile( strFilePath.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL ));
  if ( hFile.isValid() )
  {
    if ( SetFilePointer( (HANDLE)hFile, 0x104, NULL, FILE_BEGIN ) == 0x104 )
    {
      if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
      {
        if ( SetFilePointer( (HANDLE)hFile, 0x118, NULL, FILE_BEGIN ) == 0x118 )
        {
          if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
          {
            dwCertificateLocation -= dwLoadAddress;
            // Add offset into file
            dwCertificateLocation += 8;
            if ( SetFilePointer( (HANDLE)hFile, dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
            {
              dwReturn = 0;
              ReadFile( (HANDLE)hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
              if ( dwRead != sizeof(DWORD) )
              {
                dwReturn = 0;
              }
            }

          }
        }
      }
    }
  }
  return dwReturn;
}

void CUtil::CreateShortcuts(CFileItemList &items)
{
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    CreateShortcut(pItem);

  }
}

void CUtil::CreateShortcut(CFileItem* pItem)
{
  bool bOnlyDefaultXBE = g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  if ( bOnlyDefaultXBE ? pItem->IsDefaultXBE() : pItem->IsXBE() )
  {
    // xbe
    pItem->SetIconImage("defaultProgram.png");
    if ( !pItem->IsOnDVD() )
    {
      CStdString strDescription;
      if (! CUtil::GetXBEDescription(pItem->m_strPath, strDescription))
      {
        CUtil::GetDirectoryName(pItem->m_strPath, strDescription);
      }
      if (strDescription.size())
      {
        CStdString strFname;
        strFname = CUtil::GetFileName(pItem->m_strPath);
        strFname.ToLower();
        if (strFname != "dashupdate.xbe" && strFname != "downloader.xbe" && strFname != "update.xbe")
        {
          CShortcut cut;
          cut.m_strPath = pItem->m_strPath;
          cut.Save(strDescription);
        }
      }
    }
  }
}

bool CUtil::GetFolderThumb(const CStdString& strFolder, CStdString& strThumb)
{
  // get the thumbnail for the folder contained in strFolder
  // and return the filename of the thumbnail in strThumb
  //
  // if folder contains folder.jpg and is local on xbox HD then use it as the thumbnail
  // if folder contains folder.jpg but is located on a share then cache the folder.jpg
  // to q:\thumbs and return the cached image as a thumbnail
  CStdString strFolderImage;
  strThumb = "";
  CUtil::AddFileToFolder(strFolder, "folder.jpg", strFolderImage);

  CFileItem item(strFolder, true);
  // remote or local file?
  if (item.IsRemote() || item.IsOnDVD() || item.IsISO9660() )
  {
    // dont try to locate a folder.jpg for streams &  shoutcast
    if (item.IsInternetStream())
      return false;

    CUtil::GetThumbnail( strFolderImage, strThumb);
    // if local cache of thumb doesnt exists yet
    if (!CFile::Exists( strThumb) )
    {
      // then cache folder.jpg to xbox HD
      if ((g_guiSettings.GetBool("VideoFiles.FindRemoteThumbs") || item.IsOnDVD()) && CFile::Exists(strFolderImage))
      {
        if ( CFile::Cache(strFolderImage.c_str(), strThumb.c_str(), NULL, NULL))
        {
          return true;
        }
      }
    }
    else
    {
      // else used the cached version
      return true;
    }
  }
  else if (CFile::Exists(strFolderImage) )
  {
    // is local, and folder.jpg exists. Use it
    strThumb = strFolderImage;
    return true;
  }

  // no thumb found
  strThumb = "";
  return false;
}

void CUtil::GetFatXQualifiedPath(CStdString& strFileNameAndPath)
{
  vector<CStdString> tokens;
  CStdString strBasePath;
  strFileNameAndPath.Replace("/","\\");
  CUtil::GetDirectory(strFileNameAndPath,strBasePath);
  CStdString strFileName = CUtil::GetFileName(strFileNameAndPath);
  CUtil::Tokenize(strBasePath,tokens,"\\");
  strFileNameAndPath = tokens.front();
  for (vector<CStdString>::iterator token=tokens.begin()+1;token != tokens.end();++token)
  {
    CStdString strToken = token->Left(42);
    while (strToken[strToken.size()-1] == ' ')
      strToken.erase(strToken.size()-1);
    CUtil::RemoveIllegalChars(strToken);
    strFileNameAndPath += "\\"+strToken;
  }
  if (strFileName != "")
  {
    CUtil::ShortenFileName(strFileName);
    if (strFileName[0] == '\\') 
      strFileName.erase(0,1);
    CUtil::RemoveIllegalChars(strFileName);
    CStdString strExtension;
    CStdString strNoExt;
    CUtil::GetExtension(strFileName,strExtension);
    CUtil::ReplaceExtension(strFileName,"",strNoExt);
    while (strNoExt[strNoExt.size()-1] == ' ')
      strNoExt.erase(strNoExt.size()-1);
    strFileNameAndPath += "\\"+strNoExt+strExtension;
  }
}

void CUtil::ShortenFileName(CStdString& strFileNameAndPath)
{
  CStdString strFile = CUtil::GetFileName(strFileNameAndPath);
  if (strFile.size() > 42)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileNameAndPath, strExtension);
    CStdString strPath = strFileNameAndPath.Left( strFileNameAndPath.size() - strFile.size() );

    strFile = strFile.Left(42 - strExtension.size());
    strFile += strExtension;

    CStdString strNewFile = strPath;
    if (!CUtil::HasSlashAtEnd(strPath))
      strNewFile += "\\";

    strNewFile += strFile;
    strFileNameAndPath = strNewFile;
  }
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

  CStdString strAlbumDir;
  strAlbumDir.Format("%s\\*.tmp", g_stSettings.m_szAlbumDirectory);
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(strAlbumDir.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      string strFile = g_stSettings.m_szAlbumDirectory;
      strFile += "\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));

  //CStdString strTempThumbDir;
  //strTempThumbDir.Format("%s\\thumbs\\temp\\*.tbn",g_stSettings.m_szAlbumDirectory);
  //memset(&wfd,0,sizeof(wfd));

  //CAutoPtrFind hFind1( FindFirstFile(strTempThumbDir.c_str(),&wfd));
  //if (!hFind1.isValid())
  //  return ;
  //do
  //{
  //  if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
  //  {
  //    CStdString strFile;
  //    strFile.Format("%s\\thumbs\\temp\\",g_stSettings.m_szAlbumDirectory);
  //    strFile += wfd.cFileName;
  //    DeleteFile(strFile.c_str());
  //  }
  //} while (FindNextFile(hFind1, &wfd));
}

void CUtil::DeleteTDATA()
{
  // delete T:\\settings.xml only
  CLog::Log(LOGINFO, "  DeleteFile(T:\\settings.xml)");
  ::DeleteFile("T:\\settings.xml");
  /*
  WIN32_FIND_DATA wfd;
  CStdString strTDATADir;
  strTDATADir = "T:\\*.*";
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(strTDATADir.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      string strFile = "T:\\";
      strFile += wfd.cFileName;
      CLog::Log(LOGINFO, "  DeleteFile(%s)", strFile.c_str());
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));*/
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  if (strFileName.size() <= 2) return false;
  char szDriveletter = tolower(strFileName.GetAt(0));
  if ( (szDriveletter >= 'c' && szDriveletter <= 'g' && szDriveletter != 'd') || (szDriveletter == 'q') || (szDriveletter == 'z') || (szDriveletter == 'y') || (szDriveletter == 'x') )
  {
    if (strFileName.GetAt(1) == ':') return true;
  }
  return false;
}

void CUtil::RemoveIllegalChars( CStdString& strText)
{
  char szRemoveIllegal [1024];
  strcpy(szRemoveIllegal , strText.c_str());
  static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~.ßÅåÄäÖöÜüøéèçàùêÂñá ";
  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
    *cursor = '_';
  strText = szRemoveIllegal;
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile("Z:\\*.*", &wfd));

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strFile;
          strFile.Format("Z:\\%s", wfd.cFileName);
          if (strFile.Find("subtitle") >= 0 )
          {
            ::DeleteFile(strFile.c_str());
          }
          else if (strFile.Find("vobsub_queue") >= 0 )
          {
            ::DeleteFile(strFile.c_str());
          }
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

void CUtil::CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback )
{
  char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", ".ifo", NULL};
  std::vector<CStdString> vecExtensionsCached;
  strExtensionCached = "";

  ClearSubtitles();

  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return ;
  if (item.IsPlayList()) return ;
  if (!item.IsVideo()) return ;

  std::vector<CStdString> strLookInPaths;

  CStdString strFileName;
  CStdString strFileNameNoExt;
  CStdString strPath;

  CUtil::Split(strMovie, strPath, strFileName);
  ReplaceExtension(strFileName, "", strFileNameNoExt);
  strLookInPaths.push_back(strPath);

  if (strlen(g_stSettings.m_szAlternateSubtitleDirectory) != 0)
  {
    strPath = g_stSettings.m_szAlternateSubtitleDirectory;
    if (!HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }
  
  if (strMovie.substr(0,6) == "rar://")
  {
    CURL url(strMovie);
    CUtil::Split(url.GetHostName(), strPath, strFileName);
    strLookInPaths.push_back(strPath);
  }
  
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    strPath = strLookInPaths[i];
    if (CDirectory::Exists(strLookInPaths[i]+"subs/"))
      strLookInPaths.push_back(strLookInPaths[i]+"subs/");
    if (CDirectory::Exists(strLookInPaths[i]+"subtitles/"))
      strLookInPaths.push_back(strLookInPaths[i]+"subtitles/");
    CUtil::GetParentPath(strLookInPaths[i],strPath);
    if (CDirectory::Exists(strPath+"/subs/"))
      strLookInPaths.push_back(strPath+"/subs/");
    if (CDirectory::Exists(strPath+"/subtitles/"))
      strLookInPaths.push_back(strPath+"/subtitles/");
  }
  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;  

  // 2 steps for movie directory and alternate subtitles directory
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items);
      int fnl = strFileNameNoExt.size();

      for (int j = 0; j < (int)items.Size(); j++)
      {
        for (int i = 0; sub_exts[i]; i++)
        {
          int l = strlen(sub_exts[i]);
          Split(items[j]->m_strPath, strPath, strItem);

          //Cache any alternate subtitles.
          if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
          {
            strLExt = strItem.Right(strItem.GetLength() - 9);
            strDest.Format("Z:\\subtitle.alt-%s", strLExt);
            if (CFile::Cache(items[j]->m_strPath, strDest.c_str(), pCallback, NULL))
            {
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
              strExtensionCached = strLExt;
            }
          }

          //Cache subtitle with same name as movie
          if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
          {
            strLExt = strItem.Right(strItem.size() - fnl - 1); //Disregard separator char
            strDest.Format("Z:\\subtitle.%s", strLExt);
            if (std::find(vecExtensionsCached.begin(),vecExtensionsCached.end(),strLExt) == vecExtensionsCached.end())
              if (CFile::Cache(items[j]->m_strPath, strDest.c_str(), pCallback, NULL))
              {
                vecExtensionsCached.push_back(strLExt);
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
              }
          }
        }
      }
      
      // check for rarred subtitles
      bool bFoundSubs=false;

      CStdString strRarPath = CUtil::GetFileName(strMovie);
	    CUtil::ReplaceExtension(strRarPath , ".rar", strDest);
      strRarPath.Format("%s%s", strLookInPaths[step].c_str(),strDest.c_str());
      if( CFile::Exists(strRarPath) )
	      bFoundSubs |= CacheRarSubtitles( vecExtensionsCached, strRarPath,  sub_exts ); 
      
      strRarPath.Format("%s%s", strLookInPaths[step].c_str(),"subtitles.rar");
	    if( CFile::Exists(strRarPath) )
        bFoundSubs |= CacheRarSubtitles( vecExtensionsCached, strRarPath,  sub_exts ); 

      strRarPath.Format("%s%s", strLookInPaths[step].c_str(),"subs.rar");
	    if( CFile::Exists(strRarPath) )
        bFoundSubs |= CacheRarSubtitles( vecExtensionsCached, strRarPath,  sub_exts ); 
  
      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }
  // construct string of added exts?
  for (std::vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+" ";
  //strExtensionCached.erase(strExtensionCached.size()-1); // remove trailing ' '
}

bool CUtil::CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const char * const* pSubExts)
{
  bool bFoundSubs = false;
  CRarManager RarMgr;
  CFileItemList ItemList;
  if( !RarMgr.GetFilesInRar(ItemList, strRarPath, true) ) return false;
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->m_strPath;
    int iPos=0;
    while (pSubExts[iPos])
    {
      CStdString strExt = CUtil::GetExtension(strPathInRar);
      if (strExt.CompareNoCase(".rar") == 0)
      {
        CStdString strExtAdded;
        CStdString strRarInRar;
        CUtil::CreateRarPath(strRarInRar, strRarPath, strPathInRar);
        CacheRarSubtitles(vecExtensionsCached,strRarInRar,pSubExts);
      }
      if(strExt.CompareNoCase(pSubExts[iPos]) == 0)
      {
        CStdString strSourceUrl, strDestUrl;
        CUtil::CreateRarPath(strSourceUrl, strRarPath, strPathInRar); 
        CStdString strDestFile;
        strDestFile.Format("subtitle%s", pSubExts[iPos]);
        if (std::find(vecExtensionsCached.begin(),vecExtensionsCached.end(),CStdString(pSubExts[iPos]+1)) == vecExtensionsCached.end())
          if (CFile::Cache(strSourceUrl,"Z:\\"+strDestFile))
          {
            vecExtensionsCached.push_back(CStdString(pSubExts[iPos]+1));
            CLog::Log(LOGINFO, " cached subtitle %s->Z:\\%s\n", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
          }
      }
      iPos++;
    }
  }
  return bFoundSubs;
}


void CUtil::SecondsToHMSString(long lSeconds, CStdString& strHMS, bool bMustUseHHMMSS)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds % 3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (hh >= 1 || bMustUseHHMMSS)
    strHMS.Format("%2.2i:%02.2i:%02.2i", hh, mm, ss);
  else
    strHMS.Format("%i:%02.2i", mm, ss);

}
void CUtil::PrepareSubtitleFonts()
{
  if (g_guiSettings.GetString("Subtitles.Font").size() == 0) return ;
  if (g_guiSettings.GetInt("Subtitles.Height") == 0) return ;

  CStdString strPath, strHomePath, strSearchMask;
  //  if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
  strHomePath = "Q:\\system\\players";
  //  else
  //    strHomePath = "Q:";

  strPath.Format("%s\\mplayer\\font\\%s\\%i\\",
                 strHomePath.c_str(),
                 g_guiSettings.GetString("Subtitles.Font").c_str(), g_guiSettings.GetInt("Subtitles.Height"));

  strSearchMask = strPath + "*.*";
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strSource, strDest;
          strSource.Format("%s%s", strPath.c_str(), wfd.cFileName);
          strDest.Format("%s\\mplayer\\font\\%s", strHomePath.c_str(), wfd.cFileName);
          ::CopyFile(strSource.c_str(), strDest.c_str(), FALSE);
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
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

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  strResult = strFolder;
  if (!CUtil::HasSlashAtEnd(strResult))
  {
    if (strResult.Find("//") >= 0 )
      strResult += "/";
    else
      strResult += "\\";
  }
  strResult += strFile;
}

void CUtil::GetPath(const CStdString& strFileName, CStdString& strPath)
{
  int iPos1 = strFileName.Find("/");
  int iPos2 = strFileName.Find("\\");
  int iPos3 = strFileName.Find(":");
  if (iPos2 > iPos1) iPos1 = iPos2;
  if (iPos3 > iPos1) iPos1 = iPos3;

  strPath = strFileName.Left(iPos1 - 1);
}
void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
  //Will from a full filename return the directory the file resides in.
  //has no trailing slash on result. Could lead to problems when reading from root on cd
  //ISO9660://filename.bla will result in path ISO9660:/
  //This behaviour should probably be changed, but it would break other things
  if ((strFilePath.substr(0,6) == "rar://") || (strFilePath.substr(0,6) == "zip://"))
  {
    CURL url(strFilePath);
    CStdString strTemp;
    GetDirectory(url.GetFileName(),strTemp);
    strDirectoryPath.Format("%s://%s,%i,%s,%s,\\%s",url.GetProtocol().c_str(),url.GetDomain(),url.GetPort(),url.GetPassWord(),url.GetHostName(),strTemp);
    return;
  }
  int iPos1 = strFilePath.ReverseFind('/');
  int iPos2 = strFilePath.ReverseFind('\\');

  if (iPos2 > iPos1)
  {
    iPos1 = iPos2;
  }

  if (iPos1 > 0)
  {
    strDirectoryPath = strFilePath.Left(iPos1);
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
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

void CUtil::CreateRarPath(CStdString& strUrlPath, const CStdString& strRarPath, const CStdString& strFilePathInRar,const WORD wOptions,  const CStdString& strPwd, const CStdString& strCachePath)
{
  //The possibilties for wOptions are
  //RAR_AUTODELETE : the cached version of the rar (strRarPath) will be deleted in file's dtor.
  //EXFILE_AUTODELETE : the extracted file (strFilePathInRar) will be deleted in file's dtor.
  //RAR_OVERWRITE : if the rar is already cached, overwrite the local copy.
  //EXFILE_OVERWRITE : if the extracted file is already cached, overwrite the local copy.
  int iAutoDelMask = wOptions;
  strUrlPath.Format("rar://%s,%i,%s,%s,\\%s",  strCachePath, iAutoDelMask, strPwd, strRarPath, strFilePathInRar);
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
  if (g_stSettings.m_szExternalDVDPlayer[0] && strcmp(g_stSettings.m_szExternalDVDPlayer, "dvdplayerbeta") != 0)
  {
    RunXBE(g_stSettings.m_szExternalDVDPlayer);
  }
  else
  {
    CIoSupport helper;
    helper.Remount("D:", "Cdrom0");
    CFileItem item("dvd://1", false);
    g_application.PlayFile(item);
  }
}

DWORD CUtil::SetUpNetwork( bool resetmode, struct network_info& networkinfo )
{
  static unsigned char params[512];
  static DWORD vReturn;
  static XNADDR sAddress;
  char temp_str[64];
  static char mode = 0;
  XNADDR xna;
  DWORD dwState;

  if ( resetmode )
  {
    resetmode = 0;
    mode = 0;
  }

  if ( !XNetGetEthernetLinkStatus() )
  {
    return 1;
  }

  if ( mode == 100 )
  {
    CLog::Log(LOGDEBUG, "  pending...");
    dwState = XNetGetTitleXnAddr(&xna);
    if (dwState == XNET_GET_XNADDR_PENDING)
      return 1;
    mode = 5;
    char azIPAdd[256];
    memset( azIPAdd, 0, 256);
    XNetInAddrToString(xna.ina, azIPAdd, 32);
    CLog::Log(LOGINFO, "ip address:%s", azIPAdd);
  }

  // if local address is specified
  if ( mode == 0 )
  {
    if ( !networkinfo.DHCP )
    {
      TXNetConfigParams configParams;

      XNetLoadConfigParams( (LPBYTE) &configParams );
      BOOL bXboxVersion2 = (configParams.V2_Tag == 0x58425632 );  // "XBV2"
      BOOL bDirty = FALSE;

      if (bXboxVersion2)
      {
        if (configParams.V2_IP != inet_addr(networkinfo.ip))
        {
          configParams.V2_IP = inet_addr(networkinfo.ip);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_IP != inet_addr(networkinfo.ip))
        {
          configParams.V1_IP = inet_addr(networkinfo.ip);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_Subnetmask != inet_addr(networkinfo.subnet))
        {
          configParams.V2_Subnetmask = inet_addr(networkinfo.subnet);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_Subnetmask != inet_addr(networkinfo.subnet))
        {
          configParams.V1_Subnetmask = inet_addr(networkinfo.subnet);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_Defaultgateway != inet_addr(networkinfo.gateway))
        {
          configParams.V2_Defaultgateway = inet_addr(networkinfo.gateway);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_Defaultgateway != inet_addr(networkinfo.gateway))
        {
          configParams.V1_Defaultgateway = inet_addr(networkinfo.gateway);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_DNS1 != inet_addr(networkinfo.DNS1))
        {
          configParams.V2_DNS1 = inet_addr(networkinfo.DNS1);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_DNS1 != inet_addr(networkinfo.DNS1))
        {
          configParams.V1_DNS1 = inet_addr(networkinfo.DNS1);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_DNS2 != inet_addr(networkinfo.DNS2))
        {
          configParams.V2_DNS2 = inet_addr(networkinfo.DNS2);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_DNS2 != inet_addr(networkinfo.DNS2))
        {
          configParams.V1_DNS2 = inet_addr(networkinfo.DNS2);
          bDirty = TRUE;
        }
      }

      if (configParams.Flag != (0x04 | 0x08) )
      {
        configParams.Flag = 0x04 | 0x08;
        bDirty = TRUE;
      }

      XNetSaveConfigParams( (LPBYTE) &configParams );

      XNetStartupParams xnsp;
      memset(&xnsp, 0, sizeof(xnsp));
      xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

      // Bypass security so that we may connect to 'untrusted' hosts
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
      // create more memory for networking
      xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
      xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
      xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
      xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
      xnsp.cfgSockMaxSockets = 64; // default = 64
      xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
      xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
      CLog::Log(LOGINFO, "requesting local ip adres");
      int err = XNetStartup(&xnsp);
      mode = 100;
      return 1;
    }
    else
    {
      /**     Set DHCP-flags from a known DHCP mode  (maybe some day we will fix this)  **/
      XNetLoadConfigParams(params);
      memset( params, 0, (sizeof(IN_ADDR) * 5) + 20 );
      params[40] = 33; params[41] = 223; params[42] = 196; params[43] = 67; params[44] = 6;
      params[45] = 145; params[46] = 157; params[47] = 118; params[48] = 182; params[49] = 239;
      params[50] = 68; params[51] = 197; params[52] = 133; params[53] = 150; params[54] = 118;
      params[55] = 211; params[56] = 38; params[57] = 87; params[58] = 222; params[59] = 119;
      params[64] = 0; params[72] = 0; params[73] = 0; params[74] = 0; params[75] = 0;
      params[340] = 160; params[341] = 93; params[342] = 131; params[343] = 191; params[344] = 46;

      XNetStartupParams xnsp;

      memset(&xnsp, 0, sizeof(xnsp));
      xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
      // Bypass security so that we may connect to 'untrusted' hosts
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

      xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
      xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
      xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
      xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
      xnsp.cfgSockMaxSockets = 64; // default = 64
      xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
      xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

      XNetSaveConfigParams(params);
      CLog::Log(LOGINFO, "requesting DHCP");
      int err = XNetStartup(&xnsp);
      mode = 5;
    }


  }

  char g_szTitleIP[32];

  WSADATA WsaData;
  int err;

  char ftploop;
  if ( mode == 5 )
  {
    XNADDR xna;
    DWORD dwState;

    dwState = XNetGetTitleXnAddr(&xna);

    if ( dwState == XNET_GET_XNADDR_PENDING)
      return 1;

    XNetInAddrToString(xna.ina, g_szTitleIP, 32);
    err = WSAStartup( MAKEWORD(2, 2), &WsaData );
    ftploop = 1;
    mode ++;
  }
  if ( mode == 6 )
  {
    vReturn = XNetGetTitleXnAddr(&sAddress);


    ftploop = 1;

    if ( vReturn != XNET_GET_XNADDR_PENDING )
    {
      char azIPAdd[256];
	  //char  azMessage[256];
      memset(azIPAdd, 0, sizeof(azIPAdd));

      XNetInAddrToString(sAddress.ina, azIPAdd, sizeof(azIPAdd));
      //strcpy(NetworkStatus,azIPAdd);
      //strcpy( NetworkStatusInternal, NetworkStatus );
      if (sAddress.ina.S_un.S_addr != 0)
      {
        DWORD temp = XNetGetEthernetLinkStatus();
        if ( temp & XNET_ETHERNET_LINK_ACTIVE )
        {

          if ( temp & XNET_ETHERNET_LINK_FULL_DUPLEX )
          {
            // full duplex
            CLog::Log(LOGINFO, "  full duplex");
          }

          if ( temp & XNET_ETHERNET_LINK_HALF_DUPLEX )
          {
            // half duplex
            CLog::Log(LOGINFO, "  half duplex");
          }

          if ( temp & XNET_ETHERNET_LINK_100MBPS )
          {
            CLog::Log(LOGINFO, "  100 mbps");
          }

          if ( temp & XNET_ETHERNET_LINK_10MBPS )
          {
            CLog::Log(LOGINFO, "  10bmps");
          }

          if ( vReturn & XNET_GET_XNADDR_STATIC )
          {
            CLog::Log(LOGINFO, "  static ip");
          }

          if ( vReturn & XNET_GET_XNADDR_DHCP )
          {
            CLog::Log(LOGINFO, "  Dynamic IP");
          }

          if ( vReturn & XNET_GET_XNADDR_DNS )
          {
            CLog::Log(LOGINFO, "  DNS");
          }

          if ( vReturn & XNET_GET_XNADDR_ETHERNET )
          {
            CLog::Log(LOGINFO, "  ethernet");
          }

          if ( vReturn & XNET_GET_XNADDR_NONE )
          {
            CLog::Log(LOGINFO, "  none");
          }

          if ( vReturn & XNET_GET_XNADDR_ONLINE )
          {
            CLog::Log(LOGINFO, "  online");
          }

          if ( vReturn & XNET_GET_XNADDR_PENDING )
          {
            CLog::Log(LOGINFO, "  pending");
          }

          if ( vReturn & XNET_GET_XNADDR_TROUBLESHOOT )
          {
            CLog::Log(LOGINFO, "  error");
          }

          if ( vReturn & XNET_GET_XNADDR_PPPOE )
          {
            CLog::Log(LOGINFO, "  ppoe");
          }

          sprintf(temp_str, "  IP: %s", azIPAdd);
          CLog::Log(LOGINFO, temp_str);
        }
        ftploop = 0;
        mode ++;
        return 0;
      }
      return 2;
    }

    Sleep(50);
    return 1;
  }
  return 1;
}

void CUtil::GetVideoThumbnail(const CStdString& strIMDBID, CStdString& strThumb)
{
  strThumb.Format("%s\\imdb\\imdb%s.jpg", g_stSettings.szThumbnailsDirectory, strIMDBID.c_str());
}

CStdString CUtil::GetNextFilename(const char* fn_template, int max)
{
  // Open the file.
  char szName[1024];

  INT i;

  WIN32_FIND_DATA wfd;
  HANDLE hFind;


  if (NULL != strstr(fn_template, "%03d"))
  {
    for (i = 0; i <= max; i++)
    {

      wsprintf(szName, fn_template, i);

      memset(&wfd, 0, sizeof(wfd));
      if ((hFind = FindFirstFile(szName, &wfd)) != INVALID_HANDLE_VALUE)
        FindClose(hFind);
      else
      {
        // FindFirstFile didn't find the file 'szName', return it
        return szName;
      }
    }
  }

  return ""; // no fn generated
}

void CUtil::InitGamma()
{
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldramp);
}
void CUtil::RestoreBrightnessContrastGamma()
{
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(D3DSGR_IMMEDIATE , &oldramp);
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
  D3DGAMMARAMP ramp;

  Gamma = 1.0f / Gamma;
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }

  // set ramp next v sync
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? D3DSGR_IMMEDIATE : 0, &ramp);
  g_graphicsContext.Unlock();
}


void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  string str = path;
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
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
    g_graphicsContext.Get3DDevice()->GetGammaRamp(&flashramp);
    SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
  }
  else
    g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? D3DSGR_IMMEDIATE : 0, &flashramp);
  g_graphicsContext.Unlock();
}

void CUtil::TakeScreenshot(const char* fn, bool flashScreen)
{
    LPDIRECT3DSURFACE8 lpSurface = NULL;

    g_graphicsContext.Lock();
    if (g_application.IsPlayingVideo())
    {
      g_renderManager.SetupScreenshot();
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].GUIOverscan, sizeof(OVERSCAN));
      g_graphicsContext.ResetOverscan(g_graphicsContext.GetVideoResolution(), g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].GUIOverscan);
      g_application.Render();
      memcpy(&g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].GUIOverscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
    g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
    if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
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
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
      FlashScreen(true, true);
      Sleep(10);
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
      FlashScreen(true, false);
    }
}

void CUtil::TakeScreenshot()
{
  char fn[1024];
  CStdString strDir = g_stSettings.m_szScreenshotsDirectory;

  if (strlen(g_stSettings.m_szScreenshotsDirectory))
  {
    sprintf(fn, "%s\\screenshot%%03d.bmp", strDir.c_str());
    strcpy(fn, CUtil::GetNextFilename(fn, 999).c_str());

    if (strlen(fn))
    {
      TakeScreenshot(fn, true);
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}

void CUtil::ClearCache()
{
  CStdString strThumb = g_stSettings.m_szAlbumDirectory;
  strThumb += "\\thumbs";
  g_directoryCache.ClearDirectory(strThumb);
  g_directoryCache.ClearDirectory(strThumb + "\\temp");

  strThumb = g_stSettings.szThumbnailsDirectory;
  g_directoryCache.ClearDirectory(strThumb);
  g_directoryCache.ClearDirectory(strThumb + "\\imdb");

  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strThumbLoc = g_stSettings.szThumbnailsDirectory;
    CStdString strHex;
    strHex.Format("%x",hex);
    strThumbLoc += "\\" + strHex;
    g_directoryCache.ClearDirectory(strThumbLoc);
  }
}

void CUtil::SortFileItemsByName(CFileItemList& items, bool bSortAscending /*=true*/)
{
  SSortByLabel::m_bSortAscending = bSortAscending;
  items.Sort(SSortByLabel::Sort);
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
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
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
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
}

void CUtil::Stat64ToStat(struct _stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
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
  std::vector<string> strArray;
  CURL url(strPath);
  string path = url.GetFileName().c_str();
  int iSize = path.size();
  char cSep = CUtil::GetDirectorySeperator(strPath);
  if (path.at(iSize - 1) == cSep) path.erase(iSize - 1, iSize - 1); // remove slash at end
  CStdString strTemp;

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  /* split strPath up into an array
   * music\\album\\ will result in
   * music
   * music\\album
   */

  int i;
  CFileItem item(strPath,true);
  if (item.IsSmb())
  {
    i = 0;
  }
  else if (item.IsHD())
  {
    i = 2; // remove the "E:" from the filename
  }
  else
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
    CStdString strTemp1 = strTemp + strArray[i];
    CDirectory::Create(strTemp1);
  }
  strArray.clear();

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}
CStdString CUtil::MakeLegalFileName(const char* strFile, bool bKeepExtension, bool isFATX)
{
  // check if the filename is a legal FATX one.
  // this means illegal chars will be removed from the string,
  // and the remaining string is stripped back to 42 chars if needed
  if (NULL == strFile) return "";
  char cIllegalChars[] = "<>=?:;\"*+,/\\|";
  unsigned int iIllegalCharSize = strlen(cIllegalChars);
  bool isIllegalChar;
  unsigned int iSize = strlen(strFile);
  unsigned int iNewStringSize = 0;
  char* strNewString = new char[iSize + 1];

  // only copy the legal characters to the new filename
  for (unsigned int i = 0; i < iSize; i++)
  {
    isIllegalChar = false;
    // check for illigal chars
    for (unsigned j = 0; j < iIllegalCharSize; j++)
      if (strFile[i] == cIllegalChars[j]) isIllegalChar = true;
    // FATX only allows chars from 32 till 127
    if (isIllegalChar == false &&
        strFile[i] > 31 && strFile[i] < 127) strNewString[iNewStringSize++] = strFile[i];
  }
  strNewString[iNewStringSize] = '\0';

  if (isFATX)
  {
    // since we can only write to samba shares and hd, we assume this has to be a fatx filename
    // thus we have to strip it down to 42 chars (samba doesn't have this limitation)

    // no need to keep the extension, just strip it down to 42 characters
    if (iNewStringSize > 42 && bKeepExtension == false) strNewString[42] = '\0';

    // we want to keep the extension
    else if (iNewStringSize > 42 && bKeepExtension == true)
    {
      char strExtension[42];
      unsigned int iExtensionLenght = iNewStringSize - (strrchr(strNewString, '.') - strNewString);
      strcpy(strExtension, (strNewString + iNewStringSize - iExtensionLenght));

      strcpy(strNewString + (42 - iExtensionLenght), strExtension);
    }
  }

  CStdString result(strNewString);
  delete[] strNewString;
  return result;
}

void CUtil::AddDirectorySeperator(CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetProtocol().size() > 0) strPath += "/";
  else strPath += "\\";
}

char CUtil::GetDirectorySeperator(const CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetProtocol().size() > 0) return '/';
  return '\\';
}

void CUtil::ConvertFileItemToPlayListItem(const CFileItem *pItem, CPlayList::CPlayListItem &playlistitem)
{
  playlistitem.SetDescription(pItem->GetLabel());
  playlistitem.SetFileName(pItem->m_strPath);
  playlistitem.SetDuration(pItem->m_musicInfoTag.GetDuration());
  playlistitem.SetStartOffset(pItem->m_lStartOffset);
  playlistitem.SetEndOffset(pItem->m_lEndOffset);
  playlistitem.SetMusicTag(pItem->m_musicInfoTag);
  playlistitem.SetThumbnailImage(pItem->GetThumbnailImage());
}


bool CUtil::IsNaturalNumber(const CStdString& str)
{
  return IsNaturalNumber((CStdStringW)str);
}

bool CUtil::IsNaturalNumber(const CStdStringW& str)
{
  if (0 == (int)str.size())
    return false;
  for (int i = 0; i < (int)str.size(); i++)
  {
    if ((str[i] < '0') || (str[i] > '9')) return false;
  }
  return true;
}

bool CUtil::IsUsingTTFSubtitles()
{
  char* ext = strrchr(g_guiSettings.GetString("Subtitles.Font").c_str(), '.');
  if (ext && stricmp(ext, ".ttf") == 0)
    return true;
  else
    return false;
}

typedef struct
{
  char command[20];
  char description[128];
} BUILT_IN;

const BUILT_IN commands[] = {
  "Help", "This help message",
  "Reboot", "Reboot the xbox (power cycle)",
  "Restart", "Restart the xbox (power cycle)",
  "ShutDown", "Shutdown the xbox",
  "Dashboard", "Run your dashboard",
  "RestartApp", "Restart XBMC",
  "Credits", "Run XBMCs Credits",
  "Reset", "Reset the xbox (warm reboot)",
  "ActivateWindow", "Activate the specified window",
  "RunScript", "Run the specified script",
  "RunXBE", "Run the specified executeable",
  "PlayMedia", "Play the specified media file (or playlist)",
  "SlideShow", "Run a slideshow from the specified directory",
  "RecursiveSlideShow", "Run a slideshow from the specified directory, including all subdirs",
  "ReloadSkin", "Reload XBMC's skin",
  "PlayerControl", "Control the music or video player",
  "EjectTray", "Close or open the DVD tray",
  "AlarmClock", "Prompt for a length of time and start an alarm clock",
  "CancelAlarm","Cancels an alarm",
  "Action", "Executes an action for the active window (same as in keymap)",
  "Notificaton", "Shows a notification on screen, specify header, then message.",
  "PlayDVD"," Plays the inserted CD or DVD media from the DVD-ROM Drive!"
};

bool CUtil::IsBuiltIn(const CStdString& execString)
{
  CStdString function, param;
  SplitExecFunction(execString, function, param);
  for (int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    if (function.CompareNoCase(commands[i].command) == 0)
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
  for (int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
//    for (int i = 0; i < 20 - strlen(commands[i].command); i++)
//      help += " ";
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
    g_applicationMessenger.Restart();
  }
  else if (execute.Equals("shutdown"))
  {
    g_applicationMessenger.Shutdown();
  }
  else if (execute.Equals("dashboard"))
  {
    RunXBE(g_stSettings.szDashboard);
  }
  else if (execute.Equals("restartapp"))
  {
    g_applicationMessenger.RestartApp();
  }
  else if (execute.Equals("credits"))
  {
    RunCredits();
  }
  else if (execute.Equals("reset")) //Will reset the xbox, aka soft reset
  {
    g_applicationMessenger.Reset();
  }
  else if (execute.Equals("activatewindow"))
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
      CLog::Log(LOGERROR, "ActivateWindow called with invalid parameter: %s", parameter.c_str());
      return -7;
    }
    else if (iPos < 0)
    {
      // no path parameter
      // XBMC.ActivateWindow(5001)
      strWindow = parameter;
    }
    else
    {
      // path parameter included
      // XBMC.ActivateWindow(5001,F:\Music\)
      strWindow = parameter.Left(iPos);
      strPath = parameter.Mid(iPos + 1);
    }

    // confirm the window destination is actually a number
    // before switching
    int iWindow = g_buttonTranslator.TranslateWindowString(strWindow.c_str());
    if (iWindow != WINDOW_INVALID)
    {
      // disable the screensaver
      g_application.ResetScreenSaverWindow();
      // check what type of window we have (it could be a dialog)
      CGUIWindow *pWindow = m_gWindowManager.GetWindow(iWindow);
      if (pWindow && pWindow->IsDialog())
      {
        // Dialog, dont pass path!
        CGUIDialog *pDialog = (CGUIDialog *)pWindow;
        if (!pDialog->IsRunning())
          pDialog->DoModal(m_gWindowManager.GetActiveWindow(), iWindow);
        return -6;
      }
      m_gWindowManager.ActivateWindow(iWindow, strPath);
    }
    else
    {
      CLog::Log(LOGERROR, "ActivateWindow called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if (execute.Equals("runscript"))
  {
    g_pythonParser.evalFile(parameter.c_str());
  }
  else if (execute.Equals("runxbe"))
  {
    // only usefull if there is actualy a xbe to execute
    if (parameter.size() > 0)
    {
	    int iRegion;
	    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
	    {
	      CXBE xbe;
	      iRegion = xbe.ExtractGameRegion(parameter);
	      if (iRegion < 1 || iRegion > 7)
	        iRegion = 0;
	      iRegion = xbe.FilterRegion(iRegion);
	    }
	    else
	      iRegion = 0;
	      
	    CUtil::RunXBE(parameter.c_str(),NULL,F_VIDEO(iRegion));
    }
    else
    {
      CLog::Log(LOGERROR, "CUtil::ExecBuiltIn, runxbe called with no arguments.");
    }
  }
  else if (execute.Equals("playmedia"))
  {
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia called with empty parameter");
      return -3;
    }
    CFileItem item(parameter, false);
    if (!g_application.PlayMedia(item, PLAYLIST_MUSIC_TEMP))
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia could not play media: %s", parameter.c_str());
      return false;
    }
  }
  else if (execute.Equals("slideShow") || execute.Equals("recursiveslideShow"))
  {
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.SlideShow called with empty parameter");
      return -2;
    }
    CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, execute.Equals("SlideShow") ? 0 : 1, 0, 0);
    msg.SetStringParam(parameter);
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute.Equals("reloadskin"))
  {
    //	Reload the skin
    int iActiveWindowID = m_gWindowManager.GetActiveWindow();
    CGUIWindow* pWindow=m_gWindowManager.GetWindow(iActiveWindowID);
    DWORD dwFocusedControlID=pWindow->GetFocusedControl();

    g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));

    m_gWindowManager.ActivateWindow(iActiveWindowID);
    CGUIMessage msg(GUI_MSG_SETFOCUS, iActiveWindowID, dwFocusedControlID, 0);
    pWindow->OnMessage(msg);
  }
  else if (execute.Equals("playercontrol"))
  {
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
      else
      { // not currently playing, let's see if there's anything in the playlist
//        CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
//       if (playlist.size())
//         g_playlistPlayer.Play(g_playlistPlayer.Pl
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
      g_playlistPlayer.PlayNext();
    }
    else if (parameter.Equals("previous"))
    {
      g_playlistPlayer.PlayPrevious();
    }
    else if( parameter.Equals("showvideomenu") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer )
      {
        CAction action;
        memset(&action, 0, 0);
        action.wID = ACTION_SHOW_VIDEOMENU;
        g_application.m_pPlayer->OnAction(action);
      }
    }
    else if( parameter.Equals("record") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer && g_application.m_pPlayer->CanRecord())
      {
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
      }
    }

  }
  else if (execute.Equals("ejecttray"))
  {
    CIoSupport io;
    if (io.GetTrayState() == TRAY_OPEN)
      io.CloseTray();
    else
      io.EjectTray();
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
            fSecs = fSecs = static_cast<float>(atoi(szParam)*60);
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
      CStdString strHeading = g_localizeStrings.Get(13209);
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
    std::vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() < 2)
      return -1;

    g_application.m_guiDialogKaiToast.QueueNotification(params[0],params[1]);
  }
  else if (execute.Equals("cancelalarm"))
    g_alarmClock.stop(parameter);
  else if (execute.Equals("playdvd"))
  {
    CAutorun::PlayDisc();
  }
  else
    return -1;
  return 0;
}
int CUtil::GetMatchingShare(const CStdString& strPath, VECSHARES& vecShares, bool& bIsBookmarkName)
{
  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, testing path/name [%s]", strPath.c_str());

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  CStdString strDest;
  urlDest.GetURLWithoutUserDetails(strDest);
  ForceForwardSlashes(strDest);
  if (!HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();
  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, testing url [%s]", strDest.c_str());

  bIsBookmarkName = false;
  int iIndex = -1;
  int iLength = -1;
  for (int i = 0; i < (int)vecShares.size(); ++i)
  {
    CShare share = vecShares.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (IsOnDVD(share.strPath))
    {
      if (IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the bookmark name
      // since we add the drive status and disc name to the bookmark
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }

    // does it match a bookmark name?
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, comparing name [%s]", strName.c_str());
    if (strPath.Equals(strName))
    {
      bIsBookmarkName = true;
      return i;
    }

    // doesnt match a name, so try the bookmark path
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
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching bookmark at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set bookmark name to true to load the full virtualpath 
          bIsBookmarkName = false;
          if (vecPaths.size() > 1)
            bIsBookmarkName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
    CLog::Log(LOGERROR,"CUtil::GetMatchingShare... no matching bookmark found for [%s]", strDest.c_str());
  return iIndex;
}

CStdString CUtil::TranslateSpecialDir(const CStdString &strSpecial)
{
  CStdString strReturn;
  if (strSpecial[0] == '$')
  {
    if (strSpecial.Equals("$HOME"))
      strReturn = "Q:\\";
    else if (strSpecial.Equals("$SUBTITLES"))
      strReturn = g_stSettings.m_szAlternateSubtitleDirectory;
    else if (strSpecial.Equals("$THUMBNAILS"))
      strReturn = g_stSettings.szThumbnailsDirectory;
    else if (strSpecial.Equals("$SHORTCUTS"))
      strReturn = g_stSettings.m_szShortcutDirectory;
    else if (strSpecial.Equals("$ALBUMS"))
      strReturn = g_stSettings.m_szAlbumDirectory;
    else if (strSpecial.Equals("$RECORDINGS"))
      strReturn = g_stSettings.m_szMusicRecordingDirectory;
    else if (strSpecial.Equals("$SCREENSHOTS"))
      strReturn = g_stSettings.m_szScreenshotsDirectory;
    else if (strSpecial.Equals("$PLAYLISTS"))
      strReturn = (CStdString)g_stSettings.m_szAlbumDirectory + "\\playlists";
    else if (strSpecial.Equals("$CDRIPS"))
      strReturn = g_stSettings.m_strRipPath;
  }
  if (strReturn.IsEmpty())
    CLog::Log(LOGERROR,"Invalid special directory token: %s",strSpecial.c_str());
  return strReturn;
}

void CUtil::DeleteDatabaseDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile("Z:\\db-*.fi", &wfd));
  if (!hFind.isValid())
    return;
  do
  {
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      CStdString strFile = "Z:\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}
bool CUtil::IsLeapYear(int iLYear, int iLMonth, int iLTag, int &iMonMax, int &iWeekDay)  // GeminiServer
{
	// Rückgabewert: FALSE, wenn kein Schaltjahr,   TRUE, wenn Schaltjahr
	bool ret_value;
	int iIsLeapYear;
	if(iLYear%4 == 0 && (iLYear%100 != 0 || iLYear %400 == 0))
	{
		ret_value=TRUE;
		iIsLeapYear = 1;
	}
	else
	{
		ret_value=FALSE;
		iIsLeapYear = 0;
	}
	switch(iLMonth)
		{
			case 1:	iMonMax = 31 ;	
				break;
			case 2:	iMonMax = 28+iIsLeapYear; 
				break;
			case 3:	iMonMax = 31;
				break;
			case 4:	iMonMax = 30;
				break;
			case 5:	iMonMax = 31;
				break;
			case 6:	iMonMax = 30;
				break;
			case 7:	iMonMax = 31;
				break;
			case 8:	iMonMax = 31;
				break;
			case 9:	iMonMax = 30;
				break;
			case 10: iMonMax = 31;
				break;
			case 11: iMonMax = 30;
				break;
			case 12: iMonMax = 31;
				break;
			default: iMonMax = 31;
		}
	// monat: von 1... 12, tag von 1... 31.
	if(iLMonth <= 2)
	{
		iLMonth += 10;
		--iLYear;
	}
	else iLMonth -= 2;
	iWeekDay = (iLTag+(13*iLMonth-1)/5+iLYear+iLYear/4-iLYear/100+iLYear/400)%7;
	return(ret_value);
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  if (iHour == 0) iHour = 24;
	//GeminiServer
	TIME_ZONE_INFORMATION tziNew;
	SYSTEMTIME CurTime;
	SYSTEMTIME NewTime;
	GetLocalTime(&CurTime);
	GetLocalTime(&NewTime);
	int iRescBiases, iHourUTC;
	
  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations  
	int iGMTZone = (tziNew.Bias)/60;                // Cals the GMT Time
  
  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %i",iGMTZone);
	CLog::Log(LOGDEBUG, "-          Bias: %i",tziNew.Bias);
	CLog::Log(LOGDEBUG, "-  DaylightBias: %i",tziNew.DaylightBias);
	CLog::Log(LOGDEBUG, "-  StandardBias: %i",tziNew.StandardBias);
	CLog::Log(LOGDEBUG, "--------------- END ---------------");
  
	if (dwRet == TIME_ZONE_ID_STANDARD)
	{
    iHourUTC = ( ((iHour * 60) + tziNew.Bias) + tziNew.StandardBias ) / 60;
    if (iHour == 24)  iHourUTC = iHourUTC - 24;                 // if iHour is 24h, we must prevent + 1day [+24h!] so -24h!
	}
	else if (dwRet == TIME_ZONE_ID_DAYLIGHT ) 
	{
    iHourUTC = ( ((iHour * 60) + tziNew.Bias) + tziNew.StandardBias + tziNew.DaylightBias) / 60;
    if (iHour == 24)  iHourUTC = iHourUTC - 24;                 // if iHour is 24h, we must prevent + 1day [+24h!] so -24h!
  }
	else if (dwRet == TIME_ZONE_ID_UNKNOWN || dwRet == TIME_ZONE_ID_INVALID)
	{
		if (iHour >12 )
    {
      if (tziNew.Bias < 0)                                      // Max -12h (-720 Minutes)
      {
         iHourUTC = ( ((iHour * 60) + tziNew.Bias) + tziNew.StandardBias ) / 60;
      }
      else if (tziNew.Bias > 0)                                 // Max +12h (720 Minutes)
      {
        iRescBiases = ((tziNew.Bias + tziNew.StandardBias) / 60);
        iHourUTC    = iHour - abs(iRescBiases);                 // We must minus the Bias, to Prevent time > 23
      }
      else if (tziNew.Bias == 0 && iHour == 24 )iHourUTC = 0;   // GMT Zone 0
    }
    else if (iHour < 12 )
    {
      iRescBiases   = ((tziNew.Bias + tziNew.StandardBias )/ 60);
      iHourUTC      = iHour + abs(iRescBiases);
    }
    else 
    {
      iHourUTC = ( ((iHour * 60) + tziNew.Bias) + tziNew.StandardBias ) / 60;
      if (iHourUTC == 24) iHourUTC = 0;                         // if Hour is 24h Must be 0
    }
	}
  
  NewTime.wYear		= (WORD)iYear;    // Now Set the New-,Detected Time Values to System Time!
	NewTime.wMonth	= (WORD)iMonth;
	NewTime.wDay		= (WORD)iDay;	
	NewTime.wHour		= (WORD)iHourUTC;
	NewTime.wMinute	= (WORD)iMinute;

	FILETIME stNewTime, stCurTime;
	SystemTimeToFileTime(&NewTime, &stNewTime);
	SystemTimeToFileTime(&CurTime, &stCurTime);
	NtSetSystemTime(&stNewTime, &stCurTime);

	return true;
}
bool CUtil::XboxAutoDetectionPing(bool bRefresh, CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort, CStdString &strHasClientIP, CStdString &strHasClientInfo, CStdString &strNewClientIP, CStdString &strNewClientInfo )
{
  //GeminiServer
  
  CStdString strWorkTemp;
  CStdString strSendMessage = "ping\0";
  CStdString strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char  sztmp[512], szTemp[512];
	static int	udp_server_socket, inited=0;
	int  cliLen, t1,t2,t3,t4, init_counter=0, life=0;
  struct sockaddr_in	server;
  struct sockaddr_in	cliAddr;
  struct timeval timeout={0,500};
  XNADDR xna;
	DWORD dwState;
  fd_set readfds; 
  bool bState= false;
	if( ( !inited )  || ( bRefresh ) )
	{
		dwState = XNetGetTitleXnAddr(&xna);
		XNetInAddrToString(xna.ina,(char *)strWorkTemp.c_str(),64);

		// Get IP address
		sscanf( (char *)strWorkTemp.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
    if( !t1 ) return false;
    cliLen = sizeof( cliAddr);
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
		if (life == -1 )  return false;
    memset(&(server),0,sizeof(server));
		server.sin_family           = AF_INET;
		server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
		server.sin_port             = htons(iUDPPort);	
    sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
	}
	FD_ZERO(&readfds);
	FD_SET(udp_server_socket, &readfds);
	life = select( 0,&readfds, NULL, NULL, &timeout ); 
  while( life )
	{
    recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen); 
    strWorkTemp = sztmp;
    if( strWorkTemp == strReceiveMessage )
		{
			strWorkTemp.Format("%s;%s;%s;%d;%d\r\n\0",strNickName.c_str(),strFTPUserName.c_str(),strFTPPass.c_str(),iFTPPort,0 );
      sendto(udp_server_socket,(char *)strWorkTemp.c_str(),strlen((char *)strWorkTemp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      strWorkTemp.Format("%d.%d.%d.%d",cliAddr.sin_addr.S_un.S_un_b.s_b1,cliAddr.sin_addr.S_un.S_un_b.s_b2,cliAddr.sin_addr.S_un.S_un_b.s_b3,cliAddr.sin_addr.S_un.S_un_b.s_b4 );

			bool bPing = ( bool )false; // Check if we have this client in our list already, and if not respond with a ping
			// todo: a code to check the list of other clients
			if( bPing ) sendto(udp_server_socket,strSendMessage.c_str(),5,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));  
		}
		else
		{
      sprintf( szTemp, "%d.%d.%d.%d", cliAddr.sin_addr.S_un.S_un_b.s_b1,cliAddr.sin_addr.S_un.S_un_b.s_b2,cliAddr.sin_addr.S_un.S_un_b.s_b3,cliAddr.sin_addr.S_un.S_un_b.s_b4 );
      if (strHasClientIP != szTemp && strHasClientInfo != strWorkTemp)
      {
        strHasClientIP = szTemp;        //This is the Client IP Adress!
        strHasClientInfo  = strWorkTemp; // This is the Client Informations!
        if (strHasClientIP != "" && strHasClientInfo !="")
        {
          strNewClientIP = szTemp;        //This is the Client IP Adress!
          strNewClientInfo = strWorkTemp; // This is the Client Informations!
          bState = true;
        }
      }
      //todo: add it to a list of clients after parsing out user id, password, port, boost capable, etc. >
		}
		timeout.tv_sec=0;
		timeout.tv_usec = 5000;
		FD_ZERO(&readfds);
		FD_SET(udp_server_socket, &readfds);
		life = select( 0,&readfds, NULL, NULL, &timeout );
	}
  return bState;
}

bool CUtil::XboxAutoDetection() // GeminiServer: Xbox Autodetection!
{
  if (g_guiSettings.GetBool("Autodetect.OnOff"))
  {
    static DWORD pingTimer = 0;
    if( timeGetTime() - pingTimer < (DWORD)g_guiSettings.GetInt("Autodetect.PingTime" ) * 1000)
      return false;
    pingTimer = timeGetTime();

    // Todo: Extract Ftp User, PW, Port from Internal FTP Server!
    // Todo: Create a FTP Client for XBMC!
    // Todo: Create a Setting for entering FTP Password and Username!
    CStdString strLabel      = g_localizeStrings.Get(1251); // lbl Xbox Autodetection
    CStdString strNickName   = g_guiSettings.GetString("Autodetect.NickName");
    
    CStdString strSysFtpName = g_guiSettings.GetString("Servers.FTPServerUser");
    CStdString strSysFtpPw   = g_guiSettings.GetString("Servers.FTPServerPassword");

    if(!g_guiSettings.GetBool("Autodetect.SendUserPw"))
    {
      strSysFtpName = "anonymous";
      strSysFtpPw   = "anonymous";
    }
    int iSysFtpPort = 21;

    bool bget = CUtil::XboxAutoDetectionPing(true, strSysFtpName, strSysFtpPw, strNickName, iSysFtpPort,strHasClientIP,strHasClientInfo, strNewClientIP , strNewClientInfo );
    if ( bget )
    {
      //Autodetection String: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit; 
      StringUtils::SplitString(strNewClientInfo,";", arSplit);
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),strHasClientIP.c_str(),strFtpPort.c_str());

        if (g_guiSettings.GetBool("Autodetect.PopUpInfo"))    //PopUp Notification
        {
          CStdString strtemplbl;
          strtemplbl.Format("%s %s",strNickName, strNewClientIP);
          g_application.m_guiDialogKaiToast.QueueNotification(strLabel, strtemplbl);  
        }

        if (g_guiSettings.GetBool("Autodetect.CreateLink"))   //Check if this XBOX is allread in the FileManager List! If Not add it!
        {
          // Add a FTP link to MyFiles! //Todo: If there is a same Name ask to overwrite it!
          if(!g_settings.UpdateBookmark("files", strNickName, "path", strFTPPath) )
          {
            g_settings.AddBookmark("files",    strNickName,  strFTPPath);
            // Todo: Create a FTP link in My Files and PopUp a OK windows with the info, that a FTP bla is created!
          }
        }
        CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", strLabel.c_str(), strNickName.c_str(), strFTPPath.c_str());
        }
    }
    strHasClientIP = strNewClientIP, strHasClientInfo = strNewClientInfo;
  }
  else strHasClientIP ="", strHasClientInfo = "";
  return true;
}
bool CUtil::IsFTP(const CStdString& strFile)
{
  CURL url(strFile);
  if (url.GetProtocol() == "ftp") return true;
  else return false;
}
bool CUtil::CmpNoCase(const char* str1, const char* str2)
{
  int iLen = strlen(str1);
  if ( strlen(str1) != strlen(str2) ) return false;
  for (int i = 0; i < iLen;i++ )
  {
    if (tolower((unsigned char)str1[i]) != tolower((unsigned char)str2[i]) ) return false;
  }
  return true;
}
bool CUtil::GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax )
{
  class CXFUser*	m_pUser;
  std::vector<CXFUser*> users;
  g_application.m_pFileZilla->GetAllUsers(users);
  iUserMax = users.size();
	if (iUserMax > 0)
	{
		//for (int i = 1 ; i < iUserSize; i++){ delete users[i]; }
    m_pUser = users[iFTPUserID];
    strFtpUser1 = m_pUser->GetName();
    if (strFtpUser1.size() != 0) return true;
    else return false;
	}else return false;
}
bool CUtil::SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword)
{
  CStdString strTempUserName;
  class CXFUser*	p_ftpUser;
  std::vector<CXFUser*> v_ftpusers;
  bool bFoundUser = false;
  g_application.m_pFileZilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
	if (iUserSize > 0)
	{
		for (int i = 1 ; i < iUserSize; i++)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName == strFtpUserName) 
      { 
        if (p_ftpUser->SetPassword((char_t*)strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          return true;
        }
        break;
      }
    }
  }
  return false;
}
//GeminiServer SetXBOXNickName
//strXboxNickNameIn: New NickName
//strXboxNickNameOut: Same if it is in NICKNAME Cache
bool CUtil::SetXBOXNickName(CStdString strXboxNickNameIn, CStdString &strXboxNickNameOut)
{
  char pszNickName[64];
  unsigned int uiSize = MAX_NICKNAME;
  bool bfound= false;
  HANDLE hNickName = XFindFirstNickname(true,(LPWSTR)pszNickName,MAX_NICKNAME);
  if (hNickName != INVALID_HANDLE_VALUE)
  {
      do
      { 
        if (strXboxNickNameIn == pszNickName) 
        {
          strXboxNickNameOut = pszNickName; 
          bfound = true;
        }
        else if (strXboxNickNameIn == "") strXboxNickNameOut = "GeminiServer";
        else strXboxNickNameOut = strXboxNickNameIn;
      }while(XFindNextNickname(hNickName,((LPWSTR)pszNickName),uiSize) != false);
      XFindClose(hNickName);
  }
  if(bfound==false) XSetNickname((LPCWSTR)strXboxNickNameOut.c_str(), false);
  
  return true;
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,items,strMask);
    else if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(new CFileItem(*myItems[i]));
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"");
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->m_strPath.Equals(".."))
    {
      CFileItem* pItem = new CFileItem(*myItems[i]);
      item.Add(pItem);
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,item,"");
    }   
  }
  CLog::Log(LOGDEBUG,"done listing!");
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