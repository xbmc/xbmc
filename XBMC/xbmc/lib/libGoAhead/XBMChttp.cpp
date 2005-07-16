
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

#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

#define XML_MAX_INNERTEXT_SIZE 256
#define MAX_PARAS 10
#define NO_EID -1

#define XBMC_NONE      T("none")

CXbmcHttp* pXbmcHttp;
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
          strBase64 += "\r\n" ;
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

int splitParameter(char_t *parameter, CStdString paras[])
{
  int num=0, p;
  CStdString para=parameter;
  if ((*parameter==0) || !strcmp(parameter,XBMC_NONE) || (para.Trim()==""))
    return 0;
  p=para.Find(";");
  while (p!=-1 && num<MAX_PARAS)
  {
    paras[num]=para.Left(p);
    if (paras[num].Trim()=="")
      paras[num]=paras[num].Trim();
    num++;
    para=para.Right(para.length()-p-1);
    p=para.Find(";");
  }
  if (para.Trim()!="")
    paras[num++]=para;
  return num;
}

void flushResult(int eid, webs_t wp, CStdString output)
{
  if (eid==NO_EID)
    websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
  else
    ejSetResult( eid, (char_t *)output.c_str());
}

struct SSortWebFilesByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
    if (rpStart.GetLabel()=="..") return true;
    if (rpEnd.GetLabel()=="..") return false;
    bool bGreater=true;
    if (m_bSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch (m_iSortMethod) 
      {
        case 0:  //  Sort by Filename
          strcpy(szfilename1, rpStart.GetLabel().c_str());
          strcpy(szfilename2, rpEnd.GetLabel().c_str());
          break;
        case 1: // Sort by Date
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
          if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
          
          if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
          if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;
          
          if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
          if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

          if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
          if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

          if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
          if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

          if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
          if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
          return true;
        break;

        case 2:
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
          if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
          return true;
        break;

        default:  //  Sort by Filename by default
          strcpy(szfilename1, rpStart.GetLabel().c_str());
          strcpy(szfilename2, rpEnd.GetLabel().c_str());
          break;
      }

      for (int i=0; i < (int)strlen(szfilename1); i++)
      {
        szfilename1[i]=tolower((unsigned char)szfilename1[i]);
      }

      for (int i=0; i < (int)strlen(szfilename2); i++)
      {
        szfilename2[i]=tolower((unsigned char)szfilename2[i]);
      }

      if (g_stSettings.m_bMyFilesSourceSortAscending)
        return (strcmp(szfilename1,szfilename2)<0);
      else
        return (strcmp(szfilename1,szfilename2)>=0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }
  static bool m_bSortAscending;
  static int m_iSortMethod;
};

bool SSortWebFilesByName::m_bSortAscending;

int SSortWebFilesByName::m_iSortMethod;

int displayDir(int eid, webs_t wp, int numParas, CStdString paras[]) {
  //mask = ".mp3|.wma" -> matching files
  //mask = "*" -> just folders
  //mask = "" -> all files and folder
  //option = "1" -> append date&time to file name

  CFileItemList dirItems;
  CStdString output="";

  CStdString  folder, mask="", option="";

  if (numParas==0)
  {
    flushResult(eid, wp, "<li>Error:Missing folder\n");
    return 0;
  }
  folder=paras[0];
  if (numParas>1)
    mask=paras[1];
    if (numParas>2)
      option=paras[2];

    IDirectory *pDirectory = CFactoryDirectory::Create(folder);

  if (!pDirectory) 
  {
    flushResult(eid, wp, "<li>Error\n");
    return 0;
  }
  pDirectory->SetMask(mask);
  bool bResult=pDirectory->GetDirectory(folder,dirItems);
  if (!bResult)
  {
    flushResult(eid, wp, "<li>Error:Not folder\n");
    return 0;
  }
  SSortWebFilesByName::m_bSortAscending = true;
  SSortWebFilesByName::m_iSortMethod = 0;
  dirItems.Sort(SSortWebFilesByName::Sort);
  for (int i=0; i<dirItems.Size(); ++i)
  {
    CFileItem *itm = dirItems[i];
    if (mask=="*" || (mask =="" && itm->m_bIsFolder))
      if (!CUtil::HasSlashAtEnd(itm->m_strPath))
        output+="\n<li>" + itm->m_strPath + "\\" ;
      else
        output+="\n<li>" + itm->m_strPath ;
    else if (!itm->m_bIsFolder)
      output+="\n<li>" + itm->m_strPath;
    if (option="1") {
      CStdString theDate;
      CUtil::GetDate(itm->m_stTime,theDate) ;
      output+="  ;" + theDate ;
    }
  }
  flushResult(eid, wp, output);
  return 0;
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
    if (pItem->GetLabel() == "..") return;
    CStdString strDirectory=pItem->m_strPath;
    CFileItemList items;
    IDirectory *pDirectory = CFactoryDirectory::Create(strDirectory);
    if (mask!="")
      pDirectory->SetMask(mask);
    bool bResult=pDirectory->GetDirectory(strDirectory,items);
    SSortWebFilesByName::m_bSortAscending = true;
    SSortWebFilesByName::m_iSortMethod = sortMethod;
    items.Sort(SSortWebFilesByName::Sort);
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
    return g_application.PlayFile(CFileItem(item));
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

}

int CXbmcHttp::xbmcAddToPlayList(int eid, webs_t wp, int numParas, CStdString paras[])
{
  //parameters=playList;mask
  CStdString strFileName;
  CStdString mask="";
  bool changed=false;
  int playList ;

  if (numParas==0)
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
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
      flushResult(eid, wp, "<li>OK\n");
    }
    else
      flushResult(eid, wp, "<li>Error\n");
  }
  return 0;
}

