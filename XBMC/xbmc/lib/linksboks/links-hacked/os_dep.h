/* os_dep.h
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#ifndef _OS_DEP_H
#define _OS_DEP_H

#define SYS_UNIX	1
#define SYS_OS2		2
#define SYS_WIN32	3
#define SYS_BEOS	4
#define SYS_RISCOS	5
#define SYS_ATHEOS	6
#define SYS_XBOX	7

/* hardcoded limit of 10 OSes in default.c */

#if defined(__EMX__)
#define OS2
#elif defined(_XBOX)
#define XBOX
#elif defined(_WIN32) || defined(__CYGWIN__)
#define WIN32
#elif defined(__BEOS__)
#define BEOS
#elif defined(__riscos__)
#define RISCOS
#elif defined(__ATHEOS__)
#define ATHEOS
#else
#define UNIX
#endif

#if defined(OS2) || defined(WIN32) || defined(BEOS) || defined(RISCOS) || defined(ATHEOS) || defined(XBOX)
#ifdef UNIX
#undef UNIX
#endif
#endif

#ifdef __EMX__
#define _stricmp stricmp
#define strncasecmp strnicmp
#define read _read
#define write _write
#endif

#if defined(UNIX)

static inline int dir_sep(char x) { return x == '/'; }
#define NEWLINE	"\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#include <pwd.h>
#include <grp.h>
#define SYSTEM_ID SYS_UNIX
#define SYSTEM_NAME "Unix"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#ifdef HAVE_SYS_UN_H
#define USE_AF_UNIX
#else
#define DONT_USE_AF_UNIX
#endif
#define ASSOC_BLOCK
#define ASSOC_CONS_XWIN

#elif defined(OS2)

static inline int dir_sep(char x) { return x == '/' || x == '\\'; }
#define NEWLINE	"\r\n"
/*#define NO_ASYNC_LOOKUP*/
#define SYSTEM_ID SYS_OS2
#define SYSTEM_NAME "OS/2"
#define DEFAULT_SHELL "cmd.exe"
#define GETSHELL getenv("COMSPEC")
#define NO_FG_EXEC
#define DOS_FS
#define NO_FILE_SECURITY
#define NO_FORK_ON_EXIT
#define ASSOC_CONS_XWIN

#elif defined(XBOX)

static int dir_sep(char x) { return x == '/' || x == '\\'; }
#define NEWLINE	"\r\n"
/*#define NO_ASYNC_LOOKUP*/
#define SYSTEM_ID SYS_XBOX
#define SYSTEM_NAME "Xbox"
#define DEFAULT_SHELL "C:\\xboxdash.xbe"
#define GETSHELL "C:\\xboxdash.xbe"
#define NO_FG_EXEC
#define DOS_FS
#define NO_FORK_ON_EXIT

#elif defined(WIN32)

static int dir_sep(char x) { return x == '/' || x == '\\'; }
#define NEWLINE	"\r\n"
/*#define NO_ASYNC_LOOKUP*/
#define SYSTEM_ID SYS_WIN32
#define SYSTEM_NAME "Win32"
#define DEFAULT_SHELL "cmd.exe"
#define GETSHELL getenv("COMSPEC")
#define NO_FG_EXEC
#define DOS_FS
#define NO_FORK_ON_EXIT

#elif defined(BEOS)

static inline int dir_sep(char x) { return x == '/'; }
#define NEWLINE	"\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#include <pwd.h>
#include <grp.h>
#define SYSTEM_ID SYS_BEOS
#define SYSTEM_NAME "BeOS"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define NO_FORK_ON_EXIT
#define ASSOC_BLOCK

#include <sys/time.h>
#include <sys/types.h>
#include <net/socket.h>

int be_socket(int, int, int);
int be_connect(int, struct sockaddr *, int);
int be_getpeername(int, struct sockaddr *, int *);
int be_getsockname(int, struct sockaddr *, int *);
int be_listen(int, int);
int be_accept(int, struct sockaddr *, int *);
int be_bind(int, struct sockaddr *, int);
int be_pipe(int *);
int be_read(int, void *, int);
int be_write(int, void *, int);
int be_close(int);
int be_select(int, struct fd_set *, struct fd_set *, struct fd_set *, struct timeval *);
int be_getsockopt(int, int, int, void *, int *);

#elif defined(RISCOS)

static inline int dir_sep(char x) { return x == '/' || x == '\\'; }
#define NEWLINE        "\n"
#define SYSTEM_ID SYS_RISCOS
#define SYSTEM_NAME "RISC OS"
#define DEFAULT_SHELL "gos"
#define GETSHELL getenv("SHELL")
#define NO_FG_EXEC
#define NO_FILE_SECURITY
#define NO_FORK_ON_EXIT

#elif defined(ATHEOS)

static inline int dir_sep(char x) { return x == '/'; }
#define NEWLINE	"\n"
#define FS_UNIX_RIGHTS
#define FS_UNIX_HARDLINKS
#define FS_UNIX_SOFTLINKS
#define FS_UNIX_USERS
#include <pwd.h>
#include <grp.h>
#define SYSTEM_ID SYS_ATHEOS
#define SYSTEM_NAME "Atheos"
#define DEFAULT_SHELL "/bin/sh"
#define GETSHELL getenv("SHELL")
#define ASSOC_BLOCK

#endif

#if !defined(HAVE_BEGINTHREAD) && !defined(BEOS) && !defined(ATHEOS) && !defined(HAVE_PTHREADS) && !defined(HAVE_CLONE)
#define THREAD_SAFE_LOOKUP
#endif
#endif /* #ifndef_OS_DEP_H */
