///
///	@file 	shared.h
/// @brief 	Shared definitions between http.h and client.h.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_SHARED
#define _h_SHARED 1

#include	"mpr.h"

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaHttpError ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaHttpError {
  public:
	int		code;							///< Http error code 
	char	*msg;							///< Error message 
};

///////////////////////////////////// Crypt ////////////////////////////////////

typedef struct {
	uint state[4];
	uint count[2];
	uchar buffer[64];
} MD5_CONTEXT;

//////////////////////////////////// Defines ///////////////////////////////////
//
//	These constants can be overridden in http.conf
//
#if BLD_FEATURE_SQUEEZE
#define MPR_HTTP_MAX_BODY		 (64 * 1024)	// Total request size 
#define MPR_HTTP_MAX_URL		 (256)			// Max URL size
#define MPR_HTTP_MAX_FIRST_LINE	 (256)			// Max first line of request
#define MPR_HTTP_MAX_HEADER		 (512)			// Max size of the header
#define MPR_HTTP_MAX_NUM_HEADER	 (16)			// Number of header lines
#define MPR_HTTP_MAX_ERR_BODY	 (512)			// Max body for error messages
#define MPR_HTTP_MAX_RESPONSE_BODY (8 * 1024)	// Max buffer for generated data
#define MPR_HTTP_MAX_SCRIPT_SIZE (8 * 1024)		// Max script size
#else
#define MPR_HTTP_MAX_BODY		 (1024 * 1024)	// Total request size 
#define MPR_HTTP_MAX_URL		 (4096)			// Max URL size
#define MPR_HTTP_MAX_FIRST_LINE	 (4096)			// Max first line of request
#define MPR_HTTP_MAX_HEADER		 (8 * 1024)		// Max size of the header
#define MPR_HTTP_MAX_NUM_HEADER	 (40)			// Number of header lines
#define MPR_HTTP_MAX_ERR_BODY	 (4096)			// Max body for error messages
#define MPR_HTTP_MAX_RESPONSE_BODY (128 * 1024)	// Max buffer for generated data
#define MPR_HTTP_MAX_SCRIPT_SIZE (128 * 1024)	// Max script size
#endif

#define MPR_HTTP_MAX_KEEP_ALIVE	 (100)			// Default requests per TCP conn
#define MPR_HTTP_KEEP_TIMEOUT	 (60000)		// Keep connection alive timeout

#define MPR_HTTP_DEFAULT_MAX_THREADS (10)		// Default number of threads

//
//	The constants can only be varied at compile-time
//
#if SQUEEZE
#define MPR_HTTP_MAX_PASS		 (64)			// Size of password 
#define MPR_HTTP_MAX_USER		 (64)			// Size of user name 
#define MPR_HTTP_MAX_MODULES	 (16)			// Limit # of static modules
#define MPR_HTTP_MAX_SECRET		 (32)			// Number of random bytes to use
#define MPR_HTTP_FILES_HASH_SIZE (29)			// Hash size for rom file system
#define MPR_HTTP_TIMER_PERIOD	 (5000)			// Timer checks ever 5 seconds
#else
#define MPR_HTTP_MAX_PASS		 (128)			// Size of password 
#define MPR_HTTP_MAX_USER		 (64)			// Size of user name 
#define MPR_HTTP_MAX_MODULES	 (32)			// Limit # of static modules
#define MPR_HTTP_MAX_SECRET		 (32)			// Number of random bytes to use
#define MPR_HTTP_FILES_HASH_SIZE (61)			// Hash size for rom file system
#define MPR_HTTP_TIMER_PERIOD	 (5000)			// Timer checks ever 5 seconds
#endif

//
//	These constants are just to sanity check user input in the http.conf
//
#define MPR_HTTP_TOP_THREADS	 (100)
#define MPR_HTTP_TOP_STACK		 (4 * 1024 * 1024)
#define MPR_HTTP_BOT_STACK		 (16 * 1024)
#define MPR_HTTP_TOP_SEND_BUFFER (1024 * 32)
#define MPR_HTTP_BOT_SEND_BUFFER (512)
#define MPR_HTTP_TOP_BODY		 (0x7fffffff)		// 2 GB
#define MPR_HTTP_BOT_BODY	 	 (0)
#define MPR_HTTP_TOP_NUM_HEADER	 (4 * 1024)
#define MPR_HTTP_BOT_NUM_HEADER	 (64)
#define MPR_HTTP_TOP_HEADER	 	 (1024 * 1024)
#define MPR_HTTP_BOT_HEADER	 	 (512)
#define MPR_HTTP_TOP_FIRST_LINE	 (256 * 1024)
#define MPR_HTTP_BOT_FIRST_LINE	 (160)
#define MPR_HTTP_TOP_URL		 (255 * 1024)
#define MPR_HTTP_BOT_URL		 (80)
#define MPR_HTTP_TOP_RESPONSE_BODY (0x7fffffff)		// 2 GB
#define MPR_HTTP_BOT_RESPONSE_BODY (512)
#define MPR_HTTP_TOP_SCRIPT_SIZE (0x7fffffff)		// 2 GB
#define MPR_HTTP_BOT_SCRIPT_SIZE (512)
#define MPR_HTTP_TOP_UPLOAD_SIZE (0x7fffffff)		// 2 GB
#define MPR_HTTP_BOT_UPLOAD_SIZE (1)
#define MPR_HTTP_MAX_UPLOAD_SIZE MPR_HTTP_TOP_UPLOAD_SIZE 

