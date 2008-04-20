/* 
   Unix SMB/CIFS implementation.

   AIX loadable authentication module, providing identification 
   routines against Samba winbind/Windows NT Domain

   Copyright (C) Aaron Collins 2003
   
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

#include "winbind_client.h"

/* Make sure that the module gets registered needed by freebsd 5.1 */

extern enum nss_status _nss_winbind_getgrent_r(struct group *, char *, size_t,
    int *);
extern enum nss_status _nss_winbind_getgrnam_r(const char *, struct group *,
    char *, size_t, int *);
extern enum nss_status _nss_winbind_getgrgid_r(gid_t gid, struct group *, char *,
    size_t, int *);
extern enum nss_status _nss_winbind_setgrent(void);
extern enum nss_status _nss_winbind_endgrent(void);

extern enum nss_status _nss_winbind_getpwent_r(struct passwd *, char *, size_t,
    int *);
extern enum nss_status _nss_winbind_getpwnam_r(const char *, struct passwd *,
    char *, size_t, int *);
extern enum nss_status _nss_winbind_getpwuid_r(gid_t gid, struct passwd *, char *,
    size_t, int *);
extern enum nss_status _nss_winbind_setpwent(void);
extern enum nss_status _nss_winbind_endpwent(void);

NSS_METHOD_PROTOTYPE(__nss_compat_getgrnam_r);
NSS_METHOD_PROTOTYPE(__nss_compat_getgrgid_r);
NSS_METHOD_PROTOTYPE(__nss_compat_getgrent_r);
NSS_METHOD_PROTOTYPE(__nss_compat_setgrent);
NSS_METHOD_PROTOTYPE(__nss_compat_endgrent);

NSS_METHOD_PROTOTYPE(__nss_compat_getpwnam_r);
NSS_METHOD_PROTOTYPE(__nss_compat_getpwuid_r);
NSS_METHOD_PROTOTYPE(__nss_compat_getpwent_r);
NSS_METHOD_PROTOTYPE(__nss_compat_setpwent);
NSS_METHOD_PROTOTYPE(__nss_compat_endpwent);

static ns_mtab methods[] = {
{ NSDB_GROUP, "getgrnam_r", __nss_compat_getgrnam_r, _nss_winbind_getgrnam_r },
{ NSDB_GROUP, "getgrgid_r", __nss_compat_getgrgid_r, _nss_winbind_getgrgid_r },
{ NSDB_GROUP, "getgrent_r", __nss_compat_getgrent_r, _nss_winbind_getgrent_r },
{ NSDB_GROUP, "endgrent",   __nss_compat_setgrent,   _nss_winbind_setgrent },
{ NSDB_GROUP, "setgrent",   __nss_compat_endgrent,   _nss_winbind_endgrent },

{ NSDB_PASSWD, "getpwnam_r", __nss_compat_getpwnam_r, _nss_winbind_getpwnam_r },
{ NSDB_PASSWD, "getpwuid_r", __nss_compat_getpwuid_r, _nss_winbind_getpwuid_r },
{ NSDB_PASSWD, "getpwent_r", __nss_compat_getpwent_r, _nss_winbind_getpwent_r },
{ NSDB_PASSWD, "endpwent",   __nss_compat_setpwent,   _nss_winbind_setpwent },
{ NSDB_PASSWD, "setpwent",   __nss_compat_endpwent,   _nss_winbind_endpwent },

};

ns_mtab *
nss_module_register(const char *source, unsigned int *mtabsize,
    nss_module_unregister_fn *unreg)
{
        *mtabsize = sizeof(methods)/sizeof(methods[0]);
        *unreg = NULL;
        return (methods);
}
