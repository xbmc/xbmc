///
///	@file 	mpr.h
/// @brief 	Header for the Mbedthis Portable Runtime (MPR)
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
//////////////////////////////// Documentation /////////////////////////////////
//
//	See mpr.dox for additional documentation.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _h_MPR
#define _h_MPR 1
/////////////////////////////////// Includes ///////////////////////////////////

#include "buildConfig.h"
#include "mprOs.h"

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// C API ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#else
#if !MACOSX
typedef int bool;
#endif
#endif

extern void mprBreakpoint(char *file, int line, char *msg);

#if BLD_FEATURE_ASSERT
#define mprAssert(C)  \
	if (C) ; else mprError(MPR_L, MPR_TRAP, "%s", #C)
#define inlineAssert(C) \
	if (C) ; else mprBreakpoint(MPR_L, #C)
#else
	#define mprAssert(C)		if (1) ; else
	#define inlineAssert(C)		if (1) ; else
#endif

/////////////////////////////////// Constants //////////////////////////////////

#if BLD_FEATURE_SQUEEZE
///
///	Maximum length of a file path name. Reduced from the system maximum to 
///	save memory space.
///
#define MPR_MAX_PATH			256 		///< Reasonable max path name
#else
#define MPR_MAX_PATH			1024
#endif

#if BLD_FEATURE_SQUEEZE
///
///	Reasonable length of a file name used by the product. Use where you know
///	the expected file name and it is certain to be less than this limit.
///
#define MPR_MAX_FNAME			128			///< Reasonable size of a file name
#define MPR_DEFAULT_ALLOC		64			// Default small alloc size 
#define MPR_DEFAULT_HASH_SIZE	23			// Default size of hash table index 
#define MPR_DEFAULT_STACK		(32 * 1024)	// Default stack size
#else
#define MPR_MAX_FNAME			256
#define MPR_DEFAULT_ALLOC		128			// Default small alloc size 
#define MPR_DEFAULT_HASH_SIZE	43			// Default size of hash table index 
#define MPR_DEFAULT_STACK		(64 * 1024)	// Default stack size
#endif

#if BLD_FEATURE_SQUEEZE
#define MPR_MAX_ARGC			32			// Reasonable max of args
#define MPR_MAX_STRING			512			// Maximum string size 
#define MPR_MAX_LOG_STRING		512			// Maximum log message
#define MPR_MAX_URL				256			///< Reasonable size of a URL
#define MPR_BUFSIZE				512			///< Reasonable size for buffers
//	FUTURE -- could be named better
#define MPR_MAX_HEAP_SIZE		(32 * 1024)	// Maximum heap allocation size
#else
#define MPR_MAX_ARGC			128			// Reasonable max of args
#define MPR_MAX_STRING			4096		///< Maximum string size 
#define MPR_MAX_LOG_STRING		8192		// Maximum log message
#define MPR_MAX_URL				1024		///< Reasonable size of a URL
#define MPR_BUFSIZE				1024		// Reasonable size for buffers
#define MPR_MAX_HEAP_SIZE		(64 * 1024)	// Maximum heap allocation size
#endif

#define MPR_DEFAULT_BREAK_PORT	9473
#define MPR_TIMEOUT_LOG_STAMP	3600000		// Time between log time stamps
#define MPR_TIMEOUT_PRUNER		60000		// Time between pruner runs
#define MPR_TIMEOUT_STOP_TASK	10000		// Timeout to stop running tasks
#define MPR_TIMEOUT_STOP_THREAD	10000		// Timeout to stop running threads
#define MPR_TIMEOUT_CMD_WAIT	50			// Poll for cmd processes
#define MPR_TIMEOUT_STOP		5000		// Wait when stopping resources
#define MPR_NUM_DIALOG			5			// Maximum number of user dialogs 
#define MPR_MAX_LOG_SIZE		5			// Default size of a log file (MB)

#if BLD_FEATURE_IPV6
#define MPR_MAX_IP_NAME			NI_MAXHOST  // Max size of an IP name
#define MPR_MAX_IP_ADDR			128  		// Max size of an IP string addr 
#define MPR_MAX_IP_PORT         6  			// Max size of a port number 
#define MPR_MAX_IP_ADDR_PORT	(MPR_MAX_IP_ADDR + NI_MAXSERV)  
											// Max size of IP with port
#else
#define MPR_MAX_IP_NAME			128			// Max size of an IP name
#define MPR_MAX_IP_ADDR			16			// Max size of an IP string addr
#define MPR_MAX_IP_PORT         6          	// Max size of a port number 
#define MPR_MAX_IP_ADDR_PORT	32			// Max size of IP with port
#endif

#define MPR_MAX_TIME_SYNC		(10 * 1000)	// Time sync adjustments

#define	MPR_TEST_TIMEOUT		10000		// Ten seconds 
#define MPR_TEST_LONG_TIMEOUT	300000		// 5 minutes 
#define MPR_TEST_SHORT_TIMEOUT	200			// 1/5 sec
#define	MPR_TEST_NAP			50			// When we must not block 

/*
 *	NOTE: max threads is a runtime maximum and may be modified by the program.
 */
#if BLD_FEATURE_MULTITHREAD
#define MPR_DEFAULT_MIN_THREADS	0			// Default min threads (0)
#define MPR_DEFAULT_MAX_THREADS	10			// Default max threads (10)
#if BLD_FEATURE_SQUEEZE
#define MPR_MAX_SPINLOCK		64			// Max spin locks
#else
#define MPR_MAX_SPINLOCK		256			// Max spin locks
#endif
#else
#define MPR_DEFAULT_MIN_THREADS	0			// Default min threads
#define MPR_DEFAULT_MAX_THREADS	0			// Default max threads 
#endif

//
//	MPR priorities (0 to 99)
//
#define MPR_BACKGROUND_PRIORITY	15			// May only get CPU if idle
#define MPR_LOW_PRIORITY		25
#define MPR_NORMAL_PRIORITY		50
#define MPR_HIGH_PRIORITY		75
#define MPR_CRITICAL_PRIORITY	99			// May not yield

#define MPR_SELECT_PRIORITY		75			// Run select at higher priority
#define MPR_POOL_PRIORITY		60			// Slightly elevated priority
#define MPR_REQUEST_PRIORITY	50			// Normal priority

//
//	Debug control
//
#define MPR_MAX_BLOCKED_LOCKS	100			// Max threads blocked on lock 
#define MPR_MAX_RECURSION		15			// Max recursion with one thread 
#define MPR_MAX_LOCKS			512			// Total lock count max 
#define MPR_MAX_LOCK_TIME		(60 * 1000)	// Time in millisecs to hold a lock 

//
//	Service / daemon control
//
#define MPR_INSTALL_SERVICE		1
#define MPR_UNINSTALL_SERVICE	2
#define MPR_GO_SERVICE			3
#define MPR_STOP_SERVICE		4

//
//	Parameter values for serviceEvents(loopOnce)
//
#define MPR_LOOP_ONCE			1
#define MPR_LOOP_FOREVER		0

//
//	Select service flags
//
#define MPR_ASYNC_SELECT		0x1		// Using async select in windows
#define MPR_BREAK_REQUESTED		0x2		// Breakout of a select wait
#define MPR_WAITING_FOR_SELECT	0x4		// Waiting for select to complete
#define MPR_SOCKET_LINGER		2000	// Default to linger for 2 sec on close

///////////////////////////////// Error Codes //////////////////////////////////

//
//	Standard MPR return and error codes 
//

///	Base error code
#define MPR_ERR_BASE					(-200) 				
///	General error
#define MPR_ERR_GENERAL					(MPR_ERR_BASE - 1)	
///	Action aborted
#define MPR_ERR_ABORTED					(MPR_ERR_BASE - 2)	
///	Item already exists
#define MPR_ERR_ALREADY_EXISTS			(MPR_ERR_BASE - 3)	
///	Bad arguments or paramaeters
#define MPR_ERR_BAD_ARGS				(MPR_ERR_BASE - 4)	
///	Bad input format
#define MPR_ERR_BAD_FORMAT				(MPR_ERR_BASE - 5)	
#define MPR_ERR_BAD_HANDLE				(MPR_ERR_BASE - 6)	
///	Module is in a bad state
#define MPR_ERR_BAD_STATE				(MPR_ERR_BASE - 7)	
///	Input has bad syntax
#define MPR_ERR_BAD_SYNTAX				(MPR_ERR_BASE - 8)	
#define MPR_ERR_BAD_TYPE				(MPR_ERR_BASE - 9)	
#define MPR_ERR_BAD_VALUE				(MPR_ERR_BASE - 10)	
#define MPR_ERR_BUSY					(MPR_ERR_BASE - 11)	
///	Cant access the file or resource
#define MPR_ERR_CANT_ACCESS				(MPR_ERR_BASE - 12)	
#define MPR_ERR_CANT_COMPLETE			(MPR_ERR_BASE - 13)	
///	Cant create the file or resource
#define MPR_ERR_CANT_CREATE				(MPR_ERR_BASE - 14)	
#define MPR_ERR_CANT_INITIALIZE			(MPR_ERR_BASE - 15)	
///	Cant open the file or resource
#define MPR_ERR_CANT_OPEN				(MPR_ERR_BASE - 16)	
///	Cant read from the file or resource
#define MPR_ERR_CANT_READ				(MPR_ERR_BASE - 17)	
///	Cant write to the file or resource
#define MPR_ERR_CANT_WRITE				(MPR_ERR_BASE - 18)	
#define MPR_ERR_DELETED					(MPR_ERR_BASE - 19)	
#define MPR_ERR_NETWORK					(MPR_ERR_BASE - 20)	
#define MPR_ERR_NOT_FOUND				(MPR_ERR_BASE - 21)	
///	Module or resource is not initialized
#define MPR_ERR_NOT_INITIALIZED			(MPR_ERR_BASE - 22)	
#define MPR_ERR_NOT_READY				(MPR_ERR_BASE - 23)	
#define MPR_ERR_READ_ONLY				(MPR_ERR_BASE - 24)	
///	The operation timed out
#define MPR_ERR_NOT_INITIALIZED			(MPR_ERR_BASE - 22)	
#define MPR_ERR_TIMEOUT					(MPR_ERR_BASE - 25)	
#define MPR_ERR_TOO_MANY				(MPR_ERR_BASE - 26)	
#define MPR_ERR_WONT_FIT				(MPR_ERR_BASE - 27)	
#define MPR_ERR_WOULD_BLOCK				(MPR_ERR_BASE - 28)	
#define MPR_ERR_CANT_ALLOCATE			(MPR_ERR_BASE - 29)	
#define MPR_ERR_MAX						(MPR_ERR_BASE - 30)	

//
//	Standard error severity and trace levels. These are ored with the error 
//	severities below. The MPR_LOG_MASK is used to extract the trace level 
//	from a flags word. We expect most apps to run with level 2 trace.
//
#define	MPR_FATAL		0		///< Fatal error trace level. Cant continue.
#define	MPR_ERROR		1		///< Hard error trace level
#define MPR_WARN		2		///< Soft warning trace level
#define	MPR_CONFIG		2		///< Configuration settings trace level.
#define MPR_INFO		3		///< Informational trace only 
#define MPR_DEBUG		4		///< Debug information trace level
#define MPR_VERBOSE		9		///< Highest level of trace 
#define MPR_LOG_MASK	0xf		///< Level mask 

//
//	Error flags. Specify where the error should be sent to. Note that the 
//	product.xml setting "headless" will modify how errors are reported.
//	Assert errors are trapped when in DEV mode. Otherwise ignored.
//
#define	MPR_TRAP		0x10	// Assert mprError flags -- trap in debugger 
#define	MPR_LOG			0x20	///< Log to the O/S event log mprError flag
#define	MPR_USER		0x40	///< Display to the user mprError flag
#define	MPR_ALERT		0x80	// Send a management alert 
#define	MPR_TRACE		0x100	// Trace

//
//	Error format flags
//
#define MPR_RAW			0x200			// Raw trace output

//
//	Error line number information
//
#define MPR_L		__FILE__, __LINE__

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Function Remappings ////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Unsafe functions that should not be used. Define UNSAFE_STRINGS_OK before
//	including mpr.h if you really want to use these functions. A better approach
//	is to undefine them just prior to using them in your C/C++ source file.
//
#if BLD_FEATURE_SAFE_STRINGS

#if BLD_FEATURE_PHP4_MODULE || BLD_FEATURE_PHP5_MODULE
#ifndef UNSAFE_FUNCTIONS_OK
#define UNSAFE_FUNCTIONS_OK 1
#endif
#endif

#ifndef UNSAFE_FUNCTIONS_OK
#define sprintf			UseMprSprintfInstead
#define printf			UseMprPrintfInstead
#define fprintf			UseMprFprintfInstead
#define vsprintf		UseMprVsprintfInstead
#define strtok			UseMprStrTokInstead
#define gethostbyname	UseMprGetHostByNameInstead
#define ctime			UseMprCtimeInstead
#define asctime			UseMprAsctimeInstead
#define localtime		UseMprLocaltimeInstead
#define gmtime			UseMprGmtimeInstead
#define malloc			UseMprMallocInstead
#define free			UseMprFreeInstead
#define realloc			UseMprReallocInstead
#define calloc			UseMprCallocInstead
#define strncpy			UseMprStrcpyInstead
#define inet_ntoa		UseMprInetNtoaInstead

//
//	FUTURE -- to the whole way
//
//#define strlen		UseMprStrlenInstead
//#define strcpy		UseMprStrcpyInstead

#endif	// UNSAFE_FUNCTIONS_OK
#endif	// BLD_FEATURE_SAFE_STRINGS

//
//	File information structure
//
struct MprFileInfo {
	uint			size;					// File length 
	uint64			mtime;					// Modified time 
	uint			inode;					// Inode number
	bool			isDir;					// Set if directory 
	bool			isReg;					// Set if a regular file 
};

//
//	Mpr time structure. Used for mprGetTime(). Matches requirements of select().
//
struct MprTime {
	long			sec;					// Seconds 
	long			usec;					// Microseconds (NOT milliseconds)
};
typedef struct MprTime MprTime;

#ifdef __cplusplus
} // extern "C"
#endif

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// C++ APIs ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if __cplusplus

class	MprBuf;
class	MprFile;
class	MprFileSystem;
class	MprHashTable;
class	MprLink;
class	MprList;
class	MprPoolService;
class	MprPoolThread;
class	MprSelectService;
class	MprSelectHandler;
class	MprSocket;
class	MprSocketService;
class	MprStringData;
class	MprStringList;
class	MprTask;
class	MprTimerService;
class	MprTimer;

#if BLD_FEATURE_LOG
class	MprLogModule;
class	MprLogService;
class	MprLogListener;
class	MprLogToFile;
class	MprLogToWindow;
#endif

#if BLD_FEATURE_CGI_MODULE
class	MprCmd;
class	MprCmdFiles;
class	MprCmdService;
#endif

#if BLD_FEATURE_MULTITHREAD
class	MprCond;
class	MprMutex;
class	MprThreadService;
class	MprThread;
#else
typedef void *MprMutex;
#endif

#if BLD_FEATURE_RUN_AS_SERVICE && WIN
class	MprWinService;
#endif

/////////////////////////////////// MprLink ////////////////////////////////////

//
//	This define is used by classes that include a Link field member, to get
//	the base class pointer
//
#define MPR_GET_BASE(type, ptr, field) \
	((type) ((int) ptr - ((int) &(((type) 1)->field) - (int) ((type) 1))))

//
//	The MprLink class enables subclassed objects to be inserted in a MprList. It
//	provides forward and back links for fast insertion, removal and iteration.
//	To use MprLink, subclasses must inherit MprLink as a base class. Use 
//	MprList for the dummy list header and MprLink for the list members.
//	This class is NOT thread-safe. Callers must do their own thread
//	synchronization. It is designed to be "inline", very fast and no-frills.
//

class MprLink {
  // FUTURE -- revert to protected
  public:
	MprLink			*next;						// Ptr to next member in list
	MprLink			*prev;						// Ptr to prev member in list
	MprList			*head;						// Ptr to the list head

  public:
					MprLink() { 				///< Constructor
						next = prev = this; 
						head = 0;
					};
					~MprLink() {};				///< Destructor
	MprList			*getList() { return head; };///< Return the owning list

	inline void		insertAfter(MprLink *item);	///< Insert after this member
	inline void		insertPrior(MprLink *item);	///< Insert prior to this member
};

/////////////////////////////////// MprList ////////////////////////////////////
//
//	The MprList class defines the list (dummy) header for doubly linked objects.
//	It provides forward and back links for fast insertion, removal and 
//	iteration. To use MprLink, subclasses must inherit MprLink as a base class. 
//	Use MprList for the dummy list header and MprLink for the list members.
//	This class is NOT thread-safe. Callers must do their own thread
//	synchronization. It is designed to be "inline", very fast and no-frills.
//

class MprList : public MprLink {
  protected:
	int				numItems;					// Number of items in the list

  public:
					MprList() { 
						numItems = 0; 
						head = this;
						next = prev = this;
					};
					~MprList() {
					};
	inline void	insert(MprLink *item) {			/// Add to the end of the list
						inlineAssert(item->head == 0);
						if (item->head == 0) {
							numItems++;
							item->head = this;
							item->next = head;
							item->prev = head->prev;
							prev->next = item;
							prev = item;
						}
					};
	inline MprLink	*remove(MprLink *item) {	/// Remove this item
						inlineAssert(item->head == this);
						item->next->prev = item->prev;
						item->prev->next = item->next;
						item->prev = 0;
						item->next = 0;
						inlineAssert(numItems > 0);
						if (item->head == head) {
							numItems--;
						}
						item->head = 0;
						return next;
					};
	inline MprLink	*getFirst() {				/// Get first item
						return (next == head) ? 0 : next;
					};
	inline MprLink	*getLast() {				/// Get last item
						return (next == head) ? 0 : prev;
					};
	inline MprLink	*getNext(MprLink *item) {	/// Get next item
						inlineAssert(item->head == this);
						return (item->next == head) ? 0 : item->next;
					};
	inline MprLink	*getPrev(MprLink *item) {	/// Get previous item
						inlineAssert(item->head == this);
						return (item->prev == head) ? 0 : item->prev;
					};
	int				getNumItems() { 			/// Get number of items.
						return numItems; 
					};
	bool			emptyOk() { 
						if ((head == this) && (next == head) && 
							(prev == head) && (numItems == 0)) {
							return 1;
						} else {
							return 0;
						}
					};
	
	friend class	MprLink;
};

//
//	Inline methods for MprLink
//
inline void	MprLink::insertAfter(MprLink *item) {
					inlineAssert(item->head == 0);
					item->head = head;
					next->prev = item;
					item->prev = this;
					item->next = next;
					next = item;
					head->numItems++;
				}
inline void	MprLink::insertPrior(MprLink *item) {
					inlineAssert(item->head == 0);
					item->head = head;
					prev->next = item;
					item->next = this;
					item->prev = prev;
					prev = item;
					head->numItems++;
				}

//////////////////////////////// MprStringList /////////////////////////////////

class MprStringData : public MprLink {
  public:
	MprStr			string;

  public:
					MprStringData(char *s);
					~MprStringData();
	inline char		*getValue() { return string; };
};

class MprStringList : public MprList {
  public:
					MprStringList(char *str);
					MprStringList();
					~MprStringList();
	void			insert(char *str);
	void			parse(char *str);
};

////////////////////////////////// MprCmdLine //////////////////////////////////

class MprCmdLine {
  private:
	int				argc;
	char			**argv;				// Not duped
	void			*argvBuf;			// Storage for argv and args
	bool			inSwitch;
	int				optc;
	int				optind;
	char			*switches;			// Not duped

  public:
					MprCmdLine(int argc, char **argv, char *switches);
					MprCmdLine(char *args, char *switches);
					~MprCmdLine();
	char			**getArgv() { return argv; };
	int				getArgc() { return argc; };
	int				next(char **argp = 0);
	int				firstArg();
};

/////////////////////////////////// MprCond ////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

//
//	Condition variable for multi-thread synchronization
//
//	Condition variables can be used to coordinate threads when running in a
//	multi-threaded mode. These condition variables are level triggered in that
//	a condition can be signalled prior to another thread waiting. That thread
//	will then not block if it calls waitForCond().
//

class MprCond {
  private:
	#if BLD_DEBUG
		//
		//	This must be the first item in the class
		//
		MprLink		link;				// Cond-var leak monitoring
	#endif
	#if CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD
		pthread_cond_t 
					cv;					// Pthreads condition variable 
	#endif
	#if WIN
		HANDLE		cv;					// Windows event handle
		int			numWaiting;			// Number waiting to be signalled
	#endif
	#if VXWORKS
		SEM_ID		cv;					// Condition variable
	#endif
		MprMutex	*mutex;				// Thread sync
		int			triggered;			// Value of the condition

#if UNUSED
		int			wakeAll;			// Wake all waiters on signalCond()
#endif

  public:
					MprCond();
					~MprCond();
		int			waitForCond(long timeout = -1);
		void		signalCond();

#if UNUSED
		int			multiWait(MprMutex *externalMutex, long timeout = -1);
		void		reset();
		void		signalAll();
#endif
};

#endif // BLD_FEATURE_MULTITHREAD
/////////////////////////////////// MprDebug ///////////////////////////////////
#if BLD_DEBUG

class MprDebug {
  public:
#if BLD_FEATURE_MULTITHREAD
	int				getMutexNum();
#endif 
};

#endif
/////////////////////////////////// MprFile ////////////////////////////////////

class MprFile {
  private:
	MprBuf			*inBuf;
	int				fd;
  public:
					MprFile();
	virtual			~MprFile();
	virtual int 	open(char* path, int flags, int mode);
	virtual void	close();
	virtual char	*gets(char *buf, int len);
	virtual int		read(void* buf, int len);
	virtual int		write(void* buf, int len);
	virtual long 	lseek(long offset, int origin);
};

//////////////////////////////// MprFileSystem /////////////////////////////////

class MprFileSystem {
  private:
  public:
					MprFileSystem();
	virtual			~MprFileSystem();
	virtual MprFile	*newFile();
	virtual int 	stat(char* path, MprFileInfo* info);
	virtual bool 	isDir(char* path);
	virtual void 	setRoot(char* path);
};

/////////////////////////////////// MprMutex ///////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

//
//	Mutual exclusion locks
//
class MprMutex {
  public:
	#if BLD_DEBUG
		MprLink		link;				// Must be the first in the class
	#endif
	#if WIN
		CRITICAL_SECTION cs;			// O/S critical section
	#endif
	#if CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD
		pthread_mutex_t	 cs;			// O/S critical section
	#endif
	#if VXWORKS
		SEM_ID		cs;					// Semaphore
	#endif

  public:
					MprMutex();
					~MprMutex();
	int				tryLock();

	//	FUTURE -- should do Windows inline also.

	#if CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD
		inline void	lock() { pthread_mutex_lock(&cs); };
		inline void	unlock() { pthread_mutex_unlock(&cs); };
	#elif VXWORKS && NOT_USED
		inline void	lock() { semTake(cs, WAIT_FOREVER); };
		inline void	unlock() { semGive(cs); };
	#else
		void		lock();
		void		unlock();
	#endif

	friend class	MprCond;
};

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////// Mpr /////////////////////////////////////
//
//	Mpr flags
//
#define MPR_EXITING				0x1		// Mpr is exiting
#define MPR_ASYNC_SELECTING		0x2		// Using async select
#define MPR_HEADLESS			0x4		// No user interface
#define MPR_SERVICE_THREAD		0x10	// Using a service thread for events
#define MPR_IS_DAEMON			0x20	// Is a daemon / service
#define MPR_STOPPED				0x40	// Mpr services stopped
#define MPR_STARTED				0x80	// Mpr services started
#define MPR_KILLABLE			0x100	// MPR can be killed (need a pid file)
#define MPR_USER_START_FLAGS	(MPR_SERVICE_THREAD | MPR_KILLABLE)

typedef int		(*MprDllEntryProc)(void *arg);

//
//	The Mbedthis Portable Runtime (MPR) internal state class.
//	
class Mpr {
  private:
	MprStr			appName;			// One word name of the application 
	MprStr			appTitle;			// Displayable name of the product 
	int				buildNumber;		// Build number 
	MprStr			buildType;			// Build type 
	MprHashTable	*configSettings;	// Configuration settings for app
	MprStr			cpu;				// Procesor 
	MprStr			domainName;			// Domain portion
	MprStr			hostName;			// Host name (fully qualified name)
	MprStr			installDir;			// Installation directory (!= cwd)
	MprStr			os;					// Operating system
	MprStr			serverName;			// Server name portion (no domain)
	MprStr			version;			// Application version number (x.y.z)
	int				ticksPerSec;		// Clock ticks per second

	//
	//	FUTURE -- Convert to flags
	//
	int				flags;				// Processing state
	int				headless;			// Run headless
	bool			runAsService;		// Running as a service (daemon)
	bool			eventsThread;		// Running an events thread

#if WIN
	long			appInstance;		// Application instance (windows)
	HWND			hwnd;				// Window handle
#endif

#if BLD_FEATURE_LOG
	MprLogModule	*defaultLog;		// Default log module
#endif

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;				// Thread synchronization 
	MprMutex		*timeMutex;			// Thread sync for mprGetTime 
	MprMutex		*eventsMutex;		// Thread sync for serviceEvents 
#endif

  public:
#if BLD_FEATURE_LOG
	MprLogService		*logService;	// Log service object
#endif
	MprPoolService		*poolService;	// Pool service object
	MprSelectService	*selectService;	// Select service object
	MprSocketService	*socketService;	// MprSocket service object
	MprTimerService		*timerService;	// Timer service object
	MprList				modules;		// List of modules

#if BLD_FEATURE_CGI_MODULE
	MprCmdService		*cmdService;	// Run command service object
#endif
#if BLD_FEATURE_MULTITHREAD
	MprThreadService	*threadService;	// Thread service object
#endif

  public:
	//
	//	Published API
	//
					Mpr(char *appName);
					~Mpr();
#if BLD_FEATURE_LOG
	void			addListener(MprLogListener *lp);
#endif
	bool			getAsyncSelectMode();
	int				getIdleTime();
	bool			isExiting(void);
	int				runTasks();
	int				runTimers();
	void			serviceEvents(bool loopOnce, int maxTimeout);
	void			setAsyncSelectMode(bool on);
	int				setLogSpec(char *spec);
	int				start(int startFlags = 0);
	int				stop(bool immediateStop);
	void			terminate(bool graceful = 1);
#if WIN
	void 			serviceWinEvents(bool loopOnce, int maxTimeout);
	void			setHwnd(HWND appHwnd);
	void			setSocketHwnd(HWND socketHwnd);
	void			setSocketMessage(int msgId);
#endif

	//
	//	Unpublished API
	//

	int				configure(char *configFile);
	char			*getAppName(void);
	char			*getAppTitle(void);
	int				getBuildNumber(void);
	char			*getBuildType(void);
	char			*getCpu(void);
	char			*getDomainName(void);
	int				getFds(fd_set *readInterest, fd_set *writeInterest, 
						fd_set *exceptInterest, int *maxFd, int *lastGet);
	int				getHeadless(void);
	char			*getHostName(void);
	char			*getInstallDir(void);
	char			*getOs(void);
	char			*getServerName(void);
	char			*getVersion(void);
	bool			isRunningEventsThread() { return eventsThread; };
	bool			isService();
	int				killMpr();
	int				makeDaemon(int parentExit);
	void			serviceIO(int readyFds, fd_set *readFds, fd_set *writeFds, 
						fd_set *exceptFds);
	void			serviceIO(int sockFd, int winMask);
	void			setAppName(char *name);
	void			setAppTitle(char *name);
	void			setBuildNumber(int buildNumber);
	void			setBuildType(char *type);
	void			setCpu(char *cpu);
	void			setDomainName(char *host);
	void			setHeadless(int headless);
	void			setHostName(char *host);
	void			setInstallDir(char *dir);
	void			setOs(char *os);
	void			setPriority(int pri);
	void			setRunningEventsThread(bool isRunning) { 
						eventsThread = isRunning; 
					}
	void			setServerName(char *host);
	void			setService(bool service);
	void			setVersion(char *version);
	void			setWebServerAddress(char *path);
	void			writeToOsLog(char *msg, int etype);

#if BLD_FEATURE_MULTITHREAD
	void			startEventsThread();
	void			setMinPoolThreads(int n);
	void			setMaxPoolThreads(int n);
	int				getMinPoolThreads();
	int				getMaxPoolThreads();
	int				getNextThreadNum();
	MprThread		*getCurrentThread();
	inline void	lock() { 
						if (mutex) {
							mutex->lock(); 
						}
					};
	inline void		unlock() { 
						if (mutex) {
							mutex->unlock(); 
						}
					};
	void			timeLock() { timeMutex->lock(); };
	void			timeUnlock() { timeMutex->unlock(); };
#else
	int				getMaxPoolThreads() { return 0; };
	inline void		lock() {};
	inline void		unlock() {};
	inline void		timeLock() {};
	inline void		timeUnlock() {};
#endif

#if WIN
	HWND			getHwnd();
	long			getInst();
	void			setInst(long inst);
#endif

#if BLD_FEATURE_DLL
	int				loadDll(char *path, char *fnName, void *arg, void **handle);
	void			unloadDll(void *handle);
#endif

#if BLD_FEATURE_XML_CONFIG
	int				openConfigFile(char *path);
	int				openXmlFile(char *path);
	int				readXmlInt(MprHashTable *symTab, char *key, int *value);
	int				readXmlStr(MprHashTable *symTab, char *key, char **value);
#endif
	char			*getConfigStr(char *key, char *defaultValue);
	int				getConfigInt(char *key, int defaultValue);

#if BLD_FEATURE_CGI_MODULE
	MprCmdService	*getCmdService() { return cmdService; };
#endif

  private:
	int				platformInitialize();
	int				platformTerminate();
	int				platformStart(int startFlags);
	int				platformStop();
};

extern Mpr			*mprGetMpr();

//////////////////////////////// MprHashEntry //////////////////////////////////
//
//	Master hash entry type. Not thread safe.
//

class MprHashEntry : public MprLink {
  private:
	MprStr			key;
	MprList			*bucket;

  public:
					MprHashEntry();
					MprHashEntry(char *key);
	virtual			~MprHashEntry();
	char			*getKey();
	void			setKey(char *key);
	friend class	MprHashTable;
};

//
//	String entry type
//
class MprStringHashEntry : public MprHashEntry {
  private:
	MprStr			value;
  public:
					MprStringHashEntry(char *key, char *str);
	virtual			~MprStringHashEntry();
	char			*getValue() { return value; };
};

//
//	Static string (not duplicated)
//
class MprStaticHashEntry : public MprHashEntry {
  private:
	char			*value;
  public:
					MprStaticHashEntry(char *key, char *str);
	virtual			~MprStaticHashEntry();
	char			*getValue() { return value; };
};

#if PERHAPS
//
//	Object string (not duplicated)
//
class MprObjectHashEntry : public MprHashEntry {
  private:
	char			*value;
  public:
					MprObjectHashEntry(char *key, char *str);
	virtual			~MprObjectHashEntry();
	char			*getValue() { return value; };
};
#endif

///////////////////////////////// MprHashTable /////////////////////////////////

class MprHashTable {
  private:
	MprList			*buckets;
	int				size;
	int				count;

  public:
					MprHashTable(int hash_size = MPR_DEFAULT_HASH_SIZE);
					~MprHashTable();
	MprHashEntry	*getFirst();
	MprHashEntry	*getNext(MprHashEntry *ep);
	int				getNumItems() { return count; };
	int				insert(MprHashEntry *entry);
	MprHashEntry	*lookup(char *key);
	int				remove(char *key);
	int				remove(MprHashEntry *entry);
	int				removeAll();

  private:
	MprHashEntry	*lookupInner(char *key, MprList **bucket);
	int				hashIndex(char *key);
	int				getNextPrime(int size);
};

//////////////////////////////// MprLogService /////////////////////////////////
#if BLD_FEATURE_LOG

class MprLogListener : public MprLink {
  private:
	int				maxSize;				// Max size of log
  public:
	//
	//	Published API
	//
					MprLogListener();		///< Constructor
	virtual			~MprLogListener();		///< Destructor
	virtual void	logEvent(char *module, int flags, int level, char *thread, 
		char *msg);
	virtual int	 	start();

	//
	//	Unpublished API
	//
	virtual void	shuttingDown();
	virtual int	 	setLogSpec(char *path, int maxSize);
	virtual void 	stop();
};


class MprLogToFile : public MprLogListener {
  private:
	int				logFd;				// MprLogService file handle 
	MprStr			logFileName;		// Current name of log file
	bool			timeStamps;			// Output high res time stamps
	uint			maxSize;			// Maximum extent of trace file 
	int				rotationCount;		// Number of times logs rotated
	MprTimer		*timer;				// Log file time stamp timer
  public:
					MprLogToFile();		///< Constructor
					~MprLogToFile();	///< Destructor
	void			logEvent(char *module, int flags, int level, char *thread, 
						char *msg);
	void			enableTimeStamps(bool on);
	void			logConfig();
	void			rotate();
	void			shuttingDown();
	int				setLogSpec(char *path, int maxSize);
	int				start();
	void			stop();
	void			writeTimeStamp();
};


class MprLogToWindow : public MprLogListener {
  private:
  public:
					MprLogToWindow();		///< Constructor
					~MprLogToWindow();		///< Destructor
	void			logEvent(char *module, int flags, int level, char *thread, 
						char *msg);
};


//
//	Overall logging service
//
class MprLogService {
  private:
	MprList			listeners;			// Users registered for output
	uint			defaultLevel;		// Default level for modules
	MprLogModule	*defaultModule;		// Default log module for this log
	MprList			moduleList;			// List of modules to trace 
	char			*moduleSpecs;		// Module level spec string
	char			*logSpec;			// Saved logging spec string
	bool			logging;			// Logging is enabled
#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;				// Mutex lock 
#endif

  public:
					MprLogService();
					~MprLogService(void);
	void			addListener(MprLogListener *lp);
	void			insertModule(MprLogModule *module);
	void			error(char *file, int line, int flags, char *fmt, 
						va_list args);
	MprLogModule	*getDefaultModule() { return defaultModule; }; 
	int				getDefaultLevel() { return defaultLevel; };
	char			*getLogSpec() { return logSpec; };
	char			*getModuleSpecs();
	bool			isLogging();
	void			removeListener(MprLogListener *lp);
	void			removeModule(MprLogModule *module);
	void			setDefaultLevel(int l);
	void			setDefaultModule(MprLogModule *m) { defaultModule = m; };
	int				setLogSpec(char *fileSpec);
	void			shuttingDown();
	void			start();
	void			stop();
	void			traceCore(int level, int flags, MprLogModule *module, 
						char *fmt, va_list ap);
	void			writeHeader();
	void			writeLogStamp();

#if BLD_FEATURE_MULTITHREAD
	void			lock() { 
						if (mutex) {
							mutex->lock(); 
						}
					};
	void			unlock() { 
						if (mutex) {
							mutex->unlock(); 
						}
					};
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

  private:
	void			output(MprLogModule *module, int flags, int level, 
						char *msg);
	void			breakpoint(char *file, int line);
};

extern void		mprLog(int level, MprLogModule *module, char *fmt, ...);
extern void		mprLog(int level, int flags, MprLogModule *module, 
					char *fmt, ...);
extern void		mprLog(char *fmt, ...);

#else // !BLD_FEATURE_LOG
//
//	If logging is not enabled, we inline these functions to nothing
//
class MprLogModule { 
	void *x; 
  public:
	MprLogModule(char *name) {}
};
inline void		mprLog(int level, MprLogModule *module, char *fmt, ...) {};
inline void		mprLog(int level, int flags, MprLogModule *module, 
					char *fmt, ...) {};
inline void		mprLog(char *fmt, ...) {}
extern "C" void		mprLog(int level, char *fmt, ...);
#endif

///////////////////////////////// MprLogModule /////////////////////////////////
#if BLD_FEATURE_LOG
//
//	Class to describe a trace log module
//

class MprLogModule : public MprLink {
  private:
	MprStr			name;
	int				level;				// Current trace output level 
	bool			enabled;

  public:
					MprLogModule(char *name);
					~MprLogModule();
	void			innerMprLogModule(char *name);
	int				getLevel(void) { return level; };
	void			disable() { enabled = 0; };
	void			enable() { enabled = 1; };
	int				getEnabled() { return enabled; };
	char			*getName() { return name; };
	void			setLevel(int l) { level = l; };
};

#endif // BLD_FEATURE_LOG
//////////////////////////////////// MprBuf ////////////////////////////////////

typedef int			(*MprBufProc)(MprBuf* bp, void *arg);

class MprBuf {
  private:
	uchar			*buf;				// Actual buffer for data 
	uchar			*endbuf;			// Pointer one past the end of buffer 
	uchar			*start;				// Pointer to next data char 
	uchar			*end;				// Pointer one past the last data char 
	int				buflen;				// Current size of buffer 
	int				maxsize;			// Max size the buffer can ever grow 
	int				growBy;				// Next growth increment to use 
	MprBufProc		refillProc;			// Function to refill the buffer
	void			*refillArg;			// Arg to refill proc

  public:
					MprBuf();
					MprBuf(int initialSize, int maxsize = -1);
					~MprBuf();
	inline void		addNull() {
						*((char*) end) = (char) '\0';
					};
	void			adjustStart(int size);
	void			adjustEnd(int size);
	void			copyDown();
	inline void		flush() {
						start = buf;
						end = buf;
					};
	inline int		get() {
						if (start == end) {
							return -1;
						}
						int	c = (uchar) *start++;
						if (start >= endbuf) {
							start = buf;
						}
						return c;
					};
	int				get(uchar *buf, int len);
	inline char		*getBuf() { return (char*) buf; };
	inline char		*getEnd() { return (char*) end; };
	inline char		*getEndBuf() { return (char*) endbuf; };
	inline int		getLength() {
						return ((start > end) ? 
							(buflen + (end - start)) : (end - start));
					};
	inline int		getLinearData() {
						return min((endbuf - start), getLength());
					}
	inline int		getLinearSpace() {
						int len = getLength();
						int space = buflen - len - 1;
						return min((endbuf - end), space);
					}
	inline MprBufProc getRefillProc() {
						return refillProc;
					};
	inline int		getSpace() {
						return buflen - getLength() - 1;
					};
	inline char		*getStart() { return (char*) start; };
	inline int		getSize() { return buflen; };
	int				insert(char c);
	inline int		look() {
						if (start == end) {
							return -1;
						}
						return* start;
					}
	int				lookLast();
	int				put(char c);
	int				put(char *str);
	int				putInt(int i);
	int				putFmt(char *fmt, ...);
	int				put(uchar *buf, int len);
	inline int		refill() { 
						return (refillProc) ? 
							(refillProc)(this, refillArg) : 0; 
					};
	inline void		resetIfEmpty() {
						if (getLength() == 0) {
							flush();
						}
					};
	void			setBuf(uchar *userBuf, int size);
	void			setBuf(int initialSize, int maxSize);
	void			setRefillProc(MprBufProc fn, void *arg) { 
						refillProc = fn; 
						refillArg = arg; 
					};
	uchar			*takeBuffer();
  private:
	int				grow();
};

//////////////////////////////// MprCmdService /////////////////////////////////
#if BLD_FEATURE_CGI_MODULE
//
//	Flags for MprCmd
//
#define MPR_CMD_BACKGROUND	0x1			// Continue running if MPR exits 
#define MPR_CMD_REAP_MAX	24

/*
 * 	Child status structure. Designed to be async-thread safe.
 */
typedef struct MprCmdChild {
	ulong			pid;				//	Process ID
	int				exitStatus;			//	Exit status
} MprCmdChild;


//
//	Cmd service control
//

class MprCmdService {
  private:
	MprList			cmdList;			// List of commands
	MprMutex		*mutex;				// Multi-thread sync
	MprTimer		*timer;				// Timer to poll for child completion

	/*
	 * 	Dual-list of pids and their exit status. Must be async-thread-safe
	 * 	for signal level access.
	 */
	MprCmdChild		children[MPR_CMD_REAP_MAX];

  public:
					MprCmdService();
					~MprCmdService();
	void			cmdWatcher();
	void			cmdWatcherTimer(MprTimer *tp);
	void			insertCmd(MprCmd* rp);
	void			removeCmd(MprCmd* rp);
	int				start();
	void			startWatcher();
	int				stop();

#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD
	void			processStatus(ulong pid, int status);
	void			initSignals();
#endif

#if BLD_FEATURE_MULTITHREAD
	void			lock() { mutex->lock(); };
	void			unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif
};

//////////////////////////////////// MprCmd ////////////////////////////////////

//
//	Cmd procs must return the number of bytes read or -1 for errors.
//
typedef void	(*MprCmdProc)(MprCmd* rp, void *data);

#define MPR_CMD_EOF			-1			// EOF return value

//
//	Flags
//
#define MPR_CMD_DETACHED	0x1
#define MPR_CMD_NEW_SESSION	0x2
#define MPR_CMD_CHDIR		0x4
#define MPR_CMD_WAIT		0x8
#define MPR_CMD_SHOW		0x10
#define MPR_CMD_USER_FLAGS	0x1f

#define MPR_CMD_COMPLETE	0x40
#define MPR_CMD_DISPOSED	0x80
#define MPR_CMD_STDIO_MADE	0x100

//
//	Indicies for clientFd and serverFd
//
#define MPR_CMD_IN			0			// Stdin for the server
#define MPR_CMD_OUT			1			// Stdout for the server
#define MPR_CMD_MAX_FD		2

#define MPR_CMD_WATCHER_NAP		100		// Timeout for watcher while polling
#define MPR_CMD_WATCHER_TIMEOUT	5000	// Timeout for child to start

class MprCmdFiles {
  public:
	MprStr			name[MPR_CMD_MAX_FD];
	int				fd[MPR_CMD_MAX_FD];
	int				clientFd[MPR_CMD_MAX_FD];
  public:
					MprCmdFiles();
					~MprCmdFiles();
};

//
//	MprCmd class
//
class MprCmd : public MprLink {
  private:
	MprStr			cwd;				// Current working dir for the process
	void			*data;				// Callback user data
	int				exitStatus;			// Command exit status
	int				flags;				// Control flags (userFlags not here)
	MprCmdFiles		files;				// Stdin, stdout for the command
	int				inUse;				// Used by dispose()
	int				reapIndex;			// Index into MprCmdService->children[]
	MprLogModule	*log;
	MprCmdProc		cmdDoneProc;		// Handler for client completion

#if VXWORKS
	MprSelectHandler	
					*handler;
	int				waitFd;				// Pipe to await child exit
#endif
	ulong			process;			// Id/handle of the created process

#if BLD_FEATURE_MULTITHREAD
#if VXWORKS
	MprCond			*startCond;
	MprCond			*exitCond;
#endif
	MprMutex		*mutex;
#endif

  public:
					MprCmd();
					~MprCmd();
	void			closeWriteFd();
	int				dispose();
	int				getExitCode(int *code = 0);
	int				getWriteFd();
	int				getFlags() { return flags; };
	int				getReadFd();
	void			invokeCallback(MprMutex *callerMutex);
	bool			isRunning();
	int				makeStdio();
	void			setCwd(char *dir);
	int				setInuse();
	int				start(char *cmd, int flags);
	int				start(char *cmd, char **argv, char **envp, 
						MprCmdProc completionProc, void *data, int flags);
	void			stop();

	//
	//	Internal API
	//
	ulong			getProcess() { return process; };
	void			reset();
	void			resetFiles();
	void			setExitStatus(int status);

  private:
	int				waitForChild(int timeout);

#if BLD_FEATURE_MULTITHREAD
	void			lock() { mutex->lock(); };
	void			unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

	friend class	MprCmdService;
};

#endif // BLD_FEATURE_CGI_MODULE
////////////////////////////// MprSelectService ////////////////////////////////
//
//	Standard select bit mask options
//
#define MPR_READABLE			0x2
#define MPR_WRITEABLE			0x4
#define MPR_EXCEPTION			0x8

typedef void		(*MprSelectProc)(void *data, int mask, int isMprPoolThread);

class MprSelectService {
  private:
	struct sockaddr_in 
					breakAddress;		// Breakout port socket address
	int				breakSock;			// MprSocket to wakeup select service
	int				breakPort;			// Port to talk to the select service
	int				breakRetries;		// Retry attempts to open breakout port
	MprList			list;				// List of select handlers
	int				flags;				// State flags
	int				maskGeneration;		// Generation number for mask changes
	int				listGeneration;		// Generation number for list changes
	MprLogModule	*log;
	int				rebuildMasks;		// Complete select mask rebuild required
	int				delayedFds[FD_SETSIZE];
	int				maxDelayedFd;
#if WIN
	int				sockMessage;		// MprSocket message for AsyncSelect
	HWND			hwnd;				// Window handle to use for AsyncSelect
#endif

#if BLD_FEATURE_MULTITHREAD
	MprCond			*cond;				// Wait for select to breakout
	MprMutex		*mutex;				// General multi-thread synchronization
#endif

  public:
	//
	//	Published API
	//
					MprSelectService();
					~MprSelectService();
	int				doSelect(int maxFds, fd_set *readInterest, 
						fd_set *writeInterest, fd_set *exceptInterest, 
						int timeout);
	int				getFds(fd_set *readInterest, fd_set *writeInterest, 
						fd_set *exceptInterest, int *maxFd, int *lastGet);
	void			repairFds();
	void			serviceIO(int readyFds, fd_set *readFds, fd_set *writeFds, 
						fd_set *exceptFds);
	void			serviceIO(int sockFd, int winMask);

	//
	//	Unpublished API
	//
	int				insertHandler(MprSelectHandler *sp);
	void			awaken(int wait = 0);
	void			delayedClose(int fd);
	bool			getAsyncSelectMode();
	int				getFlags() { return flags; };
	int				modifyHandler(MprSelectHandler *sp, bool wakeUp);
	void			removeHandler(MprSelectHandler *sp);
	MprLogModule	*getLog() { return log; };
	int				openBreakoutPort();
	void			setPort(int n);
	int				start();
	int				stop();

#if BLD_FEATURE_MULTITHREAD
	void			lock() { mutex->lock(); };
	void			unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

#if WIN
	HWND			getHwnd() { return hwnd; };
	int				getMessage() { return sockMessage; };
	void			setHwnd(HWND h) { hwnd = (HWND) h; };
	void			setMessage(int m) { sockMessage = m; };
	void			setAsyncSelectMode(bool asyncSelect);
#endif
};

////////////////////////////// MprSelectHandler ////////////////////////////////
//
//	Flags
//
#define MPR_SELECT_DISPOSED			0x1
#define MPR_SELECT_RUNNING			0x2
#define MPR_SELECT_CLOSEFD			0x4
#define MPR_SELECT_CLIENT_CLOSED	0x8		// Client disconnection received
#define MPR_SELECT_FAILED			0x10	// Bad file handle

class MprSelectHandler : public MprLink {
  private:
	int				desiredMask;		// Mask of desired events 
	int				disableMask;		// Mask of disabled events 
	int				fd;					// O/S File descriptor (sp->sock) 
	int				flags;				// Control flags
	void			*handlerData;		// Argument to pass to proc. 
	int				inUse;				// Used by dispose()
	MprLogModule	*log;
	int				presentMask;		// Mask of events that have been seen 
	int				priority;			// Priority if events handled by threads

#if BLD_FEATURE_MULTITHREAD
	MprCond			*stoppingCond;		// Synchronization when stopping
	MprTask			*serviceTask;		// Task to handle IO event
#endif

  public:
	MprSelectService *selectService;	// Select service pointer
	MprSelectProc	proc;				// Select handler procedure 

  public:
					MprSelectHandler(int fd, int mask, MprSelectProc proc, 
						void *data, int priority);
	bool			dispose();
	void			disableEvents(bool wakeUp);
	void			enableEvents(bool wakeUp);
	int				getFd() { return fd; };
	int				getFlags() { return flags; };
	void			runHandler();
	void			selectProc(MprTask *tp);
	void			setInterest(int mask);
	void			setWinInterest();
	void			setProc(MprSelectProc newProc, int mask);
	void			setCloseOnDispose();
	int				stop(int timeout);

  private:
					~MprSelectHandler();

	friend class	MprSelectService;
};

//////////////////////////////// MprInterface //////////////////////////////////

class MprInterface : public MprLink {
  public:
	MprStr			name;				// Interface name
	MprStr			ipAddr;
	MprStr			broadcast;
	MprStr			mask;
  public:
					MprInterface(char *name, char *ipAddr, char *bcast, 
						char *mask);
					~MprInterface();
};

////////////////////////////// MprSocketService ////////////////////////////////

typedef void		(*MprSocketIoProc)(void *data, MprSocket *sp, int mask, 
						int isMprPoolThread);
typedef void		(*MprSocketAcceptProc)(void *data, MprSocket *sp, char *ip, 
						int port, MprSocket *lp, int isMprPoolThread);
//
//	Mpr socket service class
//
class MprSocketService {
  private:
	MprList			socketList;			// List of all sockets
	MprList			ipList;				// List of ip addresses 
	MprLogModule	*log;
	int				maxClients;

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;
#endif

  public:
					MprSocketService();
					~MprSocketService();
#if BLD_FEATURE_LOG
	MprLogModule	*getLogModule() { return log; };
#endif
#if DEPRECATED
	MprList			*getInterfaceList();
#endif
	int				canAcceptClient(int *numClients);
	int				getMaxClients() { return maxClients; };
	int				getNumClients();
	void			insertMprSocket(MprSocket *sp);
	void			removeMprSocket(MprSocket *sp);
	void			setMaxClients(int max) { maxClients = max; };
	int				start();
	int				stop();

#if BLD_FEATURE_MULTITHREAD
	void			lock() { mutex->lock(); };
	void			unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

  private:
#if DEPRECATED
	int				getInterfaces();
	int				getInterfacesIPv4();
#if BLD_FEATURE_IPV6
	int				getInterfacesIPv6();
	int				getInterfacesLinuxIPv6();
#endif
    char            *genIPv6Netmask(int prefix_len);
#endif
    
};

////////////////////////////////// MprSocket ///////////////////////////////////
//
//	close flags
//
#define MPR_SHUTDOWN_READ		0
#define MPR_SHUTDOWN_WRITE		1
#define MPR_SHUTDOWN_BOTH		2

//
//	Flags
//
#define MPR_SOCKET_BLOCK		0x1			// Use blocking I/O 
#define MPR_SOCKET_BROADCAST	0x2			// Broadcast mode 
#define MPR_SOCKET_CLOSED		0x4			// MprSocket has been closed 
#define MPR_SOCKET_CONNECTING	0x8			// MprSocket has been closed 
#define MPR_SOCKET_DATAGRAM		0x10		// Use datagrams 
#define MPR_SOCKET_EOF			0x20		// Seen end of file 
#define MPR_SOCKET_LISTENER		0x40		// MprSocket is server listener 
#define MPR_SOCKET_NOREUSE		0x80		// Dont set SO_REUSEADDR option  
#define MPR_SOCKET_NODELAY		0x100		// Disable Nagle algorithm 
#define MPR_SOCKET_DISPOSED		0x200		// Delete requested

#define MPR_SOCKET_READABLE		0x2
#define MPR_SOCKET_WRITABLE		0x4
#define MPR_SOCKET_EXCEPTION	0x8

class MprSocket : public MprLink 
{
  private:
	MprSocketAcceptProc 
					acceptCallback;		// Accept callback 
	void			*acceptData;		// User accept callback data 
	int				currentEvents;		// Mask of ready events (FD_x) 
	int				error;				// Last error 
	MprSelectHandler *handler;			// Select handler
	int				handlerMask;		// Handler events of interest 
	int				handlerPriority;	// Handler priority 
	int				interestEvents;		// Mask of events to watch for 
	MprSocketIoProc	ioCallback;			// User I/O callback 
	void			*ioData;			// User io callback data 
	void			*ioData2;			// Secondary user io callback data 
	int				inUse;				// In use counter. Used by dispose()
	MprStr			ipAddr;				// Host IP address
	MprLogModule	*log;				// Pointer to MprSocketService module
	int				port;				// Port to listen on 
	int				selectEvents;		// Events being selected 

#if BLD_FEATURE_IPV6
	int				ipv6;				// Socket uses IPv6
#endif

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;				// Multi-thread sync
#endif

  protected:
	int				sock;				// Actual socket handle 
	int				flags;				// Current state flags 

	//	TODO - not used
	bool			secure;				// MprSocket is using SSL

  public:
					MprSocket();
	void			forcedClose();
	void			acceptProc(int isMprPoolThread);
	bool			getEof();
	int				getError();
	int				getFlags();
	char			*getIpAddr() { return ipAddr; };
	int				getPort();
	int				getFd();
	bool			getBlockingMode();
	void			getAcceptCallback(MprSocketAcceptProc *fn, void **data);
	void			getCallback(MprSocketIoProc *fn, void **data, 
						void **data2, int *mask);
	//	TODO - not used
	bool			isSecure() { return secure; };
	int				openServer(char *ipAddr, int portNum, 
						MprSocketAcceptProc acceptFn, void *data, int flags);
	int				openClient(char *hostName, int portNum, int flags);
	void			setBlockingMode(bool on);
	int				setBufSize(int sendSize, int recvSize);
	//	FUTURE -- rename: handler vs callback
	void			setCallback(MprSocketIoProc fn, void *data, 
						void *data2, int mask, int pri = MPR_NORMAL_PRIORITY);
	int				write(char *s);

	virtual 		~MprSocket();
	virtual void	close(int timeout);
	virtual bool	dispose();
	virtual void	ioProc(int mask, int isMprPoolThread);
	virtual MprSocket	
					*newSocket();
	virtual int		read(char *buf, int len);
	virtual int		write(char *buf, int len);

#if BLD_FEATURE_MULTITHREAD
	void			lock() { mutex->lock(); };
	void			unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

  private:
	void			setMask(int handlerMask);
	void			setNoDelay(bool on);

	friend class	MprSocketService;
};

//////////////////////////////// MprPoolService ////////////////////////////////

#if BLD_DEBUG
class MprPoolStats {
  public:
	int				maxThreads;			// Configured max number of threads
	int				minThreads;			// Configured minimum
	int				numThreads;			// Configured minimum
	int				maxUse;				// Max used 
	int				pruneHighWater;		// Peak thread use in last minute
	int				idleThreads;		// Current idle
	int				busyThreads;		// Current busy
};
#endif

//
//	A task queue consists of a list of tasks and optional list of threads
//
typedef void		(*MprTaskProc)(void *data, MprTask *tp);

//
//	Class for the overall thread pool service
//
class MprPoolService {
  protected:							// Allow MprPoolThreads to access
	MprStr			name;				// Name of pool
	int				nextTaskNum;		// Unique next task number
	MprList			runningTasks;		// List of executing tasks
	int				stackSize;			// Stack size for worker threads 
	MprList			tasks;				// Prioritized list of pending tasks

#if BLD_FEATURE_MULTITHREAD
	MprList			busyThreads;		// List of threads to service tasks
	MprList			idleThreads;		// List of threads to service tasks
	int				maxThreads;			// Max # threads in pool 
	int				maxUseThreads;		// Max threads ever used
	int				minThreads;			// Max # threads in pool 
	MprMutex		*mutex;				// Per task synchronization
	int				nextThreadNum;		// Unique next thread number
	int				numThreads;			// Current number of threads in pool
	int				pruneHighWater;		// Peak thread use in last minute
	MprTimer		*pruneTimer;		// Timer for excess threads pruner
	MprMutex		*incMutex;			// Per task synchronization
#endif

  public:
	MprLogModule	*log;

  public:
					MprPoolService(char *name);
					~MprPoolService();
	int				assignNextTask(MprPoolThread *pt);
	void			dequeueTask(MprTask *tp);
#if BLD_DEBUG
	void			getStats(MprPoolStats *ps);
#endif
	void			insertTask(MprTask *np, MprTask *tp);
	void			prune();
	void			queueTask(MprTask *tp);
	void			queueRunningTask(MprTask *tp);
	void			removeTask(MprTask *tp);
	int				runTasks();
	void			setStackSize(int n);
	int				start();
	int				stop(int timeout);

#if BLD_FEATURE_MULTITHREAD
	void			dispatchTasks();
	int				getMinPoolThreads() { return minThreads; };
	int				getNumPoolThreads() { 
						return idleThreads.getNumItems() +
								busyThreads.getNumItems();
					};
	int				getMaxPoolThreads() { return maxThreads; };
	int				getNumIdleThreads() { return idleThreads.getNumItems(); };
	void			lock();
	int				getNextThreadNum();
	void			removeThread(MprPoolThread *pt);
	void			setMinPoolThreads(int n);
	void			setMaxPoolThreads(int n);
	void			unlock();
#else
	inline void		lock() {};
	inline void		unlock() {};
	int				getMaxPoolThreads() { return 0; };
#endif

	friend class	MprPoolThread;
	friend class	MprTask;
};

///////////////////////////////// MprPoolThread ////////////////////////////////
#if BLD_FEATURE_MULTITHREAD
//
//	Flags
//
#define MPR_POOL_THREAD_SLEEPING	0x1

//
//	Class for each thread in the thread pool
//
class MprPoolThread : public MprLink {
  private:
	MprPoolService	*pool;				// Which thread pool do we swim in
	MprTask			*currentTask;		// Current task being run
	int				flags;				

#if BLD_FEATURE_MULTITHREAD
	MprThread		*thread;			// Associated thread
	MprCond			*idleCond;			// Used to wait for work
#endif

  public:
					MprPoolThread(MprPoolService *pool, int stackSize);
					~MprPoolThread();
	MprPoolService	*getMprPoolService() { return pool; };
	MprThread		*getThread() { return thread; };
	MprTask			*getCurrentTask() { return currentTask; };
	void			makeIdle();
	void			start();
	void			setTask(MprTask *tp);
	void			threadMain();
	void			wakeupThread();
};

#endif
/////////////////////////////////// MprTask ////////////////////////////////////
//
//	Flags
//
#define MPR_TASK_DISPOSED	0x1
#define MPR_TASK_RUNNING	0x2

//
//	Class for each task (unit of work) 
//
class MprTask : public MprLink {
  public:
	void			*data;				// Task data 
	int				flags;				// Control flags
	int				inUse;				// In use counter. Used by dispose()
	MprPoolService	*pool;				// Managing pool
	int				priority;			// Priority of event 
	MprTaskProc		proc;				// Procedure to service this event. 

#if BLD_FEATURE_MULTITHREAD
	MprPoolThread	*pt;				// Pool thread servicing this task
	MprCond			*stoppingCond;		// Synchronization for timer->dispose()
#endif

  public:
					MprTask(MprTaskProc proc, void *data, 
						int priority = MPR_NORMAL_PRIORITY);
					MprTask(MprPoolService *pool, MprTaskProc proc, 
						void *data, int priority = MPR_NORMAL_PRIORITY);
	bool			dispose();
	void			start();
	int				stop(int timeout);

#if BLD_FEATURE_MULTITHREAD
	MprPoolThread	*getThread() { return pt; }; 
#endif

  private:
					~MprTask();
	friend class	MprPoolThread;
	friend class	MprPoolService;
};

/////////////////////////////// MprThreadService ///////////////////////////////
#if BLD_FEATURE_MULTITHREAD
// 
//	Threading primitives
//
typedef void		(*MprThreadProc)(void *arg, MprThread *tp);

class MprThreadService {
  private:
	MprList			threads;			// List of all threads
	MprThread		*mainThread;		// Main application Mpr thread id 
	MprMutex		*mutex;				// Multi-thread sync

  public:
					MprThreadService();
					~MprThreadService();
	MprThread		*getCurrentThread();
	int				getCurrentThreadId();
	void			insertThread(MprThread *tp);
	void			removeThread(MprThread *tp);
	int				start();
	int				stop(int timeout);

	inline void	lock() { 
						if (mutex) {
							mutex->lock(); 
						}
					};
	inline void		unlock() { 
						if (mutex) {
							mutex->unlock(); 
						}
					};
};

/////////////////////////////////// MprThread //////////////////////////////////
/*
 *	Thread ID type
 */
#if CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD
	typedef pthread_t	MprOsThread;
#elif X86_64
	typedef int64		MprOsThread;
#else
	typedef int			MprOsThread;
#endif


class MprThread : public MprLink {
  private:
	MprOsThread		osThreadId;			// O/S thread id

	#if WIN
		handle		threadHandle;		// Threads OS handle 
	#endif

	void			*data;				// Data argument
	MprThreadProc	entry;				// Users thread entry point
	MprStr			name;				// Name of thead for trace
	MprMutex		*mutex;				// Multi-thread synchronization
	int				pid;				// Owning process id
	int				priority;			// Current priority 
	int				stackSize;			// Only VxWorks implements
	int				id;					// Mpr thread ID

  public:
					MprThread(int pri, char *name);
					//	FUTURE -- move pri to last and default it.
					MprThread(MprThreadProc proc, int pri, void *data, 
						char *name, int stackSize = 0);
					~MprThread();
	char			*getName() { return name; };
	int				getId() { return id; };
	MprOsThread		getOsThread() { return osThreadId; };
	int				getPriority() { return priority; };
	void			lock() { mutex->lock(); };
	void			setId(int id) { this->id = id; };
	void			setOsThread(MprOsThread osThread);
	void			setPriority(int priority);
	void			setStackSize(int size);
	int				start();
	void			threadProc();
	void			unlock() { mutex->unlock(); };
 
  private:
	int				mapMprPriorityToOs(int mprPriority);
	int				mapOsPriorityToMpr(int nativePriority);
};

extern MprThread	*mprGetCurrentThread();
extern int			mprGetCurrentThreadId();
extern int			mprGetMaxPoolThreads();

/*
 * 	Spin lock support. Async thread safe
 */
typedef volatile int MprSpinLock;

extern void mprSpinInit(MprSpinLock *lock);
extern void mprSpinDestroy(MprSpinLock *lock);
extern void mprSpinLock(MprSpinLock *lock);
extern void mprSpinUnlock(MprSpinLock *lock);


#endif // BLD_FEATURE_MULTITHREAD
/////////////////////////////// MprTimerService ////////////////////////////////

#define MPR_TIMER_TOLERANCE		2		// Used in timer calculations

//
//	Timer service. One per MPR 
//
class MprTimerService {
  private:
	int				lastIdleTime;		// Last return value from getIdleTime()
	int				lastRanTimers;		// Last call to runTimers()
	MprLogModule	*log;				// Log module to identify trace
	MprList			timerList;			// List of all timers

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;				// Multi-thread sync
#endif

  public:
					MprTimerService();
					~MprTimerService();
	void			callTimer(MprTimer *tp);
	int				getIdleTime();
	int				runTimers();
	int				stop();
	int				start();

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*getMutex() { return mutex; };
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif

  protected:
	void			updateSelect(MprTimer *tp);

	friend class	MprTimer;
};

//////////////////////////////////// MprTimer //////////////////////////////////
//
//	MprTimer callback function prototype
//
typedef void		(*MprTimerProc)(void *data, MprTimer *tp);

//
//	MprTimer flags
//
#define MPR_TIMER_DISPOSED		0x1
#define MPR_TIMER_RUNNING		0x2
#define MPR_TIMER_TASK			0x4
#define MPR_TIMER_AUTO_RESCHED	0x8

class MprTimer : public MprLink {
  private:
	void			*data;				// Argument to pass to callback
	int				flags;
	int				inUse;				// In use counter. Used by dispose()
	int				period;				// Reschedule period 
	MprTimerProc	proc;				// Callback procedure
	MprTime			time;				// When timer is due to next run
	MprTimerService	*timerService;

#if BLD_FEATURE_MULTITHREAD
	MprCond			*stoppingCond;		// Synchronization when stopping
#endif

  public:
					MprTimer(int msec, MprTimerProc routine, void *arg, 
						int userFlags = 0);
	bool			dispose();
	int				getPeriod() { return period; };
	MprTimerService	*getMprTimerService() { return timerService; };
	void			reschedule();
	void			reschedule(int msec);
	int				stop(int timeout);

private:
					~MprTimer();
	friend class	MprTimerService;
};

//////////////////////////////// MprWinService /////////////////////////////////
#if BLD_FEATURE_RUN_AS_SERVICE && WIN

class MprWinService {
  private:
	MprStr			svcName;
  public:
					MprWinService(char *name);
					~MprWinService();
	int				install(char *displayName, char *cmd);
	int				registerService(HANDLE threadHandle, HANDLE waitEvent);
	int				remove(int code);
	int				startDispatcher(LPSERVICE_MAIN_FUNCTION svcMain);
	int				start();
	int				stop(int code);
	void			updateStatus(int status, int exitCode);
};

#endif
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Other C++ Stuff ////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_DEBUG && UNUSED
#if CYGWIN || LINUX || MACOSX || FREEBSD
#if MPR_CPU_IX86
	inline int64 mprGetHiResTime() {
		int64	now;
		__asm__ __volatile__ ("rdtsc" : "=A" (now));
		return now;
	}
#endif
#endif
#if WIN
	inline int64 mprGetHiResTime() {
		int64	now;
		QueryPerformanceCounter((LARGE_INTEGER*) &now);
		return now;
	}
#endif
	
////////////////////////////////////////////////////////////////////////////////

inline int64 mprGetElapsedTime() 
{
	static int64	lastMark = 0;
	int64			now, elapsed;
	
	now = mprGetHiResTime();
	if (now > lastMark) {
		elapsed = now - lastMark;
		lastMark = now;
	} else {
		elapsed = lastMark - now + 1;
		lastMark = now;
	}
	return elapsed;
};

#endif // BLD_DEBUG && UNUSED
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// C Prototypes //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Generic array type
 */
typedef struct MprArray {
	int		max;					/**< Size of the handles array. */
	int		used;					/**< Count of used entries in handles. */
	void	**handles;
} MprArray;

#if BLD_FEATURE_SQUEEZE
#define MPR_ARRAY_INCR		8
#else
#define MPR_ARRAY_INCR		16
#endif

//
//	Array
//
extern MprArray	*mprCreateArray();
extern void 	mprDestroyArray(MprArray *array);
extern int 		mprAddToArray(MprArray *array, void *item);
extern int 		mprRemoveFromArray(MprArray *array, int index);

//
//	Printf replacements
//
#define	MPR_STDOUT	1
#define	MPR_STDERR	2

extern int		mprPrintf(char *fmt, ...);
extern int		mprStaticPrintf(char *fmt, ...);
extern int		mprFprintf(int fd, char *fmt, ...);
extern int		mprAllocSprintf(char **s, int n, char *fmt, ...);
extern int		mprAllocVsprintf(char **s, int n, char *fmt, va_list arg);
extern int		mprSprintf(char *s, int n, char *fmt, ...);
extern int		mprVsprintf(char *s, int n, char *fmt, va_list arg);
extern char		*mprItoa(int value, char *userBuf, int width);
extern int 		mprAtoi(char *str, int radix);

//
//	Safe string routines
//
extern int		mprStrcpy(char *dest, int destMax, const char *src);
extern int		mprMemcpy(char *dest, int destMax, const char *src, 
						int nbytes);
extern int		mprAllocStrcpy(char **dest, int max, const char *src);
extern int		mprReallocStrcpy(char **dest, int max, const char *src);
extern int		mprAllocMemcpy(char **dest, int destMax, const char *src, 
						int nbytes);
extern int		mprStrcat(char *dest, int max, const char *delim, 
						const char *src, ...);
extern int		mprAllocStrcat(char **dest, int max, const char *delim, 
						const char *src, ...);
extern int		mprReallocStrcat(char **dest, int max, int existingLen,
						const char *delim, const char *src, ...);
extern int		mprStrlen(char *src, int max);

//	FUTURE (rename to Strcmpi, Strncmpi)
extern int		mprStrCmpAnyCase(char *s1, char *s2);
extern int		mprStrCmpAnyCaseCount(char *s1, char *s2, int len);
extern char		*mprStrLower(char *string);
extern char		*mprStrUpper(char *string);
extern char		*mprStrTrim(char *string, char c);

//
//	Reentrant string and time routines
//
extern char		*mprStrTok(char *str, const char *sep, char **last);
extern char		*mprGetWordTok(char *word, int wordLen, char *str, 
						const char *delim, char **tok);
extern int		mprCtime(const time_t* timer, char *buf, int bufsize);
extern int		mprGetTime(MprTime *tp);
extern int		mprAsctime(const struct tm *timeptr, char *buf, int bufsiz);
extern struct tm *mprGmtime(time_t* now, struct tm *tmp);
extern struct tm *mprLocaltime(time_t* now, struct tm *tmp);
extern int		mprRfcTime(char *buf, int size, const struct tm *timeptr);

//
//	General other xPlatform routines
//
extern char		*mprGetBaseName(char *name);
extern int		mprGetDirName(char *buf, int bufsize, char *path);
extern void		mprMakeArgv(char *prog, char *cmd, char ***argv, int *argc);
extern int		mprMakeDir(char *path);
extern char 	*mprInetNtoa(char *buf, int size, const struct in_addr in);
extern void		mprSleep(int msec);
extern struct hostent* mprGetHostByName(char *name);
extern void		mprFreeGetHostByName(struct hostent* hostp);
extern int		mprGetRandomBytes(uchar *buf, int size, int block);

#if BLD_FEATURE_MULTITHREAD
extern char		*mprGetCurrentThreadName();
extern void		mprLock();
extern void		mprUnlock();

#else
#if __cplusplus
inline void		mprLock() {};
inline void		mprUnlock() {};
#else
#define mprLock()
#define mprUnlock()
#endif
#endif

extern bool		mprGetDebugMode();
extern void		mprSetDebugMode(bool on);

extern int		mprGetOsError();
#if UNUSED
extern char		*mprGetErrorMsg(int errCode);
#endif
extern char		*mprMakeTempFileName(char *buf, int bufsize, char *tempDir);
extern void		mprNextFds(char *msg);
extern char		*mprGetFullPathName(char *buf, int buflen, char *path);
extern char 	*mprGetNativePathName(char *buf, int buflen, char *path);
extern void 	mprSetModuleSearchPath(char *dirs);

extern void		mprError(char *file, int line, int flags, char *fmt, ...);

extern char 	*mprGetIpAddrPort(char *ipSpec, int *port, int default_port);

#if BLD_FEATURE_LOG || !__cplusplus
extern void		mprLog(int level, char *fmt, ...);
#endif

typedef void	(*MprMemProc)(int askSize, int totalPoolMem, int limit);

extern void		*mprCalloc(uint numElem, uint size);
extern int		mprCreateMemHeap(char *userBuf, int initialSize, int limit);
extern void		mprFree(void *ptr);
extern void		*mprMalloc(uint size);
extern void		mprMemClose();
#if BLD_FEATURE_MALLOC_HOOK
extern void		mprHookMalloc();
#endif
extern void		mprMemStop();
extern void		mprPrintMemStats();
extern void		*mprRealloc(void *ptr, uint size);
extern void		mprRequestMemStats(bool on);
extern void		mprSetMemHandler(MprMemProc cback);
extern char		*mprStrdup(const char *str);

#if WIN
extern int		mprReadRegistry(char *key, char *val, char **buf, int max);
#endif

#if BLD_FEATURE_LICENSE

#define MPR_PRODUCT_EVAL				1
#define MPR_PRODUCT_APPWEB				2
#define MPR_PRODUCT_DEVICEMANAGER		3
#define MPR_PRODUCT_MYAPPWEB			4
#define MPR_PRODUCT_MAX					4

//
//	Internal license constants
//
#define MPR_LIC_SEED				0x1						// 8 bits
#define MPR_LIC_KEY					INT64(0x371aabcd7abcfe2b)
#define MPR_LIC_MAX_SERIAL			0xFFFF
#define MPR_LIC_MAX_KEY				8

//
//	Internal crypt constants
//
#define MPR_CRYPT_ROUNDS			2

////////////////////////////////// Prototypes //////////////////////////////////

extern int mprGenerateLicense(uint64 *key, char *product, int serial, 
			int version, int productBits);
extern int mprChecksumLicense(int serial);
extern int mprValidateLicense(uint64 licenseKey, char **product, int *version, 
	int *serial, int *productBits);
extern int mprCrypt(uchar *output, int outputSize, uchar *input, int inputSize, 
	uint64 *key, int keySize);
extern int mprDecrypt(uchar *output, int outputSize, uchar *input, int 
	inputSize, uint64 *key, int keySize);

#endif // BLD_FEATURE_LICENSE

#if __cplusplus
} // extern "C"
#endif

#endif // _h_MPR 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
