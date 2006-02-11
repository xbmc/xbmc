
/******************************** Description *********************************/

/*
 *  This module provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.cpp
 */

/********************************* Includes ***********************************/

#pragma once


#include "..\..\stdafx.h"
#include "WebServer.h"
#include "XbmcHttp.h"
#include "..\..\PlayListFactory.h"
#include "..\..\Application.h"
#include "..\..\util.h"
#include "..\..\playlistplayer.h"
#include "..\..\filesystem\HDDirectory.h" 
#include "..\..\filesystem\CDDADirectory.h"
#include "..\..\videodatabase.h"
#include "..\..\guilib\GUIThumbnailPanel.h"
#include "..\..\guilib\GUIButtonControl.h"
#include "..\..\guilib\GUISpinControl.h"
#include "..\..\guilib\GUIListControl.h"
#include "..\..\utils\GUIInfoManager.h"
#include "..\..\picture.h"
#include "..\..\MusicInfoTagLoaderFactory.h"
#include "..\..\utils\MusicInfoScraper.h"
#include "..\..\MusicDatabase.h"
#include "..\..\GUIWindowSlideShow.h"
#include "..\..\GUIWindowVideoFiles.h"
#include "..\..\GUIWindowMusicNav.h"
#include "..\..\GUIWindowFileManager.h"
#include "..\..\GUIWindowPictures.h"
#include "..\..\GUIWindowPrograms.h"
#include "..\..\guilib\GUIButtonScroller.h"

#define XML_MAX_INNERTEXT_SIZE 256
#define MAX_PARAS 10
#define NO_EID -1

#define XBMC_NONE      T("none")

CXbmcHttp* pXbmcHttp;
CXbmcHttpShim* pXbmcHttpShim;

//Response format
CStdString openTag="<li>", closeTag="\n", userHeader="", userFooter="";
bool incWebHeader=true, incWebFooter=true, closeFinalTag=false;

bool autoGetPictureThumbs = true;

/*
** Translation Table as described in RFC1113
*/
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
** Translation Table to decode
*/
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
** encodeblock
**
** encode 3 8-bit binary bytes as 4 '6-bit' characters
*/
void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*
** encode
**
** base64 encode a stream adding padding and line breaks as per spec.
*/
CStdString encodeFileToBase64( CStdString inFilename, int linesize )
{
  unsigned char in[3], out[4];
  int i, len, blocksout = 0;
  CStdString strBase64="";
  FILE *infile;

  infile = fopen( inFilename.c_str(), "rb" );
  if (infile != 0) 
  {
    while( !feof( infile ) ) 
    {
      len = 0;
      for( i = 0; i < 3; i++ ) 
      {
        in[i] = (unsigned char) getc( infile );
        if( !feof( infile ) ) 
          len++;
        else 
          in[i] = 0;
      }
      if( len ) 
      {
        encodeblock( in, out, len );
        for( i = 0; i < 4; i++ )
          strBase64 += out[i];
        blocksout++;
      }
      if( blocksout >= (linesize/4) || feof( infile ) ) 
      {
        if( blocksout )
          strBase64 += "\r"+closeTag ;
        blocksout = 0;
      }
    }
    fclose(infile);
  }
  return strBase64;
}

/*
** decodeblock
**
** decode 4 '6-bit' characters into 3 8-bit binary bytes
*/
void decodeblock( unsigned char in[4], unsigned char out[3] )
{   
    out[ 0 ] = (unsigned char ) ((in[0] << 2 | in[1] >> 4) & 255);
    out[ 1 ] = (unsigned char ) ((in[1] << 4 | in[2] >> 2) & 255);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*
** decode
**
** decode a base64 encoded stream discarding padding, line breaks and noise
*/
bool decodeBase64ToFile( CStdString inString, CStdString outfilename )
{
  unsigned char in[4], out[3], v;
  bool ret=true;
  int i, len ;
  unsigned int ptr=0;
  FILE *outfile;

  try
  {
    outfile = fopen( outfilename.c_str(), "wb" );
    while( ptr < inString.length() )
    {
      for( len = 0, i = 0; i < 4 && ptr < inString.length(); i++ ) 
      {
        v = 0;
        while( ptr < inString.length() && v == 0 ) 
        {
          v = (unsigned char) inString[ptr];
          ptr++;
          v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
          if( v )
            v = (unsigned char) ((v == '$') ? 0 : v - 61);
        }
        if( ptr < inString.length() ) {
          len++;
          if( v ) 
            in[ i ] = (unsigned char) (v - 1);
        }
        else 
          in[i] = 0;
      }
      if( len ) 
      {
        decodeblock( in, out );
        for( i = 0; i < len - 1; i++ )
          putc( out[i], outfile );
      }
    }
    fclose(outfile);
  }
  catch (...)
  {
    ret=false;
  }
  return ret;
}

__int64 fileSize(CStdString filename)
{
  if (CFile::Exists(filename.c_str()))
  {
    __stat64 s64;
    if (CFile::Stat(filename.c_str(), &s64) == 0)
      return s64.st_size;
    else
      return -1;
  }
  else
    return -1;
}



int splitParameter(CStdString parameter, CStdString& command, CStdString paras[], CStdString sep)
//returns -1 if no command else the number of parameters
//assumption: sep.length()==1
{
  unsigned int num=0, p;
  CStdString empty="";

  paras[0]="";
  for (p=0; p<parameter.length(); p++)
  {
    if (parameter.Mid(p,1)==sep)
    {
      if (p<parameter.length()-1)
      {
        if (parameter.Mid(p+1,1)==sep)
        {
          paras[num]+=sep;
          p+=1;
        }
        else
        {
          if (command!="")
          {
            paras[num]=paras[num].Trim();
            num++;
          }
          else
          {
            command=paras[0];
            paras[0]=empty;
            p++; //the ";" after the command is always followed by a space which we can jump over
          }
        }
      }
      else
      {
        if (command!="")
        {
          paras[num]=paras[num].Trim();
          num++;
        }
        else
        {
          command=paras[0];
          paras[0]=empty;
        }
      }
    }
    else
    {
      paras[num]+=parameter.Mid(p,1);
    }
  }
  if (command=="")
    if (paras[0]!="")
    {
      command=paras[0];
      return 0;
    }
    else
      return -1;
  else
  {
    paras[num]=paras[num].Trim();
    return num+1;
  }
}

bool playableFile(CStdString filename)
{
  CStdString strExtension;
  CUtil::GetExtension(filename, strExtension);
  return (CUtil::IsRemote(filename)) || ((CFile::Exists(filename)) && (strExtension!=""));
}

int SetResponse(CStdString response)
{
  if (response.length()>=closeTag.length())
  {
    if ((response.Right(closeTag.length())!=closeTag) && closeFinalTag) 
      return g_applicationMessenger.SetResponse(response+closeTag);
    else;
  }
  else 
    if (closeFinalTag)
      return g_applicationMessenger.SetResponse(response+closeTag);
  return g_applicationMessenger.SetResponse(response);
}

CStdString flushResult(int eid, webs_t wp, CStdString output)
{
  if (output!="")
    if (eid==NO_EID && wp!=NULL)
      websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
    else if (eid!=NO_EID)
      ejSetResult( eid, (char_t *)output.c_str());
    else
      return output;
  return "";
}

int CXbmcHttp::xbmcGetMediaLocation(int numParas, CStdString paras[])
{
  // getmediadirectory&parameter=type;location;options
  // options = showdate, pathsonly
  // returns a listing of
  // <li>label;path;0|1=folder;date

  int iType = -1;
  CStdString strType;
  CStdString strMask;
  CStdString strLocation;
  CStdString strOutput;

  if (numParas < 1)
    return SetResponse(openTag+"Error: must supply media type at minimum");
  else
  {
    if (paras[0].Equals("music"))
      iType = 0;
    else if (paras[0].Equals("video"))
      iType = 1;
    else if (paras[0].Equals("pictures"))
      iType = 2;
    else if (paras[0].Equals("files"))
      iType = 3;
    if (iType < 0)
      return SetResponse(openTag+"Error: invalid media type; valid options are music, video, pictures");

    strType = paras[0].ToLower();
    if (numParas > 1)
      strLocation = paras[1];
  }

  // handle options
  bool bShowDate = false;
  bool bPathsOnly = false;
  if (numParas > 2)
  {
    for (int i = 2; i < numParas; ++i)
    {
      if (paras[i].Equals("showdate"))
        bShowDate = true;
      else if (paras[i].Equals("pathsonly"))
        bPathsOnly = true;
    }
    // pathsonly and showdate are mutually exclusive, pathsonly wins
    if (bPathsOnly)
      bShowDate = false;
  }

  VECSHARES *pShares = NULL;
  enum SHARETYPES { MUSIC, VIDEO, PICTURES, FILES };
  switch(iType)
  {
  case MUSIC:
    {
      pShares = &g_settings.m_vecMyMusicShares;
      strMask = g_stSettings.m_szMyMusicExtensions;
    }
    break;
  case VIDEO:
    {
      pShares = &g_settings.m_vecMyVideoShares;
      strMask = g_stSettings.m_szMyVideoExtensions;
    }
    break;
  case PICTURES:
    {
      pShares = &g_settings.m_vecMyPictureShares;
      strMask = g_stSettings.m_szMyPicturesExtensions;
    }
    break;
  case FILES:
    {
      pShares = &g_settings.m_vecMyFilesShares;
      strMask = "";
    }
    break;
  }

  if (!pShares)
    return SetResponse(openTag+"Error");

  if (!strLocation.IsEmpty())
  {
    VECSHARES vecShares = *pShares;
    bool bIsShareName = false;
    int iIndex = CUtil::GetMatchingShare(strLocation, vecShares, bIsShareName);
    if (iIndex < 0)
    {
      CStdString strError = "Error: invalid location, " + strLocation;
      return SetResponse(openTag+strError);
    }
    if (bIsShareName)
      strLocation = vecShares[iIndex].strPath;
  }

  CFileItemList items;
  if (strLocation.IsEmpty())
  {
    CStdString params[2];
    params[0] = strType;
    params[1] = "appendone";
    if (bPathsOnly)
      params[1] = "pathsonly";
    return xbmcGetShares(2, params);
  }
  else if (!CDirectory::GetDirectory(strLocation, items, strMask))
  {
    CStdString strError = "Error: could not get location, " + strLocation;
    return SetResponse(openTag+strError);
  }

  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  CStdString strLine;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem *item = items[i];
    CStdString strLabel = item->GetLabel();
    strLabel.Replace(";",";;");
    CStdString strPath = item->m_strPath;
    strPath.Replace(";",";;");
    CStdString strFolder = "0";
    if (item->m_bIsFolder)
    {
      if (!CUtil::HasSlashAtEnd(strPath))
        CUtil::AddSlashAtEnd(strPath);
      strFolder = "1";
    }
    strLine = openTag;
    if (!bPathsOnly)
      strLine += strLabel + ";";
    strLine += strPath;
    if (!bPathsOnly)
      strLine += ";" + strFolder;
    if (bShowDate)
    {
      CStdString theDate;
      CUtil::GetDate(item->m_stTime, theDate);
      strLine += ";" + theDate;
    }
    strLine += closeTag;
    strOutput += strLine;
  }
  return SetResponse(strOutput);
}