int CXbmcHttp::xbmcGetTagFromFilename(int eid, webs_t wp, int numParas, CStdString paras[]) 
{
  //char buffer[XML_MAX_INNERTEXT_SIZE];
  CStdString strFileName;
  if (numParas==0) {
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
    return 0;
  }
  strFileName=CUtil::GetFileName(paras[0].c_str());
  CFileItem *pItem = new CFileItem(strFileName);
  pItem->m_strPath=paras[0].c_str();
  if (!pItem->IsAudio())
  {
    flushResult(eid, wp,"<li>Error:Not Audio\n");
    delete pItem;
    return 0;
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
        flushResult(eid, wp,"<li>Error:Could not load TagLoader\n");
        return 0;
      }
    }
    else
    {
      flushResult(eid, wp,"<li>Error:System not set to use tags\n");
      return 0;
    }
  if (tag.Loaded())
  {
    CStdString output, tmp;

    output = "<li>Artist:" + tag.GetArtist();
    output += "\n<li>Album:" + tag.GetAlbum();
    output += "\n<li>Title:" + tag.GetTitle();
    tmp.Format("%i", tag.GetTrackNumber());
    output += "\n<li>Track number:" + tmp;
    tmp.Format("%i", tag.GetDuration());
    output += "\n<li>Duration:" + tmp;
    output += "\n<li>Genre:" + tag.GetGenre();
    SYSTEMTIME stTime;
    tag.GetReleaseDate(stTime);
    tmp.Format("%i", stTime.wYear);
    output += "\n<li>Release year:" + tmp + "\n";
    pItem->SetMusicThumb();
    if (pItem->HasThumbnail())
      output += "<li>Thumb:" + pItem->GetThumbnailImage() + "\n";
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
        output += "<li>Thumb:[None] " + strThumb + "\n";
      }
    }
    flushResult(eid, wp, output);
  }
  else
    flushResult(eid, wp,"<li>Error:No tag info\n");
  delete pItem;
  return 0;
}

int CXbmcHttp::xbmcClearPlayList(int eid, webs_t wp, int numParas, CStdString paras[])
{
  int playList ;
  if (numParas==0)
    playList = g_playlistPlayer.GetCurrentPlaylist() ;
  else
    playList=atoi(paras[0]) ;
  g_playlistPlayer.GetPlaylist( playList ).Clear();
  flushResult(eid, wp, "<li>OK\n");
  return 0;
}

int  CXbmcHttp::xbmcGetDirectory(int eid, webs_t wp,  int numParas, CStdString paras[])
{
  if (numParas>0)
    displayDir(eid, wp, numParas, paras);
  else
    flushResult(eid, wp,"<li>Error:No path\n") ;
  return 0;
}

int CXbmcHttp::xbmcGetMovieDetails(int eid, webs_t wp, int numParas, CStdString paras[])
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
        output = "\n<li>Year:" + tmp;
        output += "\n<li>Director:" + aMovieRec.m_strDirector;
        output += "\n<li>Title:" + aMovieRec.m_strTitle;
        output += "\n<li>Plot:" + aMovieRec.m_strPlot;
        output += "\n<li>Genre:" + aMovieRec.m_strGenre;
        CStdString strRating;
        strRating.Format("%3.3f", aMovieRec.m_fRating);
        if (strRating=="") strRating="0.0";
        output += "\n<li>Rating:" + strRating;
        output += "\n<li>Cast:" + aMovieRec.m_strCast;
        CUtil::GetVideoThumbnail(aMovieRec.m_strIMDBNumber,thumb);
        if (!CFile::Exists(thumb))
          thumb = "[None] " + thumb;
        output += "\n<li>Thumb:" + thumb;
        flushResult(eid, wp, output);
      }
      else
        flushResult(eid, wp, "\n<li>Thumb: [None]");
      m_database.Close();
    }
    delete item;
  }
  else
    flushResult(eid, wp,"<li>Error:No file name\n") ;
  return 0;
}

int CXbmcHttp::xbmcGetCurrentlyPlaying(int eid, webs_t wp)
{
  CStdString output="", tmp="";
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pSlideShow)
    if (m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    {
      output="<li>Filename:"+pSlideShow->GetCurrentSlide()+"\n";
      output+="<li>Type:Picture\n" ;
      int width=0, height=0;
      pSlideShow->GetCurrentSlideInfo(width, height);
      tmp.Format("%i",width);
      output+="<li>Width:" + tmp ;
      tmp.Format("%i",height);
      output+="\n<li>Height:" + tmp ;
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
        {
        thumb = "[None] " + thumb;
        }
      }
      output+="\n<li>Thumb:"+thumb;
      flushResult(eid, wp, output);
      return 0;
    } 
  CStdString fn=g_application.CurrentFile();
  if (fn=="")
    output="<li>Filename:[Nothing Playing]";
  else
  { 
    output="<li>Filename:"+fn+"\n";
    tmp.Format("%i",g_playlistPlayer.GetCurrentSong());
    output+="\n<li>SongNo:"+tmp;
    __int64 lPTS=-1;
    if (g_application.IsPlayingVideo()) {
      lPTS = g_application.m_pPlayer->GetTime()/100;
      output+="\n<li>Type:Video" ;
    }
    else if (g_application.IsPlayingAudio()) {
      lPTS = g_application.m_pPlayer->GetTime()/100;
      output+="\n<li>Type:Audio" ;
    }
    CStdString thumb;
    CUtil::GetThumbnail(fn,thumb);
    if (!CFile::Exists(thumb))
      thumb = "[None] " + thumb;
    output+="\n<li>Thumb:"+thumb;
    if (g_application.m_pPlayer->IsPaused()) 
      output+="\n<li>Paused:True" ;
    else
      output+="\n<li>Paused:False" ;
    if (g_application.IsPlaying()) 
      output+="\n<li>Playing:True" ;
    else
      output+="\n<li>Playing:False" ;
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
      output+="\n<li>Time:"+tmp;
      CStdString strTotalTime;
      unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
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
        output+="\n<li>Duration:"+tmp+"\n";
      }
      tmp.Format("%i",(int)g_application.m_pPlayer->GetPercentage());
      output+="<li>Percentage:"+tmp+"\n" ;
    }
  }
  flushResult(eid, wp, output);
  return 0;
}

int CXbmcHttp::xbmcGetPercentage(int eid, webs_t wp)
{
  if (g_application.m_pPlayer)
  {
    CStdString tmp;
    tmp.Format("%i",(int)g_application.m_pPlayer->GetPercentage());
    flushResult(eid, wp, "<li>Percentage:" + tmp + "\n") ;
  }
  else
    flushResult(eid, wp, "<li>Error\n");
  return 0;
}

int CXbmcHttp::xbmcSeekPercentage(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
  else
  {
    if (g_application.m_pPlayer)
    {
      g_application.m_pPlayer->SeekPercentage((float)atoi(paras[0].c_str()));
      flushResult(eid, wp,"<li>OK\n") ;
    }
    else
      flushResult(eid, wp, "<li>Error:Loading mPlayer\n");
  }
  return 0;
}

