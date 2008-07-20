/*
 * uemf.h -- GoAhead Micro Embedded Management Framework Header
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: uemf.h,v 1.7 2003/04/11 17:59:49 bporter Exp $
 */

#ifndef _h_UEMF
#define _h_UEMF 1

/******************************** Description *********************************/

/* 
 *	GoAhead Web Server header. This defines the Web public APIs
 */

/******************************* Per O/S Includes *****************************/

#ifdef WIN
	#include	<direct.h>
	#include	<io.h>
	#include	<sys/stat.h>
	#include	<limits.h>
	#include	<tchar.h>
#ifdef _XBOX
	#include	<xtl.h>
	#include	"socketutil.h"
#else
	#include	<windows.h>
	#include	<winnls.h>
	#include	"socketutil.h"
#endif

	#include	<time.h>
	#include	<sys/types.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* WIN */

#ifdef CE
	/*#include	<errno.h>*/
	#include	<limits.h>
	#include	<tchar.h>
	#include	<windows.h>
	#include	<winsock.h>
	#include	<winnls.h>
	#include	"CE/wincompat.h"
	#include	<winsock.h>
#endif /* CE */

#ifdef NW
	#include	<direct.h>
	#include	<io.h>
	#include	<sys/stat.h>
	#include	<time.h>
	#include	<sys/types.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<fcntl.h>
	#include	<errno.h>
	#include	<niterror.h>
	#define		EINTR EINUSE
	#define		 WEBS	1
	#include	<limits.h>
	#include	<netdb.h>
	#include	<process.h>
	#include	<tiuser.h>
	#include	<sys/time.h>
	#include	<arpa/inet.h>
	#include	<sys/types.h>
	#include	<sys/socket.h>
	#include	<sys/filio.h>
	#include	<netinet/in.h>
#endif /* NW */

#ifdef SCOV5 
	#include	<sys/types.h>
	#include	<stdio.h>
	#include	"sys/socket.h"
	#include	"sys/select.h"
	#include	"netinet/in.h"
	#include 	"arpa/inet.h"
	#include 	"netdb.h"
#endif /* SCOV5 */

#ifdef UNIX
	#include	<stdio.h>
#endif /* UNIX */

#ifdef LINUX
	#include	<sys/types.h>
	#include	<sys/stat.h>
	#include	<sys/param.h>
	#include	<limits.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<unistd.h>
	#include	<sys/socket.h>
	#include	<sys/select.h>
	#include	<netinet/in.h>
	#include 	<arpa/inet.h>
	#include 	<netdb.h>
	#include	<time.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* LINUX */

#ifdef LYNX
	#include	<limits.h>
	#include	<stdarg.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<unistd.h>
	#include	<socket.h>
	#include	<netinet/in.h>
	#include 	<arpa/inet.h>
	#include 	<netdb.h>
	#include	<time.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* LYNX */

#ifdef MACOSX
	#include	<sys/stat.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<unistd.h>
	#include	<sys/socket.h>
	#include	<netinet/in.h>
	#include 	<arpa/inet.h>
	#include 	<netdb.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* MACOSX */

#ifdef UW
	#include	<stdio.h>
#endif /* UW */

#ifdef VXWORKS
	#include	<vxWorks.h>
	#include	<sockLib.h>
	#include	<selectLib.h>
	#include	<inetLib.h>
	#include	<ioLib.h>
	#include	<stdio.h>
	#include	<stat.h>
	#include	<time.h>
	#include	<usrLib.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* VXWORKS */

#ifdef sparc
# define __NO_PACK
#endif /* sparc */

#ifdef SOLARIS
	#include	<sys/types.h>
	#include	<limits.h>
	#include	<stdio.h>
	#include	<stdlib.h>
	#include	<unistd.h>
	#include	<socket.h>
	#include	<sys/select.h>
	#include	<netinet/in.h>
	#include 	<arpa/inet.h>
	#include 	<netdb.h>
	#include	<time.h>
	#include	<fcntl.h>
	#include	<errno.h>
#endif /* SOLARIS */

#ifdef QNX4
	#include	<sys/types.h>
	#include	<stdio.h>
	#include	<sys/socket.h>
	#include	<sys/select.h>
	#include	<netinet/in.h>
	#include 	<arpa/inet.h>
	#include 	<netdb.h>
    #include    <stdlib.h>
    #include    <unistd.h>
    #include    <sys/uio.h>
    #include    <sys/wait.h>
