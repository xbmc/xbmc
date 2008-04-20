/* 
   Unix SMB/CIFS implementation.

   AIX loadable authentication module, providing identification and
   authentication routines against Samba winbind/Windows NT Domain

   Copyright (C) Tim Potter 2003
   Copyright (C) Steve Roylance 2003
   Copyright (C) Andrew Tridgell 2003-2004
   
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

/*

  To install this module copy nsswitch/WINBIND to /usr/lib/security and add
  "WINBIND" in /usr/lib/security/methods.cfg and /etc/security/user

  Note that this module also provides authentication and password
  changing routines, so you do not need to install the winbind PAM
  module.

  see 
  http://publib16.boulder.ibm.com/doc_link/en_US/a_doc_lib/aixprggd/kernextc/sec_load_mod.htm
  for some information in the interface that this module implements

  Many thanks to Julianne Haugh for explaining some of the finer
  details of this interface.

  To debug this module use uess_test.c (which you can get from tridge)
  or set "options=debug" in /usr/lib/security/methods.cfg

*/

#include <stdlib.h>
#include <string.h>
#include <usersec.h>
#include <errno.h>
#include <stdarg.h>

#include "winbind_client.h"

#define WB_AIX_ENCODED '_'

static int debug_enabled;


static void logit(const char *format, ...)
{
	va_list ap;
	FILE *f;
	if (!debug_enabled) {
		return;
	}
	f = fopen("/tmp/WINBIND_DEBUG.log", "a");
	if (!f) return;
	va_start(ap, format);
	vfprintf(f, format, ap);
	va_end(ap);
	fclose(f);
}


#define HANDLE_ERRORS(ret) do { \
	if ((ret) == NSS_STATUS_NOTFOUND) { \
		errno = ENOENT; \
		return NULL; \
	} else if ((ret) != NSS_STATUS_SUCCESS) { \
		errno = EIO; \
		return NULL; \
	} \
} while (0)

#define STRCPY_RET(dest, src) \
do { \
	if (strlen(src)+1 > sizeof(dest)) { errno = EINVAL; return -1; } \
	strcpy(dest, src); \
} while (0)

#define STRCPY_RETNULL(dest, src) \
do { \
	if (strlen(src)+1 > sizeof(dest)) { errno = EINVAL; return NULL; } \
	strcpy(dest, src); \
} while (0)


/* free a passwd structure */
static void free_pwd(struct passwd *pwd)
{
	free(pwd->pw_name);
	free(pwd->pw_passwd);
	free(pwd->pw_gecos);
	free(pwd->pw_dir);
	free(pwd->pw_shell);
	free(pwd);
}

/* free a group structure */
static void free_grp(struct group *grp)
{
	int i;

	free(grp->gr_name);
	free(grp->gr_passwd);
	
	if (!grp->gr_mem) {
		free(grp);
		return;
	}
	
	for (i=0; grp->gr_mem[i]; i++) {
		free(grp->gr_mem[i]);
	}

	free(grp->gr_mem);
	free(grp);
}


/* replace commas with nulls, and null terminate */
static void replace_commas(char *s)
{
	char *p, *p0=s;
	for (p=strchr(s, ','); p; p = strchr(p+1, ',')) {
		*p=0;
		p0 = p+1;
	}

	p0[strlen(p0)+1] = 0;
}


/* the decode_*() routines are used to cope with the fact that AIX 5.2
   and below cannot handle user or group names longer than 8
   characters in some interfaces. We use the normalize method to
   provide a mapping to a username that fits, by using the form '_UID'
   or '_GID'.

   this only works if you can guarantee that the WB_AIX_ENCODED char
   is not used as the first char of any other username
*/
static unsigned decode_id(const char *name)
{
	unsigned id;
	sscanf(name+1, "%u", &id);
	return id;
}

static struct passwd *wb_aix_getpwuid(uid_t uid);

static char *decode_user(const char *name)
{
	struct passwd *pwd;
	unsigned id;
	char *ret;

	sscanf(name+1, "%u", &id);
	pwd = wb_aix_getpwuid(id);
	if (!pwd) {
		return NULL;
	}
	ret = strdup(pwd->pw_name);

	free_pwd(pwd);

	logit("decoded '%s' -> '%s'\n", name, ret);

	return ret;
}


