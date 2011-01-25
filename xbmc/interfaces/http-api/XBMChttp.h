#pragma once

#include "network/UdpClient.h"
#include "guilib/Key.h"
#include "boost/shared_ptr.hpp"

/******************************** Description *********************************/

/* 
 *  Header file that provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.h
 */

/********************************* Includes ***********************************/

typedef char char_t;

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;



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
  CStdString userHeader, userFooter;
  bool incWebFooter, incWebHeader, shuttingDown, tempSkipWebFooterHeader;

  CXbmcHttp();
  ~CXbmcHttp();

  int xbmcCommand(const CStdString &parameter);
  int xbmcAddToPlayList(int numParas, CStdString paras[]);
  int xbmcAddToPlayListFromDB(int numParas, CStdString paras[]);
  int xbmcPlayerPlayFile(int numParas, CStdString paras[]); 
  int xbmcClearPlayList(int numParas, CStdString paras[]); 
  int xbmcGetCurrentlyPlaying(int numParas, CStdString paras[]); 
  int xbmcGetXBEID(int numParas, CStdString paras[]); 
  int xbmcGetXBETitle(int numParas, CStdString paras[]); 
  int xbmcGetSources(int numParas, CStdString paras[]);
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
  int xbmcExecMusicDataBase(int numParas, CStdString paras[]);
  int xbmcExecVideoDataBase(int numParas, CStdString paras[]);
  int xbmcDownloadInternetFile(int numParas, CStdString paras[]);
  int xbmcSetKey(int numParas, CStdString paras[]);
  int xbmcSetKeyRepeat(int numParas, CStdString paras[]);
  int xbmcGetMovieDetails(int numParas, CStdString paras[]);
  int xbmcDeleteFile(int numParas, CStdString paras[]);
  int xbmcCopyFile(int numParas, CStdString paras[]);
  int xbmcSetFile(int numParas, CStdString paras[]);
  int xbmcFileExists(int numParas, CStdString paras[]);
  int xbmcFileSize(int numParas, CStdString paras[]);
  int xbmcShowPicture(int numParas, CStdString paras[]);
  int xbmcGetGUIStatus();
  int xbmcExecBuiltIn(int numParas, CStdString paras[]);
  int xbmcSTSetting(int numParas, CStdString paras[]);
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
  int xbmcBroadcast(int numParas, CStdString paras[]);
  bool xbmcBroadcast(CStdString message, int level=0);
  int xbmcSetBroadcast(int numParas, CStdString paras[]);
  int xbmcGetBroadcast();
  int xbmcOnAction(int numParas, CStdString paras[]);
  int xbmcRecordStatus(int numParas, CStdString paras[]);
  int xbmcGetMusicLabel(int numParas, CStdString paras[]);
  int xbmcGetVideoLabel(int numParas, CStdString paras[]);
  int xbmcGetSkinSetting(int numParas, CStdString paras[]);
  int xbmcWebServerStatus(int numParas, CStdString paras[]);
  int xbmcGetLogLevel();
  int xbmcSetLogLevel(int numParas, CStdString paras[]);
  CKey GetKey();
  void ResetKey();
  CStdString GetOpenTag();
  CStdString GetCloseTag();

private:
  CKey key;
  CUdpBroadcast* pUdpBroadcast;
  CUdpClient UdpClient;
  CKey lastKey;
  int repeatKeyRate; //ms
  unsigned int MarkTime;
  bool autoGetPictureThumbs;
  CStdString lastThumbFn, lastPlayingInfo, lastSlideInfo;
  CStdString openTag, closeTag,  openRecordSet, closeRecordSet, openRecord, closeRecord, openField, closeField, openBroadcast, closeBroadcast;
  bool  closeFinalTag;

  void encodeblock( unsigned char in[3], unsigned char out[4], int len );
  CStdString encodeFileToBase64(const CStdString &inFilename, int linesize );
  void decodeblock( unsigned char in[4], unsigned char out[3] );
  bool decodeBase64ToFile( const CStdString &inString, const CStdString &outfilename, bool append = false );
  int64_t fileSize(const CStdString &filename);
  void resetTags();
  CStdString procMask(CStdString mask);
  int splitParameter(const CStdString &parameter, CStdString& command, CStdString paras[], const CStdString &sep);
  bool playableFile(const CStdString &filename);
  int SetResponse(const CStdString &response);
  int displayDir(int numParas, CStdString paras[]);
  void SetCurrentMediaItem(CFileItem& newItem);
  void AddItemToPlayList(const CFileItemPtr &pItem, int playList, int sortMethod, CStdString mask, bool recursive);
  void LoadPlayListOld(const CStdString& strPlayList, int playList);
  bool LoadPlayList(CStdString strPath, int iPlaylist, bool clearList, bool autoStart);
  void copyThumb(CStdString srcFn, CStdString destFn);
  int FindPathInPlayList(int playList, CStdString path);
};

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")

extern CXbmcHttp* m_pXbmcHttp; //make it global so Application.cpp can access it for key/button messages