int CXbmcHttp::xbmcGetShares(int numParas, CStdString paras[])
{
  // returns the share listing in this format:
  // <li>type;name;path
  // literal semicolons are translated into ;;
  // options include the type, and pathsonly boolean

  int iStart = 0;
  int iEnd   = 4;
  bool bShowType = true;
  bool bShowName = true;

  if (numParas > 0)
  {
    if (paras[0].Equals("music"))
    {
      iStart = 0;
      iEnd   = 1;
      bShowType = false;
    }
    else if (paras[0].Equals("video"))
    {
      iStart = 1;
      iEnd   = 2;
      bShowType = false;
    }
    else if (paras[0].Equals("pictures"))
    {
      iStart = 2;
      iEnd   = 3;
      bShowType = false;
    }
    else if (paras[0].Equals("files"))
    {
      iStart = 3;
      iEnd   = 4;
      bShowType = false;
    }
    else
      numParas = 0;
  }

  bool bAppendOne = false;
  if (numParas > 1)
  {
    // special case where getmedialocation calls getshares
    if (paras[1].Equals("appendone"))
      bAppendOne = true;
    else if (paras[1].Equals("pathsonly"))
      bShowName = false;
  }

  CStdString strOutput;
  enum SHARETYPES { MUSIC, VIDEO, PICTURES, FILES };
  for (int i = iStart; i < iEnd; ++i)
  {
    CStdString strType;
    VECSHARES *pShares = NULL;
    switch(i)
    {
    case MUSIC:
      {
        strType = "music";
        pShares = &g_settings.m_vecMyMusicShares;
      }
      break;
    case VIDEO:
      {
        strType = "video";
        pShares = &g_settings.m_vecMyVideoShares;
      }
      break;
    case PICTURES:
      {
        strType = "pictures";
        pShares = &g_settings.m_vecMyPictureShares;
      }
      break;
    case FILES:
      {
        strType = "files";
        pShares = &g_settings.m_vecMyFilesShares;
      }
      break;
    }

    if (!pShares)
      return SetResponse(openTag+"Error");
    
    VECSHARES vecShares = *pShares;
    for (int j = 0; j < (int)vecShares.size(); ++j)
    {
      CShare share = vecShares.at(j);
      CStdString strName = share.strName;
      strName.Replace(";", ";;");
      CStdString strPath = share.strPath;
      strPath.Replace(";", ";;");
      if (!CUtil::HasSlashAtEnd(strPath))
        CUtil::AddSlashAtEnd(strPath);
      CStdString strLine = openTag;
      if (bShowType)
        strLine += strType + ";";
      if (bShowName)
        strLine += strName + ";";
      strLine += strPath;
      if (bAppendOne)
        strLine += ";1";
      strLine += closeTag;
      strOutput += strLine;
    }
  }
  return SetResponse(strOutput);
}

int displayDir(int numParas, CStdString paras[]) {
  //mask = ".mp3|.wma" -> matching files
  //mask = "*" -> just folders
  //mask = "" -> all files and folder
  //option = "1" -> append date&time to file name

  CFileItemList dirItems;
  CStdString output="";

  CStdString  folder, mask="", option="";

  if (numParas==0)
  {
    return SetResponse(openTag+"Error:Missing folder");
  }
  folder=paras[0];
  if (folder.length()<1)
  {
    return SetResponse(openTag+"Error:Missing folder");
  }
  if (numParas>1)
    mask=paras[1];
    if (numParas>2)
      option=paras[2];

  IDirectory *pDirectory = CFactoryDirectory::Create(folder);

  if (!pDirectory) 
  {
    return SetResponse(openTag+"Error");  
  }
  pDirectory->SetMask(mask);
  CStdString tail=folder.Right(folder.length()-1);
  bool bResult=((pDirectory->Exists(folder))||(tail==":")||(tail==":\\")||(tail==":/"));
  if (!bResult)
  {
    return SetResponse(openTag+"Error:Not folder");
  }
  bResult=pDirectory->GetDirectory(folder,dirItems);
  if (!bResult)
  {
    return SetResponse(openTag+"Error:Not folder");
  }

  dirItems.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  CStdString aLine="";
  for (int i=0; i<dirItems.Size(); ++i)
  {
    CFileItem *itm = dirItems[i];
    if (mask=="*" || (mask =="" && itm->m_bIsFolder))
      if (!CUtil::HasSlashAtEnd(itm->m_strPath))
        aLine=closeTag+openTag + itm->m_strPath + "\\" ;
      else
        aLine=closeTag+openTag + itm->m_strPath ;
    else
      if (!itm->m_bIsFolder)
        aLine=closeTag+openTag + itm->m_strPath;
    if (aLine!="")
    {
      if (option=="1") {
        CStdString theDate;
        CUtil::GetDate(itm->m_stTime,theDate) ;
        output+=aLine+"  ;" + theDate ;
      }
      else
        output+=aLine;
      aLine="";
    }
  }
  return SetResponse(output);
}

void SetCurrentMediaItem(CFileItem& newItem)
{
  //  No audio file, we are finished here
  if (!newItem.IsAudio() )
    return;

  //  Get a reference to the item's tag
  CMusicInfoTag& tag=newItem.m_musicInfoTag;
  //  we have a audio file.
  //  Look if we have this file in database...
  bool bFound=false;
  if (g_musicDatabase.Open())
  {
    CSong song;
    bFound=g_musicDatabase.GetSongByFileName(newItem.m_strPath, song);
    tag.SetSong(song);
    g_musicDatabase.Close();
  }
  if (!bFound && g_guiSettings.GetBool("MusicFiles.UseTags"))
  {
    //  ...no, try to load the tag of the file.
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader(factory.CreateLoader(newItem.m_strPath));
    //  Do we have a tag loader for this file type?
    if (pLoader.get() != NULL)
      pLoader->Load(newItem.m_strPath,tag);
  }

  //  If we have tag information, ...
  if (tag.Loaded())
  {
    g_infoManager.SetCurrentSongTag(tag);
  }
}

void AddItemToPlayList(const CFileItem* pItem, int playList, int sortMethod, CStdString mask)
//if playlist==-1 then use slideshow
{
  if (pItem->m_bIsFolder)
  {
    // recursive
    if (pItem->IsParentFolder()) return;
    CStdString strDirectory=pItem->m_strPath;
    CFileItemList items;
    IDirectory *pDirectory = CFactoryDirectory::Create(strDirectory);
    if (mask!="")
      pDirectory->SetMask(mask);
    bool bResult=pDirectory->GetDirectory(strDirectory,items);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
    for (int i=0; i < items.Size(); ++i)
      AddItemToPlayList(items[i], playList, sortMethod, mask);
  }
  else
  {
    //selected item is a file, add it to playlist
    if (playList==-1)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (!pSlideShow)
        return ;
      pSlideShow->Add(pItem->m_strPath);
    }
    else
    {
      PLAYLIST::CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      g_playlistPlayer.GetPlaylist(playList).Add(playlistItem);
    }

  }
}

void LoadPlayListOld(const CStdString& strPlayList, int playList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    if (!pPlayList->Load(strPlayList))
      return; 
    CPlayList& playlist=g_playlistPlayer.GetPlaylist( playList );
    playlist.Clear();
    for (int i=0; i < (int)pPlayList->size(); ++i)
    {
      const CPlayList::CPlayListItem& playListItem =(*pPlayList)[i];
      CStdString strLabel=playListItem.GetDescription();
      if (strLabel.size()==0) 
        strLabel=CUtil::GetFileName(playListItem.GetFileName());

      CPlayList::CPlayListItem playlistItem;
      playlistItem.SetFileName(playListItem.GetFileName());
      playlistItem.SetDescription(strLabel);
      playlistItem.SetDuration(playListItem.GetDuration());
      playlist.Add(playlistItem);
    }

    g_playlistPlayer.SetCurrentPlaylist(playList);
    g_applicationMessenger.PlayListPlayerPlay(0);
    
    // set current file item
    CFileItem item(playlist[0].GetDescription());
    item.m_strPath = playlist[0].GetFileName();
    SetCurrentMediaItem(item);
  }
}