int CXbmcHttp::xbmcSetVolume(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
  else
  {
    int iPercent = atoi(paras[0].c_str()) ;
    if (g_application.m_pPlayer!=0) {
      if (iPercent<0) iPercent = 0;
      if (iPercent>100) iPercent = 100;
      float fHardwareVolume = ((float)iPercent)/100.0f * (VOLUME_MAXIMUM-VOLUME_MINIMUM) + VOLUME_MINIMUM;
      g_stSettings.m_nVolumeLevel = (long)fHardwareVolume;
      // show visual feedback of volume change...
      if (!g_application.m_guiDialogVolumeBar.IsRunning())
        g_application.m_guiDialogVolumeBar.DoModal(m_gWindowManager.GetActiveWindow());
      g_application.m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
      flushResult(eid, wp, "<li>OK\n");
    }
    else
      flushResult(eid, wp, "<li>Error\n");
  }
  return 0;
}

int CXbmcHttp::xbmcGetVolume(int eid, webs_t wp)
{
  int vol;
  if (g_application.m_pPlayer!=0)
    vol = int(((float)(g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM))/(VOLUME_MAXIMUM-VOLUME_MINIMUM)*100.0f+0.5f) ;
  else
    vol = -1;
  CStdString tmp;
  tmp.Format("%i",vol);
  flushResult(eid, wp, "<li>" + tmp + "\n");
  return 0;
}

int CXbmcHttp::xbmcClearSlideshow(int eid, webs_t wp)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    flushResult(eid, wp, "<li>Error:Could not create slideshow\n");
  else
  {
    pSlideShow->Reset();
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcPlaySlideshow(int eid, webs_t wp, int numParas, CStdString paras[] )
{ // (filename(;1)) -> 1 indicates recursive
  bool recursive=false;

    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
  {
    flushResult(eid, wp, "<li>Error\n");
  }
  else
  {
    // stop playing file
    if (g_application.IsPlayingVideo()) g_application.StopPlaying();

    if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      m_gWindowManager.PreviousWindow();

    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();

    g_graphicsContext.Lock();

    if (numParas==0)
      pSlideShow->RunSlideShow("",false);
    else
    {
      if (numParas>1)
        recursive=(atoi(paras[1].c_str())==1);
      pSlideShow->RunSlideShow(paras[0], recursive);
    }
    g_graphicsContext.Unlock();
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcSlideshowSelect(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp,"<li>Error:Missing filename\n");
  else
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      flushResult(eid, wp, "<li>Error:Could not create slideshow\n");
    else
    {
      pSlideShow->Select(paras[0]);
      flushResult(eid, wp, "<li>OK\n");
    }
  }
  return 0;
}

int CXbmcHttp::xbmcAddToSlideshow(int eid, webs_t wp, int numParas, CStdString paras[])
//filename (;mask)
{
  CStdString mask="";
  if (numParas<1)
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
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
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcSetPlaySpeed(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas>1) {
    g_application.SetPlaySpeed(atoi(paras[0]));
    flushResult(eid, wp, "<li>OK\n");
  }
  else
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  return 0;
}

int CXbmcHttp::xbmcGetPlaySpeed(int eid, webs_t wp)
{
  CStdString strSpeed;
  strSpeed.Format("%i", g_application.GetPlaySpeed());
  flushResult(eid, wp,"<li>" + strSpeed + "\n");
  return 0;
}

int CXbmcHttp::xbmcGetGUIDescription(int eid, webs_t wp)
{
  CStdString strWidth, strHeight;
  strWidth.Format("%i", g_graphicsContext.GetWidth());
  strHeight.Format("%i", g_graphicsContext.GetHeight());
  flushResult(eid, wp, "<li>Width:" + strWidth + "\n<li>Height:" + strHeight + "\n" );
  return 0;
}

int CXbmcHttp::xbmcGetGUIStatus(int eid, webs_t wp)
{
  CStdString output, tmp;
  tmp.Format("%i", m_gWindowManager.GetActiveWindow());
  output = "<li>ActiveWindow:" + tmp + "\n";
  tmp.Format("%i", m_gWindowManager.GetActiveWindow()); 
  output += "<li>ActiveWindowType:" + tmp + "\n";
  CStdString strTmp;
  int iWin=m_gWindowManager.GetActiveWindow();
  CGUIWindow* pWindow=m_gWindowManager.GetWindow(iWin);
  if (pWindow)
  {
    CStdString strLine;
    wstring wstrLine;
    wstrLine=g_localizeStrings.Get(iWin);
    CUtil::Unicode2Ansi(wstrLine,strLine);
    output += "<li>ActiveWindowName:" + strLine + "\n" ; 
    int iControl=pWindow->GetFocusedControl();
    CGUIControl* pControl=(CGUIControl* )pWindow->GetControl(iControl);
    if (pControl)
    {
      if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        strTmp.Format("%s", ((CGUIButtonControl*)pControl)->GetLabel());
        output += "<li>ControlButton:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_SPIN)
      {
        CGUISpinControl* pSpinControl = (CGUISpinControl*)pControl;
        strTmp.Format("%i/%i", 1+pSpinControl->GetValue(), pSpinControl->GetMaximum());
        output += "<li>ControlSpin:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LABEL)
      {
        CGUIListControl* pListControl = (CGUIListControl*)pControl;
        pListControl->GetSelectedItem(strTmp);
        output += "<li>ControlLabel:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_THUMBNAIL)
      {
        CGUIThumbnailPanel* pThumbControl = (CGUIThumbnailPanel*)pControl;
        pThumbControl->GetSelectedItem(strTmp);
        output += "<li>ControlThumNail:" + strTmp;
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LIST)
      {
        CGUIListControl* pListControl = (CGUIListControl*)pControl;
        pListControl->GetSelectedItem(strTmp);
        output += "<li>ControlList:" + strTmp;
      }
    }
  }
  flushResult(eid, wp, output);
  return 0;
}

int CXbmcHttp::xbmcGetThumb(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString thumb="";
  if (numParas<1)
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
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
        flushResult(eid, wp, "<li>Error\n");
        return 0;
      }
    }
    else
      thumb=encodeFileToBase64(paras[0],80);
    flushResult(eid, wp, thumb) ;
  }
  return 0;
}

