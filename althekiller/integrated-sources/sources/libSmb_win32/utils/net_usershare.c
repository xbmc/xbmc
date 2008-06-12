/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 

   Copyright (C) Jeremy Allison (jra@samba.org) 2005

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"
#include "utils/net.h"

struct {
	const char *us_errstr;
	enum usershare_err us_err;
} us_errs [] = {
	{"",USERSHARE_OK},
	{"Malformed usershare file", USERSHARE_MALFORMED_FILE},
	{"Bad version number", USERSHARE_BAD_VERSION},
	{"Malformed path entry", USERSHARE_MALFORMED_PATH},
	{"Malformed comment entryfile", USERSHARE_MALFORMED_COMMENT_DEF},
	{"Malformed acl definition", USERSHARE_MALFORMED_ACL_DEF},
	{"Acl parse error", USERSHARE_ACL_ERR},
	{"Path not absolute", USERSHARE_PATH_NOT_ABSOLUTE},
	{"Path is denied", USERSHARE_PATH_IS_DENIED},
	{"Path not allowed", USERSHARE_PATH_NOT_ALLOWED},
	{"Path is not a directory", USERSHARE_PATH_NOT_DIRECTORY},
	{"System error", USERSHARE_POSIX_ERR},
	{NULL,(enum usershare_err)-1}
};

static const char *get_us_error_code(enum usershare_err us_err)
{
	static pstring out;
	int idx = 0;

	while (us_errs[idx].us_errstr != NULL) {
		if (us_errs[idx].us_err == us_err) {
			return us_errs[idx].us_errstr;
		}
		idx++;
	}

	slprintf(out, sizeof(out), "Usershare error code (0x%x)", (unsigned int)us_err);
	return out;
}

/* The help subsystem for the USERSHARE subcommand */

static int net_usershare_add_usage(int argc, const char **argv)
{
	char c = *lp_winbind_separator();
	d_printf(
		"net usershare add [-l|--long] <sharename> <path> [<comment>] [<acl>] [<guest_ok=[y|n]>]\n"
		"\tAdds the specified share name for this user.\n"
		"\t<sharename> is the new share name.\n"
		"\t<path> is the path on the filesystem to export.\n"
		"\t<comment> is the optional comment for the new share.\n"
		"\t<acl> is an optional share acl in the format \"DOMAIN%cname:X,DOMAIN%cname:X,....\"\n"
		"\t<guest_ok=y> if present sets \"guest ok = yes\" on this usershare.\n"
		"\t\t\"X\" represents a permission and can be any one of the characters f, r or d\n"
		"\t\twhere \"f\" means full control, \"r\" means read-only, \"d\" means deny access.\n"
		"\t\tname may be a domain user or group. For local users use the local server name "
		"instead of \"DOMAIN\"\n"
		"\t\tThe default acl is \"Everyone:r\" which allows everyone read-only access.\n"
		"\tAdd -l or --long to print the info on the newly added share.\n",
		c, c );
	return -1;
}

static int net_usershare_delete_usage(int argc, const char **argv)
{
	d_printf(
		"net usershare delete <sharename>\n"\
		"\tdeletes the specified share name for this user.\n");
	return -1;
}

static int net_usershare_info_usage(int argc, const char **argv)
{
	d_printf(
		"net usershare info [-l|--long] [wildcard sharename]\n"\
		"\tPrints out the path, comment and acl elements of shares that match the wildcard.\n"
		"\tBy default only gives info on shares owned by the current user\n"
		"\tAdd -l or --long to apply this to all shares\n"
		"\tOmit the sharename or use a wildcard of '*' to see all shares\n");
	return -1;
}

static int net_usershare_list_usage(int argc, const char **argv)
{
	d_printf(
		"net usershare list [-l|--long] [wildcard sharename]\n"\
		"\tLists the names of all shares that match the wildcard.\n"
		"\tBy default only lists shares owned by the current user\n"
		"\tAdd -l or --long to apply this to all shares\n"
		"\tOmit the sharename or use a wildcard of '*' to see all shares\n");
	return -1;
}