bool LoadPlayList(CStdString strPath, int iPlaylist, bool clearList, bool autoStart)
{
  //CStdString strPath = item.m_strPath;
  CFileItem *item = new CFileItem(CUtil::GetFileName(strPath.c_str()));
  item->m_strPath=strPath;
  if (item->IsInternetStream())
  {
    //we got an url, create a dummy .strm playlist,
    //pPlayList->Load will handle loading it from url instead of from a file
    strPath = "temp.strm";
  }
  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPath));
  if ( NULL == pPlayList.get())
    return false;
  if (!pPlayList->Load(item->m_strPath))
    return false;

  CPlayList& playlist = (*pPlayList);

  if (playlist.size() == 0)
    return false;

  if ((playlist.size() == 1) && (autoStart))
  {
    // just 1 song? then play it (no need to have a playlist of 1 song)
    CPlayList::CPlayListItem item = playlist[0];
    g_applicationMessenger.MediaPlay(CFileItem(item).m_strPath);
    //g_application.PlayFile(CFileItem(item));
    return true;
  }

  if (clearList)
    g_playlistPlayer.GetPlaylist(iPlaylist).Clear();

  // if autoshuffle playlist on load option is enabled
  //  then shuffle the playlist
  // (dont do this for shoutcast .pls files)
  if (playlist.size())
  {
    const CPlayList::CPlayListItem& playListItem = playlist[0];
    if (!playListItem.IsShoutCast() && g_guiSettings.GetBool("MusicPlaylist.ShufflePlaylistsOnLoad"))
      pPlayList->Shuffle();
  }

  // add each item of the playlist to the playlistplayer
  for (int i = 0; i < (int)pPlayList->size(); ++i)
  {
    const CPlayList::CPlayListItem& playListItem = playlist[i];
    CStdString strLabel = playListItem.GetDescription();
    if (strLabel.size() == 0)
      strLabel = CUtil::GetTitleFromPath(playListItem.GetFileName());

    CPlayList::CPlayListItem playlistItem;
    playlistItem.SetDescription(playListItem.GetDescription());
    playlistItem.SetDuration(playListItem.GetDuration());
    playlistItem.SetFileName(playListItem.GetFileName());
    g_playlistPlayer.GetPlaylist( iPlaylist ).Add(playlistItem);
  }
  if (autoStart)
    if (g_playlistPlayer.GetPlaylist( iPlaylist ).size() )
    {
      CPlayList& playlist = g_playlistPlayer.GetPlaylist( iPlaylist );
      const CPlayList::CPlayListItem& item = playlist[0];
      g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
      g_playlistPlayer.Reset();
      g_playlistPlayer.Play(0);
      return true;
    } 
    else
      return false;
  else
    return true;
  return false;
}

CXbmcHttp::CXbmcHttp()
{
  CKey temp;
  key = temp;
}

CXbmcHttp::~CXbmcHttp()
{
  CLog::Log(LOGDEBUG, "xbmcHttp ends");
}

int CXbmcHttp::xbmcAddToPlayList(int numParas, CStdString paras[])
{
  //parameters=playList;mask
  CStdString strFileName;
  CStdString mask="";
  bool changed=false;
  int playList ;

  if (numParas==0)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    if (numParas==1) //no playlist and no mask
      playList=g_playlistPlayer.GetCurrentPlaylist();
    else
    {
      playList=atoi(paras[1]);
      if (playList==-1)
        playList=g_playlistPlayer.GetCurrentPlaylist();
      if(numParas>2) //includes mask
        mask=paras[2];
    }
    strFileName=paras[0] ;
    CFileItem *pItem = new CFileItem(strFileName);
    pItem->m_strPath=strFileName.c_str();
    if (pItem->IsPlayList())
      changed=LoadPlayList(pItem->m_strPath, playList, false, false);
    else
    {
      IDirectory *pDirectory = CFactoryDirectory::Create(pItem->m_strPath);
      bool bResult=pDirectory->Exists(pItem->m_strPath);
      pItem->m_bIsFolder=bResult;
      pItem->m_bIsShareOrDrive=false;
      if (bResult || CFile::Exists(pItem->m_strPath))
      {
        AddItemToPlayList(pItem, playList, 0, mask);
        changed=true;
      }
    }
    delete pItem;
    if (changed)
    {
      g_playlistPlayer.HasChanged();
      return SetResponse(openTag+"OK");
    }
    else
      return SetResponse(openTag+"Error");
  }
}


int CXbmcHttp::xbmcGetTagFromFilename(int numParas, CStdString paras[]) 
{
  CStdString strFileName;
  if (numParas==0) {
    return SetResponse(openTag+"Error:Missing Parameter");
  }
  strFileName=CUtil::GetFileName(paras[0].c_str());
  CFileItem *pItem = new CFileItem(strFileName);
  pItem->m_strPath=paras[0].c_str();
  if (!pItem->IsAudio())
  {
    delete pItem;
    return SetResponse(openTag+"Error:Not Audio");
  }
  CMusicInfoTag& tag=pItem->m_musicInfoTag;
  bool bFound=false;
  CSong song;
  if (g_musicDatabase.Open())
  {
    bFound=g_musicDatabase.GetSongByFileName(pItem->m_strPath, song);
    g_musicDatabase.Close();
  }
  if (bFound)
  {
    SYSTEMTIME systime;
    systime.wYear=song.iYear;
    tag.SetReleaseDate(systime);
    tag.SetTrackNumber(song.iTrack);
    tag.SetAlbum(song.strAlbum);
    tag.SetArtist(song.strArtist);
    tag.SetGenre(song.strGenre);
    tag.SetTitle(song.strTitle);
    tag.SetDuration(song.iDuration);
    tag.SetLoaded(true);
  }
  else
    if (g_guiSettings.GetBool("MusicFiles.UseTags"))
    {
      // get correct tag parser
      CMusicInfoTagLoaderFactory factory;
      auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
      if (NULL != pLoader.get())
      {            
        // get id3tag
        if ( !pLoader->Load(pItem->m_strPath,tag))
          tag.SetLoaded(false);
      }
      else
      {
        return SetResponse(openTag+"Error:Could not load TagLoader");
      }
    }
    else
    {
      return SetResponse(openTag+"Error:System not set to use tags");
      
    }
  if (tag.Loaded())
  {
    CStdString output, tmp;

    output = openTag+"Artist:" + tag.GetArtist();
    output += closeTag+openTag+"Album:" + tag.GetAlbum();
    output += closeTag+openTag+"Title:" + tag.GetTitle();
    tmp.Format("%i", tag.GetTrackNumber());
    output += closeTag+openTag+"Track number:" + tmp;
    tmp.Format("%i", tag.GetDuration());
    output += closeTag+openTag+"Duration:" + tmp;
    output += closeTag+openTag+"Genre:" + tag.GetGenre();
    SYSTEMTIME stTime;
    tag.GetReleaseDate(stTime);
    tmp.Format("%i", stTime.wYear);
    output += closeTag+openTag+"Release year:" + tmp;
    pItem->SetMusicThumb();
    if (pItem->HasThumbnail())
      output += closeTag+openTag+"Thumb:" + pItem->GetThumbnailImage() ;
    else {
      //There isn't a thumbnail but let's still return the appropriate filename if one did exist
      CStdString strThumb, strPath, strFileName;
      if (!pItem->m_bIsFolder)
        CUtil::Split(pItem->m_strPath, strPath, strFileName);
      else
      {
        strPath=pItem->m_strPath;
        if (CUtil::HasSlashAtEnd(strPath))
          strPath.Delete(strPath.size()-1);
      }
      CStdString strAlbum;
      if (pItem->m_musicInfoTag.Loaded())
        strAlbum=pItem->m_musicInfoTag.GetAlbum();
      if (!pItem->m_bIsFolder)
      {
        CUtil::GetAlbumThumb(strAlbum, strPath, strThumb);
        output += closeTag+openTag+"Thumb:[None] " + strThumb ;
      }
    }
    delete pItem;
    return SetResponse(output);
  }
  else
  {
    delete pItem;
    return SetResponse(openTag+"Error:No tag info");
  }
}