int CXbmcHttp::xbmcGetThumbFilename(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString thumbFilename="";

  if (numParas>1)
  {
    CUtil::GetAlbumThumb(paras[0],paras[1],thumbFilename,false);
    flushResult(eid, wp, "<li>" + thumbFilename + "\n") ;
  }
  else
    flushResult(eid, wp,"<li>Error:Missing parameter (album;filename)\n") ;
  return 0;
}

int CXbmcHttp::xbmcPlayerPlayFile(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing parameter\n");
  else
  {
    CFileItem item(paras[0], FALSE);
    g_application.PlayMedia(item, g_playlistPlayer.GetCurrentPlaylist());
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcGetCurrentPlayList(int eid, webs_t wp)
{
  CStdString tmp;
  tmp.Format("%i", g_playlistPlayer.GetCurrentPlaylist());
  flushResult(eid, wp, "<li>" + tmp + "\n" );
  return 0;
}

int CXbmcHttp::xbmcSetCurrentPlayList(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1) 
    flushResult(eid, wp, "<li>Error:Missing playlist\n") ;
  else {
    g_playlistPlayer.SetCurrentPlaylist(atoi(paras[0].c_str()));
    flushResult(eid, wp, "<li>OK\n") ;
  }
  return 0;
}

int CXbmcHttp::xbmcGetPlayListContents(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString list="";
  int playList;

  if (numParas<1) 
    playList=g_playlistPlayer.GetCurrentPlaylist();
  else
    playList=atoi(paras[0]);
  CPlayList& thePlayList = g_playlistPlayer.GetPlaylist(playList);
  if (thePlayList.size()==0)
    list="<li>[Empty]" ;
  else
    for (int i=0; i< thePlayList.size(); i++) {
      const CPlayList::CPlayListItem& item=thePlayList[i];
      list += "\n<li>" + item.GetFileName();
    }
  list+="\n";
  flushResult(eid, wp, list) ;
  return 0;
}

int CXbmcHttp::xbmcGetSlideshowContents(int eid, webs_t wp)
{
  CStdString list="";
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    flushResult(eid, wp,"<li>Error\n");
  else
  {
    vector<CStdString> slideshowContents = pSlideShow->GetSlideShowContents();
    if ((int)slideshowContents.size()==0)
      list="<li>[Empty]" ;
    else
    for (int i = 0; i < (int)slideshowContents.size(); ++i)
      list += "\n<li>" + slideshowContents[i];
    list+="\n";
    flushResult(eid, wp, list) ;
  }
  return 0;
}

int CXbmcHttp::xbmcGetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[])
{
  bool success=false;
  CStdString Filename;
  int iSong;

  if (numParas<1) 
  {
    CStdString tmp;
    tmp.Format("%i", g_playlistPlayer.GetCurrentSong());
    flushResult(eid, wp, "<li>" + tmp + "\n");
    success=true;
  }
  else {
    CPlayList thePlayList;
    iSong=atoi(paras[0]);  
    if (iSong!=-1){
      thePlayList=g_playlistPlayer.GetPlaylist( g_playlistPlayer.GetCurrentPlaylist() );
      if (thePlayList.size()>iSong) {
        Filename=thePlayList[iSong].GetFileName();
        flushResult(eid, wp, "\n<li>Filename:" + Filename + "\n");
        success=true;
      }
    }
  }
  if (!success)
    flushResult(eid, wp,"<li>Error\n");
  return 0;
}

int CXbmcHttp::xbmcSetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing song number\n");
  else
  {
    flushResult(eid, wp, "<li>OK\n");
    g_playlistPlayer.Play(atoi(paras[0].c_str()));
  }
  return 0;
}

int CXbmcHttp::xbmcPlayListNext(int eid, webs_t wp)
{
  g_playlistPlayer.PlayNext();
  flushResult(eid, wp, "<li>OK\n");
  return 0;
}

int CXbmcHttp::xbmcPlayListPrev(int eid, webs_t wp)
{
  g_playlistPlayer.PlayPrevious();
  flushResult(eid, wp, "<li>OK\n");
  return 0;
}

int CXbmcHttp::xbmcRemoveFromPlayList(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas>0)
  {
    if (numParas==1)
      g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).Remove(paras[0]) ;
    else
      g_playlistPlayer.GetPlaylist(atoi(paras[1])).Remove(paras[0]) ;
    flushResult(eid, wp, "<li>OK\n");
  }
  else
    flushResult(eid, wp, "<li>Error:Missing parameter\n");
  return 0;
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