#endif /* QNX4 */

#ifdef ECOS
	#include	<limits.h>
	#include	<cyg/infra/cyg_type.h>
	#include	<cyg/kernel/kapi.h>
	#include	<time.h>
	#include	<network.h>
	#include	<errno.h>
#endif /* ECOS */

/********************************** Includes **********************************/

#include	<ctype.h>
#include	<stdarg.h>
#include	<string.h>

#ifndef WEBS
#include	"messages.h"
#endif /* ! WEBS */

/******************************* Per O/S Defines *****************************/

#ifdef UW
	#define		__NO_PACK		1
#endif /* UW */

#if (defined (SCOV5) || defined (VXWORKS) || defined (LINUX) || defined (LYNX) || defined (MACOSX))
#ifndef O_BINARY
#define O_BINARY 		0
#endif /* O_BINARY */
#define	SOCKET_ERROR	-1
#endif /* SCOV5 || VXWORKS || LINUX || LYNX || MACOSX */

#if (defined (WIN) || defined (CE))
/*
 *	__NO_FCNTL means can't access fcntl function.  Fcntl.h is still available.
 */
#define		__NO_FCNTL		1

#undef R_OK
#define R_OK	4
#undef W_OK
#define W_OK	2
#undef X_OK
#define X_OK	1
#undef F_OK
#define F_OK	0
#endif /* WIN || CE */

#if (defined (LINUX) && !defined (_STRUCT_TIMEVAL))
struct timeval
{
	time_t	tv_sec;		/* Seconds.  */
	time_t	tv_usec;	/* Microseconds.  */
};
#define _STRUCT_TIMEVAL 1
#endif /* LINUX && ! _STRUCT_TIMEVAL */

#ifdef ECOS
	#define		O_RDONLY		1
	#define		O_BINARY		2

	#define		__NO_PACK		1
	#define		__NO_EJ_FILE	1
	#define		__NO_CGI_BIN	1
	#define		__NO_FCNTL		1

/*
 *	#define LIBKERN_INLINE to avoid kernel inline functions
 */
	#define		LIBKERN_INLINE

#endif /* ECOS */

#ifdef QNX4
    typedef long        fd_mask;
    #define NFDBITS (sizeof (fd_mask) * NBBY)   /* bits per mask */
#endif /* QNX4 */

#ifdef NW
	#define fd_mask			fd_set
	#define INADDR_NONE		-1l
	#define Sleep			delay

	#define __NO_FCNTL		1

	#undef R_OK
	#define R_OK    4
	#undef W_OK
	#define W_OK    2
	#undef X_OK
	#define X_OK    1
	#undef F_OK
	#define F_OK    0
#endif /* NW */

/********************************** Unicode ***********************************/
/* 
 *	Constants and limits. Also FNAMESIZE and PATHSIZE are currently defined 
 *	in param.h to be 128 and 512
 */
#define TRACE_MAX			(4096 - 48)
#define VALUE_MAX_STRING	(4096 - 48)
#define SYM_MAX				(512)
#define XML_MAX				4096			/* Maximum size for tags/tokens */
#define BUF_MAX				4096			/* General sanity check for bufs */
#define FMT_STATIC_MAX		256				/* Maximum for fmtStatic calls */

#if (defined (LITTLEFOOT) || defined (WEBS))
#define LF_BUF_MAX		(510)
#define LF_PATHSIZE		LF_BUF_MAX
#else
#define	LF_BUF_MAX		BUF_MAX
#define LF_PATHSIZE		PATHSIZE
#define UPPATHSIZE		PATHSIZE
#endif /* LITTLEFOOT || WEBS */

#ifndef CHAR_T_DEFINED
#define CHAR_T_DEFINED 1
#ifdef UNICODE
/*
 *	To convert strings to UNICODE. We have a level of indirection so things
 *	like T(__FILE__) will expand properly.
 */
#define	T(x)				__TXT(x)
#define	__TXT(s)			L ## s
typedef unsigned short 		char_t;
typedef unsigned short		uchar_t;

/*
 *	Text size of buffer macro. A buffer bytes will hold (size / char size) 
 *	characters. 
 */
#define	TSZ(x)				(sizeof(x) / sizeof(char_t))

/*
 *	How many ASCII bytes are required to represent this UNICODE string?
 */
#define	TASTRL(x)			((wcslen(x) + 1) * sizeof(char_t))

