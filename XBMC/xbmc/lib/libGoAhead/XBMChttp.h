#pragma once

/******************************** Description *********************************/

/* 
 *  Header file that provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.h
 */

/********************************* Includes ***********************************/

#include "..\..\FileSystem\VirtualDirectory.h"
#include "includes.h"

class CXbmcHttp
{
public:
  CXbmcHttp();
  ~CXbmcHttp();

  void xbmcForm(webs_t wp, char_t *path, char_t *query);
  int	xbmcCommand( int eid, webs_t wp, int argc, char_t **argv);
  CStdString xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);
  CStdString  xbmcExternalCall(char *command);

  CStdString xbmcAddToPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcPlayerPlayFile(int eid, webs_t wp, int numParas, CStdString paras[]); 
  CStdString xbmcClearPlayList(int eid, webs_t wp, int numParas, CStdString paras[]); 
  CStdString xbmcGetCurrentlyPlaying(int eid, webs_t wp); 
  CStdString xbmcGetDirectory(int eid, webs_t wp, int numParas, CStdString paras[]); 
  CStdString xbmcGetTagFromFilename(int eid, webs_t wp, int numParas, CStdString paras[]); 
  CStdString xbmcGetCurrentPlayList(int eid, webs_t wp);
  CStdString xbmcSetCurrentPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetPlayListContents(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcRemoveFromPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcSetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcSetPlaySpeed(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetPlaySpeed(int eid, webs_t wp);
  CStdString xbmcPlayListNext(int eid, webs_t wp);
  CStdString xbmcPlayListPrev(int eid, webs_t wp);
  CStdString xbmcSetVolume(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetVolume(int eid, webs_t wp);
  CStdString xbmcGetPercentage(int eid, webs_t wp);
  CStdString xbmcSeekPercentage(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcAction(int eid, webs_t wp, int numParas, CStdString paras[], int theAction);
  CStdString xbmcExit(int eid, webs_t wp, int theAction);
  CStdString xbmcGetThumb(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetThumbFilename(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcLookupAlbum(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcChooseAlbum(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcDownloadInternetFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcSetKey(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetMovieDetails(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcDeleteFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcCopyFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcSetFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcFileExists(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcShowPicture(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetGUIStatus(int eid, webs_t wp);
  CStdString xbmcExecBuiltIn(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcConfig(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcHelp(int eid, webs_t wp);
  CStdString xbmcGetSystemInfo(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetSystemInfoByName(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcAddToSlideshow(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcClearSlideshow(int eid, webs_t wp);
  CStdString xbmcPlaySlideshow(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcSlideshowSelect(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetSlideshowContents(int eid, webs_t wp);
  CStdString xbmcGetCurrentSlide(int eid, webs_t wp);
  CStdString xbmcGUISetting(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcTakeScreenshot(int eid, webs_t wp, int numParas, CStdString paras[]);
  CStdString xbmcGetGUIDescription(int eid, webs_t wp);
  CStdString xbmcAutoGetPictureThumbs(int eid, webs_t wp, int numParas, CStdString paras[]);
  CKey GetKey();
  void ResetKey();

private:
  CKey key;

};

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")

extern CXbmcHttp* pXbmcHttp; //make it global so Application.cpp can access it for key/button messages