int CXbmcHttp::xbmcSetKey(int eid, webs_t wp, int numParas, CStdString paras[])
{
  DWORD dwButtonCode=0;
  BYTE bLeftTrigger=0, bRightTrigger=0;
  float fLeftThumbX=0.0f, fLeftThumbY=0.0f, fRightThumbX=0.0f, fRightThumbY=0.0f ;
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing parameters\n");
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
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcAction(int eid, webs_t wp, int numParas, CStdString paras[], int theAction)
{
  bool showingSlideshow=(m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW);
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  CAction action;

  switch(theAction)
  {
  case 1:
    flushResult(eid, wp, "<li>OK\n");
    if (pSlideShow) {
      action.wID = ACTION_PAUSE;
      pSlideShow->OnAction(action);    
    }
    else
      g_applicationMessenger.MediaPause();
    break;
  case 2:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.MediaStop();
    break;
  case 3:
    flushResult(eid, wp, "<li>OK\n");
    if (showingSlideshow) {
      if (pSlideShow) {
        action.wID = ACTION_NEXT_PICTURE;
        pSlideShow->OnAction(action);
      }
    }
    else
      g_playlistPlayer.PlayNext();
    break;
  case 4:
    flushResult(eid, wp, "<li>OK\n");
    if (showingSlideshow) {
      if (pSlideShow) {
        action.wID = ACTION_PREV_PICTURE;
        pSlideShow->OnAction(action);    
      }
    }
    else
      g_playlistPlayer.PlayPrevious();
    break;
  case 5:
    if (showingSlideshow)
    {
      if (pSlideShow) {
        flushResult(eid, wp, "<li>OK\n");
        action.wID = ACTION_ROTATE_PICTURE;
        pSlideShow->OnAction(action);  
      }
    }
    else
      flushResult(eid, wp, "<li>Error\n");
    break;
  case 6:
    if (showingSlideshow)
    {
      if (pSlideShow) {
        if (numParas>1) {
          flushResult(eid, wp, "<li>OK\n");
          action.wID = ACTION_ANALOG_MOVE;
          action.fAmount1=(float) atof(paras[0]);
          action.fAmount2=(float) atof(paras[1]);
          pSlideShow->OnAction(action);    
        }
        else
          flushResult(eid, wp, "<li>Error:Missing parameters\n");
      }
    }
    else
      flushResult(eid, wp, "<li>Error\n");
    break;
  case 7:
    if (showingSlideshow)
    {
      if (pSlideShow) {
        if (numParas>0)
        {
          flushResult(eid, wp, "<li>OK\n");
          action.wID = ACTION_ZOOM_LEVEL_NORMAL+atoi(paras[0]);
          pSlideShow->OnAction(action);    
        }
        else
          flushResult(eid, wp, "<li>Error:Missing parameters\n");
      }
    }
    else
      flushResult(eid, wp, "<li>Error\n");
    break;
  default:
    flushResult(eid, wp, "<li>Error\n");
  }
  return 0;
}

int CXbmcHttp::xbmcExit(int eid, webs_t wp, int theAction)
{
  switch(theAction)
  {
  case 1:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.Restart();
    break;
  case 2:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.Shutdown();
    break;
  case 3:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.RebootToDashBoard();
    break;
  case 4:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.Reset();
    break;
  case 5:
    flushResult(eid, wp, "<li>OK\n");
    g_applicationMessenger.RestartApp();
    break;
  default:
    flushResult(eid, wp, "<li>Error\n");
  }
  return 0;
}

int CXbmcHttp::xbmcLookupAlbum(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString albums="";
  CMusicInfoScraper scraper;

  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing album name\n");
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
          albums += "\n<li>" + info.GetTitle2() + "<@@>" + info.GetAlbumURL();
        }
        flushResult(eid, wp, albums) ;
        }
      }
    }
    catch (...)
    {
      flushResult(eid, wp,"<li>Error");
    }
  }
  return 0;
}

int CXbmcHttp::xbmcChooseAlbum(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString output="";

  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing album name\n");
  else
    try
    {
      CMusicAlbumInfo musicInfo("", paras[0]) ;
      CHTTP http;
      if (musicInfo.Load(http))
      {
        output="<li>image:" + musicInfo.GetImageURL() + "\n";
        output+="<li>review:" + musicInfo.GetReview()+ "\n";
        flushResult(eid, wp, output) ;
      }
      else
        flushResult(eid, wp,"<li>Error:Loading musinInfo");
    }
    catch (...)
    {
      flushResult(eid, wp,"<li>Error:Exception");
    }
  return 0;
}

int CXbmcHttp::xbmcDownloadInternetFile(int eid, webs_t wp, int numParas, CStdString paras[])
{
  CStdString src, dest="";

  if (numParas<1)
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    src=paras[0];
    if (numParas>1)
      dest=paras[1];
    if (dest=="")
      dest="Z:\\xbmcDownloadInternetFile.tmp" ;
    if (src=="")
      flushResult(eid, wp,"<li>Error:Missing parameter\n");
    else
    {
      try
      {
        CHTTP http;
        http.Download(src, dest);
        CStdString encoded="";
        encoded=encodeFileToBase64(dest, 80);
        if (encoded=="")
          flushResult(eid, wp,"<li>Error:Nothing downloaded");
        {
          flushResult(eid, wp, encoded) ;
          if (dest=="Z:\\xbmcDownloadInternetFile.tmp")
          ::DeleteFile(dest);
        }
      }
      catch (...)
      {
        flushResult(eid, wp,"<li>Error:Exception");
      }
    }
  }
  return 0;
}

int CXbmcHttp::xbmcSetFile(int eid, webs_t wp, int numParas, CStdString paras[])
//parameter = destFilename ; base64String
{
  if (numParas<2)
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    paras[1].Replace(" ","+");
    decodeBase64ToFile(paras[1], "Z:\\xbmcTemp.tmp");
    CFile::Cache("Z:\\xbmcTemp.tmp", paras[0].c_str(), NULL, NULL) ;
    ::DeleteFile("Z:\\xbmcTemp.tmp");
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcCopyFile(int eid, webs_t wp, int numParas, CStdString paras[])
//parameter = srcFilename ; destFilename
// both file names are relative to the XBox not the calling client
{
  if (numParas<2)
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    if (CFile::Exists(paras[0].c_str()))
    {
      CFile::Cache(paras[0].c_str(), paras[1].c_str(), NULL, NULL) ;
      flushResult(eid, wp, "<li>OK\n");
    }
    else
      flushResult(eid, wp, "<li>Error:Source file not found\n");
  }
  return 0;
}

int CXbmcHttp::xbmcDeleteFile(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1) 
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    try
    {
      if (CFile::Exists(paras[0].c_str()))
      {
        ::DeleteFile(paras[0].c_str());
        flushResult(eid, wp, "<li>OK\n");
      }
      else
        flushResult(eid, wp, "<li>Error:File not found\n");
    }
    catch (...)
    {
      flushResult(eid, wp,"<li>Error");
    }
  }
  return 0;
}

int CXbmcHttp::xbmcFileExists(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1) 
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    try
    {
      if (CFile::Exists(paras[0].c_str()))
      {
        flushResult(eid, wp, "<li>True\n");
      }
      else
        flushResult(eid, wp, "<li>False\n");
    }
    catch (...)
    {
      flushResult(eid, wp,"<li>Error");
    }
  }
  return 0;
}

int CXbmcHttp::xbmcShowPicture(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1) 
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    g_applicationMessenger.PictureShow(paras[0]);
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcGetCurrentSlide(int eid, webs_t wp)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    flushResult(eid, wp,"<li>Error:Could not access slideshown");
  else
    flushResult(eid, wp, "<li>" + pSlideShow->GetCurrentSlide() + "\n");
  return 0;
}