#else
#define	T(s) 				s
typedef char				char_t;
#define	TSZ(x)				(sizeof(x))
#define	TASTRL(x)			(strlen(x) + 1)
#ifdef WIN
typedef unsigned char		uchar_t;
#endif /* WIN */

#endif /* UNICODE */

#endif /* ! CHAR_T_DEFINED */

/*
 *	"Boolean" constants
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 *	GoAhead Copyright.
 */
#define GOAHEAD_COPYRIGHT \
	T("Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.")

/*
 *	The following include has to be after the unicode defines.  By putting it
 *	here, many modules in various parts of the tree are cleaner.
 */
#if (defined (LITTLEFOOT) && defined (INMEM))
	#include	"lf/inmem.h"
#endif /* LITTLEFOOT && INMEM */

/*
 *	Type for unicode systems
 */
#ifdef UNICODE

#define gmain		wmain

#define gasctime	_wasctime
#define gsprintf	swprintf
#define gprintf		wprintf
#define gfprintf	fwprintf
#define gsscanf		swscanf
#define gvsprintf	vswprintf

#define gstrcpy		wcscpy
#define gstrncpy	wcsncpy
#define gstrncat	wcsncat
#define gstrlen		wcslen
#define gstrcat		wcscat
#define gstrcmp		wcscmp
#define gstrncmp	wcsncmp
#define gstricmp	wcsicmp
#define gstrchr		wcschr
#define gstrrchr	wcsrchr
#define gstrtok		wcstok
#define gstrnset	wcsnset
#define gstrrchr	wcsrchr
#define gstrstr		wcsstr
#define gstrtol		wcstol

#define gfopen		_wfopen
#define gopen		_wopen
#define gclose		close
#define gcreat		_wcreat
#define gfgets		fgetws
#define gfputs		fputws
#define gfscanf		fwscanf
#define ggets		_getws
#define glseek		lseek
#define gunlink		_wunlink
#define gread		read
#define grename		_wrename
#define gwrite		write
#define gtmpnam		_wtmpnam
#define gtempnam	_wtempnam
#define gfindfirst	_wfindfirst
#define gfinddata_t	_wfinddata_t
#define gfindnext	_wfindnext
#define gfindclose	_findclose
#define gstat		_wstat
#define gaccess		_waccess
#define gchmod		_wchmod

typedef struct _stat gstat_t;

#define gmkdir		_wmkdir
#define gchdir		_wchdir
#define grmdir		_wrmdir
#define ggetcwd		_wgetcwd

#define gtolower	towlower
#define gtoupper	towupper
#ifdef CE
#define gisspace	isspace
#define gisdigit	isdigit
#define gisxdigit	isxdigit
#define gisupper	isupper
#define gislower	islower
#define gisprint	isprint
#else
#define gremove		_wremove
#define gisspace	iswspace
#define gisdigit	iswdigit
#define gisxdigit	iswxdigit
#define gisupper	iswupper
#define gislower	iswlower
#endif	/* if CE */
#define gisalnum	iswalnum
#define gisalpha	iswalpha
#define gatoi(s)	wcstol(s, NULL, 10)

#define gctime		_wctime
#define ggetenv		_wgetenv
#define gexecvp		_wexecvp

#else /* ! UNICODE */

#ifdef VXWORKS
#define gchdir		vxchdir
#define gmkdir		vxmkdir
#define grmdir		vxrmdir
#elif (defined (LYNX) || defined (LINUX) || defined (MACOSX) || defined (SOLARIS))
#define gchdir		chdir
#define gmkdir(s)	mkdir(s,0755)
#define grmdir		rmdir
#else
#define gchdir		chdir
#define gmkdir		mkdir
#define grmdir		rmdir
#endif /* VXWORKS #elif LYNX || LINUX || MACOSX || SOLARIS*/

#define gclose		close
#define gclosedir	closedir
#define gchmod		chmod
#define ggetcwd		getcwd
#define glseek		lseek
#define gloadModule	loadModule
#define gopen		open
#define gopendir	opendir
#define gread		read
#define greaddir	readdir
#define grename		rename
#define gstat		stat
#define gunlink		unlink
#define gwrite		write

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
#define gstricmp	strcmpci
#define gstrchr		strchr
#define gstrrchr	strrchr
#define gstrtok		strtok
#define gstrnset	strnset
#define gstrrchr	strrchr
#define gstrstr		strstr
#define gstrtol		strtol

