/* 
 *	webs.h -- GoAhead Web public header
 *
 * Copyright (c) GoAhead Software Inc., 1992-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for information on usage and redistribution
 *
 * $Id: webs.h,v 1.9 2003/09/29 20:02:19 bporter Exp $
 */

#ifndef _h_WEBS
#define _h_WEBS 1

/******************************** Description *********************************/

/* 
 *	GoAhead Web Server header. This defines the Web public APIs.
 *	Include this header for files that contain ASP or Form procedures.
 *	Include wsIntrn.h when creating URL handlers.
 */

/********************************* Includes ***********************************/

#include	"ej.h"
#ifdef WEBS_SSL_SUPPORT
	#include	"websSSL.h"
#endif

/********************************** Defines ***********************************/
/*
 *	By license terms the server software name defined in the following line of
 *	code must not be modified.
 */
#define WEBS_NAME				T("GoAhead-Webs")
#define WEBS_VERSION			T("2.1.7")

#define WEBS_HEADER_BUFINC 		512			/* Header buffer size */
#define WEBS_ASP_BUFINC			512			/* Asp expansion increment */
#define WEBS_MAX_PASS			32			/* Size of password */
#define WEBS_BUFSIZE			1000		/* websWrite max output string */
#define WEBS_MAX_HEADER			(5 * 1024)	/* Sanity check header */
#define WEBS_MAX_URL			4096		/* Maximum URL size for sanity */
#define WEBS_SOCKET_BUFSIZ		256			/* Bytes read from socket */

#define WEBS_HTTP_PORT			T("httpPort")
#define CGI_BIN					T("cgi-bin")

/* 
 *	Request flags. Also returned by websGetRequestFlags().
 */
#define WEBS_LOCAL_PAGE			0x1			/* Request for local webs page */ 
#define WEBS_KEEP_ALIVE			0x2			/* HTTP/1.1 keep alive */
#define WEBS_DONT_USE_CACHE		0x4			/* Not implemented cache support */
#define WEBS_COOKIE				0x8			/* Cookie supplied in request */
#define WEBS_IF_MODIFIED		0x10		/* If-modified-since in request */
#define WEBS_POST_REQUEST		0x20		/* Post request operation */
#define WEBS_LOCAL_REQUEST		0x40		/* Request from this system */
#define WEBS_HOME_PAGE			0x80		/* Request for the home page */ 
#define WEBS_ASP				0x100		/* ASP request */ 
#define WEBS_HEAD_REQUEST		0x200		/* Head request */
#define WEBS_CLEN				0x400		/* Request had a content length */
#define WEBS_FORM				0x800		/* Request is a form */
#define WEBS_REQUEST_DONE		0x1000		/* Request complete */
#define WEBS_POST_DATA			0x2000		/* Already appended post data */
#define WEBS_CGI_REQUEST		0x4000		/* cgi-bin request */
#define WEBS_SECURE				0x8000		/* connection uses SSL */
#define WEBS_AUTH_BASIC			0x10000		/* Basic authentication request */
#define WEBS_AUTH_DIGEST		0x20000		/* Digest authentication request */
#define WEBS_HEADER_DONE		0x40000		/* Already output the HTTP header */
#define WEBS_SPY						0x80000		/* Spyce request */
/*
 *	URL handler flags
 */
#define WEBS_HANDLER_FIRST	0x1			/* Process this handler first */
#define WEBS_HANDLER_LAST	0x2			/* Process this handler last */

/* 
 *	Per socket connection webs structure
 */