int CXbmcHttp::xbmcExecBuiltIn(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1) 
    flushResult(eid, wp,"<li>Error:Missing parameter\n");
  else
  {
    ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
    tMsg.strParam = paras[0];
    g_applicationMessenger.SendMessage(tMsg);
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int CXbmcHttp::xbmcGUISetting(int eid, webs_t wp, int numParas, CStdString paras[])
//parameter=type;name(;value)
//type=0->int, 1->bool, 2->float
{
  if (numParas<2)
    flushResult(eid, wp, "<li>Error:Missing parameters\n");
  else
  {
    CStdString tmp;
    if (numParas<3)
      switch (atoi(paras[0])) 
      {
        case 0:  //  int
          tmp.Format("%i", g_guiSettings.GetInt(paras[1]));
          flushResult(eid, wp, "<li>" + tmp );
          break;
        case 1: // bool
          g_guiSettings.GetBool(paras[1])==0?
          flushResult(eid, wp, "<li>False"):
          flushResult(eid, wp, "<li>True");
          break;
        case 2: // float
          tmp.Format("%f", g_guiSettings.GetFloat(paras[1]));
          flushResult(eid, wp, "<li>" + tmp);
          break;
      }
    else
    {
      switch (atoi(paras[0])) 
      {
        case 0:  //  int
          g_guiSettings.SetInt(paras[1], atoi(paras[2]));
          break;
        case 1: // bool
          g_guiSettings.SetBool(paras[1], (paras[2].ToLower()=="true"));
          break;
        case 2: // float
          g_guiSettings.SetFloat(paras[1], (float)atof(paras[2]));
          break;
      }     
      flushResult(eid, wp, "<li>OK");
    }
  }
  return 0;
}

int CXbmcHttp::xbmcConfig(int eid, webs_t wp, int numParas, CStdString paras[])
{
  int argc=0;
  char_t* argv[20];

  if (numParas<1) {
    flushResult(eid, wp, "<li>Error:Missing paramters");
    return 0;
  }
  if (numParas>1){
    for (argc=0; argc<numParas-1;argc++)
      argv[argc]=(char_t*)paras[argc+1].c_str();
  }
  argv[argc]=NULL;
  if (paras[0]=="bookmarksize")
    XbmcWebsAspConfigBookmarkSize(-1, wp, argc, argv);
  else if (paras[0]=="getbookmark")
    XbmcWebsAspConfigGetBookmark(-1, wp, argc, argv);
  else if (paras[0]=="addbookmark") 
  {
    if (XbmcWebsAspConfigAddBookmark(-1, wp, argc, argv)==0)
      flushResult(eid, wp, "<li>OK\n");
  }
  else if (paras[0]=="savebookmark")
  {
    if (XbmcWebsAspConfigSaveBookmark(-1, wp, argc, argv)==0)
      flushResult(eid, wp, "<li>OK\n");
  }
  else if (paras[0]=="removebookmark")
  {
    if (XbmcWebsAspConfigRemoveBookmark(-1, wp, argc, argv)==0)
      flushResult(eid, wp, "<li>OK\n");
  }
  else if (paras[0]=="saveconfiguration")
  {
    if (XbmcWebsAspConfigSaveConfiguration(-1, wp, argc, argv)==0)
      flushResult(eid, wp, "<li>OK\n");
  }
  else if (paras[0]=="getoption")
    XbmcWebsAspConfigGetOption(-1, wp, argc, argv);
  else if (paras[0]=="setoption")
  {
    if (XbmcWebsAspConfigSetOption(-1, wp, argc, argv)==0)
      flushResult(eid, wp, "<li>OK\n");
  }
  else
    flushResult(eid, wp, "<li>Error:Unknown Config Command");
  return 0;
}

int CXbmcHttp::xbmcGetSystemInfo(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
  else
  {
    CStdString strInfo = "";
    int i;
    for (i=0; i<numParas; i++)
      strInfo += "<li>" + (CStdString) g_infoManager.GetLabel(atoi(paras[i])) + "\n";
    if (strInfo == "")
      strInfo="Error:No information retrieved\n";
    flushResult(eid, wp, strInfo);
  }
  return 0;
}

int CXbmcHttp::xbmcGetSystemInfoByName(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing Parameter\n");
  else
  {
    CStdString strInfo = "";
    int i;

    for (i=0; i<numParas; i++)
      strInfo += "<li>" + (CStdString) g_infoManager.GetLabel(g_infoManager.TranslateString(paras[i])) + "\n";
    if (strInfo == "")
      strInfo="Error:No information retrieved\n";
    flushResult(eid, wp, strInfo);
  }
  return 0;
}

int CXbmcHttp::xbmcTakeScreenshot(int eid, webs_t wp, int numParas, CStdString paras[])
//no paras
//filename, flash
//filename, flash, rotation, width, height, quality
//filename, flash, rotation, width, height, quality, download
//filename can be blank
{
  if (numParas<1)
    CUtil::TakeScreenshot();
  else
  {
    CStdString filename;
    if (paras[0]=="")
      filename="q:\\screenshot.jpg";
    else
      filename=paras[0];
    if (numParas<2)
    {
        flushResult(eid, wp, "<li>Error:Could not convert image\n");
        return 0;
    }
    else
    {
      CUtil::TakeScreenshot("q:\\temp.bmp", paras[1].ToLower()=="true");
      if (numParas>5)
      {
        int height, width;
        if (paras[4]=="")
          if (paras[3]=="")
          {
            flushResult(eid, wp, "<li>Error:Both height and width parameters cannot be absent\n");
            return 0;
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
        ret=pic.ConvertFile("q:\\temp.bmp", filename, (float) atof(paras[2]), width, height, atoi(paras[5]));
        if (ret==0)
        {
          ::DeleteFile("q:\\temp.bmp");
          if (numParas>6)
            if (paras[6].ToLower()="true")
              {
                CStdString b64;
                b64=encodeFileToBase64(filename,80);
                flushResult(eid, wp, b64) ;
                if (filename=="q:\\screenshot.jpg")
                  ::DeleteFile(filename.c_str());
                return 0;
              }
        }
        else
        {
          CStdString strInt;
          strInt.Format("%", ret);
          flushResult(eid, wp, "<li>Error:Could not convert image, error: " + strInt + "\n");
          return 0;
        }
      }
      else
      {
        flushResult(eid, wp, "<li>Error:Missing parameters\n");
        return 0;
      }
    }
  }
  flushResult(eid, wp, "<li>OK\n");
  return 0;
}

int CXbmcHttp::xbmcAutoGetPictureThumbs(int eid, webs_t wp, int numParas, CStdString paras[])
{
  if (numParas<1)
    flushResult(eid, wp, "<li>Error:Missing parameter\n");
  else
  {
    autoGetPictureThumbs = (paras[0].ToLower()=="true");
    flushResult(eid, wp, "<li>OK\n");
  }
  return 0;
}

int  CXbmcHttp::xbmcHelp(int eid, webs_t wp)
{
  CStdString output;
  output="<p><b>XBMC HTTP API Commands</b></p><p><b>Syntax: http://xbox/xbmcCmds/xbmcHttp?command=</b>one_of_the_following_commands<b>&ampparameter=</b>first_parameter<b>;</b>second_parameter<b>;...</b></p><p>Note the use of the semi colon to separate multiple parameters</p><p>The commands are case insensitive.</p><p>For more information see the readme.txt and the source code of the demo client application on SourceForge.</p>";

  output+= "<p>The full documentation as a Word file can be downloaded here: <a  href=\"http://prdownloads.sourceforge.net/xbmc/apiDoc.doc?download\">http://prdownloads.sourceforge.net/xbmc/apiDoc.doc?download</a></p>";

  output+= "<li>clearplaylist\n<li>addtoplaylist\n<li>playfile\n<li>pause\n<li>stop\n<li>restart\n<li>shutdown\n<li>exit\n<li>reset\n<li>restartapp\n<li>getcurrentlyplaying\n<li>getdirectory\n<li>gettagfromfilename\n<li>getcurrentplaylist\n<li>setcurrentplaylist\n<li>getplaylistcontents\n<li>removefromplaylist\n<li>setplaylistsong\n<li>getplaylistsong\n<li>playlistnext\n<li>playlistprev\n<li>getpercentage\n<li>seekpercentage\n<li>setvolume\n<li>getvolume\n<li>getthumbfilename\n<li>lookupalbum\n<li>choosealbum\n<li>downloadinternetfile\n<li>getmoviedetails\n<li>showpicture\n<li>sendkey\n<li>filedelete\n<li>filecopy\n<li>fileexists\n<li>fileupload\n<li>getguistatus\n<li>execbuiltin\n<li>config\n<li>getsysteminfo\n<li>getsysteminfobyname\n<li>guisetting\n<li>addtoslideshow\n<li>clearslideshow\n<li>playslideshow\n<li>getslideshowcontents\n<li>slideshowselect\n<li>getcurrentslide\n<li>rotate\n<li>move\n<li>zoom\n<li>playnext\n<li>playprev\n<li>TakeScreenShot\n<li>GetGUIDescription\n<li>GetPlaySpeed\n<li>SetPlaySpeed\n<li>help";

  flushResult(eid, wp,output);
  return 0;
}

/* Parse an XBMC HTTP API command */
int CXbmcHttp::xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter)
{
  int retVal, numParas;
  CStdString paras[MAX_PARAS];
  if (strlen(parameter)<300)
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: %s", command, parameter);
  else
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: [not recorded]", command);
  numParas = splitParameter(parameter, paras);
  if( eid == NO_EID)
    websHeader(wp);
  if (!stricmp(command, "clearplaylist"))                 retVal =  xbmcClearPlayList(eid, wp, numParas, paras);  
  else if (!stricmp(command, "addtoplaylist"))            retVal =  xbmcAddToPlayList(eid, wp, numParas, paras);  
  else if (!stricmp(command, "playfile"))                 retVal =  xbmcPlayerPlayFile(eid, wp, numParas, paras); 
  else if (!stricmp(command, "pause"))                    retVal =  xbmcAction(eid, wp, numParas, paras,1);
  else if (!stricmp(command, "stop"))                     retVal =  xbmcAction(eid, wp, numParas, paras,2);
  else if (!stricmp(command, "playnext"))                 retVal =  xbmcAction(eid, wp, numParas, paras,3);
  else if (!stricmp(command, "playprev"))                 retVal =  xbmcAction(eid, wp, numParas, paras,4);
  else if (!stricmp(command, "rotate"))                   retVal =  xbmcAction(eid, wp, numParas, paras,5);
  else if (!stricmp(command, "move"))                     retVal =  xbmcAction(eid, wp, numParas, paras,6);
  else if (!stricmp(command, "zoom"))                     retVal =  xbmcAction(eid, wp, numParas, paras,7);
  else if (!stricmp(command, "restart"))                  retVal =  xbmcExit(eid, wp,1);
  else if (!stricmp(command, "shutdown"))                 retVal =  xbmcExit(eid, wp,2);
  else if (!stricmp(command, "exit"))                     retVal =  xbmcExit(eid, wp,3);
  else if (!stricmp(command, "reset"))                    retVal =  xbmcExit(eid, wp,4);
  else if (!stricmp(command, "restartapp"))               retVal =  xbmcExit(eid, wp,5);
  else if (!stricmp(command, "getcurrentlyplaying"))      retVal =  xbmcGetCurrentlyPlaying(eid, wp); 
  else if (!stricmp(command, "getdirectory"))             retVal =  xbmcGetDirectory(eid, wp, numParas, paras); 
  else if (!stricmp(command, "gettagfromfilename"))       retVal =  xbmcGetTagFromFilename(eid, wp, numParas, paras);
  else if (!stricmp(command, "getcurrentplaylist"))       retVal =  xbmcGetCurrentPlayList(eid, wp);
  else if (!stricmp(command, "setcurrentplaylist"))       retVal =  xbmcSetCurrentPlayList(eid, wp, numParas, paras);
  else if (!stricmp(command, "getplaylistcontents"))      retVal =  xbmcGetPlayListContents(eid, wp, numParas, paras);
  else if (!stricmp(command, "removefromplaylist"))       retVal =  xbmcRemoveFromPlayList(eid, wp, numParas, paras);
  else if (!stricmp(command, "setplaylistsong"))          retVal =  xbmcSetPlayListSong(eid, wp, numParas, paras);
  else if (!stricmp(command, "getplaylistsong"))          retVal =  xbmcGetPlayListSong(eid, wp, numParas, paras);
  else if (!stricmp(command, "playlistnext"))             retVal =  xbmcPlayListNext(eid, wp);
  else if (!stricmp(command, "playlistprev"))             retVal =  xbmcPlayListPrev(eid, wp);
  else if (!stricmp(command, "getpercentage"))            retVal =  xbmcGetPercentage(eid, wp);
  else if (!stricmp(command, "seekpercentage"))           retVal =  xbmcSeekPercentage(eid, wp, numParas, paras);
  else if (!stricmp(command, "setvolume"))                retVal =  xbmcSetVolume(eid, wp, numParas, paras);
  else if (!stricmp(command, "getvolume"))                retVal =  xbmcGetVolume(eid, wp);
  else if (!stricmp(command, "setplayspeed"))             retVal =  xbmcSetPlaySpeed(eid, wp, numParas, paras);
  else if (!stricmp(command, "getplayspeed"))             retVal =  xbmcGetPlaySpeed(eid, wp);
  else if (!stricmp(command, "filedownload"))             retVal =  xbmcGetThumb(eid, wp, numParas, paras);
  else if (!stricmp(command, "getthumbfilename"))         retVal =  xbmcGetThumbFilename(eid, wp, numParas, paras);
  else if (!stricmp(command, "lookupalbum"))              retVal =  xbmcLookupAlbum(eid, wp, numParas, paras);
  else if (!stricmp(command, "choosealbum"))              retVal =  xbmcChooseAlbum(eid, wp, numParas, paras);
  else if (!stricmp(command, "filedownloadfrominternet")) retVal =  xbmcDownloadInternetFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "filedelete"))               retVal =  xbmcDeleteFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "filecopy"))                 retVal =  xbmcCopyFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "getmoviedetails"))          retVal =  xbmcGetMovieDetails(eid, wp, numParas, paras);
  else if (!stricmp(command, "showpicture"))              retVal =  xbmcShowPicture(eid, wp, numParas, paras);
  else if (!stricmp(command, "sendkey"))                  retVal =  xbmcSetKey(eid, wp, numParas, paras);
  else if (!stricmp(command, "fileexists"))               retVal =  xbmcFileExists(eid, wp, numParas, paras);
  else if (!stricmp(command, "fileupload"))               retVal =  xbmcSetFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "getguistatus"))             retVal =  xbmcGetGUIStatus(eid, wp);
  else if (!stricmp(command, "execbuiltin"))              retVal =  xbmcExecBuiltIn(eid, wp, numParas, paras);
  else if (!stricmp(command, "config"))                   retVal =  xbmcConfig(eid, wp, numParas, paras);
  else if (!stricmp(command, "help"))                     retVal =  xbmcHelp(eid, wp);
  else if (!stricmp(command, "getsysteminfo"))            retVal =  xbmcGetSystemInfo(eid, wp, numParas, paras);
  else if (!stricmp(command, "getsysteminfobyname"))      retVal =  xbmcGetSystemInfoByName(eid, wp, numParas, paras);
  else if (!stricmp(command, "addtoslideshow"))           retVal =  xbmcAddToSlideshow(eid, wp, numParas, paras);
  else if (!stricmp(command, "clearslideshow"))           retVal =  xbmcClearSlideshow(eid, wp);
  else if (!stricmp(command, "playslideshow"))            retVal =  xbmcPlaySlideshow(eid, wp, numParas, paras);
  else if (!stricmp(command, "getslideshowcontents"))     retVal =  xbmcGetSlideshowContents(eid, wp);
  else if (!stricmp(command, "slideshowselect"))          retVal =  xbmcSlideshowSelect(eid, wp, numParas, paras);
  else if (!stricmp(command, "getcurrentslide"))          retVal =  xbmcGetCurrentSlide(eid, wp);
  else if (!stricmp(command, "getguisetting"))            retVal =  xbmcGUISetting(eid, wp, numParas, paras);
  else if (!stricmp(command, "setguisetting"))            retVal =  xbmcGUISetting(eid, wp, numParas, paras);
  else if (!stricmp(command, "takescreenshot"))           retVal =  xbmcTakeScreenshot(eid, wp, numParas, paras);
  else if (!stricmp(command, "getguidescription"))        retVal =  xbmcGetGUIDescription(eid, wp);
  else if (!stricmp(command, "setautogetpicturethumbs"))  retVal =  xbmcAutoGetPictureThumbs(eid, wp, numParas, paras);

  //Old command names
  else if (!stricmp(command, "deletefile"))               retVal =  xbmcDeleteFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "copyfile"))                 retVal =  xbmcCopyFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "downloadinternetfile"))     retVal =  xbmcDownloadInternetFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "getthumb"))                 retVal =  xbmcGetThumb(eid, wp, numParas, paras);
  else if (!stricmp(command, "guisetting"))               retVal =  xbmcGUISetting(eid, wp, numParas, paras);
  else if (!stricmp(command, "setfile"))                  retVal =  xbmcSetFile(eid, wp, numParas, paras);
  else if (!stricmp(command, "setkey"))                   retVal =  xbmcSetKey(eid, wp, numParas, paras);

  else {
    if( eid == NO_EID)
      flushResult(eid, wp, "<li>Error:unknown command\n");
    retVal = 0;
  }
  if( eid == NO_EID)
    websFooter(wp);
  CLog::Log(LOGDEBUG, "HttpApi End command: %s", command);
  return retVal;
}


/* XBMC Javascript binding for ASP. This will be invoked when "APICommand" is
 *  embedded in an ASP page.
 */
int CXbmcHttp::xbmcCommand( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*command, *parameter;

	int parameters = ejArgs(argc, argv, T("%s %s"), &command, &parameter);
	if (parameters < 1) {
		websError(wp, 500, T("Insufficient args\n"));
		return -1;
	}
	else if (parameters < 2) parameter = "";

	return xbmcProcessCommand( eid, wp, command, parameter);
}

/* XBMC form for posted data (in-memory CGI). 
 */
void CXbmcHttp::xbmcForm(webs_t wp, char_t *path, char_t *query)
{
  char_t  *command, *parameter;

  command = websGetVar(wp, WEB_COMMAND, XBMC_NONE); 
  parameter = websGetVar(wp, WEB_PARAMETER, XBMC_NONE);

  // do the command

  xbmcProcessCommand( NO_EID, wp, command, parameter);

  if (wp->timeout!=-1)
    websDone(wp, 200);
  else
    CLog::Log(LOGERROR, "HttpApi Timeout command: %s  paras: %s", command, parameter);
}