#define gfopen		fopen
#define gcreat		creat
#define gfgets		fgets
#define gfputs		fputs
#define gfscanf		fscanf
#define ggets		gets
#define gtmpnam		tmpnam
#define gtempnam	tempnam
#define gfindfirst	_findfirst
#define gfinddata_t	_finddata_t
#define gfindnext	_findnext
#define gfindclose	_findclose
#define gaccess		access

typedef struct stat gstat_t;

#define gremove		remove

#define gtolower	tolower
#define gtoupper	toupper
#define gisspace	isspace
#define gisdigit	isdigit
#define gisxdigit	isxdigit
#define gisalnum	isalnum
#define gisalpha	isalpha
#define gisupper	isupper
#define gislower	islower
#define gatoi		atoi

#define gctime		ctime
#define ggetenv		getenv
#define gexecvp		execvp
#ifndef VXWORKS
#define gmain		main
#endif /* ! VXWORKS */
#ifdef VXWORKS
#define	fcntl(a, b, c)
#endif /* VXWORKS */
#endif /* ! UNICODE */

/*
 *	Include inmem.h here because it redefines many of the file access fucntions.
 *	Otherwise there would be lots more #if-#elif-#else-#endif ugliness.
 */
#ifdef INMEM
	#include	"lf/inmem.h"
#endif

/********************************** Defines ***********************************/

#ifndef FNAMESIZE
#define FNAMESIZE			254			/* Max length of file names */
#endif /* FNAMESIZE */

#define E_MAX_ERROR			4096
#define URL_MAX				4096

/*
 * Error types
 */
#define	E_ASSERT			0x1			/* Assertion error */
#define	E_LOG				0x2			/* Log error to log file */
#define	E_USER				0x3			/* Error that must be displayed */

#define E_L					T(__FILE__), __LINE__
#define E_ARGS_DEC			char_t *file, int line
#define E_ARGS				file, line

#if (defined (ASSERT) || defined (ASSERT_CE))
	#define a_assert(C)		if (C) ; else error(E_L, E_ASSERT, T("%s"), T(#C))
#else
	#define a_assert(C)		if (1) ; else
#endif /* ASSERT || ASSERT_CE */

/******************************************************************************/
/*                                 VALUE                                      */
/******************************************************************************/
/*
 *	These values are not prefixed so as to aid code readability
 */

typedef enum {
	undefined	= 0,
	byteint		= 1,
	shortint	= 2,
	integer		= 3,
	hex			= 4,
	percent 	= 5,
	octal		= 6,
	big			= 7,
	flag		= 8,
	floating	= 9,
	vtype_string 		= 10,
	bytes 		= 11,
	symbol 		= 12,
	errmsg 		= 13
} vtype_t;

#ifndef __NO_PACK
#pragma pack(2)
#endif /* _NO_PACK */

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
#ifdef FLOATING_POINT_SUPPORT
		double	floating;
#endif /* FLOATING_POINT_SUPPORT */
		char_t	*string;
		char	*bytes;
		char_t	*errmsg;
		void	*symbol;
	} value;

	vtype_t			type;
	unsigned int	valid		: 8;
	unsigned int	allocated	: 8;		/* String was balloced */
} value_t;

#ifndef __NO_PACK
#pragma pack()
#endif /* __NO_PACK */

/*
 *	Allocation flags 
 */
#define VALUE_ALLOCATE		0x1

#define value_numeric(t)	(t >= byteint && t <= big)
#define value_str(t) 		(t >= string && t <= bytes)
#define value_ok(t) 		(t > undefined && t <= symbol)

#define VALUE_VALID			{ {0}, integer, 1 }
#define VALUE_INVALID		{ {0}, undefined, 0 }

