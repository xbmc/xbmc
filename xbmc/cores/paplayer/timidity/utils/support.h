/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ___SUPPORT_H_
#define ___SUPPORT_H_

#ifndef HAVE_VSNPRINTF
#include <stdarg.h> /* for va_list */
extern void vsnprintf(char *buff, size_t bufsiz, const char *fmt, va_list ap);
#endif

#ifndef HAVE_SNPRINTF
extern void snprintf(char *buff, size_t bufsiz, const char *fmt, ...);
#endif /* HAVE_SNPRINTF */

#ifndef HAVE_STRERROR
extern char *strerror(int errnum);
#endif /* HAVE_STRERROR */

/* There is no prototype of usleep() on Solaris. Why? */
#if !defined(HAVE_USLEEP) || defined(SOLARIS)
extern int usleep(unsigned int usec);
#endif

#ifdef __W32__
#define sleep(time) Sleep(time)
#else
#ifndef HAVE_SLEEP
#define sleep(s) usleep((s) * 1000000)
#endif /* HAVE_SLEEP */
#endif

#ifndef HAVE_STRDUP
extern char *strdup(const char *s);
#endif /* HAVE_STRDUP */

#ifndef HAVE_GETCWD
extern char *getcwd(char *buf, size_t size);
#endif /* HAVE_GETCWD */

#ifndef HAVE_STRSTR
#define strstr(s,c)	index(s,c)
#endif /* HAVE_STRSTR */

#ifndef HAVE_STRNCASECMP
extern int strncasecmp(char *s1, char *s2, unsigned int len);
#endif /* HAVE_STRNCASECMP */

#ifndef HAVE_MKSTEMP
extern int mkstemp(char *template);
#endif /* HAVE_MKSTEMP */

#ifndef HAVE_SYS_STAT_H
#ifdef __W32__
#include <sys/stat.h>          /* they have. */
#elif defined(__MACOS__)
#define S_IFDIR 1
#define S_ISDIR(m)   ((m) & S_IFDIR)
struct stat {
	short st_mode;
	short st_dev;
	long st_ino;
	unsigned long st_size;
	unsigned long st_mtime, st_ctime, st_btime;
};
int stat(const char *filename, struct stat *st);
#endif /* __W32__ */
#endif /* HAVE_SYS_STAT_H*/
#endif /* ___SUPPORT_H_ */
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&0xF000) == 0x4000)
#endif /* S_ISDIR */

#ifndef HAVE_STRLCPY
#include <stddef.h>
extern size_t strlcpy(char *dst, const char *src, size_t size);
#endif

#ifndef HAVE_STRLCAT
#include <stddef.h>
extern size_t strlcat(char *dst, const char *src, size_t size);
#endif
