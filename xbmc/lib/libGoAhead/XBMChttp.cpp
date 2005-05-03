
/******************************** Description *********************************/

/*
 *	This module provides an API over HTTP between the web server and XBMC
 *
 *						heavily based on XBMCweb.cpp
 */

/********************************* Includes ***********************************/

#pragma once


#include "../../stdafx.h"
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
//#include "..\..\guilib\key.h"
//#include "..\..\url.h" 

#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

#define XML_MAX_INNERTEXT_SIZE 256

#define NO_EID -1

/***************************** Definitions ***************************/
/* Define common commands */

#define XBMC_NONE			T("none")

CXbmcHttp* pXbmcHttp;


//int displayDir(webs_t wp, char *dir) {
//	CStdString output="";
//	WIN32_FIND_DATA FindFileData;
//	HANDLE hFind;
//	vector<string> vecFiles;
//	strcat(dir, "\\*");
//	hFind=FindFirstFile(dir, &FindFileData);
//
//	do
//	{
//
//		CStdString fn = FindFileData.cFileName;
//		CStdString start=fn.substr(0,1);
//
//		fn=start.ToUpper()+fn.substr(1,fn.length()-1);
//		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) fn+= "/";
//		vecFiles.push_back(fn);
//	}
//	while (FindNextFile(hFind, &FindFileData));
//	FindClose(hFind);
//
//	sort(vecFiles.begin(), vecFiles.end());
//	vector<string>::iterator it = vecFiles.begin();
//	while (it != vecFiles.end())
//	{
//		output+="\n<li>" + *it;
//		++it;
//	}
//	websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
//	return 0;
//}



/*
** Translation Table as described in RFC1113
*/
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
	if (infile != 0) {
		while( !feof( infile ) ) {
			len = 0;
			for( i = 0; i < 3; i++ ) {
				in[i] = (unsigned char) getc( infile );
				if( !feof( infile ) ) {
					len++;
				}
				else {
					in[i] = 0;
				}
			}
			if( len ) {
				encodeblock( in, out, len );
				for( i = 0; i < 4; i++ ) {
					strBase64 += out[i];
				}
				blocksout++;
			}
			if( blocksout >= (linesize/4) || feof( infile ) ) {
				if( blocksout ) {
					strBase64 += "\r\n" ;
				}
				blocksout = 0;
			}
		}
		fclose(infile);
	}
	return strBase64;
}