int CXbmcHttp::xbmcClearPlayList(int numParas, CStdString paras[])
{
  int playList ;
  if (numParas==0)
    playList = g_playlistPlayer.GetCurrentPlaylist() ;
  else
    playList=atoi(paras[0]) ;
  g_playlistPlayer.GetPlaylist( playList ).Clear();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcGetDirectory(int numParas, CStdString paras[])
{
  if (numParas>0)
    return displayDir(numParas, paras);
  else
    return SetResponse(openTag+"Error:No path") ;
}

int CXbmcHttp::xbmcGetMovieDetails(int numParas, CStdString paras[])
{
  if (numParas>0)
  {
    CFileItem *item = new CFileItem(paras[0]);
    item->m_strPath = paras[0].c_str() ;
    if (item->IsVideo()) {
      CVideoDatabase m_database;
      CIMDBMovie aMovieRec;
      m_database.Open();
      if (m_database.HasMovieInfo(paras[0].c_str()))
      {
        CStdString thumb, output, tmp;
        m_database.GetMovieInfo(paras[0].c_str(),aMovieRec);
        tmp.Format("%i", aMovieRec.m_iYear);
        output = closeTag+openTag+"Year:" + tmp;
        output += closeTag+openTag+"Director:" + aMovieRec.m_strDirector;
        output += closeTag+openTag+"Title:" + aMovieRec.m_strTitle;
        output += closeTag+openTag+"Plot:" + aMovieRec.m_strPlot;
        output += closeTag+openTag+"Genre:" + aMovieRec.m_strGenre;
        CStdString strRating;
        strRating.Format("%3.3f", aMovieRec.m_fRating);
        if (strRating=="") strRating="0.0";
        output += closeTag+openTag+"Rating:" + strRating;
        output += closeTag+openTag+"Cast:" + aMovieRec.m_strCast;
        CUtil::GetVideoThumbnail(aMovieRec.m_strIMDBNumber,thumb);
        if (!CFile::Exists(thumb))
          thumb = "[None] " + thumb;
        output += closeTag+openTag+"Thumb:" + thumb;
        m_database.Close();
        delete item;
        return SetResponse(output);
      }
      else
      {
        m_database.Close();
        delete item;
        return SetResponse(openTag+"Error:Not found");
      }
    }
    else
    {
      delete item;
      return SetResponse(openTag+"Error:Not a video") ;
    }
  }
  else
    return SetResponse(openTag+"Error:No file name") ;
}

int CXbmcHttp::xbmcGetCurrentlyPlaying()
{
  CStdString output="", tmp="", tag="";
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pSlideShow)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    {
      output=openTag+"Filename:"+pSlideShow->GetCurrentSlide();
      output+=closeTag+openTag+"Type:Picture" ;
      int width=0, height=0;
      pSlideShow->GetCurrentSlideInfo(width, height);
      tmp.Format("%i",width);
      output+=closeTag+openTag+"Width:" + tmp ;
      tmp.Format("%i",height);
      output+=closeTag+openTag+"Height:" + tmp ;
      CStdString thumb, picFn=pSlideShow->GetCurrentSlide();
      CUtil::GetThumbnail(picFn,thumb);
      if (!CFile::Exists(thumb))
      {
        if (autoGetPictureThumbs)
        {
          CPicture pic;
          pic.CreateThumbnail(picFn);
          CUtil::GetThumbnail(picFn,thumb);
        }
        if (!CFile::Exists(thumb))
          thumb = "[None] " + thumb;
      }
      output+=closeTag+openTag+"Thumb:"+thumb;
      tag="1";
    } 
  CStdString fn=g_application.CurrentFile();
  if (fn=="")
  {
    if (tag=="")
      output=openTag+"Filename:[Nothing Playing]";
  }
  else
  { 
    if (tag=="")
      output=openTag+"Filename:"+fn;
    else
      output+=closeTag+openTag+"Filename"+tag+":"+fn;
    tmp.Format("%i",g_playlistPlayer.GetCurrentSong());
    output+=closeTag+openTag+"SongNo:"+tmp;
    __int64 lPTS=-1;
    if (g_application.IsPlayingVideo()) {
      lPTS = (__int64)(g_application.GetTime() * 10);
      output+=closeTag+openTag+"Type"+tag+":Video" ;
      const CIMDBMovie& tagVal=g_infoManager.GetCurrentMovie();
      if (tagVal.m_strTitle!="")
        output+=closeTag+openTag+"Title"+tag+":"+tagVal.m_strTitle ;
      if (tagVal.m_strGenre!="")
        output+=closeTag+openTag+"Genre"+tag+":"+tagVal.m_strGenre;
    }
    else if (g_application.IsPlayingAudio()) {
      lPTS = (__int64)(g_application.GetTime() * 10);
      output+=closeTag+openTag+"Type"+tag+":Audio" ;
      const CMusicInfoTag& tagVal=g_infoManager.GetCurrentSongTag();
      if (tagVal.GetTitle()!="")
        output+=closeTag+openTag+"Title"+tag+":"+tagVal.GetTitle() ;
      if (tagVal.GetArtist()!="")
        output+=closeTag+openTag+"Artist"+tag+":"+tagVal.GetArtist() ;
      if (tagVal.GetAlbum()!="")
        output+=closeTag+openTag+"Album"+tag+":"+tagVal.GetAlbum() ;
      if (tagVal.GetGenre()!="")
        output+=closeTag+openTag+"Genre"+tag+":"+tagVal.GetGenre() ;
      if (tagVal.GetYear()!="")
        output+=closeTag+openTag+"Year"+tag+":"+tagVal.GetYear() ;
      if (!g_infoManager.GetMusicLabel(211).IsEmpty())
        output+=closeTag+openTag+"Bitrate"+tag+":"+g_infoManager.GetMusicLabel(211);  // MUSICPLAYER_BITRATE
      if (!g_infoManager.GetMusicLabel(216).IsEmpty())
        output+=closeTag+openTag+"Samplerate"+tag+":"+g_infoManager.GetMusicLabel(216);  // MUSICPLAYER_SAMPLERATE
    }
    if (fn.Find("://")<0)
    {
      CStdString thumb;
      CUtil::GetThumbnail(fn,thumb);
      if (!CFile::Exists(thumb))
        thumb = "[None] " + thumb;
      output+=closeTag+openTag+"Thumb"+tag+":"+thumb;
    }
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsPaused()) 
      output+=closeTag+openTag+"Paused:True" ;
    else
      output+=closeTag+openTag+"Paused:False" ;
    if (g_application.IsPlaying()) 
      output+=closeTag+openTag+"Playing:True" ;
    else
      output+=closeTag+openTag+"Playing:False" ;
    if (lPTS!=-1)
    {          
      int hh = (int)(lPTS / 36000) % 100;
      int mm = (int)((lPTS / 600) % 60);
      int ss = (int)((lPTS /  10) % 60);
      if (hh >=1)
      {
        tmp.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
      }
      else
      {
        tmp.Format("%02.2i:%02.2i",mm,ss);
      }
      output+=closeTag+openTag+"Time:"+tmp;
      CStdString strTotalTime;
      unsigned int tmpvar = (unsigned int)g_application.GetTotalTime();
      if(tmpvar != 0)
      {
        int hh = tmpvar / 3600;
        int mm  = (tmpvar-hh*3600) / 60;
        int ss = (tmpvar-hh*3600) % 60;
        if (hh >=1)
        {
          tmp.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
        }
        else
        {
          tmp.Format("%02.2i:%02.2i",mm,ss);
        }
        output+=closeTag+openTag+"Duration:"+tmp;
      }
      tmp.Format("%i",(int)g_application.GetPercentage());
      output+=closeTag+openTag+"Percentage:"+tmp ;
      _int64 fs=fileSize(fn);
      if (fs>-1)
      {
        tmp.Format("%I64d",fs);
        output+=closeTag+openTag+"File size:"+tmp ;
      }
    }
  }
  return SetResponse(output);
}

int CXbmcHttp::xbmcGetPercentage()
{
  if (g_application.m_pPlayer)
  {
    CStdString tmp;
    tmp.Format("%i",(int)g_application.GetPercentage());
    return SetResponse(openTag + tmp ) ;
  }
  else
    return SetResponse(openTag+"Error");
}

int CXbmcHttp::xbmcSeekPercentage(int numParas, CStdString paras[], bool relative)
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    if (g_application.m_pPlayer)
    {
      float percent=(float)atof(paras[0].c_str());
      if (relative)
      {
        double newPos = g_application.GetTime() + percent * 0.01 * g_application.GetTotalTime();
        if ((newPos>=0) && (newPos/1000<=g_infoManager.GetTotalPlayTime()))
        {
          g_application.SeekTime(newPos);
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Out of range");
      }
      else
      {
        g_application.SeekPercentage(percent);
        return SetResponse(openTag+"OK");
      }
    }
    else
      return SetResponse(openTag+"Error:Loading mPlayer");
  }
}

