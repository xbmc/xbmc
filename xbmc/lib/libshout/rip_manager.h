#ifndef __RIP_MANANGER_H__
#define __RIP_MANANGER_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define MAX_STATUS_LEN		256
#define MAX_FILENAME_LEN	255
#define MAX_STREAMNAME_LEN	1024
#define MAX_SERVER_LEN		1024
#define MAX_CONTENTTYPE_STR 1024

//
// Messages for status_callback hook in rip_manager_init()
// used for notifing to client whats going on *DO NOT* call rip_mananger_start or rip_mananger_stop from
// these functions!!! it will cause a deadlock
//
#define RM_UPDATE		0x01		// returns a pointer RIP_MANAGER_INFO struct
#define RM_ERROR		0x02		// returns the error code
#define RM_DONE			0x03		// NULL
#define RM_STARTED		0x04		// NULL
#define RM_NEW_TRACK	0x05		// Name of the new track
#define RM_TRACK_DONE	0x06		// pull path of the track completed
#define RM_OUTPUT_DIR	0x07		// Full path of the output directory

//
// The following are the possible status values for RIP_MANAGER_INFO
//
#define RM_STATUS_BUFFERING		0x01
#define RM_STATUS_RIPPING		0x02
#define RM_STATUS_RECONNECTING	0x03

typedef struct RIP_MANAGER_INFOst
{
	char	streamname[MAX_STREAMNAME_LEN];
	char	server_name[MAX_SERVER_LEN];
	int		bitrate;
	int		meta_interval;
	char	filename[MAX_FILENAME_LEN];		// JCBUG -- it's not the filename, it's the trackname
  char  contenttype[MAX_CONTENTTYPE_STR];
	u_long	filesize;
	int	status;
} RIP_MANAGER_INFO;

// Use to set and get the flags

#define OPT_AUTO_RECONNECT		0x00000001		// reconnect automatticly if dropped
#define OPT_SEPERATE_DIRS		0x00000002		// create a directory named after the server
#define OPT_OVER_WRITE_TRACKS	0x00000004		// should files in the complete directory be overwritten
#define OPT_SEARCH_PORTS		0x00000008		// relay server should search for a open port
#define OPT_MAKE_RELAY			0x00000010		// don't make a relay server
#define OPT_COUNT_FILES			0x00000020		// add a index counter to the filenames
#define OPT_ADD_ID3				0x00000040		// add ID3 info to the mp3s (thanks oddsock)
#define OPT_DATE_STAMP			0x00000100		// add a date stamp to the output directory
#define OPT_CHECK_MAX_BYTES		0x00000200		// use the maxMB_rip_size value to know how much to rip
#define OPT_KEEP_INCOMPLETE		0x00000400		// overwrite files in the incomplete directory, add counter instead

//
// Helper macros for checking options
//

#define OPT_FLAG_ISSET(flags, opt)	((flags & opt) > 0)
#define OPT_FLAG_SET(flags, opt)	(flags =| opt)

#define GET_AUTO_RECONNECT(flags)			(OPT_FLAG_ISSET(flags, OPT_AUTO_RECONNECT))
#define GET_SEPERATE_DIRS(flags)			(OPT_FLAG_ISSET(flags, OPT_SEPERATE_DIRS))
#define GET_OVER_WRITE_TRACKS(flags)		(OPT_FLAG_ISSET(flags, OPT_OVER_WRITE_TRACKS))
#define GET_SEARCH_PORTS(flags)				(OPT_FLAG_ISSET(flags, OPT_SEARCH_PORTS))
#define GET_MAKE_RELAY(flags)				(OPT_FLAG_ISSET(flags, OPT_MAKE_RELAY))
#define GET_COUNT_FILES(flags)				(OPT_FLAG_ISSET(flags, OPT_COUNT_FILES))
#define GET_ADD_ID3(flags)					(OPT_FLAG_ISSET(flags, OPT_ADD_ID3))
#define GET_DATE_STAMP(flags)				(OPT_FLAG_ISSET(flags, OPT_DATE_STAMP))
#define GET_CHECK_MAX_BYTES(flags)			(OPT_FLAG_ISSET(flags, OPT_CHECK_MAX_BYTES))
#define GET_KEEP_INCOMPLETE(flags)			(OPT_FLAG_ISSET(flags, OPT_KEEP_INCOMPLETE))

#define SET_AUTO_RECONNECT(flags)			(OPT_FLAG_SET(flags, OPT_AUTO_RECONNECT))
#define SET_SEPERATE_DIRS(flags)			(OPT_FLAG_SET(flags, OPT_SEPERATE_DIRS))
#define SET_OVER_WRITE_TRACKS(flags)		(OPT_FLAG_SET(flags, OPT_OVER_WRITE_TRACKS))
#define SET_SEARCH_PORTS(flags)				(OPT_FLAG_SET(flags, OPT_SEARCH_PORTS))
#define SET_MAKE_RELAY(flags)				(OPT_FLAG_SET(flags, OPT_MAKE_RELAY))
#define SET_COUNT_FILES(flags)				(OPT_FLAG_SET(flags, OPT_COUNT_FILES))
#define SET_ADD_ID3(flags)					(OPT_FLAG_SET(flags, OPT_ADD_ID3))
#define SET_DATE_STAMP(flags)				(OPT_FLAG_SET(flags, OPT_DATE_STAMP))
#define SET_CHECK_MAX_BYTES(flags)			(OPT_FLAG_SET(flags, OPT_CHECK_MAX_BYTES))
#define SET_KEEP_INCOMPLETE(flags)			(OPT_FLAG_SET(flags, OPT_KEEP_INCOMPLETE))


typedef struct RIP_MANAGER_OPTIONSst
{
	char	url[MAX_URL_LEN];					// url of the stream to connect to
	char	proxyurl[MAX_URL_LEN];				// url of a http proxy server, '\0' otherwise
	char	output_directory[MAX_PATH_LEN];		// base directory to output files too
	int	relay_port;								// port to use for the relay server
	u_short	max_port;							// highest port the relay server can look if it needs to search
	u_long	maxMB_rip_size;						// max number of megabytes that can by writen out before we stop
	u_short	flags;								// all booleans logically OR'd together (see above)
	char	useragent[MAX_USERAGENT_STR];		// optional, use a different useragent
} RIP_MANAGER_OPTIONS;

typedef struct ERROR_INFOst
{
	char		error_str[MAX_ERROR_STR];
	error_code	error_code;
} ERROR_INFO;


//
// Starts a 'rip', status_callback is the callback function for status messages
// RIP_MANAGER_OPTIONS is the above struct of rip options
//
extern error_code	rip_manager_start(void (*status_callback)(int message, void *data), RIP_MANAGER_OPTIONS *options);

//
// tell the rip_mananger to stop :)
//
extern void			rip_manager_stop();

//
// translats a error code into a string
//
extern char			*rip_manager_get_error_str(int code);

//
// used to find out what the relay port is, being that 
// the relay might search for one
//
extern u_short		rip_mananger_get_relay_port();	

#ifdef __cplusplus
}
#endif

#endif //__RIP_MANANGER_H__


 
