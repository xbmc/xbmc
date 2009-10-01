#ifndef __SRTYPES_H__
#define __SRTYPES_H__

////////////////////////////////////////////////
// Types
////////////////////////////////////////////////
#include "srconfig.h"
#if WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

/* Note: uint32_t is standardized in ISO C99, so let's use that one */
#if !HAVE_UINT32_T
# if HAVE_U_INT32_T
typedef u_int32_t uint32_t;
# else
typedef unsigned int uint32_t;
# endif
#endif

#if HAVE_WCHAR_SUPPORT
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if HAVE_WCTYPE_H
#include <wctype.h>
#endif
#endif
#if STDC_HEADERS
#include <stddef.h>
#endif

typedef int error_code;
#define BOOL	int
#define TRUE	1
#define FALSE	0

#define NO_META_INTERVAL	-1

/* GCS - Grr. I don't care.  Max path is 254 until I get around to
    fixing this for other platforms. */
#define SR_MAX_PATH		254
#define MAX_HOST_LEN		512
#define MAX_IP_LEN			3+1+3+1+3+1+3+1
#define MAX_HEADER_LEN		8192
#define MAX_URL_LEN		8192
#define MAX_ICY_STRING		4024
#define MAX_SERVER_LEN		1024
//#define MAX_TRACK_LEN		MAX_PATH
#define MAX_TRACK_LEN		SR_MAX_PATH /* GCS - be careful here... */
#define MAX_URI_STRING		1024
#define MAX_ERROR_STR           (4096)
#define MAX_USERAGENT_STR	1024
#define MAX_AUTH_LEN            255
//#define MAX_DROPSTRING_LEN      255

#define MAX_METADATA_LEN (127*16)


#ifdef WIN32
  #ifndef _WINSOCKAPI_
    #define __DEFINE_TYPES__
  #endif
#endif

#ifdef __DEFINE_TYPES__
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
#endif

/* Different types of streams */
#define CONTENT_TYPE_MP3		1
#define CONTENT_TYPE_NSV		2
#define CONTENT_TYPE_OGG    		3
#define CONTENT_TYPE_ULTRAVOX		4
#define CONTENT_TYPE_AAC		5
#define CONTENT_TYPE_PLS		6
#define CONTENT_TYPE_M3U		7
#define CONTENT_TYPE_UNKNOWN		99

/* 
 * IO_DATA_INPUT is a interface for socket input data, it has one 
 * method 'get_data' and is called by a "ripper" which is effectivly 
 * only ripshout.c (and the R.I.P. riplive365.c)
 */
typedef struct IO_DATA_INPUTst{
	int (*get_input_data)(char* buffer, int size);
} IO_DATA_INPUT;

#define NO_TRACK_STR	"No track info..."

/* 
 * IO_GET_STREAM is an interface for getting data and track info from
 * a better splite on the track seperation. it keeps a back buffer and 
 * does the "find silent point" shit.
 */
#if defined (commentout)
typedef struct IO_GET_STREAMst{
	int (*get_stream_data)(char* data_buf, char *track_buf);
	u_long getsize;
} IO_GET_STREAM;
#endif

/* 
 * SPLITPOINT_OPTIONS are the options used to tweek how the silence 
 * separation is done.
 */
typedef struct SPLITPOINT_OPTIONSst
{
    int	xs;
    int xs_min_volume;
    int xs_silence_length;
    int xs_search_window_1;
    int xs_search_window_2;
    int xs_offset;
    //int xd_offset;
    //int xpadding_1;
    //int xpadding_2;
    int xs_padding_1;
    int xs_padding_2;
} SPLITPOINT_OPTIONS;

/* 
 * CODESET_OPTIONS are the options used to decide how to parse
 * and convert the metadata
 */
#define MAX_CODESET_STRING 128
typedef struct CODESET_OPTIONSst
{
    char codeset_locale[MAX_CODESET_STRING];
    char codeset_filesys[MAX_CODESET_STRING];
    char codeset_id3[MAX_CODESET_STRING];
    char codeset_metadata[MAX_CODESET_STRING];
    char codeset_relay[MAX_CODESET_STRING];
} CODESET_OPTIONS;

/* 
 * Various CODESET types
 */
#define CODESET_UTF8          1
#define CODESET_LOCALE        2
#define CODESET_FILESYS       3
#define CODESET_ID3           4
#define CODESET_METADATA      5
#define CODESET_RELAY         6

/* 
 * Wide character support
 */
#if HAVE_WCHAR_SUPPORT
typedef wchar_t mchar;
#else
typedef char mchar;
#endif

/* 
 * TRACK_INFO is the parsed metadata
 */
typedef struct TRACK_INFOst
{
    int have_track_info;
    char raw_metadata[MAX_TRACK_LEN];
    mchar w_raw_metadata[MAX_TRACK_LEN];
    mchar artist[MAX_TRACK_LEN];
    mchar title[MAX_TRACK_LEN];
    mchar album[MAX_TRACK_LEN];
    mchar track[MAX_TRACK_LEN];
    char composed_metadata[MAX_METADATA_LEN+1];      /* For relay stream */
    BOOL save_track;
} TRACK_INFO;


#ifndef WIN32
typedef int SOCKET;
#endif

typedef struct HSOCKETst
{
	SOCKET	s;
	BOOL	closed;
} HSOCKET;

/* 
 * OverwriteOpt controls how files in complete directory are overwritten
 */
enum OverwriteOpt {
    OVERWRITE_UNKNOWN,	// Error case
    OVERWRITE_ALWAYS,	// Always replace file in complete with newer
    OVERWRITE_NEVER,	// Never replace file in complete with newer
    OVERWRITE_LARGER	// Replace file in complete if newer is larger
};