/******************************************************************************/
/*
 *	A ring queue allows maximum utilization of memory for data storage and is
 *	ideal for input/output buffering. This module provides a highly effecient
 *	implementation and a vehicle for dynamic strings.
 *
 *	WARNING:  This is a public implementation and callers have full access to
 *	the queue structure and pointers.  Change this module very carefully.
 *
 *	This module follows the open/close model.
 *
 *	Operation of a ringq where rq is a pointer to a ringq :
 *
 *		rq->buflen contains the size of the buffer.
 *		rq->buf will point to the start of the buffer.
 *		rq->servp will point to the first (un-consumed) data byte.
 *		rq->endp will point to the next free location to which new data is added
 *		rq->endbuf will point to one past the end of the buffer.
 *
 *	Eg. If the ringq contains the data "abcdef", it might look like :
 *
 *	+-------------------------------------------------------------------+
 *  |   |   |   |   |   |   |   | a | b | c | d | e | f |   |   |   |   |
 *	+-------------------------------------------------------------------+
 *    ^                           ^                       ^               ^
 *    |                           |                       |               |
 *  rq->buf                    rq->servp               rq->endp      rq->enduf
 *     
 *	The queue is empty when servp == endp.  This means that the queue will hold
 *	at most rq->buflen -1 bytes.  It is the fillers responsibility to ensure
 *	the ringq is never filled such that servp == endp.
 *
 *	It is the fillers responsibility to "wrap" the endp back to point to
 *	rq->buf when the pointer steps past the end. Correspondingly it is the
 *	consumers responsibility to "wrap" the servp when it steps to rq->endbuf.
 *	The ringqPutc and ringqGetc routines will do this automatically.
 */

/*
 *	Ring queue buffer structure
 */
typedef struct {
	unsigned char	*buf;				/* Holding buffer for data */
	unsigned char	*servp;				/* Pointer to start of data */
	unsigned char	*endp;				/* Pointer to end of data */
	unsigned char	*endbuf;			/* Pointer to end of buffer */
	int				buflen;				/* Length of ring queue */
	int				maxsize;			/* Maximum size */
	int				increment;			/* Growth increment */
} ringq_t;

/*
 *	Block allocation (balloc) definitions
 */
#ifdef	B_STATS
#ifndef B_L
#define B_L				T(__FILE__), __LINE__
#define B_ARGS_DEC		char_t *file, int line
#define B_ARGS			file, line
#endif /* B_L */
#endif /* B_STATS */

/*
 *	Block classes are: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 
 *					   16384, 32768, 65536 
 */
typedef struct {
	union {
		void	*next;							/* Pointer to next in q */
		int		size;							/* Actual requested size */
	} u;
	int			flags;							/* Per block allocation flags */
} bType;

#define B_SHIFT			4					/* Convert size to class */
#define B_ROUND			((1 << (B_SHIFT)) - 1)
#define B_MAX_CLASS		13					/* Maximum class number + 1 */
#define B_MALLOCED		0x80000000			/* Block was malloced */
#define B_DEFAULT_MEM	(64 * 1024)			/* Default memory allocation */
#define B_MAX_FILES		(512)				/* Maximum number of files */
#define B_FILL_CHAR		(0x77)				/* Fill byte for buffers */
#define B_FILL_WORD		(0x77777777)		/* Fill word for buffers */
#define B_MAX_BLOCKS	(64 * 1024)			/* Maximum allocated blocks */

/*
 *	Flags. The integrity value is used as an arbitrary value to fill the flags.
 */
#define B_INTEGRITY			0x8124000		/* Integrity value */
#define B_INTEGRITY_MASK	0xFFFF000		/* Integrity mask */
#define B_USE_MALLOC		0x1				/* Okay to use malloc if required */
#define B_USER_BUF			0x2				/* User supplied buffer for mem */

/*
 *	The symbol table record for each symbol entry
 */

typedef struct sym_t {
	struct sym_t	*forw;					/* Pointer to next hash list */
	value_t			name;					/* Name of symbol */
	value_t			content;				/* Value of symbol */
	int				arg;					/* Parameter value */
} sym_t;

typedef int sym_fd_t;						/* Returned by symOpen */

/*
 *	Script engines
 */
#define EMF_SCRIPT_JSCRIPT			0		/* javascript */
#define EMF_SCRIPT_TCL	 			1		/* tcl */
#define EMF_SCRIPT_EJSCRIPT 		2		/* Ejscript */
#define EMF_SCRIPT_MAX	 			3

#define	MAXINT		INT_MAX
#define BITSPERBYTE 8
#define BITS(type)	(BITSPERBYTE * (int) sizeof(type))
#define	STRSPACE	T("\t \n\r\t")

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif /* max */

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif /* min */

/******************************************************************************/
/*                                  CRON                                      */
/******************************************************************************/

typedef struct {
	char_t	*minute;
	char_t	*hour;
	char_t	*day;
	char_t	*month;
	char_t	*dayofweek;
} cron_t;

extern long		cronUntil(cron_t *cp, int period, time_t testTime);
extern int		cronAlloc(cron_t *cp, char_t *str);
extern int		cronFree(cron_t *cp);