int net_usershare_usage(int argc, const char **argv)
{
	d_printf("net usershare add <sharename> <path> [<comment>] [<acl>] [<guest_ok=[y|n]>] to "
				"add or change a user defined share.\n"
		"net usershare delete <sharename> to delete a user defined share.\n"
		"net usershare info [-l|--long] [wildcard sharename] to print info about a user defined share.\n"
		"net usershare list [-l|--long] [wildcard sharename] to list user defined shares.\n"
		"net usershare help\n"\
		"\nType \"net usershare help <option>\" to get more information on that option\n\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

/***************************************************************************
***************************************************************************/

static void get_basepath(pstring basepath)
{
	pstrcpy(basepath, lp_usershare_path());
	if ((basepath[0] != '\0') && (basepath[strlen(basepath)-1] == '/')) {
		basepath[strlen(basepath)-1] = '\0';
	}
}

/***************************************************************************
 Delete a single userlevel share.
***************************************************************************/

static int net_usershare_delete(int argc, const char **argv)
{
	pstring us_path;
	char *sharename;

	if (argc != 1) {
		return net_usershare_delete_usage(argc, argv);
	}

	if ((sharename = strdup_lower(argv[0])) == NULL) {
		d_fprintf(stderr, "strdup failed\n");
		return -1;
	}

	if (!validate_net_name(sharename, INVALID_SHARENAME_CHARS, strlen(sharename))) {
		d_fprintf(stderr, "net usershare delete: share name %s contains "
                        "invalid characters (any of %s)\n",
                        sharename, INVALID_SHARENAME_CHARS);
		SAFE_FREE(sharename);
		return -1;
	}

	pstrcpy(us_path, lp_usershare_path());
	pstrcat(us_path, "/");
	pstrcat(us_path, sharename);

	if (unlink(us_path) != 0) {
		d_fprintf(stderr, "net usershare delete: unable to remove usershare %s. "
			"Error was %s\n",
                        us_path, strerror(errno));
		SAFE_FREE(sharename);
		return -1;
	}
	SAFE_FREE(sharename);
	return 0;
}

/***************************************************************************
 Data structures to handle a list of usershare files.
***************************************************************************/

struct file_list {
	struct file_list *next, *prev;
	const char *pathname;
};

static struct file_list *flist;

/***************************************************************************
***************************************************************************/

static int get_share_list(TALLOC_CTX *ctx, const char *wcard, BOOL only_ours)
{
	SMB_STRUCT_DIR *dp;
	SMB_STRUCT_DIRENT *de;
	uid_t myuid = geteuid();
	struct file_list *fl = NULL;
	pstring basepath;

	get_basepath(basepath);
	dp = sys_opendir(basepath);
	if (!dp) {
		d_fprintf(stderr, "get_share_list: cannot open usershare directory %s. Error %s\n",
			basepath, strerror(errno) );
		return -1;
	}

	while((de = sys_readdir(dp)) != 0) {
		SMB_STRUCT_STAT sbuf;
		pstring path;
		const char *n = de->d_name;

		/* Ignore . and .. */
		if (*n == '.') {
			if ((n[1] == '\0') || (n[1] == '.' && n[2] == '\0')) {
				continue;
			}
		}

		if (!validate_net_name(n, INVALID_SHARENAME_CHARS, strlen(n))) {
			d_fprintf(stderr, "get_share_list: ignoring bad share name %s\n",n);
			continue;
		}
		pstrcpy(path, basepath);
		pstrcat(path, "/");
		pstrcat(path, n);

		if (sys_lstat(path, &sbuf) != 0) {
			d_fprintf(stderr, "get_share_list: can't lstat file %s. Error was %s\n",
				path, strerror(errno) );
			continue;
		}

		if (!S_ISREG(sbuf.st_mode)) {
			d_fprintf(stderr, "get_share_list: file %s is not a regular file. Ignoring.\n",
				path );
			continue;
		}

		if (only_ours && sbuf.st_uid != myuid) {
			continue;
		}

		if (!unix_wild_match(wcard, n)) {
			continue;
		}

		/* (Finally) - add to list. */ 
		fl = TALLOC_P(ctx, struct file_list);
		if (!fl) {
			return -1;
		}
		fl->pathname = talloc_strdup(ctx, n);
		if (!fl->pathname) {
			return -1;
		}

		DLIST_ADD(flist, fl);
	}

	sys_closedir(dp);
	return 0;
}

enum us_priv_op { US_LIST_OP, US_INFO_OP};

struct us_priv_info {
	TALLOC_CTX *ctx;
	enum us_priv_op op;
};

/***************************************************************************
 Call a function for every share on the list.
***************************************************************************/

static int process_share_list(int (*fn)(struct file_list *, void *), void *priv)
{
	struct file_list *fl;
	int ret = 0;

	for (fl = flist; fl; fl = fl->next) {
		ret = (*fn)(fl, priv);
	}

	return ret;
}

/***************************************************************************
 Info function.
***************************************************************************/

static int info_fn(struct file_list *fl, void *priv)
{
	SMB_STRUCT_STAT sbuf;
	char **lines = NULL;
	struct us_priv_info *pi = (struct us_priv_info *)priv;
	TALLOC_CTX *ctx = pi->ctx;
	int fd = -1;
	int numlines = 0;
	SEC_DESC *psd = NULL;
	pstring basepath;
	pstring sharepath;
	pstring comment;
	pstring acl_str;
	int num_aces;
	char sep_str[2];
	enum usershare_err us_err;
	BOOL guest_ok = False;

	sep_str[0] = *lp_winbind_separator();
	sep_str[1] = '\0';

	get_basepath(basepath);
	pstrcat(basepath, "/");
	pstrcat(basepath, fl->pathname);

#ifdef O_NOFOLLOW
	fd = sys_open(basepath, O_RDONLY|O_NOFOLLOW, 0);
#else
	fd = sys_open(basepath, O_RDONLY, 0);
#endif

	if (fd == -1) {
		d_fprintf(stderr, "info_fn: unable to open %s. %s\n",
                        basepath, strerror(errno) );
                return -1;
        }

	/* Paranoia... */
	if (sys_fstat(fd, &sbuf) != 0) {
		d_fprintf(stderr, "info_fn: can't fstat file %s. Error was %s\n",
			basepath, strerror(errno) );
		close(fd);
		return -1;
	}

	if (!S_ISREG(sbuf.st_mode)) {
		d_fprintf(stderr, "info_fn: file %s is not a regular file. Ignoring.\n",
			basepath );
		close(fd);
		return -1;
	}

	lines = fd_lines_load(fd, &numlines, 10240);
	close(fd);

	if (lines == NULL) {
		return -1;
	}

	/* Ensure it's well formed. */
	us_err = parse_usershare_file(ctx, &sbuf, fl->pathname, -1, lines, numlines,
				sharepath,
				comment,
				&psd,
				&guest_ok);

	file_lines_free(lines);

	if (us_err != USERSHARE_OK) {
		d_fprintf(stderr, "info_fn: file %s is not a well formed usershare file.\n",
			basepath );
		d_fprintf(stderr, "info_fn: Error was %s.\n",
			get_us_error_code(us_err) );
		return -1;
	}

	pstrcpy(acl_str, "usershare_acl=");

	for (num_aces = 0; num_aces < psd->dacl->num_aces; num_aces++) {
		const char *domain;
		const char *name;
		NTSTATUS ntstatus;

		ntstatus = net_lookup_name_from_sid(ctx, &psd->dacl->ace[num_aces].trustee, &domain, &name);

		if (NT_STATUS_IS_OK(ntstatus)) {
			if (domain && *domain) {
				pstrcat(acl_str, domain);
				pstrcat(acl_str, sep_str);
			}
			pstrcat(acl_str,name);
		} else {
			fstring sidstr;
			sid_to_string(sidstr, &psd->dacl->ace[num_aces].trustee);
			pstrcat(acl_str,sidstr);
		}
		pstrcat(acl_str, ":");

		if (psd->dacl->ace[num_aces].type == SEC_ACE_TYPE_ACCESS_DENIED) {
			pstrcat(acl_str, "D,");
		} else {
			if (psd->dacl->ace[num_aces].info.mask & GENERIC_ALL_ACCESS) {
				pstrcat(acl_str, "F,");
			} else {
				pstrcat(acl_str, "R,");
			}
		}
	}

	acl_str[strlen(acl_str)-1] = '\0';

	if (pi->op == US_INFO_OP) {
		d_printf("[%s]\n", fl->pathname );
		d_printf("path=%s\n", sharepath );
		d_printf("comment=%s\n", comment);
		d_printf("%s\n", acl_str);
		d_printf("guest_ok=%c\n\n", guest_ok ? 'y' : 'n');
	} else if (pi->op == US_LIST_OP) {
		d_printf("%s\n", fl->pathname);
	}

	return 0;
}

/***************************************************************************
 Print out info (internal detail) on userlevel shares.
***************************************************************************/

static int net_usershare_info(int argc, const char **argv)
{
	fstring wcard;
	BOOL only_ours = True;
	int ret = -1;
	struct us_priv_info pi;
	TALLOC_CTX *ctx;

	fstrcpy(wcard, "*");

	if (opt_long_list_entries) {
		only_ours = False;
	}

	switch (argc) {
		case 0:
			break;
		case 1:
			fstrcpy(wcard, argv[0]);
			break;
		default:
			return net_usershare_info_usage(argc, argv);
	}

	strlower_m(wcard);

	ctx = talloc_init("share_info");
	ret = get_share_list(ctx, wcard, only_ours);
	if (ret) {
		return ret;
	}

	pi.ctx = ctx;
	pi.op = US_INFO_OP;

	ret = process_share_list(info_fn, &pi);
	talloc_destroy(ctx);
	return ret;
}

/***************************************************************************
 Count the current total number of usershares.
***************************************************************************/

static int count_num_usershares(void)
{
	SMB_STRUCT_DIR *dp;
	SMB_STRUCT_DIRENT *de;
	pstring basepath;
	int num_usershares = 0;

	get_basepath(basepath);
	dp = sys_opendir(basepath);
	if (!dp) {
		d_fprintf(stderr, "count_num_usershares: cannot open usershare directory %s. Error %s\n",
			basepath, strerror(errno) );
		return -1;
	}

	while((de = sys_readdir(dp)) != 0) {
		SMB_STRUCT_STAT sbuf;
		pstring path;
		const char *n = de->d_name;

		/* Ignore . and .. */
		if (*n == '.') {
			if ((n[1] == '\0') || (n[1] == '.' && n[2] == '\0')) {
				continue;
			}
		}

		if (!validate_net_name(n, INVALID_SHARENAME_CHARS, strlen(n))) {
			d_fprintf(stderr, "count_num_usershares: ignoring bad share name %s\n",n);
			continue;
		}
		pstrcpy(path, basepath);
		pstrcat(path, "/");
		pstrcat(path, n);

		if (sys_lstat(path, &sbuf) != 0) {
			d_fprintf(stderr, "count_num_usershares: can't lstat file %s. Error was %s\n",
				path, strerror(errno) );
			continue;
		}

		if (!S_ISREG(sbuf.st_mode)) {
			d_fprintf(stderr, "count_num_usershares: file %s is not a regular file. Ignoring.\n",
				path );
			continue;
		}
		num_usershares++;
	}

	sys_closedir(dp);
	return num_usershares;
}

/***************************************************************************
 Add a single userlevel share.
***************************************************************************/

static int net_usershare_add(int argc, const char **argv)
{
	TALLOC_CTX *ctx = NULL;
	SMB_STRUCT_STAT sbuf;
	SMB_STRUCT_STAT lsbuf;
	char *sharename;
	pstring full_path;
	pstring full_path_tmp;
	const char *us_path;
	const char *us_comment;
	const char *arg_acl;
	char *us_acl;
	char *file_img;
	int num_aces = 0;
	int i;
	int tmpfd;
	const char *pacl;
	size_t to_write;
	uid_t myeuid = geteuid();
	BOOL guest_ok = False;
	int num_usershares;

	us_comment = "";
	arg_acl = "S-1-1-0:R";

	switch (argc) {
		case 0:
		case 1:
		default:
			return net_usershare_add_usage(argc, argv);
		case 2:
			sharename = strdup_lower(argv[0]);
			us_path = argv[1];
			break;
		case 3:
			sharename = strdup_lower(argv[0]);
			us_path = argv[1];
			us_comment = argv[2];
			break;
		case 4:
			sharename = strdup_lower(argv[0]);
			us_path = argv[1];
			us_comment = argv[2];
			arg_acl = argv[3];
			break;
		case 5:
			sharename = strdup_lower(argv[0]);
			us_path = argv[1];
			us_comment = argv[2];
			arg_acl = argv[3];
			if (!strnequal(argv[4], "guest_ok=", 9)) {
				return net_usershare_add_usage(argc, argv);
			}
			switch (argv[4][9]) {
				case 'y':
				case 'Y':
					guest_ok = True;
					break;
				case 'n':
				case 'N':
					guest_ok = False;
					break;
				default: 
					return net_usershare_add_usage(argc, argv);
			}
			break;
	}

	/* Ensure we're under the "usershare max shares" number. Advisory only. */
	num_usershares = count_num_usershares();
	if (num_usershares > lp_usershare_max_shares()) {
		d_fprintf(stderr, "net usershare add: too many usershares already defined (%d), "
			"maximum number allowed is %d.\n",
			num_usershares, lp_usershare_max_shares() );
		SAFE_FREE(sharename);
		return -1;
	}

	if (!validate_net_name(sharename, INVALID_SHARENAME_CHARS, strlen(sharename))) {
		d_fprintf(stderr, "net usershare add: share name %s contains "
                        "invalid characters (any of %s)\n",
                        sharename, INVALID_SHARENAME_CHARS);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Disallow shares the same as users. */
	if (getpwnam(sharename)) {
		d_fprintf(stderr, "net usershare add: share name %s is already a valid system user name\n",
			sharename );
		SAFE_FREE(sharename);
		return -1;
	}

	/* Construct the full path for the usershare file. */
	get_basepath(full_path);
	pstrcat(full_path, "/");
	pstrcpy(full_path_tmp, full_path);
	pstrcat(full_path, sharename);
	pstrcat(full_path_tmp, ":tmpXXXXXX");

	/* The path *must* be absolute. */
	if (us_path[0] != '/') {
		d_fprintf(stderr,"net usershare add: path %s is not an absolute path.\n",
			us_path);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Check the directory to be shared exists. */
	if (sys_stat(us_path, &sbuf) != 0) {
		d_fprintf(stderr, "net usershare add: cannot stat path %s to ensure "
			"this is a directory. Error was %s\n",
			us_path, strerror(errno) );
		SAFE_FREE(sharename);
		return -1;
	}

	if (!S_ISDIR(sbuf.st_mode)) {
		d_fprintf(stderr, "net usershare add: path %s is not a directory.\n",
			us_path );
		SAFE_FREE(sharename);
		return -1;
	}

	/* If we're not root, check if we're restricted to sharing out directories
	   that we own only. */

	if ((myeuid != 0) && lp_usershare_owner_only() && (myeuid != sbuf.st_uid)) {
		d_fprintf(stderr, "net usershare add: cannot share path %s as "
			"we are restricted to only sharing directories we own.\n"
			"\tAsk the administrator to add the line \"usershare owner only = False\" \n"
			"\tto the [global] section of the smb.conf to allow this.\n",
			us_path );
		SAFE_FREE(sharename);
		return -1;
	}

	/* No validation needed on comment. Now go through and validate the
	   acl string. Convert names to SID's as needed. Then run it through
	   parse_usershare_acl to ensure it's valid. */

	ctx = talloc_init("share_info");

	/* Start off the string we'll append to. */
	us_acl = talloc_strdup(ctx, "");

	pacl = arg_acl;
	num_aces = 1;

	/* Add the number of ',' characters to get the number of aces. */
	num_aces += count_chars(pacl,',');

	for (i = 0; i < num_aces; i++) {
		DOM_SID sid;
		const char *pcolon = strchr_m(pacl, ':');
		const char *name;

		if (pcolon == NULL) {
			d_fprintf(stderr, "net usershare add: malformed acl %s (missing ':').\n",
				pacl );
			talloc_destroy(ctx);
			SAFE_FREE(sharename);
			return -1;
		}

		switch(pcolon[1]) {
			case 'f':
			case 'F':
			case 'd':
			case 'r':
			case 'R':
				break;
			default:
				d_fprintf(stderr, "net usershare add: malformed acl %s "
					"(access control must be 'r', 'f', or 'd')\n",
					pacl );
				talloc_destroy(ctx);
				SAFE_FREE(sharename);
				return -1;
		}

		if (pcolon[2] != ',' && pcolon[2] != '\0') {
			d_fprintf(stderr, "net usershare add: malformed terminating character for acl %s\n",
				pacl );
			talloc_destroy(ctx);
			SAFE_FREE(sharename);
			return -1;
		}

		/* Get the name */
		if ((name = talloc_strndup(ctx, pacl, pcolon - pacl)) == NULL) {
			d_fprintf(stderr, "talloc_strndup failed\n");
			talloc_destroy(ctx);
			SAFE_FREE(sharename);
			return -1;
		}
		if (!string_to_sid(&sid, name)) {
			/* Convert to a SID */
			NTSTATUS ntstatus = net_lookup_sid_from_name(ctx, name, &sid);
			if (!NT_STATUS_IS_OK(ntstatus)) {
				d_fprintf(stderr, "net usershare add: cannot convert name \"%s\" to a SID. %s.",
					name, get_friendly_nt_error_msg(ntstatus) );
				if (NT_STATUS_EQUAL(ntstatus, NT_STATUS_CONNECTION_REFUSED)) {
					d_fprintf(stderr,  " Maybe smbd is not running.\n");
				} else {
					d_fprintf(stderr, "\n");
				}
				talloc_destroy(ctx);
				SAFE_FREE(sharename);
				return -1;
			}
		}
		us_acl = talloc_asprintf_append(us_acl, "%s:%c,", sid_string_static(&sid), pcolon[1]);

		/* Move to the next ACL entry. */
		if (pcolon[2] == ',') {
			pacl = &pcolon[3];
		}
	}

	/* Remove the last ',' */
	us_acl[strlen(us_acl)-1] = '\0';

	if (guest_ok && !lp_usershare_allow_guests()) {
		d_fprintf(stderr, "net usershare add: guest_ok=y requested "
			"but the \"usershare allow guests\" parameter is not enabled "
			"by this server.\n");
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Create a temporary filename for this share. */
	tmpfd = smb_mkstemp(full_path_tmp);

	if (tmpfd == -1) {
		d_fprintf(stderr, "net usershare add: cannot create tmp file %s\n",
				full_path_tmp );
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Ensure we opened the file we thought we did. */
	if (sys_lstat(full_path_tmp, &lsbuf) != 0) {
		d_fprintf(stderr, "net usershare add: cannot lstat tmp file %s\n",
				full_path_tmp );
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Check this is the same as the file we opened. */
	if (sys_fstat(tmpfd, &sbuf) != 0) {
		d_fprintf(stderr, "net usershare add: cannot fstat tmp file %s\n",
				full_path_tmp );
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	if (!S_ISREG(sbuf.st_mode) || sbuf.st_dev != lsbuf.st_dev || sbuf.st_ino != lsbuf.st_ino) {
		d_fprintf(stderr, "net usershare add: tmp file %s is not a regular file ?\n",
				full_path_tmp );
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}
	
	if (fchmod(tmpfd, 0644) == -1) {
		d_fprintf(stderr, "net usershare add: failed to fchmod tmp file %s to 0644n",
				full_path_tmp );
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Create the in-memory image of the file. */
	file_img = talloc_strdup(ctx, "#VERSION 2\npath=");
	file_img = talloc_asprintf_append(file_img, "%s\ncomment=%s\nusershare_acl=%s\nguest_ok=%c\n",
			us_path, us_comment, us_acl, guest_ok ? 'y' : 'n');

	to_write = strlen(file_img);

	if (write(tmpfd, file_img, to_write) != to_write) {
		d_fprintf(stderr, "net usershare add: failed to write %u bytes to file %s. Error was %s\n",
			(unsigned int)to_write, full_path_tmp, strerror(errno));
		unlink(full_path_tmp);
		talloc_destroy(ctx);
		SAFE_FREE(sharename);
		return -1;
	}

	/* Attempt to replace any existing share by this name. */
	if (rename(full_path_tmp, full_path) != 0) {
		unlink(full_path_tmp);
		d_fprintf(stderr, "net usershare add: failed to add share %s. Error was %s\n",
			sharename, strerror(errno));
		talloc_destroy(ctx);
		close(tmpfd);
		SAFE_FREE(sharename);
		return -1;
	}

	close(tmpfd);
	talloc_destroy(ctx);

	if (opt_long_list_entries) {
		const char *my_argv[2];
		my_argv[0] = sharename;
		my_argv[1] = NULL;
		net_usershare_info(1, my_argv);
	}

	SAFE_FREE(sharename);
	return 0;
}

#if 0
/***************************************************************************
 List function.
***************************************************************************/

static int list_fn(struct file_list *fl, void *priv)
{
	d_printf("%s\n", fl->pathname);
	return 0;
}
#endif

/***************************************************************************
 List userlevel shares.
***************************************************************************/

static int net_usershare_list(int argc, const char **argv)
{
	fstring wcard;
	BOOL only_ours = True;
	int ret = -1;
	struct us_priv_info pi;
	TALLOC_CTX *ctx;

	fstrcpy(wcard, "*");

	if (opt_long_list_entries) {
		only_ours = False;
	}

	switch (argc) {
		case 0:
			break;
		case 1:
			fstrcpy(wcard, argv[0]);
			break;
		default:
			return net_usershare_list_usage(argc, argv);
	}

	strlower_m(wcard);

	ctx = talloc_init("share_list");
	ret = get_share_list(ctx, wcard, only_ours);
	if (ret) {
		return ret;
	}

	pi.ctx = ctx;
	pi.op = US_LIST_OP;

	ret = process_share_list(info_fn, &pi);
	talloc_destroy(ctx);
	return ret;
}

/***************************************************************************
 Handle "net usershare help *" subcommands.
***************************************************************************/

int net_usershare_help(int argc, const char **argv)
{
	struct functable func[] = {
		{"ADD", net_usershare_add_usage},
		{"DELETE", net_usershare_delete_usage},
		{"INFO", net_usershare_info_usage},
		{"LIST", net_usershare_list_usage},
		{NULL, NULL}};

	return net_run_function(argc, argv, func, net_usershare_usage);
}

/***************************************************************************
 Entry-point for all the USERSHARE functions.
***************************************************************************/

int net_usershare(int argc, const char **argv)
{
	SMB_STRUCT_DIR *dp;

	struct functable func[] = {
		{"ADD", net_usershare_add},
		{"DELETE", net_usershare_delete},
		{"INFO", net_usershare_info},
		{"LIST", net_usershare_list},
		{"HELP", net_usershare_help},
		{NULL, NULL}
	};
	
	if (lp_usershare_max_shares() == 0) {
		d_fprintf(stderr, "net usershare: usershares are currently disabled\n");
		return -1;
	}

	dp = sys_opendir(lp_usershare_path());
	if (!dp) {
		int err = errno;
		d_fprintf(stderr, "net usershare: cannot open usershare directory %s. Error %s\n",
			lp_usershare_path(), strerror(err) );
		if (err == EACCES) {
			d_fprintf(stderr, "You do not have permission to create a usershare. Ask your "
				"administrator to grant you permissions to create a share.\n");
		} else if (err == ENOENT) {
			d_fprintf(stderr, "Please ask your system administrator to "
				"enable user sharing.\n");
		}
		return -1;
	}
	sys_closedir(dp);

	return net_run_function(argc, argv, func, net_usershare_usage);
}
