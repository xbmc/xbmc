/*
 *	@file 	compatApi.h
 *	@brief 	GoAhead WebServer "C" language compatability API
 */

/********************************* Copyright **********************************/
/*
 *	@copy	default.g
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	Portions Copyright (c) GoAhead Software, 1995-2000. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Mbedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Mbedthis 
 *	Software at http://www.mbedthis.com 
 *	
 *	@end
 */

/********************************** Includes **********************************/

#ifndef _h_GACOMPAT
#define _h_GACOMPAT 1

#if BLD_FEATURE_EJS
#include "ejs.h"
#endif

#if BLD_FEATURE_GACOMPAT_MODULE

#ifdef  __cplusplus
extern "C" {
#endif

/*********************************** Types ************************************/

/*
 *	Convenient constants
 */
#define VALUE_MAX_STRING	MPR_MAX_STRING
#define BUF_MAX				MPR_BUFSIZE
#define SOCKET_BINARY 		O_BINARY
#define SOCKET_RDONLY 		O_RDONLY
#define VALUE_ALLOCATE 		0x1
#define WEBS_NAME 			"Mbedthis-Appweb/" BLD_VERSION
#define WEBS_SYM_INIT 		64

typedef void				*webs_t;

#ifndef CHAR_T_DEFINED
typedef char char_t;
#define CHAR_T_DEFINED
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct stat		gstat_t;
typedef int				bool_t;

#define	T(s)			s
#define	TSZ(x)			(sizeof(x))
#define	TASTRL(x)		(strlen(x) + 1)

#define gmain		main
#define gasctime	asctime
#define gsprintf	sprintf
#define gprintf		printf
#define gfprintf	fprintf
#define gsscanf		sscanf
#define gvsprintf	vsprintf
#define gstrcpy		strcpy
#define gstrncpy	strncpy
#define gstrncat	strncat
#define gstrlen		strlen
#define gstrcat		strcat
#define gstrcmp		strcmp
#define gstrncmp	strncmp
#define gstricmp	mprStrCmpAnyCase
#define gstrtoi		atoi
#define gstrchr		strchr
#define gstrrchr	strrchr
#define gstrtok		strtok
#define gstrnset	strnset
#define gstrrchr	strrchr
#define gstrstr		strstr
#define gstrtol		strtol

#define gfopen		fopen
#define gopen		open
#define gclose		close
#define gcreat		creat
#define gfgets		fgets
#define gfputs		fputs
#define gfscanf		fscanf
#define ggets		gets
#define glseek		lseek
#define gunlink		unlink
#define gread		read
#define grename		rename
#define gwrite		write
#define gtmpnam		tmpnam
#define gtempnam	tempnam
#define gfindfirst	_findfirst
#define gfinddata_t	_finddata_t
#define gfindnext	_findnext
#define gfindclose	_findclose
#define gstat		stat
#define gaccess		access
#define gchmod		chmod
#define gmkdir		mkdir
#define gchdir		chdir
#define grmdir		rmdir
#define ggetcwd		_getcwd
#define gtolower	tolower
#define gtoupper	toupper
#define gremove		remove
#define gisspace	isspace
#define gisdigit	isdigit
#define gisxdigit	isxdigit
#define gisupper	isupper
#define gislower	islower
#define gisalnum	isalnum
#define gisalpha	isalpha
#define gatoi(s)	atoi(s)
#define gctime		ctime
#define ggetenv		getenv
#define gexecvp		execvp

#define B_L			__FILE__, __LINE__
#define a_assert(C)	if (C) ; else mprAssert(#C)

#define balloc(B_L_SPec, num)	 	mprMalloc(num)
#define bfree(B_L_Spec, p) 			mprFree(p)
#define bfreeSafe(B_L_Spec, p) 		mprFree(p)
#define brealloc(B_L_Spec, p, num) 	mprRealloc(p, num)
#define bstrdup(B_L_Spec, s) 		mprStrdup(s)

extern int fmtAlloc(char **buf, int maxSize, char *fmt, ...);
extern int fmtValloc(char **buf, int maxSize, char *fmt, va_list arg);
#define fmtStatic mprSprintf

typedef struct {
	ulong			size;
	int				isDir;
	time_t			mtime;
} websStatType;

typedef void	(*WebsFormCb)(webs_t wp, char_t *path, char_t *query);
typedef int 	(*WebsAspCb)(int ejid, webs_t wp, int argc, char_t **argv);
typedef int 	(*WebsHandlerCb)(webs_t wp, char_t *urlPrefix, char_t *webDir, 
					int arg, char_t *url, char_t *path, char_t *query);
typedef void	(emfSchedProc)(void *data, int id);

typedef int 	sym_fd_t;

/*
 *	Solaris already has a vtype_t
 */
#define vtype_t maVtype_t

typedef enum {
	undefined	= 0,	byteint		= 1, shortint	= 2,	integer		= 3,
	hex			= 4,	percent 	= 5, octal		= 6,	big			= 7,
	flag		= 8,	floating	= 9, string 	= 10,	bytes 		= 11,
	symbol 		= 12,	errmsg 		= 13 
} vtype_t;

typedef struct {
	union {
		char	flag;
		char	byteint;
		short	shortint;
		char	percent;
		long	integer;
		long	hex;
		long	octal;
		long	big[2];
		/* double	floating; */
		char_t	*string;
		char	*bytes;
		char_t	*errmsg;
		void	*symbol;
	} value;
	vtype_t			type;
	unsigned int	valid		: 8;
	unsigned int	allocated	: 8;		/* String was balloced */
} value_t;

typedef struct sym_t {
	struct sym_t	*forw;
	value_t			name;
	value_t			content;
	int				arg;
} sym_t;

extern sym_fd_t	symOpen(int hash_size);
extern void		symClose(sym_fd_t sd);
extern sym_t	*symLookup(sym_fd_t sd, char_t *name);
extern sym_t	*symEnter(sym_fd_t sd, char_t *name, value_t v, int arg);
extern int		symDelete(sym_fd_t sd, char_t *name);
extern sym_t	*symFirstEx(sym_fd_t sd, void **current);
extern sym_t	*symNextEx(sym_fd_t sd, void **current);

/*
 *	Socket definitions
 */

#define SOCKET_READABLE			0x2		/* Make socket readable */ 
#define SOCKET_WRITABLE			0x4		/* Make socket writable */
#define SOCKET_EXCEPTION		0x8		/* Interested in exceptions */

#define SOCKET_BROADCAST		0x4		/* Broadcast mode */
#define SOCKET_DATAGRAM			0x20	/* Use datagrams */
#define SOCKET_BLOCK			0x80	/* Use blocking I/O */

/*
 *	User management definitions
 */
#define UM_OK				0
#define UM_ERR_GENERAL		-1
#define UM_ERR_NOT_FOUND	-2
#define UM_ERR_PROTECTED	-3
#define UM_ERR_DUPLICATE	-4
#define UM_ERR_IN_USE		-5
#define UM_ERR_BAD_NAME		-6
#define UM_ERR_BAD_PASSWORD -7

/*
 *	Privilege Masks
 */
#define PRIV_NONE	0x00
#define PRIV_READ	0x01
#define PRIV_WRITE	0x02
#define PRIV_ADMIN	0x04

typedef enum {
	AM_NONE = 0,
	AM_FULL,
	AM_BASIC,
	AM_DIGEST,
	AM_INVALID
} accessMeth_t;

/*
 *	EMFDB
 */
#define		T_INT					0
#define		T_STRING				1

#define		DB_OK					0
#define		DB_ERR_GENERAL			-1
#define		DB_ERR_COL_NOT_FOUND	-2
#define		DB_ERR_COL_DELETED		-3
#define		DB_ERR_ROW_NOT_FOUND	-4
#define		DB_ERR_ROW_DELETED		-5
#define		DB_ERR_TABLE_NOT_FOUND	-6
#define		DB_ERR_TABLE_DELETED	-7
#define		DB_ERR_BAD_FORMAT		-8

/*
 * 	Pass DB_CASE_INSENSITIVE as the "flags" argument to dbSearchString() to 
 *	force a case-insensitive search.
 */
#define     DB_CASE_INSENSITIVE  1

typedef struct dbTable_s {
	char_t	*name;
	int		nColumns;
	char_t	**columnNames;
	int		*columnTypes;
	int		nRows;
	int		**rows;
} dbTable_t;

/********************************* Prototypes *********************************/

typedef void 	(*socketHandler_t)(int sid, int mask, int data);
typedef int		(*socketAccept_t)(int sid, char *ipaddr, int port, 
					int listenSid);

extern int 		socketOpenConnection(char *host, int port, 
					socketAccept_t accept, int flags);
extern void		socketCloseConnection(int sid);
extern void		socketCreateHandler(int sid, int mask, socketHandler_t 
					handler, int arg);
extern void		socketDeleteHandler(int sid);
extern int		socketFlush(int sid);
extern int		socketWrite(int sid, char *buf, int len);
extern int		socketWriteString(int sid, char_t *buf);
extern int 		socketGetHandle(int sid);
extern int 		socketSetBlock(int sid, int flags);
extern int 		socketGetBlock(int sid);
extern int		socketRead(int sid, char *buf, int len);
extern int		socketEof(int sid);

extern int		emfSchedCallback(int delay, emfSchedProc *proc, void *arg);
extern void 	emfUnschedCallback(int id);
extern void 	emfReschedCallback(int id, int delay);

extern int 		websAspDefine(char_t *name, WebsAspCb fn);
extern void 	websDecodeUrl(char_t *decoded, char *token, int len);
extern void 	websDone(webs_t wp, int code);
extern void 	websError(webs_t wp, int code, char_t *msg, ...);
extern char_t 	*websErrorMsg(int code);
extern void 	websFooter(webs_t wp);
extern int 		websFormDefine(char_t *name, WebsFormCb fn);
extern char_t 	*websGetDateString(websStatType *sbuf);
extern char_t 	*websGetRequestLpath(webs_t wp);
extern char_t 	*websGetPath(webs_t wp);
extern char_t 	*websGetVar(webs_t wp, char_t *var, char_t *def);
extern void 	websHeader(webs_t wp);
extern int 		websPageOpen(webs_t wp, char_t *fileName, char_t *uri, 
						int mode, int perm);
extern int 		websPageStat(webs_t wp, char_t *fileName, char_t *uri, 
						websStatType* sbuf);
extern void 	websRedirect(webs_t wp, char_t *url);
extern void 	websSetRealm(char_t *realmName);
extern void 	websSetRequestLpath(webs_t wp, char_t *fileName);
extern int 		websUrlHandlerDefine(char_t *urlPrefix, char_t *webDir, 
						int arg, int (*fn)(webs_t wp, char_t *urlPrefix, 
						char_t *webDir, int arg, char_t *url, char_t *path, 
						char_t *query), int flags);
extern char_t 	*websGetUserName(webs_t wp);
extern char_t 	*websGetUrl(webs_t wp);
extern int 		websValid(webs_t wp);
extern int 		websValidateUrl(webs_t wp, char_t *path);
extern int 		websWrite(webs_t wp, char_t *fmt, ...);
extern int 		websWriteBlock(webs_t wp, char_t *buf, int nChars);

extern int		umOpen();
extern void 	umClose();
extern int		umRestore(char_t *filename);
extern int		umCommit(char_t *filename);
extern bool_t	umAccessLimitExists(char_t *url);
extern int		umAddAccessLimit(char_t *url, accessMeth_t am,
						short secure, char_t *group);
extern int		umAddGroup(char_t *group, short privilege, accessMeth_t am,
						bool_t protect, bool_t disabled);
extern int		umAddUser(char_t *user, char_t *password, char_t *group, 
						bool_t protect, bool_t disabled);
extern int		umDeleteAccessLimit(char_t *url);
extern int		umDeleteGroup(char_t *group);
extern int		umDeleteUser(char_t *user);
extern char_t	*umGetAccessLimit(char_t *url);
extern char_t	*umGetAccessLimitGroup(char_t *url);
extern accessMeth_t	umGetAccessLimitMethod(char_t *url);
extern short	umGetAccessLimitSecure(char_t *url);
extern accessMeth_t	umGetAccessMethodForURL(char_t *url);
extern char_t	*umGetFirstAccessLimit();
extern char_t 	*umGetFirstGroup();
extern char_t 	*umGetNextGroup(char_t *lastUser);
extern char_t 	*umGetFirstUser();
extern char_t	*umGetNextAccessLimit(char_t *lastUser);
extern char_t 	*umGetNextUser(char_t *lastUser);
extern accessMeth_t umGetGroupAccessMethod(char_t *group);
extern bool_t 	umGetGroupEnabled(char_t *group);
extern bool_t	umGetGroupInUse(char_t *group);
extern short 	umGetGroupPrivilege(char_t *group);
extern bool_t	umGetGroupProtected(char_t *group);
extern bool_t 	umGetUserEnabled(char_t *user);
extern char_t 	*umGetUserGroup(char_t *user);
extern char_t 	*umGetUserPassword(char_t *user);
extern bool_t	umGetUserProtected(char_t *user);
extern bool_t 	umGroupExists(char_t *group);
extern int		umSetAccessLimitGroup(char_t *url, char_t *group);
extern int		umSetAccessLimitMethod(char_t *url, accessMeth_t am);
extern int		umSetAccessLimitSecure(char_t *url, short secure);
extern int 		umSetGroupAccessMethod(char_t *group, accessMeth_t am);
extern int 		umSetGroupEnabled(char_t *group, bool_t enabled);
extern int 		umSetGroupPrivilege(char_t *group, short privileges);
extern int		umSetGroupProtected(char_t *group, bool_t protect);
extern int 		umSetUserEnabled(char_t *user, bool_t enabled);
extern int 		umSetUserGroup(char_t *user, char_t *password);
extern int 		umSetUserPassword(char_t *user, char_t *password);
extern int		umSetUserProtected(char_t *user, bool_t protect);
extern bool_t	umUserCanAccessURL(char_t *user, char_t *url);
extern bool_t 	umUserExists(char_t *user);

/*
 *	Misc
 */
extern char_t 	*strlower(char_t *string);
extern char_t 	*strupper(char_t *string);
extern value_t 	valueInteger(long value);
extern value_t 	valueString(char_t *value, int flags);
extern void 	trace(int level, char_t *fmt, ...);
extern void		traceRaw(char_t *buf);
extern int		hAlloc(void ***map);
extern int 		hFree(void ***map, int handle);
extern int 		hAllocEntry(void ***list, int *max, int size);

/*
 *	EJS APIs
 */

#if BLD_FEATURE_EJS

/*
 *	These APIs are deprecated. Only use them to support legacy applications.
 *	They will be removed in a future release.
 */

#ifndef CHAR_T_DEFINED
typedef char char_t;
#define CHAR_T_DEFINED
#endif

/*
 *	Some functions can just be renamed
 */
#define ejCloseBlock 	ejsCloseBlock
#define ejGetLineNumber ejsGetLineNumber
#define ejGetUserHandle ejsGetUserHandle
#define ejOpenBlock		ejsOpenBlock
#define ejSetUserHandle	ejsSetUserHandle

/*
 *	Some need to be wrapped. See ejsCompat.c
 */
extern int 		ejArgs(int argc, char_t **argv, char_t *fmt, ...);
extern char_t	*ejGetResult(int eid);
extern char_t	*ejEvalBlock(int eid, char_t *script, char_t **emsg);
extern int 		ejSetGlobalFunction(int eid, char_t *name, 
					int (*fn)(int eid, void *handle, int argc, char_t **argv));
extern void		ejSetGlobalVar(int eid, char_t *var, char_t *value);
extern void		ejSetLocalVar(int eid, char_t *var, char_t *value);
extern void		ejSetResult(int eid, char_t *s);
extern void		ejSetVar(int eid, char_t *var, char_t *value);

#if NOT_THREAD_SAFE
/*
 *	Must port to the appweb APIs
 */
extern int		ejGetVar(int eid, char_t *var, char_t **value);
extern char_t	*ejEval(int eid, char_t *script, char_t **emsg);
extern char_t	*ejEvalFile(int eid, char_t *path, char_t **emsg);
#endif

#endif /* BLD_FEATURE_EJS */

/*
 *	EMFDB
 */
extern int		dbRegisterDBSchema(dbTable_t *sTable);

extern int		dbOpen(char_t *databasename, char_t *filename,
					int (*gettime)(int did), int flags);
extern void		dbClose(int did);
extern int		dbGetTableId(int did, char_t *tname);
extern char_t	*dbGetTableName(int did, int tid);
extern int		dbReadInt(int did, char_t *table, char_t *column, int row,
					int *returnValue);
extern int		dbReadStr(int did, char_t *table, char_t *column, int row,
					char_t **returnValue);
extern int		dbWriteInt(int did, char_t *table, char_t *column, int row,
					int idata);
extern int		dbWriteStr(int did, char_t *table, char_t *column, int row,
					char_t *s);
extern int		dbAddRow(int did, char_t *table);
extern int		dbDeleteRow(int did, char_t *table, int rid);
extern int		dbSetTableNrow(int did, char_t *table, int nNewRows);
extern int		dbGetTableNrow(int did, char_t *table);

/*
 *	Dump the contents of a database to file
 */
extern int		dbSave(int did, char_t *filename, int flags);

/*
 *	Load the contents of a database to file
 */
extern int		dbLoad(int did, char_t *filename, int flags);

/*
 *	Search for a data in a given column. If the value of 'flags' is 
 *	DB_CASE_INSENSITIVE, use a case-insensitive string compare when searching.
 */
extern int		dbSearchStr(int did, char_t *table, char_t *column,
					char_t *value, int flags);

extern void		dbZero(int did);

extern char_t	*basicGetProductDir();
extern void		basicSetProductDir(char_t *proddir);

/*
 *	Replacement routines for direct wp->field accesses
 */

extern char_t	*websGetUserName(webs_t wp);		/* wp->userName */
	/* maGetUserName */
extern char_t	*websGetCookie(webs_t wp);			/* wp->cookie */
	/* maGetCookie */
extern char_t	*websGetPath(webs_t wp);			/* wp->path */
	/* maGetUri */
extern char_t	*websGetUrl(webs_t wp);				/* wp->url */
	/* maGetXXXX */
extern char_t	*websGetFlags(webs_t wp);			/* wp->flags */
	/* XXXX */
extern char_t	*websGetPort(webs_t wp);			/* wp->port */
	/* rq->getPort() */
extern char_t	*websGetCgiVars(webs_t wp);			/* wp->cgiVars */
	/* XXXX */
extern char_t	*websGetPostDataLen(webs_t wp);		/* wp->lenPostData */
	/* XXXX */
extern char_t	*websGetPostData(webs_t wp);		/* wp->postData[] */
	/* XXXX */

/*****************************************************************************/

#ifdef  __cplusplus
} 	/* extern "C" */
#endif

#endif /* BLD_FEATURE_GACOMPAT_MODULE */
#endif /* _h_COMPAT */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
