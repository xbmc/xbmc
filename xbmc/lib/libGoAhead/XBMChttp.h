#include "../../utils/UdpClient.h"

#pragma once

/******************************** Description *********************************/

/* 
 *  Header file that provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.h
 */

/********************************* Includes ***********************************/

typedef char char_t;
typedef struct websRec *webs_t;

class CXbmcHttpShim
{
public:
  CXbmcHttpShim();
  ~CXbmcHttpShim();

  void xbmcForm(webs_t wp, char_t *path, char_t *query);
  int	xbmcCommand( int eid, webs_t wp, int argc, char_t **argv);
  CStdString xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);
  CStdString xbmcExternalCall(char *command);

};

class CUdpBroadcast : public CUdpClient
{
public:
  CUdpBroadcast();
  ~CUdpBroadcast();
  bool broadcast(CStdString message, int port);
};

class CXbmcHttp
{
public:
  CXbmcHttp();
  ~CXbmcHttp();

  int xbmcCommand(const CStdString &parameter);
  int xbmcAddToPlayList(int numParas, CStdString paras[]);
  int xbmcPlayerPlayFile(int numParas, CStdString paras[]); 
  int xbmcClearPlayList(int numParas, CStdString paras[]); 
  int xbmcGetCurrentlyPlaying(); 
  int xbmcGetXBEID(int numParas, CStdString paras[]); 
  int xbmcGetXBETitle(int numParas, CStdString paras[]); 
  int xbmcGetShares(int numParas, CStdString paras[]);
  int xbmcGetMediaLocation(int numParas, CStdString paras[]);
  int xbmcGetDirectory(int numParas, CStdString paras[]);
  int xbmcGetTagFromFilename(int numParas, CStdString paras[]); 
  int xbmcGetCurrentPlayList();
  int xbmcSetCurrentPlayList(int numParas, CStdString paras[]);
  int xbmcGetPlayListContents(int numParas, CStdString paras[]);
  int xbmcGetPlayListLength(int numParas, CStdString paras[]);
  int xbmcRemoveFromPlayList(int numParas, CStdString paras[]);
  int xbmcSetPlayListSong(int numParas, CStdString paras[]);
  int xbmcGetPlayListSong(int numParas, CStdString paras[]);
  int xbmcSetPlaySpeed(int numParas, CStdString paras[]);
  int xbmcGetPlaySpeed();
  int xbmcPlayListNext();
  int xbmcPlayListPrev();
  int xbmcSetVolume(int numParas, CStdString paras[]);
  int xbmcGetVolume();
  int xbmcMute();
  int xbmcGetPercentage();
  int xbmcSeekPercentage(int numParas, CStdString paras[], bool relative);
  int xbmcAction(int numParas, CStdString paras[], int theAction);
  int xbmcExit(int theAction);
  int xbmcGetThumb(int numParas, CStdString paras[], bool bGetThumb);
  int xbmcGetThumbFilename(int numParas, CStdString paras[]);
  int xbmcLookupAlbum(int numParas, CStdString paras[]);
  int xbmcChooseAlbum(int numParas, CStdString paras[]);
  int xbmcQueryMusicDataBase(int numParas, CStdString paras[]);
  int xbmcQueryVideoDataBase(int numParas, CStdString paras[]);
  int xbmcDownloadInternetFile(int numParas, CStdString paras[]);
  int xbmcSetKey(int numParas, CStdString paras[]);
  int xbmcGetMovieDetails(int numParas, CStdString paras[]);
  int xbmcDeleteFile(int numParas, CStdString paras[]);
  int xbmcCopyFile(int numParas, CStdString paras[]);
  int xbmcSetFile(int numParas, CStdString paras[]);
  int xbmcFileExists(int numParas, CStdString paras[]);
  int xbmcFileSize(int numParas, CStdString paras[]);
  int xbmcShowPicture(int numParas, CStdString paras[]);
  int xbmcGetGUIStatus();
  int xbmcExecBuiltIn(int numParas, CStdString paras[]);
  int xbmcConfig(int numParas, CStdString paras[]);
  int xbmcHelp();
  int xbmcGetSystemInfo(int numParas, CStdString paras[]);
  int xbmcGetSystemInfoByName(int numParas, CStdString paras[]);
  int xbmcAddToSlideshow(int numParas, CStdString paras[]);
  int xbmcClearSlideshow();
  int xbmcPlaySlideshow(int numParas, CStdString paras[]);
  int xbmcSlideshowSelect(int numParas, CStdString paras[]);
  int xbmcGetSlideshowContents();
  int xbmcGetCurrentSlide();
  int xbmcGUISetting(int numParas, CStdString paras[]);
  int xbmcTakeScreenshot(int numParas, CStdString paras[]);
  int xbmcGetGUIDescription();
  int xbmcAutoGetPictureThumbs(int numParas, CStdString paras[]);
  int xbmcSetResponseFormat(int numParas, CStdString paras[]);
  int xbmcSpinDownHardDisk();
  int xbmcBroadcast(int numParas, CStdString paras[]);
  bool xbmcBroadcast(CStdString message, int level=0);
  int xbmcSetBroadcast(int numParas, CStdString paras[]);
  int xbmcGetBroadcast();
  int xbmcOnAction(int numParas, CStdString paras[]);
  int xbmcRecordStatus(int numParas, CStdString paras[]);

  int xbmcGetSkinSetting(int numParas, CStdString paras[]);
  CKey GetKey();
  void ResetKey();

private:
  CKey key;
  CUdpClient UdpClient;
  bool UdpClientInit;

};

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")

extern CXbmcHttp* pXbmcHttp; //make it global so Application.cpp can access it for key/button messages
extern CXbmcHttpShim* pXbmcHttpShim;