typedef struct websRec {
	ringq_t			header;				/* Header dynamic string */
	time_t			since;				/* Parsed if-modified-since time */
	sym_fd_t		cgiVars;			/* CGI standard variables */
	sym_fd_t		cgiQuery;			/* CGI decoded query string */
	time_t			timestamp;			/* Last transaction with browser */
	int				timeout;			/* Timeout handle */
	char_t			ipaddr[32];			/* Connecting ipaddress */
	char_t			type[64];			/* Mime type */
	char_t			*dir;				/* Directory containing the page */
	char_t			*path;				/* Path name without query */
	char_t			*url;				/* Full request url */
	char_t			*host;				/* Requested host */
	char_t			*lpath;				/* Cache local path name */
	char_t			*query;				/* Request query */
	char_t			*decodedQuery;		/* Decoded request query */
	char_t			*authType;			/* Authorization type (Basic/DAA) */
	char_t			*password;			/* Authorization password */
	char_t			*userName;			/* Authorization username */
	char_t			*cookie;			/* Cookie string */
	char_t			*userAgent;			/* User agent (browser) */
	char_t			*protocol;			/* Protocol (normally HTTP) */
	char_t			*protoVersion;		/* Protocol version */
	int				sid;				/* Socket id (handler) */
	int				listenSid;			/* Listen Socket id */
	int				port;				/* Request port number */
	int				state;				/* Current state */
	int				flags;				/* Current flags -- see above */
	int				code;				/* Request result code */
	int				clen;				/* Content length */
	int				wid;				/* Index into webs */
	char_t			*cgiStdin;			/* filename for CGI stdin */
	int				docfd;				/* Document file descriptor */
	int				numbytes;			/* Bytes to transfer to browser */
	int				written;			/* Bytes actually transferred */
	void			(*writeSocket)(struct websRec *wp);
#ifdef DIGEST_ACCESS_SUPPORT
    char_t			*realm;		/* usually the same as "host" from websRec */
    char_t			*nonce;		/* opaque-to-client string sent by server */
    char_t			*digest;	/* digest form of user password */
    char_t			*uri;		/* URI found in DAA header */
    char_t			*opaque;	/* opaque value passed from server */
    char_t			*nc;		/* nonce count */
    char_t			*cnonce;	/* check nonce */
    char_t			*qop;		/* quality operator */
#endif
#ifdef WEBS_SSL_SUPPORT
	websSSL_t		*wsp;		/* SSL data structure */
#endif
} websRec;

typedef websRec	*webs_t;
typedef websRec websType;

/******************************** Prototypes **********************************/
extern int		 websAccept(int sid, char *ipaddr, int port, int listenSid);
extern int 		 websAspDefine(char_t *name, 
					int (*fn)(int ejid, webs_t wp, int argc, char_t **argv));
extern int 		 websAspRequest(webs_t wp, char_t *lpath);
extern int		 websSpyRequest(webs_t wp, char_t *lpath);
extern void		 websCloseListen();
extern int 		 websDecode64(char_t *outbuf, char_t *string, int buflen);
extern void		 websDecodeUrl(char_t *token, char_t *decoded, int len);
extern void  	 websDone(webs_t wp, int code);
extern void 	 websEncode64(char_t *outbuf, char_t *string, int buflen);
extern void  	 websError(webs_t wp, int code, char_t *msg, ...);
/* function websErrorMsg() made extern 03 Jun 02 BgP */
extern char_t 	*websErrorMsg(int code);
extern void  	 websFooter(webs_t wp);
extern int 		 websFormDefine(char_t *name, void (*fn)(webs_t wp, 
					char_t *path, char_t *query));
extern char_t 	*websGetDefaultDir();
extern char_t 	*websGetDefaultPage();
extern char_t 	*websGetHostUrl();
extern char_t 	*websGetIpaddrUrl();
extern char_t 	*websGetPassword();
extern int		 websGetPort();
extern char_t 	*websGetPublishDir(char_t *path, char_t **urlPrefix);
extern char_t 	*websGetRealm();
extern int 		 websGetRequestBytes(webs_t wp);
extern char_t	*websGetRequestDir(webs_t wp);
extern int		 websGetRequestFlags(webs_t wp);
extern char_t	*websGetRequestIpaddr(webs_t wp);
extern char_t 	*websGetRequestLpath(webs_t wp);
extern char_t	*websGetRequestPath(webs_t wp);
extern char_t	*websGetRequestPassword(webs_t wp);
extern char_t	*websGetRequestType(webs_t wp);
extern int 		 websGetRequestWritten(webs_t wp);
extern char_t 	*websGetVar(webs_t wp, char_t *var, char_t *def);
extern int 		 websCompareVar(webs_t wp, char_t *var, char_t *value);
extern void 	 websHeader(webs_t wp);
extern int		 websOpenListen(int port, int retries);
extern int 		 websPageOpen(webs_t wp, char_t *lpath, char_t *path,
					int mode, int perm);
