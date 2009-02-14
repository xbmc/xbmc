#pragma once

/******************************** Description *********************************/

/* 
 *	Header fil for the module that provides the links between the web server and 
 *  XBMC. This defines the Web private APIs
 *	Include this header when you want to create XBMC handlers.
 */

/********************************* Includes ***********************************/

#include "FileSystem/VirtualDirectory.h"
#include "includes.h"
#include "boost/shared_ptr.hpp"

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;

class CXbmcWeb
{
public:
	CXbmcWeb();
	~CXbmcWeb();
	char*		GetCurrentDir();
	void		SetCurrentDir(const char* newDir);
	DWORD		GetNavigatorState();
	void		SetNavigatorState(DWORD state);

	void		AddItemToPlayList(const CFileItemPtr &pItem);

	int			xbmcCommand( int eid, webs_t wp, int argc, char_t **argv);

	void		xbmcForm(webs_t wp, char_t *path, char_t *query);
	int			xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter);


	int 		xbmcPlayerPlay( int eid, webs_t wp, char_t *parameter);
	int			xbmcPlayerPrevious( int eid, webs_t wp, char_t *parameter);
	int			xbmcPlayerNext( int eid, webs_t wp, char_t *parameter);
	int 		xbmcNavigate( int eid, webs_t wp, char_t *parameter);
	int 		xbmcNavigatorState( int eid, webs_t wp, char_t *parameter);
	int 		xbmcCatalog( int eid, webs_t wp, char_t *parameter);
	int 		xbmcRemoteControl( int eid, webs_t wp, char_t *parameter);
	int 		xbmcSubtitles( int eid, webs_t wp, char_t *parameter);

	void		SetCurrentMediaItem(CFileItem& newItem);

private:
	DWORD		navigatorState;
	char		currentDir[1024];
	CFileItemList* webDirItems;
	int			catalogItemCounter;
	DIRECTORY::CVirtualDirectory *directory;
	CFileItem*	currentMediaItem;
};

/*
 *	Remote Control handler structure. Maps parameters from a remote command
 *	function to an XBMC ir remote command.
 */ 
typedef struct {
	char_t	*xbmcRemoteParameter;				
	int		xbmcIRCode;
} xbmcRemoteControlHandlerType;

/*
 *	Navagation handler structure. Maps parameters from a navagation request
 *	to an XBMC application state.
 */ 
typedef struct {
	char_t	*xbmcNavigateParameter;				
	int		xbmcAppStateCode;
} xbmcNavigationHandlerType;

/****************
 *  Command names
 */
#define WEB_COMMAND T("command")
#define WEB_PARAMETER T("parameter")
#define WEB_NEXT_PAGE T("redirect")

//actions for the use of messaging betweeen threads
#define XBMC_SHUTDOWN			10
#define XBMC_DASHBOARD		11
#define XBMC_RESTART			12
#define XBMC_MEDIA_PLAY		13
#define XBMC_MEDIA_STOP		14
#define XBMC_MEDIA_PAUSE	15
#define XBMC_PICTURE_SHOW	16

//navigator states
#define WEB_NAV_PROGRAMS 1
#define WEB_NAV_PICTURES 2
#define WEB_NAV_FILES 3
#define WEB_NAV_MUSIC 5
#define WEB_NAV_VIDEOS 6
#define WEB_NAV_MUSICPLAYLIST 10
#define WEB_NAV_VIDEOPLAYLIST 11

/* defines for use in xbmc */
#define WEBSERVER_UPDATE	1
#define WEBSERVER_CLICKED	2