/*
  fill a struct passwd from a winbindd_pw struct, allocating as a single block
*/
static struct passwd *fill_pwent(struct winbindd_pw *pw)
{
	struct passwd *result;

	result = calloc(1, sizeof(struct passwd));
	if (!result) {
		errno = ENOMEM;
		return NULL;
	}

	result->pw_uid = pw->pw_uid;
	result->pw_gid = pw->pw_gid;
	result->pw_name   = strdup(pw->pw_name);
	result->pw_passwd = strdup(pw->pw_passwd);
	result->pw_gecos  = strdup(pw->pw_gecos);
	result->pw_dir    = strdup(pw->pw_dir);
	result->pw_shell  = strdup(pw->pw_shell);
	
	return result;
}


/*
  fill a struct group from a winbindd_pw struct, allocating as a single block
*/
static struct group *fill_grent(struct winbindd_gr *gr, char *gr_mem)
{
	int i;
	struct group *result;
	char *p, *name;

	result = calloc(1, sizeof(struct group));
	if (!result) {
		errno = ENOMEM;
		return NULL;
	}

	result->gr_gid = gr->gr_gid;

	result->gr_name   = strdup(gr->gr_name);
	result->gr_passwd = strdup(gr->gr_passwd);

	/* Group membership */
	if ((gr->num_gr_mem < 0) || !gr_mem) {
		gr->num_gr_mem = 0;
	}
	
	if (gr->num_gr_mem == 0) {
		/* Group is empty */		
		return result;
	}
	
	result->gr_mem = (char **)malloc(sizeof(char *) * (gr->num_gr_mem+1));
	if (!result->gr_mem) {
		errno = ENOMEM;
		return NULL;
	}

	/* Start looking at extra data */
	i=0;
	for (name = strtok_r(gr_mem, ",", &p); 
	     name; 
	     name = strtok_r(NULL, ",", &p)) {
		if (i == gr->num_gr_mem) {
			break;
		}
		result->gr_mem[i] = strdup(name);
		i++;
	}

	/* Terminate list */
	result->gr_mem[i] = NULL;

	return result;
}



/* take a group id and return a filled struct group */	
static struct group *wb_aix_getgrgid(gid_t gid)
{
	struct winbindd_response response;
	struct winbindd_request request;
	struct group *grp;
	NSS_STATUS ret;

	logit("getgrgid %d\n", gid);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);
	
	request.data.gid = gid;

	ret = winbindd_request_response(WINBINDD_GETGRGID, &request, &response);

	logit("getgrgid ret=%d\n", ret);

	HANDLE_ERRORS(ret);

	grp = fill_grent(&response.data.gr, response.extra_data.data);

	free_response(&response);

	return grp;
}

/* take a group name and return a filled struct group */
static struct group *wb_aix_getgrnam(const char *name)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct group *grp;

	if (*name == WB_AIX_ENCODED) {
		return wb_aix_getgrgid(decode_id(name));
	}

	logit("getgrnam '%s'\n", name);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	STRCPY_RETNULL(request.data.groupname, name);

	ret = winbindd_request_response(WINBINDD_GETGRNAM, &request, &response);
	
	HANDLE_ERRORS(ret);

	grp = fill_grent(&response.data.gr, response.extra_data.data);

	free_response(&response);

	return grp;
}


/* this call doesn't have to fill in the gr_mem, but we do anyway
   for simplicity */
static struct group *wb_aix_getgracct(void *id, int type)
{
	if (type == 1) {
		return wb_aix_getgrnam((char *)id);
	}
	if (type == 0) {
		return wb_aix_getgrgid(*(int *)id);
	}
	errno = EINVAL;
	return NULL;
}


/* take a username and return a string containing a comma-separated
   list of group id numbers to which the user belongs */
