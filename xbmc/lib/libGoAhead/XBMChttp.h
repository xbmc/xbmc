#pragma once

/******************************** Description *********************************/

/* 
 *	Header file that provides an API over HTTP between the web server and XBMC
 *
 *						heavily based on XBMCweb.h
 */

/********************************* Includes ***********************************/

#include "..\..\FileSystem\VirtualDirectory.h"
#include "includes.h"

class CXbmcHttp
{
public:
	CXbmcHttp();
	~CXbmcHttp();

	void		xbmcForm(webs_t wp, char_t *path, char_t *query);
	int			xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);


	int			xbmcAddToPlayList(webs_t wp, char_t *parameter);
	int			xbmcPlayerPlayFile(webs_t wp, char_t *parameter); 
	int			xbmcClearPlayList(webs_t wp, char_t *parameter); 
	int			xbmcGetCurrentlyPlaying(  webs_t wp); 
	int			xbmcGetDirectory(webs_t wp, char_t *parameter); 
	int			xbmcGetTagFromFilename(webs_t wp, char_t *parameter); 
	int			xbmcGetCurrentPlayList(webs_t wp);
	int			xbmcSetCurrentPlayList(webs_t wp, char_t *parameter);
	int			xbmcGetPlayListContents(webs_t wp, char_t *parameter);
	int			xbmcRemoveFromPlayList(webs_t wp, char_t *parameter);
	int			xbmcSetPlayListSong(webs_t wp, char_t *parameter);
	int			xbmcGetPlayListSong(webs_t wp, char_t *parameter);
	int			xbmcPlayListNext(webs_t wp);
	int			xbmcPlayListPrev(webs_t wp);
	int			xbmcSetVolume(webs_t wp, char_t *parameter);
	int			xbmcGetVolume(webs_t wp);
	int			xbmcGetPercentage(webs_t wp);
	int			xbmcSeekPercentage(webs_t wp, char_t *parameter);
	int			xbmcAction(webs_t wp, char_t *parameter, int theAction);
	int			xbmcExit(webs_t wp, int theAction);
	int			xbmcGetThumb(webs_t wp, char_t *parameter);
	int			xbmcGetThumbFilename(webs_t wp, char_t *parameter);
	int			xbmcLookupAlbum(webs_t wp, char_t *parameter);
	int			xbmcChooseAlbum(webs_t wp, char_t *parameter);
	int			xbmcDownloadInternetFile(webs_t wp, char_t *parameter);
	int			xbmcSetKey(webs_t wp, char_t *parameter);
	int			xbmcGetMovieDetails(webs_t wp, char_t *parameter);
	int			xbmcDeleteFile(webs_t wp, char_t *parameter);
	int			xbmcCopyFile(webs_t wp, char_t *parameter);
	int			xbmcSetFile(webs_t wp, char_t *parameter);
	int			xbmcFileExists(webs_t wp, char_t *parameter);
	int			xbmcShowPicture(webs_t wp, char_t *parameter);
	int			xbmcGetGUIStatus(webs_t wp);
  int     xbmcExecBuiltIn(webs_t wp, char_t *parameter);
  int     xbmcConfig(webs_t wp, char_t *parameter);
  int			xbmcHelp(webs_t wp);
  int     xbmcGetSystemInfo(webs_t wp, char_t *parameter);
  int     xbmcGetSystemInfoByName(webs_t wp, char_t *parameter);
  int     xbmcAddToSlideshow(webs_t wp, char_t *parameter);
  int			xbmcClearSlideshow(webs_t wp);
  int			xbmcPlaySlideshow(webs_t wp, char_t *parameter);
  int     xbmcSlideshowSelect(webs_t wp, char_t *parameter);
  int     xbmcGetSlideshowContents( webs_t wp);
  int     xbmcGetCurrentSlide( webs_t wp);
  int			xbmcGUISetting(webs_t wp, char_t *parameter);
	CKey		GetKey();
	void		ResetKey();

private:
	CKey key;

};

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")

extern CXbmcHttp* pXbmcHttp; //make it global so Application.cpp can access it for key/button messages