int CXbmcHttp::xbmcSetVolume(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    int iPercent = atoi(paras[0].c_str());
    g_application.SetVolume(iPercent);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGetVolume()
{
  CStdString tmp;
  tmp.Format("%i",g_application.GetVolume());
  return SetResponse(openTag + tmp);
}

int CXbmcHttp::xbmcClearSlideshow()
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error:Could not create slideshow");
  else
  {
    pSlideShow->Reset();
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcPlaySlideshow(int numParas, CStdString paras[] )
{ // (filename(;1)) -> 1 indicates recursive
    int recursive;
    if (numParas>1)
      recursive=atoi(paras[1].c_str());
    else
      recursive=0;
    CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, recursive, 0, 0);
    if (numParas==0)
      msg.SetStringParam("");
    else
      msg.SetStringParam(paras[0]);
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
    return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcSlideshowSelect(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing filename");
  else
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return SetResponse(openTag+"Error:Could not create slideshow");
    else
    {
      pSlideShow->Select(paras[0]);
      return SetResponse(openTag+"OK");
    }
  }
}

int CXbmcHttp::xbmcAddToSlideshow(int numParas, CStdString paras[])
//filename (;mask)
{
  CStdString mask="";
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    if (numParas>1)
      mask=paras[1];
    CFileItem *pItem = new CFileItem(paras[0]);
    pItem->m_strPath=paras[0].c_str();
    IDirectory *pDirectory = CFactoryDirectory::Create(pItem->m_strPath);
    if (mask!="")
      pDirectory->SetMask(mask);
    bool bResult=pDirectory->Exists(pItem->m_strPath);
    pItem->m_bIsFolder=bResult;
    pItem->m_bIsShareOrDrive=false;
    AddItemToPlayList(pItem, -1, 0, mask); //add to slideshow
    delete pItem;
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcSetPlaySpeed(int numParas, CStdString paras[])
{
  if (numParas>0) {
    g_application.SetPlaySpeed(atoi(paras[0]));
    return SetResponse(openTag+"OK");
  }
  else
    return SetResponse(openTag+"Error:Missing parameter");
}

int CXbmcHttp::xbmcGetPlaySpeed()
{
  CStdString strSpeed;
  strSpeed.Format("%i", g_application.GetPlaySpeed());
  return SetResponse(openTag + strSpeed );
}

int CXbmcHttp::xbmcGetGUIDescription()
{
  CStdString strWidth, strHeight;
  strWidth.Format("%i", g_graphicsContext.GetWidth());
  strHeight.Format("%i", g_graphicsContext.GetHeight());
  return SetResponse(openTag+"Width:" + strWidth + closeTag+openTag+"Height:" + strHeight  );
}

int CXbmcHttp::xbmcGetGUIStatus()
{
  CStdString output, tmp, strTmp;

  output = closeTag+openTag+"MusicPath:" + ((CGUIWindowMusicNav *)m_gWindowManager.GetWindow(WINDOW_MUSIC_FILES))->CurrentDirectory().m_strPath;
  output += closeTag+openTag+"VideoPath:" + ((CGUIWindowVideoFiles *)m_gWindowManager.GetWindow(WINDOW_VIDEOS))->CurrentDirectory().m_strPath;
  output += closeTag+openTag+"PicturePath:" + ((CGUIWindowPictures *)m_gWindowManager.GetWindow(WINDOW_PICTURES))->CurrentDirectory().m_strPath;
  output += closeTag+openTag+"ProgramsPath:" + ((CGUIWindowPrograms *)m_gWindowManager.GetWindow(WINDOW_PROGRAMS))->CurrentDirectory().m_strPath;
  output += closeTag+openTag+"FilesPath1:" + ((CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES))->CurrentDirectory(0).m_strPath;
  output += closeTag+openTag+"FilesPath1:" + ((CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES))->CurrentDirectory(1).m_strPath;
  int iWin=m_gWindowManager.GetActiveWindow();
  CGUIWindow* pWindow=m_gWindowManager.GetWindow(iWin);  
  tmp.Format("%i", iWin);
  output += openTag+"ActiveWindow:" + tmp;
  if (pWindow)
  {
    CStdString strLine;
    wstring wstrLine;
    wstrLine=g_localizeStrings.Get(iWin);
    CUtil::Unicode2Ansi(wstrLine,strLine);
    output += closeTag+openTag+"ActiveWindowName:" + strLine ; 
    int iControl=pWindow->GetFocusedControl();
    CGUIControl* pControl=(CGUIControl* )pWindow->GetControl(iControl);
    strLine.Format("%d",(int)pControl->GetID());
    output += closeTag+openTag+"ControlId:" + strLine;
    
    if (pControl)
    {
      strTmp = pControl->GetDescription();
      if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        output += closeTag+openTag+"Type:Button";
        if (strTmp!="")
          output += closeTag+openTag+"Description:" + strTmp;
        strTmp = ((CGUIButtonControl *)pControl)->GetClickAction();
        if (strTmp!="")
          output += closeTag+openTag+"Execution:" + strTmp;
        long lHyperLinkWindowID = ((CGUIButtonControl *)pControl)->GetHyperLink();
        if (lHyperLinkWindowID != WINDOW_INVALID)
        {
          CGUIWindow *pHyperWindow = m_gWindowManager.GetWindow(lHyperLinkWindowID);
          if (pHyperWindow)
          {
            int iHyperControl=pHyperWindow->GetFocusedControl();
            CGUIControl* pHyperControl=(CGUIControl* )pHyperWindow->GetControl(iHyperControl);
            if (pHyperControl)
            {
              output += closeTag+openTag+"DescriptionSub:" + (CStdString) ((CGUIButtonControl *)pHyperControl)->GetLabel();
              strTmp.Format("%d", ((CGUIButtonControl *)pHyperControl)->GetID());
              output += closeTag+openTag+"ControlIdSub:" + strTmp;
              output += closeTag+openTag+"ExecutionSub:" + ((CGUIButtonControl *)pHyperControl)->GetClickAction();
            }
          }
        }
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTONBAR)
      {
        output += closeTag+openTag+"Type:ButtonBar"+closeTag+openTag+"Description:" + strTmp;
        strLine.Format("%d",((CGUIButtonScroller *)pControl)->GetActiveButton());
        output += closeTag+openTag+"ActiveButton:" + strLine;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_SPIN)
      {
        output += closeTag+openTag+"Type:Spin"+closeTag+openTag+"Description:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_THUMBNAIL)
      {
        output += closeTag+openTag+"Type:ThumbNail"+closeTag+openTag+"Description:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LIST)
      {
        output += closeTag+openTag+"Type:List"+closeTag+openTag+"Description:" + strTmp;
      }
    }
  }
  return SetResponse(output);
}

int CXbmcHttp::xbmcGetThumb(int numParas, CStdString paras[])
{
  CStdString thumb="";
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    if (CUtil::IsRemote(paras[0]))
    {
      CStdString strDest="Z:\\xbmcDownloadFile.tmp";
      CFile::Cache(paras[0], strDest.c_str(),NULL,NULL) ;
      if (CFile::Exists(strDest))
      {
        thumb=encodeFileToBase64(strDest,80);
        ::DeleteFile(strDest.c_str());
      }
      else
      {
        return SetResponse(openTag+"Error");
      }
    }
    else
      thumb=encodeFileToBase64(paras[0],80);
    return SetResponse(thumb) ;
  }
}

int CXbmcHttp::xbmcGetThumbFilename(int numParas, CStdString paras[])
{
  CStdString thumbFilename="";

  if (numParas>1)
  {
    CUtil::GetAlbumThumb(paras[0],paras[1],thumbFilename,false);
    return SetResponse(openTag+thumbFilename ) ;
  }
  else
    return SetResponse(openTag+"Error:Missing parameter (album;filename)") ;
}

int CXbmcHttp::xbmcPlayerPlayFile(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    CFileItem item(paras[0], FALSE);
    if (item.IsPlayList())
    {
      LoadPlayList(paras[0], g_playlistPlayer.GetCurrentPlaylist(), false, false);
      g_applicationMessenger.PlayListPlayerPlay(0);
      return SetResponse(openTag+"OK:PlayList");
    }
    else
    {
      if (playableFile(paras[0]))
      {
        g_applicationMessenger.MediaPlay(paras[0]);
        return SetResponse(openTag+"OK");
      }
      else
        return SetResponse(openTag+"Error:File not found");
    }
  }
}



int CXbmcHttp::xbmcGetCurrentPlayList()
{
  CStdString tmp;
  tmp.Format("%i", g_playlistPlayer.GetCurrentPlaylist());
  return SetResponse(openTag + tmp  );
}

int CXbmcHttp::xbmcSetCurrentPlayList(int numParas, CStdString paras[])
{
  if (numParas<1) 
    return SetResponse(openTag+"Error:Missing playlist") ;
  else {
    g_playlistPlayer.SetCurrentPlaylist(atoi(paras[0].c_str()));
    return SetResponse(openTag+"OK") ;
  }
}

int CXbmcHttp::xbmcGetPlayListContents(int numParas, CStdString paras[])
{
  CStdString list="";
  int playList;

  if (numParas<1) 
    playList=g_playlistPlayer.GetCurrentPlaylist();
  else
    playList=atoi(paras[0]);
  CPlayList& thePlayList = g_playlistPlayer.GetPlaylist(playList);
  if (thePlayList.size()==0)
    list=openTag+"[Empty]" ;
  else
    for (int i=0; i< thePlayList.size(); i++) {
      const CPlayList::CPlayListItem& item=thePlayList[i];
      list += closeTag+openTag + item.GetFileName();
    }
  return SetResponse(list) ;
}

int CXbmcHttp::xbmcGetSlideshowContents()
{
  CStdString list="";
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error");
  else
  {
    vector<CStdString> slideshowContents = pSlideShow->GetSlideShowContents();
    if ((int)slideshowContents.size()==0)
      list=openTag+"[Empty]" ;
    else
    for (int i = 0; i < (int)slideshowContents.size(); ++i)
      list += closeTag+openTag + slideshowContents[i];
    return SetResponse(list) ;
  }
}

int CXbmcHttp::xbmcGetPlayListSong(int numParas, CStdString paras[])
{
  CStdString Filename;
  int iSong;

  if (numParas<1) 
  {
    CStdString tmp;
    tmp.Format("%i", g_playlistPlayer.GetCurrentSong());
    return SetResponse(openTag + tmp );
  }
  else {
    CPlayList thePlayList;
    iSong=atoi(paras[0]);  
    if (iSong!=-1){
      thePlayList=g_playlistPlayer.GetPlaylist( g_playlistPlayer.GetCurrentPlaylist() );
      if (thePlayList.size()>iSong) {
        Filename=thePlayList[iSong].GetFileName();
        return SetResponse(openTag + Filename );
      }
    }
  }
  return SetResponse(openTag+"Error");
}

int CXbmcHttp::xbmcSetPlayListSong(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing song number");
  else
  {
    g_playlistPlayer.Play(atoi(paras[0].c_str()));
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcPlayListNext()
{
  g_playlistPlayer.PlayNext();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcPlayListPrev()
{
  g_playlistPlayer.PlayPrevious();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcRemoveFromPlayList(int numParas, CStdString paras[])
{
  if (numParas>0)
  {
    if (numParas==1)
      g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).Remove(paras[0]) ;
    else
      g_playlistPlayer.GetPlaylist(atoi(paras[1])).Remove(paras[0]) ;
    return SetResponse(openTag+"OK");
  }
  else
    return SetResponse(openTag+"Error:Missing parameter");
}

CKey CXbmcHttp::GetKey()
{
  return key;
}

void CXbmcHttp::ResetKey()
{
  CKey newKey;
  key = newKey;
}

int CXbmcHttp::xbmcSetKey(int numParas, CStdString paras[])
{
  DWORD dwButtonCode=0;
  BYTE bLeftTrigger=0, bRightTrigger=0;
  float fLeftThumbX=0.0f, fLeftThumbY=0.0f, fRightThumbX=0.0f, fRightThumbY=0.0f ;
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameters");
    
  else
  {
    dwButtonCode=(DWORD) atoi(paras[0]);
    if (numParas>1) {
      bLeftTrigger=(BYTE) atoi(paras[1]) ;
      if (numParas>2) {
        bRightTrigger=(BYTE) atoi(paras[2]) ;
        if (numParas>3) {
          fLeftThumbX=(float) atof(paras[3]) ;
          if (numParas>4) {
            fLeftThumbY=(float) atof(paras[4]) ;
            if (numParas>5) {
              fRightThumbX=(float) atof(paras[5]) ;
              if (numParas>6)
                fRightThumbY=(float) atof(paras[6]) ;
            }
          }
        }
      }
    }
    CKey tempKey(dwButtonCode, bLeftTrigger, bRightTrigger, fLeftThumbX, fLeftThumbY, fRightThumbX, fRightThumbY) ;
    key = tempKey ;
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcAction(int numParas, CStdString paras[], int theAction)
{
  bool showingSlideshow=(m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW);

  switch(theAction)
  {
  case 1:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        CAction action;
        action.wID = ACTION_PAUSE;
        pSlideShow->OnAction(action);    
      }
    }
    else
      g_applicationMessenger.MediaPause();
    return SetResponse(openTag+"OK");
    break;
  case 2:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        CAction action;
        action.wID = ACTION_STOP;
        pSlideShow->OnAction(action);    
      }
    }
    else
      //g_application.StopPlaying();
      g_applicationMessenger.MediaStop();
    return SetResponse(openTag+"OK");
    break;
  case 3:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        CAction action;
        action.wID = ACTION_NEXT_PICTURE;
        pSlideShow->OnAction(action);
      }
    }
    else
      g_playlistPlayer.PlayNext();
    return SetResponse(openTag+"OK");
    break;
  case 4:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        CAction action;
        action.wID = ACTION_PREV_PICTURE;
        pSlideShow->OnAction(action);    
      }
    }
    else
      g_playlistPlayer.PlayPrevious();
    return SetResponse(openTag+"OK");
    break;
  case 5:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        CAction action;
        action.wID = ACTION_ROTATE_PICTURE;
        pSlideShow->OnAction(action);  
        return SetResponse(openTag+"OK");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  case 6:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        if (numParas>1) {
          CAction action;
          action.wID = ACTION_ANALOG_MOVE;
          action.fAmount1=(float) atof(paras[0]);
          action.fAmount2=(float) atof(paras[1]);
          pSlideShow->OnAction(action);    
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Missing parameters");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  case 7:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        if (numParas>0)
        {
          CAction action;
          action.wID = ACTION_ZOOM_LEVEL_NORMAL+atoi(paras[0]);
          pSlideShow->OnAction(action);    
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Missing parameters");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  default:
    return SetResponse(openTag+"Error");
  }
}