extern void 	 websPageClose(webs_t wp);
extern int 		 websPublish(char_t *urlPrefix, char_t *path);
extern void		 websRedirect(webs_t wp, char_t *url);
extern void 	 websSecurityDelete();
extern int 		 websSecurityHandler(webs_t wp, char_t *urlPrefix, 
					char_t *webDir, int arg, char_t *url, char_t *path, 
					char_t *query);
extern void 	 websSetDefaultDir(char_t *dir);
extern void 	 websSetDefaultPage(char_t *page);
extern void 	 websSetEnv(webs_t wp);
extern void 	 websSetHost(char_t *host);
extern void 	 websSetIpaddr(char_t *ipaddr);
extern void 	 websSetPassword(char_t *password);
extern void 	 websSetRealm(char_t *realmName);
extern void 	 websSetRequestBytes(webs_t wp, int bytes);
extern void		 websSetRequestFlags(webs_t wp, int flags);
extern void 	 websSetRequestLpath(webs_t wp, char_t *lpath);
extern void 	 websSetRequestPath(webs_t wp, char_t *dir, char_t *path);
extern char_t	*websGetRequestUserName(webs_t wp);
extern void 	 websSetRequestWritten(webs_t wp, int written);
extern void 	 websSetVar(webs_t wp, char_t *var, char_t *value);
extern int 		 websTestVar(webs_t wp, char_t *var);
extern void		 websTimeoutCancel(webs_t wp);
extern int 		 websUrlHandlerDefine(char_t *urlPrefix, char_t *webDir, 
					int arg, int (*fn)(webs_t wp, char_t *urlPrefix, 
					char_t *webDir, int arg, char_t *url, char_t *path, 
					char_t *query), int flags);
extern int 		 websUrlHandlerDelete(int (*fn)(webs_t wp, char_t *urlPrefix,
					char_t *webDir, int arg, char_t *url, char_t *path, 
					char_t *query));
extern int		 websUrlHandlerRequest(webs_t wp);
extern int 		 websUrlParse(char_t *url, char_t **buf, char_t **host, 
					char_t **path, char_t **port, char_t **query, 
					char_t **proto, char_t **tag, char_t **ext);
extern char_t 	*websUrlType(char_t *webs, char_t *buf, int charCnt);
extern int 		 websWrite(webs_t wp, char_t* fmt, ...);
extern int 		 websWriteBlock(webs_t wp, char_t *buf, int nChars);
extern int 		 websWriteDataNonBlock(webs_t wp, char *buf, int nChars);
extern int 		 websValid(webs_t wp);
extern int 		 websValidateUrl(webs_t wp, char_t *path);
extern void		websSetTimeMark(webs_t wp);

/*
 *	The following prototypes are used by the SSL patch found in websSSL.c
 */
extern int 		websAlloc(int sid);
extern void 	websFree(webs_t wp);
extern void 	websTimeout(void *arg, int id);
extern void 	websReadEvent(webs_t wp);

/*
 *	Prototypes for functions available when running as part of the 
 *	GoAhead Embedded Management Framework (EMF)
 */
#ifdef EMF
extern void 	 websFormExplain(webs_t wp, char_t *path, char_t *query);
#endif

#endif /* _h_WEBS */

/******************************************************************************/