/******************************************************************************/
/*                                 SOCKET                                     */
/******************************************************************************/
/*
 *	Socket flags 
 */

#if ((defined (WIN) || defined (CE)) && defined (WEBS))
#define EWOULDBLOCK             WSAEWOULDBLOCK
#define ENETDOWN                WSAENETDOWN
#define ECONNRESET              WSAECONNRESET
#endif /* (WIN || CE) && WEBS) */

#define SOCKET_EOF				0x1		/* Seen end of file */
#define SOCKET_CONNECTING		0x2		/* Connect in progress */
#define SOCKET_BROADCAST		0x4		/* Broadcast mode */
#define SOCKET_PENDING			0x8		/* Message pending on this socket */
#define SOCKET_FLUSHING			0x10	/* Background flushing */
#define SOCKET_DATAGRAM			0x20	/* Use datagrams */
#define SOCKET_ASYNC			0x40	/* Use async connect */
#define SOCKET_BLOCK			0x80	/* Use blocking I/O */
#define SOCKET_LISTENING		0x100	/* Socket is server listener */
#define SOCKET_CLOSING			0x200	/* Socket is closing */
#define SOCKET_CONNRESET		0x400	/* Socket connection was reset */

#define SOCKET_PORT_MAX			0xffff	/* Max Port size */

/*
 *	Socket error values
 */
#define SOCKET_WOULDBLOCK		1		/* Socket would block on I/O */
#define SOCKET_RESET			2		/* Socket has been reset */
#define SOCKET_NETDOWN			3		/* Network is down */
#define SOCKET_AGAIN			4		/* Issue the request again */
#define SOCKET_INTR				5		/* Call was interrupted */
#define SOCKET_INVAL			6		/* Invalid */

/*
 *	Handler event masks
 */
#define SOCKET_READABLE			0x2		/* Make socket readable */ 
#define SOCKET_WRITABLE			0x4		/* Make socket writable */
#define SOCKET_EXCEPTION		0x8		/* Interested in exceptions */
#define EMF_SOCKET_MESSAGE		(WM_USER+13)

#ifdef LITTLEFOOT
#define SOCKET_BUFSIZ			510		/* Underlying buffer size */
#else
#define SOCKET_BUFSIZ			1024	/* Underlying buffer size */
#endif /* LITTLEFOOT */

typedef void 	(*socketHandler_t)(int sid, int mask, int data);
typedef int		(*socketAccept_t)(int sid, char *ipaddr, int port, 
					int listenSid);
typedef struct {
	char			host[64];				/* Host name */
	ringq_t			inBuf;					/* Input ring queue */
	ringq_t			outBuf;					/* Output ring queue */
	ringq_t			lineBuf;				/* Line ring queue */
	socketAccept_t	accept;					/* Accept handler */
	socketHandler_t	handler;				/* User I/O handler */
	int				handler_data;			/* User handler data */
	int				handlerMask;			/* Handler events of interest */
	int				sid;					/* Index into socket[] */
	int				port;					/* Port to listen on */
	int				flags;					/* Current state flags */
	int				sock;					/* Actual socket handle */
	int				fileHandle;				/* ID of the file handler */
	int				interestEvents;			/* Mask of events to watch for */
	int				currentEvents;			/* Mask of ready events (FD_xx) */
	int				selectEvents;			/* Events being selected */
	int				saveMask;				/* saved Mask for socketFlush */
	int				error;					/* Last error */
} socket_t;

/********************************* Prototypes *********************************/
/*
 *	Balloc module
 *
 */

extern void 	 bclose();
extern int 		 bopen(void *buf, int bufsize, int flags);

/*
 *	Define NO_BALLOC to turn off our balloc module altogether
 *		#define NO_BALLOC 1
 */

#ifdef NO_BALLOC
#define balloc(B_ARGS, num) malloc(num)
#define bfree(B_ARGS, p) free(p)
#define bfreeSafe(B_ARGS, p) \
	if (p) { free(p); } else
#define brealloc(B_ARGS, p, num) realloc(p, num)
extern char_t *bstrdupNoBalloc(char_t *s);
extern char *bstrdupANoBalloc(char *s);
#define bstrdup(B_ARGS, s) bstrdupNoBalloc(s)
#define bstrdupA(B_ARGS, s) bstrdupANoBalloc(s)
#define gstrdup(B_ARGS, s) bstrdupNoBalloc(s)

#else /* BALLOC */