static char *wb_aix_getgrset(char *user)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	int i, idx;
	char *tmpbuf;
	int num_gids;
	gid_t *gid_list;
	char *r_user = user;

	if (*user == WB_AIX_ENCODED) {
		r_user = decode_user(r_user);
		if (!r_user) {
			errno = ENOENT;
			return NULL;
		}
	}

	logit("getgrset '%s'\n", r_user);

        ZERO_STRUCT(response);
        ZERO_STRUCT(request);

	STRCPY_RETNULL(request.data.username, r_user);

	if (*user == WB_AIX_ENCODED) {
		free(r_user);
	}

	ret = winbindd_request_response(WINBINDD_GETGROUPS, &request, &response);

	HANDLE_ERRORS(ret);

	num_gids = response.data.num_entries;
	gid_list = (gid_t *)response.extra_data.data;
		
	/* allocate a space large enough to contruct the string */
	tmpbuf = malloc(num_gids*12);
	if (!tmpbuf) {
		return NULL;
	}

	for (idx=i=0; i < num_gids-1; i++) {
		idx += sprintf(tmpbuf+idx, "%u,", gid_list[i]);	
	}
	idx += sprintf(tmpbuf+idx, "%u", gid_list[i]);	

	free_response(&response);

	return tmpbuf;
}


/* take a uid and return a filled struct passwd */	
static struct passwd *wb_aix_getpwuid(uid_t uid)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct passwd *pwd;

	logit("getpwuid '%d'\n", uid);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);
		
	request.data.uid = uid;
	
	ret = winbindd_request_response(WINBINDD_GETPWUID, &request, &response);

	HANDLE_ERRORS(ret);

	pwd = fill_pwent(&response.data.pw);

	free_response(&response);

	logit("getpwuid gave ptr %p\n", pwd);

	return pwd;
}


/* take a username and return a filled struct passwd */
static struct passwd *wb_aix_getpwnam(const char *name)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct passwd *pwd;

	if (*name == WB_AIX_ENCODED) {
		return wb_aix_getpwuid(decode_id(name));
	}

	logit("getpwnam '%s'\n", name);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	STRCPY_RETNULL(request.data.username, name);

	ret = winbindd_request_response(WINBINDD_GETPWNAM, &request, &response);

	HANDLE_ERRORS(ret);
	
	pwd = fill_pwent(&response.data.pw);

	free_response(&response);

	logit("getpwnam gave ptr %p\n", pwd);

	return pwd;
}