//
//	Character matching masks
//
#define MPR_HTTP_ESCAPE_HTML	0x01
#define MPR_HTTP_ESCAPE_SHELL	0x02
#define MPR_HTTP_ESCAPE_PATH	0x04

//
//	FUTURE -- should migrate all shared definitions here
//
#define MPR_HTTP_1_0			0			// HTTP/1.0
#define MPR_HTTP_1_1			1			// HTTP/1.1

//
//	Standard HTTP/1.1 response codes. See url.cpp for the actual strings 
//	used for each code.
//
//	FUTURE -- should prefix these MA_HTTP_CODE_
//
#define	MPR_HTTP_CONTINUE				100
#define MPR_HTTP_OK						200
#define MPR_HTTP_CREATED				201
#define MPR_HTTP_ACCEPTED				202
#define MPR_HTTP_NO_CONTENT				204
#define MPR_HTTP_RESET_CONTENT			205
#define MPR_HTTP_PARTIAL_CONTENT		206
#define	MPR_HTTP_MOVED_PERMANENTLY		301
#define	MPR_HTTP_MOVED_TEMPORARILY		302
#define	MPR_HTTP_NOT_MODIFIED			304
#define	MPR_HTTP_BAD_REQUEST			400
#define	MPR_HTTP_UNAUTHORIZED			401
#define	MPR_HTTP_PAYMENT_REQUIRED		402
#define MPR_HTTP_FORBIDDEN				403
#define	MPR_HTTP_NOT_FOUND				404
#define	MPR_HTTP_BAD_METHOD				405
#define	MPR_HTTP_REQUEST_TIME_OUT		408
#define	MPR_HTTP_LENGTH_REQUIRED		411
#define	MPR_HTTP_REQUEST_TOO_LARGE		413
#define	MPR_HTTP_REQUEST_URL_TOO_LARGE	414
#define	MPR_HTTP_UNSUPPORTED_MEDIA_TYPE	415
#define	MPR_HTTP_RANGE_NOT_SATISFIABLE	416
#define	MPR_HTTP_INTERNAL_SERVER_ERROR	500
#define	MPR_HTTP_NOT_IMPLEMENTED		501
#define	MPR_HTTP_BAD_GATEWAY			502
#define	MPR_HTTP_SERVICE_UNAVAILABLE	503
#define	MPR_HTTP_GATEWAY_TIME_OUT		504
#define	MPR_HTTP_BAD_VERSION			505
#define	MPR_HTTP_INSUFFICIENT_STORAGE	507

//
//	Proprietary HTTP codes. Only used internally.
//
#define	MPR_HTTP_COMMS_ERROR			550
#define	MPR_HTTP_CLIENT_ERROR			551

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaUrl /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaUrl {
  public:
	MprStr		parsedUrlBuf;			// Allocated storage for parsed url 
	//
	//	These are just pointers into the parsedUrlBuf
	//
	char		*proto;					// Protocol 
	char		*host;					// Url host name 
	int			port;					// Port number
	char		*uri;					// Uri path name 
	char		*ext;					// Document extension
	char		*query;					// Query string
  public:
				MaUrl();
				~MaUrl();
	int			parse(char *url);
};

////////////////////////////////////////////////////////////////////////////////

extern void maMD5Init(MD5_CONTEXT *);
extern void maMD5Update(MD5_CONTEXT *, uchar *, unsigned int);
extern void maMD5Final(uchar [16], MD5_CONTEXT *);

extern int	maCalcNonce(char **nonce, char *secret, char *etag, char *relm);
extern int	maCalcDigest(char *userName, char *password, char *realm, 
					char *uri, char *nonce, char *qop, char *nc, char *cnonce, 
					char *method, char **digest);
extern char	*maUrlEncode(char *buf, int len, char *uri, bool isAbsolute);
extern char	*maUrlDecode(char *buf, int len, char *uri, 
				bool isUrl, bool multipleArgs = 0);
extern char	*maEscapeCmd(char *buf, int len, char *cmd, int escChar);
extern char	*maEscapeHtml(char *buf, int len, char *html);
extern int	maDecode64(char *buffer, int bufsize, char *str);
extern void	maEncode64(char *buffer, int bufsize, char *str);
extern char	*maGetDateString(MprFileInfo *info);
extern char	*maMD5binary(uchar *buf, int length);
extern char	*maMD5(char *string);
extern char *maGetHttpErrorMsg(int code);
extern char	*maValidateUri(char *uri);

#if BLD_FEATURE_IF_MODIFIED
extern int	maDateParse(char *cmd);
#endif

////////////////////////////////////////////////////////////////////////////////
#endif // _h_SHARED 


//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