#ifndef B_STATS
#define balloc(B_ARGS, num) balloc(num)
#define bfree(B_ARGS, p) bfree(p)
#define bfreeSafe(B_ARGS, p) bfreeSafe(p)
#define brealloc(B_ARGS, p, size) brealloc(p, size)
#define bstrdup(B_ARGS, p) bstrdup(p)

#ifdef UNICODE
#define bstrdupA(B_ARGS, p) bstrdupA(p)
#else /* UNICODE */
#define bstrdupA bstrdup
#endif /* UNICODE */

#endif /* B_STATS */

#define gstrdup	bstrdup
extern void		*balloc(B_ARGS_DEC, int size);
extern void		bfree(B_ARGS_DEC, void *mp);
extern void		bfreeSafe(B_ARGS_DEC, void *mp);
extern void		*brealloc(B_ARGS_DEC, void *buf, int newsize);
extern char_t	*bstrdup(B_ARGS_DEC, char_t *s);

#ifdef UNICODE
extern char *bstrdupA(B_ARGS_DEC, char *s);
#else /* UNICODE */
#define bstrdupA bstrdup
#endif /* UNICODE */
#endif /* BALLOC */

extern void bstats(int handle, void (*writefn)(int handle, char_t *fmt, ...));

/*
 *	Flags. The integrity value is used as an arbitrary value to fill the flags.
 */
#define B_USE_MALLOC		0x1				/* Okay to use malloc if required */
#define B_USER_BUF			0x2				/* User supplied buffer for mem */


#ifndef LINUX
extern char_t	*basename(char_t *name);
#endif /* !LINUX */

#if (defined (UEMF) && defined (WEBS))
/*
 *	The open source webserver uses a different callback/timer mechanism
 *	than other emf derivative products such as FieldUpgrader agents
 *	so redefine those API for webserver so that they can coexist in the
 *	same address space as the others.
 */
#define emfSchedCallback	websSchedCallBack
#define emfUnschedCallback	websUnschedCallBack
#define emfReschedCallback	websReschedCallBack
#endif /* UEMF && WEBS */

typedef void	(emfSchedProc)(void *data, int id);
extern int		emfSchedCallback(int delay, emfSchedProc *proc, void *arg);
extern void 	emfUnschedCallback(int id);
extern void 	emfReschedCallback(int id, int delay);
extern void		emfSchedProcess();
extern int		emfInstGet();
extern void		emfInstSet(int inst);
extern void		error(E_ARGS_DEC, int flags, char_t *fmt, ...);
extern void		(*errorSetHandler(void (*function)(int etype, char_t *msg))) \
					(int etype, char_t *msg);

#ifdef B_STATS
#define 		hAlloc(x) 				HALLOC(B_L, x)
#define			hAllocEntry(x, y, z)	HALLOCENTRY(B_L, x, y, z)
extern int		HALLOC(B_ARGS_DEC, void ***map);
extern int 		HALLOCENTRY(B_ARGS_DEC, void ***list, int *max, int size);
#else
extern int		hAlloc(void ***map);
extern int 		hAllocEntry(void ***list, int *max, int size);
#endif /* B_STATS */

extern int		hFree(void ***map, int handle);

extern int	 	ringqOpen(ringq_t *rq, int increment, int maxsize);
extern void 	ringqClose(ringq_t *rq);
extern int 		ringqLen(ringq_t *rq);

extern int 		ringqPutc(ringq_t *rq, char_t c);
extern int	 	ringqInsertc(ringq_t *rq, char_t c);
extern int	 	ringqPutStr(ringq_t *rq, char_t *str);
extern int 		ringqGetc(ringq_t *rq);

extern int		fmtValloc(char_t **s, int n, char_t *fmt, va_list arg);
extern int		fmtAlloc(char_t **s, int n, char_t *fmt, ...);
extern int		fmtStatic(char_t *s, int n, char_t *fmt, ...);

#ifdef UNICODE
extern int 		ringqPutcA(ringq_t *rq, char c);
extern int	 	ringqInsertcA(ringq_t *rq, char c);
extern int	 	ringqPutStrA(ringq_t *rq, char *str);
extern int 		ringqGetcA(ringq_t *rq);
#else
#define ringqPutcA ringqPutc
#define ringqInsertcA ringqInsertc
#define ringqPutStrA ringqPutStr
#define ringqGetcA ringqGetc
#endif /* UNICODE */