/*
** Translation Table to decode (created by author)
*/
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";


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
	//char_t *outString;
	//websDecode64(outString,inString.c_str(),outfilename.length());
	try
	{
		outfile= fopen( outfilename.c_str(), "wb" );
		while( ptr < inString.length() ) {
			for( len = 0, i = 0; i < 4 && ptr < inString.length(); i++ ) {
				v = 0;
				while( ptr < inString.length() && v == 0 ) {
					v = (unsigned char) inString[ptr];
					ptr++;
					v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
					if( v ) {
						v = (unsigned char) ((v == '$') ? 0 : v - 61);
					}
				}
				if( ptr < inString.length() ) {
					len++;
					if( v ) {
						in[ i ] = (unsigned char) (v - 1);
					}
				}
				else {
					in[i] = 0;
				}
			}
			if( len ) {
				decodeblock( in, out );
				for( i = 0; i < len - 1; i++ ) {
					putc( out[i], outfile );
				}
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
				case 0:	//	Sort by Filename
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

				default:	//	Sort by Filename by default
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


int displayDir(webs_t wp, char *dir) {
	//mask = ".mp3|.wma" -> matching files
	//mask = "*" -> just folders
	//mask = "" -> all files and folder

	CFileItemList dirItems;
	CStdString output="";

	CStdString folderAndMask=dir, folder, mask;
	int p ;

	p=folderAndMask.Find(";");
	if (p>=0) {
		folder=folderAndMask.Left(p);
		mask=folderAndMask.Right(folderAndMask.size()-p-1);
	}
	else {
		folder=folderAndMask;
		mask="";
	}

	CFactoryDirectory factory;
	IDirectory *pDirectory = factory.Create(folder);

	if (!pDirectory) 
	{
		websWrite(wp, "<li>Error\n");
		return 0;
	}
	pDirectory->SetMask(mask);
	bool bResult=pDirectory->GetDirectory(folder,dirItems);
	if (!bResult)
	{
		websWrite(wp, "<li>Error:Not folder\n");
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
	}
	websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
	return 0;
}
void SetCurrentMediaItem(CFileItem& newItem)
{
	//	No audio file, we are finished here
	if (!newItem.IsAudio() )
		return;

	//	Get a reference to the item's tag
	CMusicInfoTag& tag=newItem.m_musicInfoTag;
	//	we have a audio file.
	//	Look if we have this file in database...
	bool bFound=false;
	if (g_musicDatabase.Open())
	{
		CSong song;
		bFound=g_musicDatabase.GetSongByFileName(newItem.m_strPath, song);
		tag.SetSong(song);
		g_musicDatabase.Close();
	}

	if (!bFound && g_guiSettings.GetBool("MyMusic.UseTags"))
	{
		//	...no, try to load the tag of the file.
		CMusicInfoTagLoaderFactory factory;
		auto_ptr<IMusicInfoTagLoader> pLoader(factory.CreateLoader(newItem.m_strPath));
		//	Do we have a tag loader for this file type?
		if (pLoader.get() != NULL)
			pLoader->Load(newItem.m_strPath,tag);
	}

	//	If we have tag information, ...
	if (tag.Loaded())
	{
		g_infoManager.SetCurrentSongTag(tag);
	}
}



void AddItemToPlayList(const CFileItem* pItem, int playList)
{
	if (pItem->m_bIsFolder)
	{
		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=pItem->m_strPath;
		CFileItemList items;
		CFactoryDirectory factory;
		IDirectory *pDirectory = factory.Create(strDirectory);
		bool bResult=pDirectory->GetDirectory(strDirectory,items);
    SSortWebFilesByName::m_bSortAscending = true;
    SSortWebFilesByName::m_iSortMethod = 0;
	  items.Sort(SSortWebFilesByName::Sort);
		for (int i=0; i < items.Size(); ++i)
			AddItemToPlayList(items[i], playList);
	}
	else
	{
		//selected item is a file, add it to playlist
		PLAYLIST::CPlayList::CPlayListItem playlistItem;
    CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
		//playlistItem.SetFileName(pItem->m_strPath);
		//playlistItem.SetDescription(pItem->GetLabel());
		//playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
		g_playlistPlayer.GetPlaylist(playList).Add(playlistItem);

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
  // load it
  if (!pPlayList->Load(item->m_strPath))
    return false;

  CPlayList& playlist = (*pPlayList);

  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // how many songs are in the new playlist
  if ((playlist.size() == 1) && (autoStart))
  {
    // just 1 song? then play it (no need to have a playlist of 1 song)
    CPlayList::CPlayListItem item = playlist[0];
    return g_application.PlayFile(CFileItem(item));
  }

  if (clearList)
  // clear current playlist
    g_playlistPlayer.GetPlaylist(iPlaylist).Clear();

  // if autoshuffle playlist on load option is enabled
  //  then shuffle the playlist
  // (dont do this for shoutcast .pls files)
  if (playlist.size())
  {
    const CPlayList::CPlayListItem& playListItem = playlist[0];
    if (!playListItem.IsShoutCast() && g_guiSettings.GetBool("MusicLibrary.ShufflePlaylistsOnLoad"))
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
    // if we got a playlist
    if (g_playlistPlayer.GetPlaylist( iPlaylist ).size() )
    {
      // then get 1st song
      CPlayList& playlist = g_playlistPlayer.GetPlaylist( iPlaylist );
      const CPlayList::CPlayListItem& item = playlist[0];

      // and start playing it
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

int CXbmcHttp::xbmcAddToPlayList( webs_t wp, char_t *parameter)
{


	CStdString strFileName;
	CStdString nameAndLevel=parameter, name, level;
	int playList, p ;

	p=nameAndLevel.Find(";");
	if (p>=0) {
		name=nameAndLevel.Left(p);
		playList=atoi(nameAndLevel.Right(nameAndLevel.size()-p-1));
	}
	else {
		name=parameter;
		playList=g_playlistPlayer.GetCurrentPlaylist();
	}
	strFileName=name ;
	strFileName=strFileName.Right(strFileName.size() - 1);
	CFileItem *pItem = new CFileItem(strFileName);
	pItem->m_strPath=name.c_str();
	if (pItem->IsPlayList())
		LoadPlayList(parameter,playList,false,false);
	else
	{
		CFactoryDirectory factory;
		IDirectory *pDirectory = factory.Create(pItem->m_strPath);
		bool bResult=pDirectory->Exists(pItem->m_strPath);
		pItem->m_bIsFolder=bResult;
		pItem->m_bIsShareOrDrive=false;
		//CMusicInfoTag& tag=pItem->m_musicInfoTag;

		//if (g_guiSettings.GetBool("MyMusic.UseTags"))
		//{
		//	// get correct tag parser
		//	CMusicInfoTagLoaderFactory factory;
		//	auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
		//	if (NULL != pLoader.get())
		//	{						
		//		// get id3tag
		//		if ( pLoader->Load(pItem->m_strPath,tag))
		//		{
		//		}
		//	}
		//}
		AddItemToPlayList(pItem, playList);
	}
	g_playlistPlayer.HasChanged();
	delete pItem;
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}


int CXbmcHttp::xbmcGetTagFromFilename( webs_t wp, char_t *parameter) 
{
	char buffer[XML_MAX_INNERTEXT_SIZE];
	CStdString strFileName;
	websHeader(wp);

	strFileName=CUtil::GetFileName(parameter);
	CFileItem *pItem = new CFileItem(strFileName);
	pItem->m_strPath=parameter;
	if (!pItem->IsAudio())
	{
		websWrite(wp,"<li>Error:Not Audio\n");
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
		if (g_guiSettings.GetBool("MyMusic.UseTags"))
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
				websWrite(wp,"<li>Error:Could not load TagLoader\n");
		}
		else
			websWrite(wp,"<li>Error:System not set to use tags\n");
	if (tag.Loaded())
	{
		websWrite(wp,"<li>Artist:");
		strcpy(buffer, tag.GetArtist().c_str());
		websWrite(wp, buffer);
		websWrite(wp,"\n<li>Album:");
		strcpy(buffer, tag.GetAlbum().c_str());
		websWrite(wp, buffer);
		websWrite(wp,"\n<li>Title:");
		strcpy(buffer, tag.GetTitle().c_str());
		websWrite(wp, buffer);
		websWrite(wp,"\n<li>Track number: %i",tag.GetTrackNumber());
		websWrite(wp,"\n<li>Duration: %i",tag.GetDuration());
		websWrite(wp,"\n<li>Genre:");
		strcpy(buffer, tag.GetGenre().c_str());
		websWrite(wp, buffer);
		SYSTEMTIME stTime;
		tag.GetReleaseDate(stTime);
		websWrite(wp,"\n<li>Release year: %i\n",stTime.wYear);
		pItem->SetMusicThumb();
		if (pItem->HasThumbnail())
			websWrite(wp, "<li>Thumb: %s\n",pItem->GetThumbnailImage().c_str());
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
				websWrite(wp, "<li>Thumb:[None] %s\n",strThumb.c_str());
			}
		}
	}
	else
		websWrite(wp,"<li>Error:No tag info\n");
	delete pItem;
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcClearPlayList( webs_t wp, char_t *parameter)
{
	int playList ;
	if ((*parameter==0) || !strcmp(parameter,"none"))
		playList = g_playlistPlayer.GetCurrentPlaylist() ;
	else
		playList=atoi(parameter) ;
	g_playlistPlayer.GetPlaylist( playList ).Clear();
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}

int  CXbmcHttp::xbmcGetDirectory(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if (strcmp(parameter,"none"))
		displayDir(wp, parameter);
	else
		websWrite(wp,"<li>Error:No path\n") ;

	websFooter(wp);
	return 0;
}


int CXbmcHttp::xbmcGetMovieDetails( webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if (strcmp(parameter,"none"))
	{
		CFileItem *item = new CFileItem(parameter);
		item->m_strPath = parameter ;
		if (item->IsVideo()) {
			CVideoDatabase m_database;
			CIMDBMovie aMovieRec;
			m_database.Open();
			if (m_database.HasMovieInfo(parameter))
			{
				CStdString thumb;
				m_database.GetMovieInfo(parameter,aMovieRec);
				websWrite(wp, "\n<li>Year: %i",aMovieRec.m_iYear);
				websWrite(wp, "\n<li>Director: %s",aMovieRec.m_strDirector.c_str());
				websWrite(wp, "\n<li>Title: %s",aMovieRec.m_strTitle.c_str());
				websWrite(wp, "\n<li>Plot: %s",aMovieRec.m_strPlot.c_str());
				websWrite(wp, "\n<li>Genre: %s",aMovieRec.m_strGenre.c_str());
				CStdString strRating;
				strRating.Format("%3.3f", aMovieRec.m_fRating);
				if (strRating=="") strRating="0.0";
				websWrite(wp, "\n<li>Rating: %s",strRating.c_str());
				websWrite(wp, "\n<li>Cast: %s",aMovieRec.m_strCast.c_str());
				CUtil::GetVideoThumbnail(aMovieRec.m_strIMDBNumber,thumb);
				if (!CUtil::FileExists(thumb))
					thumb = "[None] " + thumb;
				websWrite(wp, "\n<li>Thumb: %s",thumb.c_str());
			}
			else
				websWrite(wp, "\n<li>Thumb: [None]");
			m_database.Close();
		}
		delete item;
	}
	else
		websWrite(wp,"<li>Error:No file name\n") ;
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcGetCurrentlyPlaying( webs_t wp)
{
	CStdString fn=g_application.CurrentFile();
	websHeader(wp);
	if (fn=="")
		websWrite(wp, "<li>Filename:[Nothing Playing]");
	else
	{ 
		websWrite(wp, "<li>Filename:%s\n",fn.c_str());
		websWrite(wp,"\n<li>SongNo:%i\n", g_playlistPlayer.GetCurrentSong());
		__int64 lPTS=-1;
		if (g_application.IsPlayingVideo()) {
      lPTS = g_application.m_pPlayer->GetTime()/100;
			websWrite(wp, "\n<li>Type:Video") ;
		}
		else if (g_application.IsPlayingAudio()) {
      lPTS = g_application.m_pPlayer->GetTime()/100;
			websWrite(wp, "\n<li>Type:Audio") ;
		}
		CStdString thumb;

		CUtil::GetThumbnail(fn,thumb);
		if (!CUtil::FileExists(thumb))
			thumb = "[None] " + thumb;
		websWrite(wp, "\n<li>Thumb: %s",thumb.c_str());
		if (g_application.m_pPlayer->IsPaused()) 
			websWrite(wp, "\n<li>Paused:True") ;
		else
			websWrite(wp, "\n<li>Paused:False") ;
		if (g_application.IsPlaying()) 
			websWrite(wp, "\n<li>Playing:True") ;
		else
			websWrite(wp, "\n<li>Playing:False") ;
		if (lPTS!=-1)
		{					
			int hh = (int)(lPTS / 36000) % 100;
			int mm = (int)((lPTS / 600) % 60);
			int ss = (int)((lPTS /  10) % 60);

			char szTime[32];
			if (hh >=1)
			{
				sprintf(szTime,"%02.2i:%02.2i:%02.2i",hh,mm,ss);
			}
			else
			{
				sprintf(szTime,"%02.2i:%02.2i",mm,ss);
			}
			websWrite(wp, "\n<li>Time:");
			websWrite(wp, szTime);
			CStdString strTotalTime;
			unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
			if(tmpvar != 0)
			{
				int hh = tmpvar / 3600;
				int mm  = (tmpvar-hh*3600) / 60;
				int ss = (tmpvar-hh*3600) % 60;
				char szTime[32];
				if (hh >=1)
				{
					sprintf(szTime,"%02.2i:%02.2i:%02.2i",hh,mm,ss);
				}
				else
				{
					sprintf(szTime,"%02.2i:%02.2i",mm,ss);
				}
				websWrite(wp, "\n<li>Duration:");
				websWrite(wp, szTime);
				websWrite(wp, "\n");
			}
			websWrite(wp,"<li>Percentage:%i\n",(int)g_application.m_pPlayer->GetPercentage()) ;
		}
	}
	websFooter(wp);
	return 0;
}


int CXbmcHttp::xbmcGetPercentage( webs_t wp)
{
	websHeader(wp);
	if (g_application.m_pPlayer)
	{
		websWrite(wp,"<li>Percentage:%i\n",(int)g_application.m_pPlayer->GetPercentage()) ;
	}
	else
		websWrite(wp, "<li>Error\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcSeekPercentage( webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if (g_application.m_pPlayer)
	{
		g_application.m_pPlayer->SeekPercentage((float)atoi(parameter));

		websWrite(wp,"<li>OK\n") ;
	}
	else
		websWrite(wp, "<li>Error\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcSetVolume( webs_t wp, char_t *parameter)
{
	int iPercent = atoi(parameter) ;
	websHeader(wp);
	if (g_application.m_pPlayer!=0) {

	if (iPercent<0) iPercent = 0;
	if (iPercent>100) iPercent = 100;
	float fHardwareVolume = ((float)iPercent)/100.0f * (VOLUME_MAXIMUM-VOLUME_MINIMUM) + VOLUME_MINIMUM;
	g_stSettings.m_nVolumeLevel = (long)fHardwareVolume;
		// show visual feedback of volume change...
		if (!g_application.m_guiDialogVolumeBar.IsRunning())
			g_application.m_guiDialogVolumeBar.DoModal(m_gWindowManager.GetActiveWindow());
		g_application.m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
		websWrite(wp, "<li>OK\n");
	}
	else
		websWrite(wp, "<li>Error\n");
	websFooter(wp);
	return 0;
}


int CXbmcHttp::xbmcGetVolume( webs_t wp)
{
	int vol;
	websHeader(wp);
	if (g_application.m_pPlayer!=0)
		vol = int(((float)(g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM))/(VOLUME_MAXIMUM-VOLUME_MINIMUM)*100.0f+0.5f) ;
	else
		vol = -1;
	websWrite(wp, "<li>%i\n",vol);
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcGetGUIStatus( webs_t wp)
{
	websHeader(wp);
	websWrite(wp, "<li>ActiveWindow: %i\n",m_gWindowManager.GetActiveWindow());
	CStdString strTmp;
	int iWin=m_gWindowManager.GetActiveWindow();
	CGUIWindow* pWindow=m_gWindowManager.GetWindow(iWin);
	if (pWindow)
	{
		CStdString strLine;
		wstring wstrLine;
		wstrLine=g_localizeStrings.Get(iWin);
		CUtil::Unicode2Ansi(wstrLine,strLine);
		websWrite(wp, "<li>ActiveWindowName:%s",strLine.c_str()) ; 
		int iControl=pWindow->GetFocusedControl();
		CGUIControl* pControl=(CGUIControl* )pWindow->GetControl(iControl);
		if (pControl)
		{
			if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
				websWrite(wp, "<li>ControlButton:%s",((CGUIButtonControl*)pControl)->GetLabel().c_str());
			else if (pControl->GetControlType() == CGUIControl::GUICONTROL_SPIN)
			{
				CGUISpinControl* pSpinControl = (CGUISpinControl*)pControl;
				strTmp.Format("%i/%i", 1+pSpinControl->GetValue(), pSpinControl->GetMaximum());
				websWrite(wp, "<li>ControlSpin:%s",strTmp.c_str());
			}
			else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LABEL)
			{
				CGUIListControl* pListControl = (CGUIListControl*)pControl;
				pListControl->GetSelectedItem(strTmp);
				websWrite(wp, "<li>ControlLabel:%s",strTmp.c_str());
			}
			else if (pControl->GetControlType() == CGUIControl::GUICONTROL_THUMBNAIL)
			{
				CGUIThumbnailPanel* pThumbControl = (CGUIThumbnailPanel*)pControl;
				pThumbControl->GetSelectedItem(strTmp);
				websWrite(wp, "<li>ControlThumNail:%s",strTmp.c_str());
			}
			else if (pControl->GetControlType() == CGUIControl::GUICONTROL_LIST)
			{
				CGUIListControl* pListControl = (CGUIListControl*)pControl;
				pListControl->GetSelectedItem(strTmp);
				websWrite(wp, "<li>ControlList:%s",strTmp.c_str());
			}
		}
  }
  websFooter(wp);
  return 0;
}
int CXbmcHttp::xbmcGetThumb(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	CStdString thumb="";
	if (CUtil::IsRemote(parameter))
	{
		CStdString strDest="Z:\\xbmcDownloadFile.tmp";
		CFile file;
		file.Cache(parameter, strDest.c_str(),NULL,NULL) ;
		if (CUtil::FileExists(strDest))
		{
			thumb=encodeFileToBase64(strDest,80);
			::DeleteFile(strDest.c_str());
		}
		else
		{
			websWrite(wp, "<li>Error\n");
			websFooter(wp);
			return 0;
		}
	}
	else
		thumb=encodeFileToBase64(parameter,80);
	websWriteBlock(wp, (char_t *) thumb.c_str(), thumb.length()) ;
	websFooter(wp);
	return 0;
}


int CXbmcHttp::xbmcGetThumbFilename(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	CStdString thumbFilename="", filename="", album="", albumAndFilename=parameter;
	int p ;

	p=albumAndFilename.Find(";");
	if (p>=0) {
		album=albumAndFilename.Left(p);
		filename=albumAndFilename.Right(albumAndFilename.size()-p-1);
		CUtil::GetAlbumThumb(album,filename,thumbFilename,false);
		websWrite(wp, "<li>%s\n",thumbFilename.c_str()) ;
	}
	else
		websWrite(wp,"<li>Error:Missing parameter (album;filename)") ;
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcPlayerPlayFile( webs_t wp, char_t *parameter)
{
	websHeader(wp);
	CFileItem *item = new CFileItem(parameter);
	item->m_strPath = parameter ;
  g_application.PlayMedia((CStdString) parameter,g_playlistPlayer.GetCurrentPlaylist());
	//if (item->IsPlayList())
	//	LoadPlayList(parameter,g_playlistPlayer.GetCurrentPlaylist());
	//else {
	//	SetCurrentMediaItem(*item); 
	//	g_applicationMessenger.MediaStop();
	//	g_applicationMessenger.MediaPlay(parameter);
	//}
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	delete item;
	return 0;
}


int CXbmcHttp::xbmcGetCurrentPlayList( webs_t wp)
{
	websHeader(wp);
	websWrite(wp, "<li>%i\n", g_playlistPlayer.GetCurrentPlaylist());
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcSetCurrentPlayList( webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		websWrite(wp, "<li>Error:missing playlist\n") ;
	else {
		g_playlistPlayer.SetCurrentPlaylist(atoi(parameter));
		websWrite(wp, "<li>OK\n") ;
	}
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcGetPlayListContents( webs_t wp, char_t *parameter)
{
	CStdString list="";
	int playList;

	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		playList=g_playlistPlayer.GetCurrentPlaylist();
	else
		playList=atoi(parameter);
	CPlayList& thePlayList = g_playlistPlayer.GetPlaylist(playList);
	if (thePlayList.size()==0)
		list="<li>[Empty]" ;
	else
		for (int i=0; i< thePlayList.size(); i++) {
			const CPlayList::CPlayListItem& item=thePlayList[i];
			list += "\n<li>" + item.GetFileName();
		}
	list+="\n";
	websWriteBlock(wp, (char_t *) list.c_str(), list.length()) ;
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcGetPlayListSong( webs_t wp, char_t *parameter)
{
	bool success=false;
	CStdString Filename;
	int iSong;

	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
  {
		websWrite(wp, "<li>%i\n", g_playlistPlayer.GetCurrentSong());
    success=true;
  }
	else {
		CPlayList thePlayList;
		iSong=atoi(parameter);	
		if (iSong!=-1){
			websWrite(wp, "%s", "<li>Description:");
			thePlayList=g_playlistPlayer.GetPlaylist( g_playlistPlayer.GetCurrentPlaylist() );
			if (thePlayList.size()>iSong) {
				Filename=thePlayList[iSong].GetFileName();
				websWrite(wp, "\n<li>Filename:%s\n", Filename.c_str());
				success=true;
			}
		}
	}
	if (!success)
		websWrite(wp,"<li>Error\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcSetPlayListSong( webs_t wp, char_t *parameter)
{
	g_applicationMessenger.PlayListPlayerPlay(atoi(parameter));
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcPlayListNext( webs_t wp)
{
	g_applicationMessenger.PlayListPlayerNext();
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcPlayListPrev( webs_t wp)
{
	g_applicationMessenger.PlayListPlayerPrevious();
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}
int CXbmcHttp::xbmcRemoveFromPlayList( webs_t wp, char_t *parameter)
{

	CStdString strFileName;
	CStdString nameAndList=parameter, name;
	int playList=-1, p ;

	websHeader(wp);
	p=nameAndList.Find(";");
	if (p>=0) {
		name=nameAndList.Left(p);
		playList=atoi(nameAndList.Right(nameAndList.size()-p-1));
	}
	else 
		name=parameter;
	if (name!="") {
		if (playList==-1)
			g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).Remove(name) ;
		else
			g_playlistPlayer.GetPlaylist(playList).Remove(name) ;
		websWrite(wp, "<li>OK\n");
	}
	else
		websWrite(wp, "<li>Error\n");
	websFooter(wp);
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

int CXbmcHttp::xbmcSetKey( webs_t wp, char_t *parameter)
{
	CStdString paras=parameter;
	int p, pPrev=0;
	DWORD dwButtonCode=0;
	BYTE bLeftTrigger=0, bRightTrigger=0;
	float fLeftThumbX=0.0f, fLeftThumbY=0.0f, fRightThumbX=0.0f, fRightThumbY=0.0f ;
	p=paras.Find(";",pPrev);
	if (p==-1) {
		dwButtonCode=(DWORD) atoi(paras);
	}
	else {
		dwButtonCode=(DWORD) atoi(paras.Mid(pPrev,p-pPrev)) ;
		pPrev=p+1 ;
		p=paras.Find(";",pPrev);
		if (p>=0) {
			bLeftTrigger=(BYTE) atoi(paras.Mid(pPrev,p-pPrev)) ;
			pPrev=p+1 ;
			p=paras.Find(";",pPrev);
			if (p>=0) {
				bRightTrigger=(BYTE) atoi(paras.Mid(pPrev,p-pPrev)) ;
				pPrev=p+1 ;
				p=paras.Find(";",pPrev);
				if (p>=0) {
					fLeftThumbX=(float) atof(paras.Mid(pPrev,p-pPrev)) ;
					pPrev=p+1 ;
					p=paras.Find(";",pPrev);
					if (p>=0) {
						fLeftThumbY=(float) atof(paras.Mid(pPrev,p-pPrev)) ;
						pPrev=p+1 ;
						p=paras.Find(";",pPrev);
						if (p>=0) {
							fRightThumbX=(float) atof(paras.Mid(pPrev,p-pPrev)) ;
							pPrev=p+1 ;
							p=paras.Find(";",pPrev);
							if (p==-1)
								p=paras.length();
							if (p>=0) {
								fRightThumbY=(float) atof(paras.Mid(pPrev,p-pPrev)) ;
								pPrev=p+1 ;
							}
						}
					}
				}
			}
		}
	}
	CKey tempKey(dwButtonCode, bLeftTrigger, bRightTrigger, fLeftThumbX, fLeftThumbY, fRightThumbX, fRightThumbY) ;
	key = tempKey ;
	websHeader(wp);
	websWrite(wp, "<li>OK\n");
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcAction( webs_t wp, int theAction)
{
	websHeader(wp);
	switch(theAction)
	{
	case 1:
		websWrite(wp, "<li>OK\n");
		g_applicationMessenger.MediaPause();
		break;
	case 2:
		websWrite(wp, "<li>OK\n");
		g_applicationMessenger.MediaStop();
		break;
	default:
		websWrite(wp, "<li>Error\n");
	}
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcExit( webs_t wp, int theAction)
{
	websHeader(wp);
	switch(theAction)
	{
	case 1:
		websWrite(wp, "<li>OK\n");
		websFooter(wp);
		g_applicationMessenger.Restart();
		break;
	case 2:
		websWrite(wp, "<li>OK\n");
		websFooter(wp);
		g_applicationMessenger.Shutdown();
		break;
	case 3:
		websWrite(wp, "<li>OK\n");
		websFooter(wp);
		g_applicationMessenger.RebootToDashBoard();
		break;
	case 4:
		websWrite(wp, "<li>OK\n");
		websFooter(wp);
		g_applicationMessenger.Reset();
		break;
	case 5:
		websWrite(wp, "<li>OK\n");
		websFooter(wp);
		g_applicationMessenger.RestartApp();
		break;
	default:
		websWrite(wp, "<li>Error\n");
		websFooter(wp);
	}
	return 0;
}

int	CXbmcHttp::xbmcLookupAlbum(webs_t wp, char_t *parameter)
{
	CStdString albums="";
	CMusicInfoScraper scraper;

	websHeader(wp);
	try
	{
	  if (scraper.FindAlbuminfo(parameter))
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
			websWriteBlock(wp, (char_t *) albums.c_str(), albums.length()) ;
		  }
	  }
	}
	catch (...)
	{
		websWrite(wp,"<li>Error");
	}

	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcChooseAlbum(webs_t wp, char_t *parameter)
{
	CStdString output="";

	websHeader(wp);
	try
	{
		CMusicAlbumInfo musicInfo("",parameter) ;
		if (musicInfo.Load())
		{
			output="<li>image:" + musicInfo.GetImageURL() + "\n";
			output+="<li>review:" + musicInfo.GetReview()+ "\n";
			websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
		}
		else
			websWrite(wp,"<li>Error");
	}
	catch (...)
	{
		websWrite(wp,"<li>Error");
	}
	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcDownloadInternetFile(webs_t wp, char_t *parameter)
{
	CStdString srcAndDest=parameter, src, dest="";
	int p ;

	websHeader(wp);
	if (srcAndDest=="none")
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		p=srcAndDest.Find(";");
		if (p>=0) {
			src=srcAndDest.Left(p);
			dest=srcAndDest.Right(srcAndDest.size()-p-1);
		}
		else 
			src=parameter;
		if (dest=="")
			dest="Z:\\xbmcDownloadInternetFile.tmp" ;
		if (src=="")
			websWrite(wp,"<li>Error:Missing parameter");
		else
		{
			try
			{
				CHTTP http;
				http.Download(src,dest);
				CStdString encoded="";
				encoded=encodeFileToBase64(dest,80);
				if (encoded=="")
					websWrite(wp,"<li>Error:Nothing downloaded");
				{
					websWriteBlock(wp, (char_t *) encoded.c_str(), encoded.length()) ;
					if (dest=="Z:\\xbmcDownloadInternetFile.tmp")
					::DeleteFile(dest);
				}
			}
			catch (...)
			{
				websWrite(wp,"<li>Error");
			}
		}
	}
	websFooter(wp);
	return 0;

}
int	CXbmcHttp::xbmcSetFile(webs_t wp, char_t *parameter)
//parameter = destFilename ; base64String
{
	CStdString destAndb64String=parameter, b64String="", dest="";
	int p ;

	websHeader(wp);
	if (destAndb64String=="none")
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		p=destAndb64String.Find(";");
		if (p>=0) {
			dest=destAndb64String.Left(p);
			b64String=destAndb64String.Right(destAndb64String.size()-p-1);
			if (dest=="" || b64String=="")
				websWrite(wp,"<li>Error:Missing parameter");
			else
			{
				decodeBase64ToFile(b64String, "Z:\\xbmcTemp.tmp");
				CFile file;
				file.Cache("Z:\\xbmcTemp.tmp", dest,NULL,NULL) ;
				::DeleteFile("Z:\\xbmcTemp.tmp");
				websWrite(wp, "<li>OK\n");
			}
		}
		else 
			websWrite(wp,"<li>Error:Missing parameter");
	}
	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcCopyFile(webs_t wp, char_t *parameter)
//parameter = srcFilename ; destFilename
// both file names are relative to the XBox not the calling client
{
	CStdString srcAndDest=parameter, src="", dest="";
	int p ;

	websHeader(wp);
	if (srcAndDest=="none")
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		p=srcAndDest.Find(";");
		if (p>=0) {
			src=srcAndDest.Left(p);
			dest=srcAndDest.Right(srcAndDest.size()-p-1);
			if (dest=="" || src=="")
				websWrite(wp,"<li>Error:Missing parameter");
			else
			{
				CFile file;
				if (file.Exists(src))
				{
					file.Cache(src, dest,NULL,NULL) ;
					websWrite(wp, "<li>OK\n");
				}
				else
					websWrite(wp, "<li>Error:Source file not found\n");
			}
		}
		else 
			websWrite(wp,"<li>Error:Missing parameter");
	}
	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcDeleteFile(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		try
		{
			CFile file;
			if (file.Exists(parameter))
			{
				::DeleteFile(parameter);
				websWrite(wp, "<li>OK\n");
			}
			else
				websWrite(wp, "<li>Error:File not found\n");
		}
		catch (...)
		{
			websWrite(wp,"<li>Error");
		}
	}
	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcFileExists(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		try
		{
			CFile file;
			if (file.Exists(parameter))
			{
				websWrite(wp, "<li>True\n");
			}
			else
				websWrite(wp, "<li>False\n");
		}
		catch (...)
		{
			websWrite(wp,"<li>Error");
		}
	}
	websFooter(wp);
	return 0;
}

int	CXbmcHttp::xbmcShowPicture(webs_t wp, char_t *parameter)
{
	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
		CStdString filename = parameter;
		g_applicationMessenger.PictureShow(filename);
		websWrite(wp, "<li>OK\n");
	}
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcExecBuiltIn(webs_t wp, char_t *parameter)
{
  	websHeader(wp);
	if ((*parameter==0) || !strcmp(parameter,"none")) 
		websWrite(wp,"<li>Error:Missing parameter");
	else
	{
    CUtil::ExecBuiltIn((CStdString) parameter);
		websWrite(wp, "<li>OK\n");
	}
	websFooter(wp);
	return 0;
}

int CXbmcHttp::xbmcConfig(webs_t wp, char_t *parameter)
{
  int argc=0, i=0;
  unsigned int p=0, pPrev=0;
  CStdString cmdAndParas=parameter, cmd="";
  CStdString tmp[20];
  char_t* argv[20];
  websHeader(wp);
  p=cmdAndParas.Find(";");
  if (p!=-1){
    cmd=cmdAndParas.Left(p);
    pPrev=p;
    p=cmdAndParas.Find(";",p+1);
    while ((p!=-1) && (p<cmdAndParas.length()) && (argc<19))
    {
      tmp[argc++]=cmdAndParas.Mid(pPrev+1,p-pPrev-1);
      pPrev=p;
      p=cmdAndParas.Find(";",p+1);
    }
    if ((argc<19) && (pPrev<cmdAndParas.length()-1))
      tmp[argc++]=cmdAndParas.Right(cmdAndParas.length()-pPrev-1);
    for (i=0; i<argc; i++)
      argv[i]=(char_t*)tmp[i].c_str();
  }
  else
    cmd=cmdAndParas;
  argv[argc]=NULL;
  if (cmd=="bookmarksize")
    XbmcWebsAspConfigBookmarkSize(-1, wp, argc, argv);
  else if (cmd=="getbookmark")
    XbmcWebsAspConfigGetBookmark(-1, wp, argc, argv);
  else if (cmd=="addbookmark") 
  {
    if (XbmcWebsAspConfigAddBookmark(-1, wp, argc, argv)==0)
      websWrite(wp, "<li>OK\n");
  }
  else if (cmd=="savebookmark")
  {
    if (XbmcWebsAspConfigSaveBookmark(-1, wp, argc, argv)==0)
      websWrite(wp, "<li>OK\n");
  }
  else if (cmd=="removebookmark")
  {
    if (XbmcWebsAspConfigRemoveBookmark(-1, wp, argc, argv)==0)
      websWrite(wp, "<li>OK\n");
  }
  else if (cmd=="saveconfiguration")
  {
    if (XbmcWebsAspConfigSaveConfiguration(-1, wp, argc, argv)==0)
      websWrite(wp, "<li>OK\n");
  }
  else if (cmd=="getoption")
    XbmcWebsAspConfigGetOption(-1, wp, argc, argv);
  else if (cmd=="setoption")
  {
    if (XbmcWebsAspConfigSetOption(-1, wp, argc, argv)==0)
      websWrite(wp, "<li>OK\n");
  }
  else
    websWrite(wp, "<li>Error:Unknown Config Command");
  websFooter(wp);
  return 0;
}

int	CXbmcHttp::xbmcHelp(webs_t wp)
{
	websHeader(wp);

  websWrite(wp, "<p><b>XBMC HTTP API Commands</b></p><p><b>Syntax: http://xbox/xbmcCmds/xbmcHttp?command=</b>one_of_the_following_commands<b>&ampparameter=</b>first_parameter<b>;</b>second_parameter<b>;...</b></p><p>Note the use of the semi colon to separate multiple parameters</p><p>For more information see the readme.txt and the source code of the demo client application on SourceForge.</p>");

	websWrite(wp, "<li>clearplaylist\n<li>addtoplaylist\n<li>playfile\n<li>pause\n<li>stop\n<li>restart\n<li>shutdown\n<li>exit\n<li>reset\n<li>restartapp\n<li>getcurrentlyplaying\n<li>getdirectory\n<li>gettagfromfilename\n<li>getcurrentplaylist\n<li>setcurrentplaylist\n<li>getplaylistcontents\n<li>removefromplaylist\n<li>setplaylistsong\n<li>getplaylistsong\n<li>playlistnext\n<li>playlistprev\n<li>getpercentage\n<li>seekpercentage\n<li>setvolume\n<li>getvolume\n<li>getthumb\n<li>getthumbfilename\n<li>lookupalbum\n<li>choosealbum\n<li>downloadinternetfile\n<li>getmoviedetails\n<li>showpicture\n<li>setkey\n<li>deletefile\n<li>copyfile\n<li>fileexists\n<li>setfile\n<li>getguistatus\n<li>execbuiltin\n<li>config\n<li>help");

	websFooter(wp);
	return 0;
}

/* Parse an XBMC HTTP API command */
int CXbmcHttp::xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter)
{
	if (!strcmp(command, "clearplaylist"))              return xbmcClearPlayList(wp, parameter);  
	else if (!strcmp(command, "addtoplaylist"))         return xbmcAddToPlayList(wp, parameter);  
	else if (!strcmp(command, "playfile"))              return xbmcPlayerPlayFile(wp, parameter); 
	else if (!strcmp(command, "pause"))                 return xbmcAction(wp,1);
	else if (!strcmp(command, "stop"))					        return xbmcAction(wp,2);
	else if (!strcmp(command, "restart"))				        return xbmcExit(wp,1);
	else if (!strcmp(command, "shutdown"))				      return xbmcExit(wp,2);
	else if (!strcmp(command, "exit"))					        return xbmcExit(wp,3);
	else if (!strcmp(command, "reset"))					        return xbmcExit(wp,4);
	else if (!strcmp(command, "restartapp"))			  		return xbmcExit(wp,5);
	else if (!strcmp(command, "getcurrentlyplaying"))	  return xbmcGetCurrentlyPlaying(wp); 
	else if (!strcmp(command, "getdirectory"))			    return xbmcGetDirectory(wp, parameter); 
	else if (!strcmp(command, "gettagfromfilename"))  	return xbmcGetTagFromFilename(wp, parameter);
	else if (!strcmp(command, "getcurrentplaylist"))	  return xbmcGetCurrentPlayList(wp);
	else if (!strcmp(command, "setcurrentplaylist"))	  return xbmcSetCurrentPlayList(wp, parameter);
	else if (!strcmp(command, "getplaylistcontents"))	  return xbmcGetPlayListContents(wp, parameter);
	else if (!strcmp(command, "removefromplaylist"))  	return xbmcRemoveFromPlayList(wp, parameter);
	else if (!strcmp(command, "setplaylistsong"))		    return xbmcSetPlayListSong(wp, parameter);
	else if (!strcmp(command, "getplaylistsong"))	    	return xbmcGetPlayListSong(wp, parameter);
	else if (!strcmp(command, "playlistnext"))			    return xbmcPlayListNext(wp);
	else if (!strcmp(command, "playlistprev"))			    return xbmcPlayListPrev(wp);
	else if (!strcmp(command, "getpercentage"))		    	return xbmcGetPercentage(wp);
	else if (!strcmp(command, "seekpercentage"))	    	return xbmcSeekPercentage(wp, parameter);
	else if (!strcmp(command, "setvolume"))				      return xbmcSetVolume(wp, parameter);
	else if (!strcmp(command, "getvolume"))			      	return xbmcGetVolume(wp);
	else if (!strcmp(command, "getthumb"))			      	return xbmcGetThumb(wp, parameter);
	else if (!strcmp(command, "getthumbfilename"))  		return xbmcGetThumbFilename(wp, parameter);
	else if (!strcmp(command, "lookupalbum"))			      return xbmcLookupAlbum(wp, parameter);
	else if (!strcmp(command, "choosealbum"))			      return xbmcChooseAlbum(wp, parameter);
	else if (!strcmp(command, "downloadinternetfile"))	return xbmcDownloadInternetFile(wp, parameter);
	else if (!strcmp(command, "getmoviedetails"))		    return xbmcGetMovieDetails(wp, parameter);
	else if (!strcmp(command, "showpicture"))			      return xbmcShowPicture(wp, parameter);
	else if (!strcmp(command, "setkey"))				        return xbmcSetKey(wp, parameter);
	else if (!strcmp(command, "deletefile"))		      	return xbmcDeleteFile(wp, parameter);
	else if (!strcmp(command, "copyfile"))			      	return xbmcCopyFile(wp, parameter);
	else if (!strcmp(command, "fileexists"))		      	return xbmcFileExists(wp, parameter);
	else if (!strcmp(command, "setfile"))				        return xbmcSetFile(wp, parameter);
	else if (!strcmp(command, "getguistatus"))			    return xbmcGetGUIStatus(wp);
	else if (!strcmp(command, "execbuiltin"))			      return xbmcExecBuiltIn(wp, parameter);
  else if (!strcmp(command, "config"))	              return xbmcConfig(wp, parameter);
  else if (!strcmp(command, "help"))			            return xbmcHelp(wp);
	else {
		websHeader(wp);
		websWrite(wp, "<li>Error:unknown command\n");
		websFooter(wp);
	}
	return 0;
}



/* XBMC form for posted data (in-memory CGI). 
 */
void CXbmcHttp::xbmcForm(webs_t wp, char_t *path, char_t *query)
{
	char_t	*command, *parameter;

	command = websGetVar(wp, WEB_COMMAND, XBMC_NONE); 
	parameter = websGetVar(wp, WEB_PARAMETER, XBMC_NONE);

	// do the command

	xbmcProcessCommand( NO_EID, wp, command, parameter);

  if (wp->timeout!=-1)
	  websDone(wp, 200);
}
