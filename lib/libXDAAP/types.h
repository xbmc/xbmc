#ifndef __SRIPPER_H__
#define __SRIPPER_H__

////////////////////////////////////////////////
// Types
////////////////////////////////////////////////

//#if WIN32
#if WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

typedef int error_code;
#define BOOL	int
#define TRUE	1
#define FALSE	0

#define NO_META_INTERVAL	-1

//#ifndef WIN32
//#define MAX_PATH 256
//#endif

#define MAX_HOST_LEN		512
#define MAX_IP_LEN			3+1+3+1+3+1+3+1
#define MAX_PATH_LEN		255
#define MAX_HEADER_LEN		8192
#define MAX_URL_LEN			8192
#define MAX_ICY_STRING		4024
#define MAX_SERVER_LEN		1024
#define MAX_TRACK_LEN		MAX_PATH
#define MAX_URI_STRING		1024
#define MAX_ERROR_STR       (4096)
#define MAX_USERAGENT_STR	1024

#if WIN32
	#ifndef _WINSOCKAPI_
		#define __DEFINE_TYPES__
	#endif
#endif

#ifdef __DEFINE_TYPES__
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
#endif

#define SR_SUCCESS					0x00
#define SR_ERROR_INVALID_PARAM				-	0x0d
 

#endif //__SRIPPER_H__