int CXbmcHttp::xbmcExit(int theAction)
{
  switch(theAction)
  {
  case 1:
    g_applicationMessenger.Restart();
    return SetResponse(openTag+"OK");
    break;
  case 2:
    g_applicationMessenger.Shutdown();
    return SetResponse(openTag+"OK");
    break;
  case 3:
    g_applicationMessenger.RebootToDashBoard();
    return SetResponse(openTag+"OK");
    break;
  case 4:
    g_applicationMessenger.Reset();
    return SetResponse(openTag+"OK");
    break;
  case 5:
    g_applicationMessenger.RestartApp();
    return SetResponse(openTag+"OK");
    break;
  default:
    return SetResponse(openTag+"Error");
  }
}

int CXbmcHttp::xbmcLookupAlbum(int numParas, CStdString paras[])
{
  CStdString albums="";
  CMusicInfoScraper scraper;

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing album name");
  else
    {
    try
    {
      scraper.FindAlbuminfo(paras[0]);
      while (!scraper.Completed()) {}
      if (scraper.Successfull())
      {
        // did we found at least 1 album?
        int iAlbumCount=scraper.GetAlbumCount();
        if (iAlbumCount >=1)
        {
          for (int i=0; i < iAlbumCount; ++i)
          {
            CMusicAlbumInfo& info = scraper.GetAlbum(i);
            albums += closeTag+openTag + info.GetTitle2() + "<@@>" + info.GetAlbumURL();
          }
          return SetResponse(albums) ;
        }
        else
          return SetResponse(openTag+"Error:No albums found") ;
      }
      else
        return SetResponse(openTag+"Error:Scraping") ;
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }}

int CXbmcHttp::xbmcChooseAlbum(int numParas, CStdString paras[])
{
  CStdString output="";

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing album name");
  else
    try
    {
      CMusicAlbumInfo musicInfo("", paras[0]) ;
      CHTTP http;
      if (musicInfo.Load(http))
      {
        output=openTag+"image:" + musicInfo.GetImageURL();
        output+=closeTag+openTag+"review:" + musicInfo.GetReview();
        return SetResponse(output) ;
      }
      else
        return SetResponse(openTag+"Error:Loading musinInfo");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error:Exception");
    }
}

int CXbmcHttp::xbmcDownloadInternetFile(int numParas, CStdString paras[])
{
  CStdString src, dest="";

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    src=paras[0];
    if (numParas>1)
      dest=paras[1];
    if (dest=="")
      dest="Z:\\xbmcDownloadInternetFile.tmp" ;
    if (src=="")
      return SetResponse(openTag+"Error:Missing parameter");
    else
    {
      try
      {
        CHTTP http;
        http.Download(src, dest);
        CStdString encoded="";
        encoded=encodeFileToBase64(dest, 80);
        if (encoded=="")
          return SetResponse(openTag+"Error:Nothing downloaded");
        {
          if (dest=="Z:\\xbmcDownloadInternetFile.tmp")
          ::DeleteFile(dest);
          return SetResponse(encoded) ;
        }
      }
      catch (...)
      {
        return SetResponse(openTag+"Error:Exception");
      }
    }
  }
}

int CXbmcHttp::xbmcSetFile(int numParas, CStdString paras[])
//parameter = destFilename ; base64String
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    paras[1].Replace(" ","+");
    decodeBase64ToFile(paras[1], "Z:\\xbmcTemp.tmp");
    CFile::Cache("Z:\\xbmcTemp.tmp", paras[0].c_str(), NULL, NULL) ;
    ::DeleteFile("Z:\\xbmcTemp.tmp");
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcCopyFile(int numParas, CStdString paras[])
//parameter = srcFilename ; destFilename
// both file names are relative to the XBox not the calling client
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    if (CFile::Exists(paras[0].c_str()))
    {
      CFile::Cache(paras[0].c_str(), paras[1].c_str(), NULL, NULL) ;
      return SetResponse(openTag+"OK");
    }
    else
      return SetResponse(openTag+"Error:Source file not found");
  }
}


int CXbmcHttp::xbmcFileSize(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    __int64 filesize=fileSize(paras[0]);
    if (filesize>-1)
    {
      CStdString tmp;
      tmp.Format("%I64d",filesize);
      return SetResponse(openTag+tmp);
    }
    else
      return SetResponse(openTag+"Error:Source file not found");
  }
}

int CXbmcHttp::xbmcDeleteFile(int numParas, CStdString paras[])
{
  if (numParas<1) 
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    try
    {
      if (CFile::Exists(paras[0].c_str()))
      {
        ::DeleteFile(paras[0].c_str());
        return SetResponse(openTag+"OK");
      }
      else
        return SetResponse(openTag+"Error:File not found");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }
}

int CXbmcHttp::xbmcFileExists(int numParas, CStdString paras[])
{
  if (numParas<1) 
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    try
    {
      if (CFile::Exists(paras[0].c_str()))
      {
        return SetResponse(openTag+"True");
      }
      else
        return SetResponse(openTag+"False");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }
}

int CXbmcHttp::xbmcShowPicture(int numParas, CStdString paras[])
{
  if (numParas<1) 
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    g_applicationMessenger.PictureShow(paras[0]);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGetCurrentSlide()
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error:Could not access slideshown");
  else
    return SetResponse(openTag + pSlideShow->GetCurrentSlide() );
}

int CXbmcHttp::xbmcExecBuiltIn(int numParas, CStdString paras[])
{
  if (numParas<1) 
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
    tMsg.strParam = paras[0];
    g_applicationMessenger.SendMessage(tMsg);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGUISetting(int numParas, CStdString paras[])
//parameter=type;name(;value)
//type=0->int, 1->bool, 2->float
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameters");
  else
  {
    CStdString tmp;
    if (numParas<3)
      switch (atoi(paras[0])) 
      {
        case 0:  //  int
          tmp.Format("%i", g_guiSettings.GetInt(paras[1]));
          return SetResponse(openTag + tmp );
          break;
        case 1: // bool
          if (g_guiSettings.GetBool(paras[1])==0)
            return SetResponse(openTag+"False");
          else
            return SetResponse(openTag+"True");
          break;
        case 2: // float
          tmp.Format("%f", g_guiSettings.GetFloat(paras[1]));
          return SetResponse(openTag + tmp);
          break;
        default:
          return SetResponse(openTag+"Error:Unknown type");
          break;
      }
    else
    {
      switch (atoi(paras[0])) 
      {
        case 0:  //  int
          g_guiSettings.SetInt(paras[1], atoi(paras[2]));
          return SetResponse(openTag+"OK");
          break;
        case 1: // bool
          g_guiSettings.SetBool(paras[1], (paras[2].ToLower()=="true"));
          return SetResponse(openTag+"OK");
          break;
        case 2: // float
          g_guiSettings.SetFloat(paras[1], (float)atof(paras[2]));
          return SetResponse(openTag+"OK");
          break;
        default:
          return SetResponse(openTag+"Error:Unknown type");
          break;
      }     
    }
  }
}