/*
  list users
*/
static int wb_aix_lsuser(char *attributes[], attrval_t results[], int size)
{
	NSS_STATUS ret;
	struct winbindd_request request;
	struct winbindd_response response;
	int len;
	char *s;

	if (size != 1 || strcmp(attributes[0], S_USERS) != 0) {
		logit("invalid lsuser op\n");
		errno = EINVAL;
		return -1;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	
	ret = winbindd_request_response(WINBINDD_LIST_USERS, &request, &response);
	if (ret != 0) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(response.extra_data.data);

	s = malloc(len+2);
	if (!s) {
		free_response(&response);
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(s, response.extra_data.data, len+1);

	replace_commas(s);

	results[0].attr_un.au_char = s;
	results[0].attr_flag = 0;

	free_response(&response);
	
	return 0;
}


/*
  list groups
*/
static int wb_aix_lsgroup(char *attributes[], attrval_t results[], int size)
{
	NSS_STATUS ret;
	struct winbindd_request request;
	struct winbindd_response response;
	int len;
	char *s;

	if (size != 1 || strcmp(attributes[0], S_GROUPS) != 0) {
		logit("invalid lsgroup op\n");
		errno = EINVAL;
		return -1;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	
	ret = winbindd_request_response(WINBINDD_LIST_GROUPS, &request, &response);
	if (ret != 0) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(response.extra_data.data);

	s = malloc(len+2);
	if (!s) {
		free_response(&response);
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(s, response.extra_data.data, len+1);

	replace_commas(s);

	results[0].attr_un.au_char = s;
	results[0].attr_flag = 0;

	free_response(&response);
	
	return 0;
}


static attrval_t pwd_to_group(struct passwd *pwd)
{
	attrval_t r;
	struct group *grp = wb_aix_getgrgid(pwd->pw_gid);
	
	if (!grp) {
		r.attr_flag = EINVAL;				
	} else {
		r.attr_flag = 0;
		r.attr_un.au_char = strdup(grp->gr_name);
		free_grp(grp);
	}

	return r;
}

static attrval_t pwd_to_groupsids(struct passwd *pwd)
{
	attrval_t r;
	char *s, *p;

	s = wb_aix_getgrset(pwd->pw_name);
	if (!s) {
		r.attr_flag = EINVAL;
		return r;
	}

	p = malloc(strlen(s)+2);
	if (!p) {
		r.attr_flag = ENOMEM;
		return r;
	}

	strcpy(p, s);
	replace_commas(p);
	free(s);

	r.attr_un.au_char = p;

	return r;
}

static attrval_t pwd_to_sid(struct passwd *pwd)
{
	struct winbindd_request request;
	struct winbindd_response response;
	attrval_t r;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = pwd->pw_uid;

	if (winbindd_request_response(WINBINDD_UID_TO_SID, &request, &response) !=
	    NSS_STATUS_SUCCESS) {
		r.attr_flag = ENOENT;
	} else {
		r.attr_flag = 0;
		r.attr_un.au_char = strdup(response.data.sid.sid);
	}

	return r;
}

static int wb_aix_user_attrib(const char *key, char *attributes[],
			      attrval_t results[], int size)
{
	struct passwd *pwd;
	int i;

	pwd = wb_aix_getpwnam(key);
	if (!pwd) {
		errno = ENOENT;
		return -1;
	}

	for (i=0;i<size;i++) {
		results[i].attr_flag = 0;

		if (strcmp(attributes[i], S_ID) == 0) {
			results[i].attr_un.au_int = pwd->pw_uid;
		} else if (strcmp(attributes[i], S_PWD) == 0) {
			results[i].attr_un.au_char = strdup(pwd->pw_passwd);
		} else if (strcmp(attributes[i], S_HOME) == 0) {
			results[i].attr_un.au_char = strdup(pwd->pw_dir);
		} else if (strcmp(attributes[i], S_SHELL) == 0) {
			results[i].attr_un.au_char = strdup(pwd->pw_shell);
		} else if (strcmp(attributes[i], S_REGISTRY) == 0) {
			results[i].attr_un.au_char = strdup("WINBIND");
		} else if (strcmp(attributes[i], S_GECOS) == 0) {
			results[i].attr_un.au_char = strdup(pwd->pw_gecos);
		} else if (strcmp(attributes[i], S_PGRP) == 0) {
			results[i] = pwd_to_group(pwd);
		} else if (strcmp(attributes[i], S_GROUPS) == 0) {
			results[i] = pwd_to_groupsids(pwd);
		} else if (strcmp(attributes[i], "SID") == 0) {
			results[i] = pwd_to_sid(pwd);
		} else {
			logit("Unknown user attribute '%s'\n", attributes[i]);
			results[i].attr_flag = EINVAL;
		}
	}

	free_pwd(pwd);

	return 0;
}

static int wb_aix_group_attrib(const char *key, char *attributes[],
			       attrval_t results[], int size)
{
	struct group *grp;
	int i;

	grp = wb_aix_getgrnam(key);
	if (!grp) {
		errno = ENOENT;
		return -1;
	}

	for (i=0;i<size;i++) {
		results[i].attr_flag = 0;

		if (strcmp(attributes[i], S_PWD) == 0) {
			results[i].attr_un.au_char = strdup(grp->gr_passwd);
		} else if (strcmp(attributes[i], S_ID) == 0) {
			results[i].attr_un.au_int = grp->gr_gid;
		} else {
			logit("Unknown group attribute '%s'\n", attributes[i]);
			results[i].attr_flag = EINVAL;
		}
	}

	free_grp(grp);

	return 0;
}


/*
  called for user/group enumerations
*/
static int wb_aix_getentry(char *key, char *table, char *attributes[], 
			   attrval_t results[], int size)
{
	logit("Got getentry with key='%s' table='%s' size=%d attributes[0]='%s'\n", 
	      key, table, size, attributes[0]);

	if (strcmp(key, "ALL") == 0 && 
	    strcmp(table, "user") == 0) {
		return wb_aix_lsuser(attributes, results, size);
	}

	if (strcmp(key, "ALL") == 0 && 
	    strcmp(table, "group") == 0) {
		return wb_aix_lsgroup(attributes, results, size);
	}

	if (strcmp(table, "user") == 0) {
		return wb_aix_user_attrib(key, attributes, results, size);
	}

	if (strcmp(table, "group") == 0) {
		return wb_aix_group_attrib(key, attributes, results, size);
	}

	logit("Unknown getentry operation key='%s' table='%s'\n", key, table);

	errno = ENOSYS;
	return -1;
}



/*
  called to start the backend
*/
static void *wb_aix_open(const char *name, const char *domain, int mode, char *options)
{
	if (strstr(options, "debug")) {
		debug_enabled = 1;
	}
	logit("open name='%s' mode=%d domain='%s' options='%s'\n", name, domain, 
	      mode, options);
	return NULL;
}

static void wb_aix_close(void *token)
{
	logit("close\n");
	return;
}

#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST
/* 
   return a list of additional attributes supported by the backend 
*/
static attrlist_t **wb_aix_attrlist(void)
{
	attrlist_t **ret;
	logit("method attrlist called\n");
	ret = malloc(2*sizeof(attrlist_t *) + sizeof(attrlist_t));
	if (!ret) {
		errno = ENOMEM;
		return NULL;
	}

	ret[0] = (attrlist_t *)(ret+2);

	/* just one extra attribute - the windows SID */
	ret[0]->al_name = strdup("SID");
	ret[0]->al_flags = AL_USERATTR;
	ret[0]->al_type = SEC_CHAR;
	ret[1] = NULL;

	return ret;
}
#endif


/*
  turn a long username into a short one. Needed to cope with the 8 char 
  username limit in AIX 5.2 and below
*/
static int wb_aix_normalize(char *longname, char *shortname)
{
	struct passwd *pwd;

	logit("normalize '%s'\n", longname);

	/* automatically cope with AIX 5.3 with longer usernames
	   when it comes out */
	if (S_NAMELEN > strlen(longname)) {
		strcpy(shortname, longname);
		return 1;
	}

	pwd = wb_aix_getpwnam(longname);
	if (!pwd) {
		errno = ENOENT;
		return 0;
	}

	sprintf(shortname, "%c%07u", WB_AIX_ENCODED, pwd->pw_uid);

	free_pwd(pwd);

	return 1;
}


/*
  authenticate a user
 */
static int wb_aix_authenticate(char *user, char *pass, 
			       int *reenter, char **message)
{
	struct winbindd_request request;
	struct winbindd_response response;
        NSS_STATUS result;
	char *r_user = user;

	logit("authenticate '%s' response='%s'\n", user, pass);

	*reenter = 0;
	*message = NULL;

	/* Send off request */
	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	if (*user == WB_AIX_ENCODED) {
		r_user = decode_user(r_user);
		if (!r_user) {
			return AUTH_NOTFOUND;
		}
	}

	STRCPY_RET(request.data.auth.user, r_user);
	STRCPY_RET(request.data.auth.pass, pass);

	if (*user == WB_AIX_ENCODED) {
		free(r_user);
	}

	result = winbindd_request_response(WINBINDD_PAM_AUTH, &request, &response);

	free_response(&response);

	logit("auth result %d for '%s'\n", result, user);

	if (result == NSS_STATUS_SUCCESS) {
		errno = 0;
		return AUTH_SUCCESS;
	}

	return AUTH_FAILURE;
}


/*
  change a user password
*/
static int wb_aix_chpass(char *user, char *oldpass, char *newpass, char **message)
{
	struct winbindd_request request;
	struct winbindd_response response;
        NSS_STATUS result;
	char *r_user = user;

	if (*user == WB_AIX_ENCODED) {
		r_user = decode_user(r_user);
		if (!r_user) {
			errno = ENOENT;
			return -1;
		}
	}

	logit("chpass '%s' old='%s' new='%s'\n", r_user, oldpass, newpass);

	*message = NULL;

	/* Send off request */
	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	STRCPY_RET(request.data.chauthtok.user, r_user);
	STRCPY_RET(request.data.chauthtok.oldpass, oldpass);
	STRCPY_RET(request.data.chauthtok.newpass, newpass);

	if (*user == WB_AIX_ENCODED) {
		free(r_user);
	}

	result = winbindd_request_response(WINBINDD_PAM_CHAUTHTOK, &request, &response);

	free_response(&response);

	if (result == NSS_STATUS_SUCCESS) {
		errno = 0;
		return 0;
	}

	errno = EINVAL;
	return -1;
}

/*
  don't do any password strength testing for now
*/
static int wb_aix_passwdrestrictions(char *user, char *newpass, char *oldpass, 
				     char **message)
{
	logit("passwdresrictions called for '%s'\n", user);
	return 0;
}


static int wb_aix_passwdexpired(char *user, char **message)
{
	logit("passwdexpired '%s'\n", user);
	/* we should check the account bits here */
	return 0;
}


/*
  we can't return a crypt() password
*/
static char *wb_aix_getpasswd(char *user)
{
	logit("getpasswd '%s'\n", user);
	errno = ENOSYS;
	return NULL;
}

/*
  this is called to update things like the last login time. We don't 
  currently pass this onto the DC
*/
static int wb_aix_putentry(char *key, char *table, char *attributes[], 
			   attrval_t values[], int size)
{
	logit("putentry key='%s' table='%s' attrib='%s'\n", 
	      key, table, size>=1?attributes[0]:"<null>");
	errno = ENOSYS;
	return -1;
}

static int wb_aix_commit(char *key, char *table)
{
	logit("commit key='%s' table='%s'\n");
	errno = ENOSYS;
	return -1;
}

static int wb_aix_getgrusers(char *group, void *result, int type, int *size)
{
	logit("getgrusers group='%s'\n", group);
	errno = ENOSYS;
	return -1;
}


#define DECL_METHOD(x) \
int method_ ## x(void) \
{ \
	logit("UNIMPLEMENTED METHOD '%s'\n", #x); \
	errno = EINVAL; \
	return -1; \
}

#if LOG_UNIMPLEMENTED_CALLS
DECL_METHOD(delgroup);
DECL_METHOD(deluser);
DECL_METHOD(newgroup);
DECL_METHOD(newuser);
DECL_METHOD(putgrent);
DECL_METHOD(putgrusers);
DECL_METHOD(putpwent);
DECL_METHOD(lock);
DECL_METHOD(unlock);
DECL_METHOD(getcred);
DECL_METHOD(setcred);
DECL_METHOD(deletecred);
#endif

int wb_aix_init(struct secmethod_table *methods)
{
	ZERO_STRUCTP(methods);

#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_VERSION
	methods->method_version = SECMETHOD_VERSION_520;
#endif

	methods->method_getgrgid           = wb_aix_getgrgid;
	methods->method_getgrnam           = wb_aix_getgrnam;
	methods->method_getgrset           = wb_aix_getgrset;
	methods->method_getpwnam           = wb_aix_getpwnam;
	methods->method_getpwuid           = wb_aix_getpwuid;
	methods->method_getentry           = wb_aix_getentry;
	methods->method_open               = wb_aix_open;
	methods->method_close              = wb_aix_close;
	methods->method_normalize          = wb_aix_normalize;
	methods->method_passwdexpired      = wb_aix_passwdexpired;
	methods->method_putentry           = wb_aix_putentry;
	methods->method_getpasswd          = wb_aix_getpasswd;
	methods->method_authenticate       = wb_aix_authenticate;	
	methods->method_commit             = wb_aix_commit;
	methods->method_chpass             = wb_aix_chpass;
	methods->method_passwdrestrictions = wb_aix_passwdrestrictions;
	methods->method_getgracct          = wb_aix_getgracct;
	methods->method_getgrusers         = wb_aix_getgrusers;
#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST
	methods->method_attrlist           = wb_aix_attrlist;
#endif

#if LOG_UNIMPLEMENTED_CALLS
	methods->method_delgroup      = method_delgroup;
	methods->method_deluser       = method_deluser;
	methods->method_newgroup      = method_newgroup;
	methods->method_newuser       = method_newuser;
	methods->method_putgrent      = method_putgrent;
	methods->method_putgrusers    = method_putgrusers;
	methods->method_putpwent      = method_putpwent;
	methods->method_lock          = method_lock;
	methods->method_unlock        = method_unlock;
	methods->method_getcred       = method_getcred;
	methods->method_setcred       = method_setcred;
	methods->method_deletecred    = method_deletecred;
#endif

	return AUTH_SUCCESS;
}