extern int 		ringqPutBlk(ringq_t *rq, unsigned char *buf, int len);
extern int 		ringqPutBlkMax(ringq_t *rq);
extern void 	ringqPutBlkAdj(ringq_t *rq, int size);
extern int 		ringqGetBlk(ringq_t *rq, unsigned char *buf, int len);
extern int 		ringqGetBlkMax(ringq_t *rq);
extern void 	ringqGetBlkAdj(ringq_t *rq, int size);
extern void 	ringqFlush(ringq_t *rq);
extern void 	ringqAddNull(ringq_t *rq);

extern int		scriptSetVar(int engine, char_t *var, char_t *value);
extern int		scriptEval(int engine, char_t *cmd, char_t **rslt, int chan);

extern void		socketClose();
extern void		socketCloseConnection(int sid);
extern void		socketCreateHandler(int sid, int mask, socketHandler_t 
					handler, int arg);
extern void		socketDeleteHandler(int sid);
extern int		socketEof(int sid);
extern int 		socketCanWrite(int sid);
extern void 	socketSetBufferSize(int sid, int in, int line, int out);
extern int		socketFlush(int sid);
extern int		socketGets(int sid, char_t **buf);
extern int		socketGetPort(int sid);
extern int		socketInputBuffered(int sid);
extern int		socketOpen();
extern int 		socketOpenConnection(char *host, int port, 
					socketAccept_t accept, int flags);
extern void 	socketProcess(int hid);
extern int		socketRead(int sid, char *buf, int len);
extern int 		socketReady(int hid);
extern int		socketWrite(int sid, char *buf, int len);
extern int		socketWriteString(int sid, char_t *buf);
extern int 		socketSelect(int hid, int timeout);
extern int 		socketGetHandle(int sid);
extern int 		socketSetBlock(int sid, int flags);
extern int 		socketGetBlock(int sid);
extern int 		socketAlloc(char *host, int port, socketAccept_t accept, 
					int flags);
extern void 	socketFree(int sid);
extern int		socketGetError();
extern socket_t *socketPtr(int sid);
extern int 		socketWaitForEvent(socket_t *sp, int events, int *errCode);
extern void 	socketRegisterInterest(socket_t *sp, int handlerMask);
extern int 		socketGetInput(int sid, char *buf, int toRead, int *errCode);

extern char_t	*webs_strlower(char_t *string);
extern char_t	*webs_strupper(char_t *string);

extern char_t	*webs_stritoa(int n, char_t *string, int width);

extern sym_fd_t	symOpen(int hash_size);
extern void		symClose(sym_fd_t sd);
extern sym_t	*symLookup(sym_fd_t sd, char_t *name);
extern sym_t	*symEnter(sym_fd_t sd, char_t *name, value_t v, int arg);
extern int		symDelete(sym_fd_t sd, char_t *name);
extern void 	symWalk(sym_fd_t sd, void (*fn)(sym_t *symp));
extern sym_t	*symFirst(sym_fd_t sd);
extern sym_t	*symNext(sym_fd_t sd);
extern int		symSubOpen();
extern void 	symSubClose();

extern void		trace(int lev, char_t *fmt, ...);
extern void		traceRaw(char_t *buf);
extern void		(*traceSetHandler(void (*function)(int level, char_t *buf))) 
					(int level, char_t *buf);
 
extern value_t 	valueInteger(long value);
extern value_t	valueString(char_t *value, int flags);
extern value_t	valueErrmsg(char_t *value);
extern void 	valueFree(value_t *v);
extern int		vxchdir(char *dirname);

extern unsigned int hextoi(char_t *hexstring);
extern unsigned int gstrtoi(char_t *s);
extern				time_t	timeMsec();

extern char_t 	*ascToUni(char_t *ubuf, char *str, int nBytes);
extern char 	*uniToAsc(char *buf, char_t *ustr, int nBytes);
extern char_t	*ballocAscToUni(char  *cp, int alen);
extern char		*ballocUniToAsc(char_t *unip, int ulen);

extern char_t	*basicGetHost();
extern char_t	*basicGetAddress();
extern char_t	*basicGetProduct();
extern void		basicSetHost(char_t *host);
extern void		basicSetAddress(char_t *addr);

extern int		harnessOpen(char_t **argv);
extern void		harnessClose(int status);
extern void		harnessTesting(char_t *msg, ...);
extern void		harnessPassed();
extern void		harnessFailed(int line);
extern int		harnessLevel();

#endif /* _h_UEMF */

/******************************************************************************/