int CXbmcHttp::xbmcConfig(int numParas, CStdString paras[])
{
  int argc=0, ret=-1;
  char_t* argv[20]; 
  CStdString response="";
  
  if (numParas<1) {
    return SetResponse(openTag+"Error:Missing paramters");
  }
  if (numParas>1){
    for (argc=0; argc<numParas-1;argc++)
      argv[argc]=(char_t*)paras[argc+1].c_str();
  }
  argv[argc]=NULL;
  if (paras[0]=="bookmarksize")
  {
    ret=XbmcWebsHttpAPIConfigBookmarkSize(response, argc, argv);
    if (ret!=-1)
      ret=1;
  }
  else if (paras[0]=="getbookmark")
  {
    ret=XbmcWebsHttpAPIConfigGetBookmark(response, argc, argv);
    if (ret!=-1)
      ret=1;
  }
  else if (paras[0]=="addbookmark") 
    ret=XbmcWebsHttpAPIConfigAddBookmark(response, argc, argv);
  else if (paras[0]=="savebookmark")
    ret=XbmcWebsHttpAPIConfigSaveBookmark(response, argc, argv);
  else if (paras[0]=="removebookmark")
    ret=XbmcWebsHttpAPIConfigRemoveBookmark(response, argc, argv);
  else if (paras[0]=="saveconfiguration")
    ret=XbmcWebsHttpAPIConfigSaveConfiguration(response, argc, argv);
  else if (paras[0]=="getoption")
  {
    ret=XbmcWebsHttpAPIConfigGetOption(response, argc, argv);
    if (ret!=-1)
      ret=1;
  }
  else if (paras[0]=="setoption")
    ret=XbmcWebsHttpAPIConfigSetOption(response, argc, argv);
  else
  {
    return SetResponse(openTag+"Error:Unknown Config Command");
  }

  return SetResponse(response);
}

int CXbmcHttp::xbmcGetSystemInfo(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    CStdString strInfo = "";
    int i;
    for (i=0; i<numParas; i++)
      strInfo += openTag + (CStdString) g_infoManager.GetLabel(atoi(paras[i])) ;
    if (strInfo == "")
      strInfo="Error:No information retrieved";
    return SetResponse(strInfo);
  }
}

int CXbmcHttp::xbmcGetSystemInfoByName(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    CStdString strInfo = "";
    int i;

    for (i=0; i<numParas; i++)
      strInfo += openTag + (CStdString) g_infoManager.GetLabel(g_infoManager.TranslateString(paras[i])) ;
    if (strInfo == "")
      strInfo="Error:No information retrieved";
    return SetResponse(strInfo);
  }
}

int CXbmcHttp::xbmcTakeScreenshot(int numParas, CStdString paras[])
//no paras
//filename, flash, rotation, width, height, quality
//filename, flash, rotation, width, height, quality, download
//filename can be blank
{
  if (numParas<1)
    CUtil::TakeScreenshot();
  else
  {
    CStdString filepath, path, filename;
    if (paras[0]=="")
      filepath="Z:\\screenshot.jpg";
    else
      filepath=paras[0];
    // check we have a valid path
    filename = CUtil::GetFileName(filepath);
    if (!CUtil::GetParentPath(filepath, path) || !CDirectory::Exists(path))
    {
      CLog::Log(LOGERROR, "Invalid path in xbmcTakeScreenShot - saving to Z:");
      CUtil::AddFileToFolder("Z:", filename, filepath);
    }
    if (numParas>5)
    {
      CUtil::TakeScreenshot("Z:\\temp.bmp", paras[1].ToLower()=="true");
      int height, width;
      if (paras[4]=="")
        if (paras[3]=="")
        {
          return SetResponse(openTag+"Error:Both height and width parameters cannot be absent");
        }
        else
        {
          width=atoi(paras[3]);
          height=-1;
        }
      else
        if (paras[3]=="")
        {
          height=atoi(paras[4]);
          width=-1;
        }
        else
        {
          width=atoi(paras[3]);
          height=atoi(paras[4]);
        }
      CPicture pic;
      int ret;
      ret=pic.ConvertFile("Z:\\temp.bmp", filepath, (float) atof(paras[2]), width, height, atoi(paras[5]));
      if (ret==0)
      {
        ::DeleteFile("Z:\\temp.bmp");
        if (numParas>6)
          if (paras[6].ToLower()="true")
          {
            CStdString b64;
            b64=encodeFileToBase64(filepath,80);
            if (filepath=="Z:\\screenshot.jpg")
              ::DeleteFile(filepath.c_str());
            return SetResponse(b64) ;
          }
      }
      else
      {
        CStdString strInt;
        strInt.Format("%", ret);
        return SetResponse(openTag+"Error:Could not convert image, error: " + strInt );
      }
    }
    else
      return SetResponse(openTag+"Error:Missing parameters");
  }
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcAutoGetPictureThumbs(int numParas, CStdString paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    autoGetPictureThumbs = (paras[0].ToLower()=="true");
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcSetResponseFormat(int numParas, CStdString paras[])
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    CStdString para=paras[0].ToLower();
    if (para=="webheader")
      incWebHeader=(paras[1].ToLower()=="true");
    else if (para=="webfooter")
      incWebFooter=(paras[1].ToLower()=="true");
    else if (para=="header")
      userHeader=paras[1];
    else if (para=="footer")
      userFooter=paras[1];
    else if (para=="opentag")
      openTag=paras[1];
    else if (para=="closetag")
      closeTag=paras[1];
    else if (para=="closefinaltag")
      closeFinalTag=(paras[1].ToLower()=="true");
    else
      return SetResponse(openTag+"Error:Unknown parameter");
    return SetResponse(openTag+"OK");
  }
}


int CXbmcHttp::xbmcHelp()
{
  CStdString output;
  output="<p><b>XBMC HTTP API Commands</b></p><p><b>Syntax: http://xbox/xbmcCmds/xbmcHttp?command=</b>one_of_the_following_commands<b>&ampparameter=</b>first_parameter<b>;</b>second_parameter<b>;...</b></p><p>Note the use of the semi colon to separate multiple parameters</p><p>The commands are case insensitive.</p><p>For more information see the readme.txt and the source code of the demo client application on SourceForge.</p>";

  output+= "<p>The full documentation as a Word file can be downloaded here: <a  href=\"http://prdownloads.sourceforge.net/xbmc/apiDoc.doc?download\">http://prdownloads.sourceforge.net/xbmc/apiDoc.doc?download</a></p>";

  output+= openTag+"clearplaylist"+closeTag+openTag+"addtoplaylist"+closeTag+openTag+"playfile"+closeTag+openTag+"pause"+closeTag+openTag+"stop"+closeTag+openTag+"restart"+closeTag+openTag+"shutdown"+closeTag+openTag+"exit"+closeTag+openTag+"reset"+closeTag+openTag+"restartapp"+closeTag+openTag+"getcurrentlyplaying"+closeTag+openTag+"getdirectory"+closeTag+openTag+"gettagfromfilename"+closeTag+openTag+"getcurrentplaylist"+closeTag+openTag+"setcurrentplaylist"+closeTag+openTag+"getplaylistcontents"+closeTag+openTag+"removefromplaylist"+closeTag+openTag+"setplaylistsong"+closeTag+openTag+"getplaylistsong"+closeTag+openTag+"playlistnext"+closeTag+openTag+"playlistprev"+closeTag+openTag+"getpercentage"+closeTag+openTag+"seekpercentage"+closeTag+openTag+"seekpercentagerelative"+closeTag+openTag+"setvolume"+closeTag+openTag+"getvolume"+closeTag+openTag+"getthumbfilename"+closeTag+openTag+"lookupalbum"+closeTag+openTag+"choosealbum"+closeTag+openTag+"downloadinternetfile"+closeTag+openTag+"getmoviedetails"+closeTag+openTag+"showpicture"+closeTag+openTag+"sendkey"+closeTag+openTag+"filedelete"+closeTag+openTag+"filecopy"+closeTag+openTag+"fileexists"+closeTag+openTag+"fileupload"+closeTag+openTag+"getguistatus"+closeTag+openTag+"execbuiltin"+closeTag+openTag+"config"+closeTag+openTag+"getsysteminfo"+closeTag+openTag+"getsysteminfobyname"+closeTag+openTag+"guisetting"+closeTag+openTag+"addtoslideshow"+closeTag+openTag+"clearslideshow"+closeTag+openTag+"playslideshow"+closeTag+openTag+"getslideshowcontents"+closeTag+openTag+"slideshowselect"+closeTag+openTag+"getcurrentslide"+closeTag+openTag+"rotate"+closeTag+openTag+"move"+closeTag+openTag+"zoom"+closeTag+openTag+"playnext"+closeTag+openTag+"playprev"+closeTag+openTag+"TakeScreenShot"+closeTag+openTag+"GetGUIDescription"+closeTag+openTag+"GetPlaySpeed"+closeTag+openTag+"SetPlaySpeed"+closeTag+openTag+"SetResponseFormat"+closeTag+openTag+"Help";

  return SetResponse(output);
}



