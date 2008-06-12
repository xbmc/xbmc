/* 
   Unix SMB/CIFS implementation.

   Winbind daemon for ntdom nss module

   Copyright (C) Tim Potter 2000
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA  02111-1307, USA.   
*/

#ifndef _WINBIND_NSS_SOLARIS_H
#define _WINBIND_NSS_SOLARIS_H

/* Solaris has a broken nss_common header file containing C++ reserved names. */
#ifndef __cplusplus
#undef class
#undef private
#undef public
#undef protected
#undef template
#undef this
#undef new
#undef delete
#undef friend
#endif

#include <nss_common.h>

#ifndef __cplusplus
#define class #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define private #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define public #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define protected #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define template #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define this #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define new #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define delete #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#define friend #error DONT_USE_CPLUSPLUS_RESERVED_NAMES
#endif

#include <nss_dbdefs.h>
#include <nsswitch.h>

typedef nss_status_t NSS_STATUS;

#define NSS_STATUS_SUCCESS     NSS_SUCCESS
#define NSS_STATUS_NOTFOUND    NSS_NOTFOUND
#define NSS_STATUS_UNAVAIL     NSS_UNAVAIL
#define NSS_STATUS_TRYAGAIN    NSS_TRYAGAIN

/* The solaris winbind is implemented as a wrapper around the linux
   version. */

NSS_STATUS _nss_winbind_setpwent(void);
NSS_STATUS _nss_winbind_endpwent(void);
NSS_STATUS _nss_winbind_getpwent_r(struct passwd* result, char* buffer,
				   size_t buflen, int* errnop);
NSS_STATUS _nss_winbind_getpwuid_r(uid_t, struct passwd*, char* buffer,
				   size_t buflen, int* errnop);
NSS_STATUS _nss_winbind_getpwnam_r(const char* name, struct passwd* result,
				   char* buffer, size_t buflen, int* errnop);

NSS_STATUS _nss_winbind_setgrent(void);
NSS_STATUS _nss_winbind_endgrent(void);
NSS_STATUS _nss_winbind_getgrent_r(struct group* result, char* buffer,
				   size_t buflen, int* errnop);
NSS_STATUS _nss_winbind_getgrnam_r(const char *name,
				   struct group *result, char *buffer,
				   size_t buflen, int *errnop);
NSS_STATUS _nss_winbind_getgrgid_r(gid_t gid,
				   struct group *result, char *buffer,
				   size_t buflen, int *errnop);

#endif /* _WINBIND_NSS_SOLARIS_H */
