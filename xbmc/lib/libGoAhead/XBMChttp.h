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
  int  xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);

  int  xbmcAddToPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcPlayerPlayFile(int eid, webs_t wp, int numParas, CStdString paras[]); 
  int  xbmcClearPlayList(int eid, webs_t wp, int numParas, CStdString paras[]); 
  int  xbmcGetCurrentlyPlaying(int eid, webs_t wp); 
  int  xbmcGetDirectory(int eid, webs_t wp, int numParas, CStdString paras[]); 
  int  xbmcGetTagFromFilename(int eid, webs_t wp, int numParas, CStdString paras[]); 
  int  xbmcGetCurrentPlayList(int eid, webs_t wp);
  int  xbmcSetCurrentPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetPlayListContents(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcRemoveFromPlayList(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcSetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetPlayListSong(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcPlayListNext(int eid, webs_t wp);
  int  xbmcPlayListPrev(int eid, webs_t wp);
  int  xbmcSetVolume(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetVolume(int eid, webs_t wp);
  int  xbmcGetPercentage(int eid, webs_t wp);
  int  xbmcSeekPercentage(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcAction(int eid, webs_t wp, int numParas, CStdString paras[], int theAction);
  int  xbmcExit(int eid, webs_t wp, int theAction);
  int  xbmcGetThumb(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetThumbFilename(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcLookupAlbum(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcChooseAlbum(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcDownloadInternetFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcSetKey(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetMovieDetails(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcDeleteFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcCopyFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcSetFile(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcFileExists(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcShowPicture(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetGUIStatus(int eid, webs_t wp);
  int  xbmcExecBuiltIn(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcConfig(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcHelp(int eid, webs_t wp);
  int  xbmcGetSystemInfo(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetSystemInfoByName(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcAddToSlideshow(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcClearSlideshow(int eid, webs_t wp);
  int  xbmcPlaySlideshow(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcSlideshowSelect(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetSlideshowContents(int eid, webs_t wp);
  int  xbmcGetCurrentSlide(int eid, webs_t wp);
  int  xbmcGUISetting(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcTakeScreenshot(int eid, webs_t wp, int numParas, CStdString paras[]);
  int  xbmcGetGUIDescription(int eid, webs_t wp);
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