int CXbmcHttp::xbmcCommand(CStdString parameter)
{
  int numParas, retVal;
  CStdString command, paras[MAX_PARAS];
  numParas = splitParameter(parameter, command, paras, ";");
  if (parameter.length()<300)
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: %s", command.c_str(), parameter.c_str());
  else
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: [not recorded]", command.c_str());
  command=command.ToLower();
  if (numParas>=0)
    if (command == "clearplaylist")                   retVal = xbmcClearPlayList(numParas, paras);  
      else if (command == "addtoplaylist")            retVal = xbmcAddToPlayList(numParas, paras);  
      else if (command == "playfile")                 retVal = xbmcPlayerPlayFile(numParas, paras); 
      else if (command == "pause")                    retVal = xbmcAction(numParas, paras,1);
      else if (command == "stop")                     retVal = xbmcAction(numParas, paras,2);
      else if (command == "playnext")                 retVal = xbmcAction(numParas, paras,3);
      else if (command == "playprev")                 retVal = xbmcAction(numParas, paras,4);
      else if (command == "rotate")                   retVal = xbmcAction(numParas, paras,5);
      else if (command == "move")                     retVal = xbmcAction(numParas, paras,6);
      else if (command == "zoom")                     retVal = xbmcAction(numParas, paras,7);
      else if (command == "restart")                  retVal = xbmcExit(1);
      else if (command == "shutdown")                 retVal = xbmcExit(2);
      else if (command == "exit")                     retVal = xbmcExit(3);
      else if (command == "reset")                    retVal = xbmcExit(4);
      else if (command == "restartapp")               retVal = xbmcExit(5);
      else if (command == "getcurrentlyplaying")      retVal = xbmcGetCurrentlyPlaying(); 
      else if (command == "getshares")                retVal = xbmcGetShares(numParas, paras); 
      else if (command == "getdirectory")             retVal = xbmcGetDirectory(numParas, paras); 
      else if (command == "getmedialocation")         retVal = xbmcGetMediaLocation(numParas, paras); 
      else if (command == "gettagfromfilename")       retVal = xbmcGetTagFromFilename(numParas, paras);
      else if (command == "getcurrentplaylist")       retVal = xbmcGetCurrentPlayList();
      else if (command == "setcurrentplaylist")       retVal = xbmcSetCurrentPlayList(numParas, paras);
      else if (command == "getplaylistcontents")      retVal = xbmcGetPlayListContents(numParas, paras);
      else if (command == "removefromplaylist")       retVal = xbmcRemoveFromPlayList(numParas, paras);
      else if (command == "setplaylistsong")          retVal = xbmcSetPlayListSong(numParas, paras);
      else if (command == "getplaylistsong")          retVal = xbmcGetPlayListSong(numParas, paras);
      else if (command == "playlistnext")             retVal = xbmcPlayListNext();
      else if (command == "playlistprev")             retVal = xbmcPlayListPrev();
      else if (command == "getpercentage")            retVal = xbmcGetPercentage();
      else if (command == "seekpercentage")           retVal = xbmcSeekPercentage(numParas, paras, false);
      else if (command == "seekpercentagerelative")   retVal = xbmcSeekPercentage(numParas, paras, true);
      else if (command == "setvolume")                retVal = xbmcSetVolume(numParas, paras);
      else if (command == "getvolume")                retVal = xbmcGetVolume();
      else if (command == "setplayspeed")             retVal = xbmcSetPlaySpeed(numParas, paras);
      else if (command == "getplayspeed")             retVal = xbmcGetPlaySpeed();
      else if (command == "filedownload")             retVal = xbmcGetThumb(numParas, paras);
      else if (command == "getthumbfilename")         retVal = xbmcGetThumbFilename(numParas, paras);
      else if (command == "lookupalbum")              retVal = xbmcLookupAlbum(numParas, paras);
      else if (command == "choosealbum")              retVal = xbmcChooseAlbum(numParas, paras);
      else if (command == "filedownloadfrominternet") retVal = xbmcDownloadInternetFile(numParas, paras);
      else if (command == "filedelete")               retVal = xbmcDeleteFile(numParas, paras);
      else if (command == "filecopy")                 retVal = xbmcCopyFile(numParas, paras);
      else if (command == "filesize")                 retVal = xbmcFileSize(numParas, paras);
      else if (command == "getmoviedetails")          retVal = xbmcGetMovieDetails(numParas, paras);
      else if (command == "showpicture")              retVal = xbmcShowPicture(numParas, paras);
      else if (command == "sendkey")                  retVal = xbmcSetKey(numParas, paras);
      else if (command == "fileexists")               retVal = xbmcFileExists(numParas, paras);
      else if (command == "fileupload")               retVal = xbmcSetFile(numParas, paras);
      else if (command == "getguistatus")             retVal = xbmcGetGUIStatus();
      else if (command == "execbuiltin")              retVal = xbmcExecBuiltIn(numParas, paras);
      else if (command == "config")                   retVal = xbmcConfig(numParas, paras);
      else if (command == "help")                     retVal = xbmcHelp();
      else if (command == "getsysteminfo")            retVal = xbmcGetSystemInfo(numParas, paras);
      else if (command == "getsysteminfobyname")      retVal = xbmcGetSystemInfoByName(numParas, paras);
      else if (command == "addtoslideshow")           retVal = xbmcAddToSlideshow(numParas, paras);
      else if (command == "clearslideshow")           retVal = xbmcClearSlideshow();
      else if (command == "playslideshow")            retVal = xbmcPlaySlideshow(numParas, paras);
      else if (command == "getslideshowcontents")     retVal = xbmcGetSlideshowContents();
      else if (command == "slideshowselect")          retVal = xbmcSlideshowSelect(numParas, paras);
      else if (command == "getcurrentslide")          retVal = xbmcGetCurrentSlide();
      else if (command == "getguisetting")            retVal = xbmcGUISetting(numParas, paras);
      else if (command == "setguisetting")            retVal = xbmcGUISetting(numParas, paras);
      else if (command == "takescreenshot")           retVal = xbmcTakeScreenshot(numParas, paras);
      else if (command == "getguidescription")        retVal = xbmcGetGUIDescription();
      else if (command == "setautogetpicturethumbs")  retVal = xbmcAutoGetPictureThumbs(numParas, paras);
      else if (command == "setresponseformat")        retVal = xbmcSetResponseFormat(numParas, paras);
      //Old command names
      else if (command == "deletefile")               retVal = xbmcDeleteFile(numParas, paras);
      else if (command == "copyfile")                 retVal = xbmcCopyFile(numParas, paras);
      else if (command == "downloadinternetfile")     retVal = xbmcDownloadInternetFile(numParas, paras);
      else if (command == "getthumb")                 retVal = xbmcGetThumb(numParas, paras);
      else if (command == "guisetting")               retVal = xbmcGUISetting(numParas, paras);
      else if (command == "setfile")                  retVal = xbmcSetFile(numParas, paras);
      else if (command == "setkey")                   retVal = xbmcSetKey(numParas, paras);

      else
        retVal = SetResponse(openTag+"Error:Unknown command");
  else
    retVal = SetResponse(openTag+"Error:Missing command");
  //if (g_applicationMessenger.GetResponse() == "")
  //  SetResponse(openTag+"Error:Unknown error");
  Sleep(100);
  return retVal;
}

CXbmcHttpShim::CXbmcHttpShim()
{
  CLog::Log(LOGDEBUG, "xbmcHttpShim starts");
}

CXbmcHttpShim::~CXbmcHttpShim()
{
CLog::Log(LOGDEBUG, "xbmcHttpShim ends");
}


CStdString CXbmcHttpShim::xbmcExternalCall(char *command)
{
  int open, close;
  CStdString parameter="", cmd=command, execute;
  open = cmd.Find("(");
  close = cmd.Find(")", open);
  if (open > 0 && close > 0)
  {
    parameter = cmd.Mid(open + 1, close - open - 1);
    parameter.Replace(",",";");
    execute = cmd.Left(open);
  }
  else if (open>0) //open bracket but no close
    return "";
  else //no parameters
    execute=cmd;

  return xbmcProcessCommand(NO_EID, NULL, (char_t *) execute.c_str(), (char_t *) parameter.c_str());
}


/* Parse an XBMC HTTP API command */
CStdString CXbmcHttpShim::xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter)
{
  CStdString cmd=command, paras=parameter, response="[No response yet]", retVal;
  int cnt=0;
  if ((wp != NULL) && (eid==NO_EID) && (incWebHeader))
    websHeader(wp);
  if ((*parameter==0) || !strcmp(parameter,XBMC_NONE))
    g_applicationMessenger.HttpApi(cmd);
  else
    g_applicationMessenger.HttpApi(cmd+"; "+paras);
  //wait for response - max 10s
  while (response=="[No response yet]" && cnt<100) 
  {
    response=g_applicationMessenger.GetResponse();
    Sleep(100);
    cnt++;
  }
  if (cnt==100)
  {
    response="Error:Timed out";
    CLog::Log(LOGDEBUG, "HttpApi Waiting");
  }
  //flushresult
  retVal=flushResult(eid, wp, userHeader+response+userFooter);
  if ((wp!=NULL) && (incWebFooter))
    websFooter(wp);
  return retVal;
}


/* XBMC Javascript binding for ASP. This will be invoked when "APICommand" is
 *  embedded in an ASP page.
 */
int CXbmcHttpShim::xbmcCommand( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*command, *parameter;

	int parameters = ejArgs(argc, argv, T("%s %s"), &command, &parameter);
	if (parameters < 1) {
    websError(wp, 500, T("Error:Insufficient args"));
		return -1;
	}
	else if (parameters < 2) parameter = "";

	xbmcProcessCommand( eid, wp, command, parameter);
  return 0;
}

/* XBMC form for posted data (in-memory CGI). 
 */
void CXbmcHttpShim::xbmcForm(webs_t wp, char_t *path, char_t *query)
{
  char_t  *command, *parameter;

  command = websGetVar(wp, WEB_COMMAND, XBMC_NONE); 
  parameter = websGetVar(wp, WEB_PARAMETER, XBMC_NONE);

  // do the command

  xbmcProcessCommand( NO_EID, wp, command, parameter);

  if (wp->timeout!=-1)
    websDone(wp, 200);
  else
    CLog::Log(LOGERROR, "HttpApi Timeout command: %s", query);
}