////////////////////////////////////////////////
// StreamRipper Codes
////////////////////////////////////////////////
// JCBUG -- no way to make custom error strings for http errors, also errors
// are not organized at all, should have space to insert in places.
//
/* ************** IMPORTANT IF YOU ADD ERROR CODES!!!! ***********************/
#define NUM_ERROR_CODES					((0x41)+1)
/* ************** IMPORTANT IF YOU ADD ERROR CODES!!!! ***********************/
#define SR_SUCCESS				  0x00
#define SR_SUCCESS_BUFFERING			  0x01
#define SR_ERROR_CANT_FIND_TRACK_SEPERATION	- 0x01
#define SR_ERROR_DECODE_FAILURE			- 0x02
#define SR_ERROR_INVALID_URL			- 0x03
#define	SR_ERROR_WIN32_INIT_FAILURE		- 0x04
#define SR_ERROR_CONNECT_FAILED			- 0x05
#define SR_ERROR_CANT_RESOLVE_HOSTNAME		- 0x06
#define SR_ERROR_RECV_FAILED			- 0x07
#define SR_ERROR_SEND_FAILED			- 0x08
#define SR_ERROR_PARSE_FAILURE			- 0x09
#define SR_ERROR_NO_RESPOSE_HEADER		- 0x0a
#define SR_ERROR_NO_ICY_CODE			- 0x0b
#define SR_ERROR_NO_META_INTERVAL		- 0x0c
#define SR_ERROR_INVALID_PARAM			- 0x0d
#define SR_ERROR_NO_HTTP_HEADER			- 0x0e
#define SR_ERROR_CANT_GET_LIVE365_ID		- 0x0f
#define SR_ERROR_CANT_ALLOC_MEMORY		- 0x10
#define SR_ERROR_CANT_FIND_IP_PORT		- 0x11
#define SR_ERROR_CANT_FIND_MEMBERNAME		- 0x12
#define SR_ERROR_CANT_FIND_TRACK_NAME		- 0x13
#define SR_ERROR_NULL_MEMBER_NAME		- 0x14
#define SR_ERROR_CANT_FIND_TIME_TAG		- 0x15
#define SR_ERROR_BUFFER_EMPTY			- 0x16
#define	SR_ERROR_BUFFER_FULL			- 0x17
#define	SR_ERROR_CANT_INIT_XAUDIO		- 0x18
#define SR_ERROR_BUFFER_TOO_SMALL		- 0x19
#define SR_ERROR_CANT_CREATE_THREAD		- 0x1A
#define SR_ERROR_CANT_FIND_MPEG_HEADER		- 0x1B
#define SR_ERROR_INVALID_METADATA		- 0x1C
#define SR_ERROR_NO_TRACK_INFO			- 0x1D
#define SR_EEROR_CANT_FIND_SUBSTR		- 0x1E
#define SR_ERROR_CANT_BIND_ON_PORT		- 0x1F
#define SR_ERROR_HOST_NOT_CONNECTED		- 0x20
#define SR_ERROR_HTTP_404_ERROR			- 0x21
#define SR_ERROR_HTTP_401_ERROR			- 0x22	
#define SR_ERROR_HTTP_502_ERROR			- 0x23  // Connection Refused
#define SR_ERROR_CANT_CREATE_FILE		- 0x24
#define SR_ERROR_CANT_WRITE_TO_FILE		- 0x25
#define SR_ERROR_CANT_CREATE_DIR		- 0x26
#define SR_ERROR_HTTP_400_ERROR			- 0x27	// Server Full
#define SR_ERROR_CANT_SET_SOCKET_OPTIONS	- 0x28
#define SR_ERROR_SOCK_BASE			- 0x29
#define SR_ERROR_INVALID_DIRECTORY		- 0x2a
#define SR_ERROR_FAILED_TO_MOVE_FILE		- 0x2b
#define SR_ERROR_CANT_LOAD_MPGLIB		- 0x2c
#define SR_ERROR_CANT_INIT_MPGLIB		- 0x2d
#define SR_ERROR_CANT_UNLOAD_MPGLIB		- 0x2e
#define SR_ERROR_PCM_BUFFER_TO_SMALL		- 0x2f
#define SR_ERROR_CANT_DECODE_MP3		- 0x30
#define SR_ERROR_SOCKET_CLOSED			- 0x31
#define SR_ERROR_LIVE365			- 0x32
#define SR_ERROR_MAX_BYTES_RIPPED		- 0x33
#define SR_ERROR_CANT_WAIT_ON_THREAD		- 0x34
#define SR_ERROR_CANT_CREATE_EVENT		- 0x35
#define SR_ERROR_NOT_SHOUTCAST_STREAM		- 0x36
#define SR_ERROR_HTTP_407_ERROR			- 0x37
#define	SR_ERROR_HTTP_403_ERROR			- 0x38
#define SR_ERROR_DIR_PATH_TOO_LONG		- 0x39
#define SR_ERROR_PROGRAM_ERROR			- 0x3a
#define SR_ERROR_TIMEOUT                        - 0x3b
#define SR_ERROR_SELECT_FAILED                  - 0x3c
#define SR_ERROR_REQUIRED_WINDOW_EMPTY          - 0x3d  // Not an error
#define SR_ERROR_CANT_BIND_ON_INTERFACE		- 0x3e
#define SR_ERROR_NO_OGG_PAGES_FOR_RELAY		- 0x3f
#define SR_ERROR_CANT_PARSE_PLS	                - 0x40
#define SR_ERROR_CANT_PARSE_M3U	                - 0x41

#endif //__SRIPPER_H__
