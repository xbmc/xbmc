/** \ingroup popt
 * \file popt/poptconfig.c
 */

/* (C) 1998-2002 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

#include "system.h"
#include "poptint.h"

/*@-compmempass@*/	/* FIX: item->option.longName kept, not dependent. */
static void configLine(poptContext con, char * line)
	/*@modifies con @*/
{
    /*@-type@*/
    int nameLength = strlen(con->appName);
    /*@=type@*/
    const char * entryType;
    const char * opt;
    poptItem item = alloca(sizeof(*item));
    int i, j;
    
/*@-boundswrite@*/
    memset(item, 0, sizeof(*item));

    /*@-type@*/
    if (strncmp(line, con->appName, nameLength)) return;
    /*@=type@*/

    line += nameLength;
    if (*line == '\0' || !isspace(*line)) return;

    while (*line != '\0' && isspace(*line)) line++;
    entryType = line;
    while (*line == '\0' || !isspace(*line)) line++;
    *line++ = '\0';

    while (*line != '\0' && isspace(*line)) line++;
    if (*line == '\0') return;
    opt = line;
    while (*line == '\0' || !isspace(*line)) line++;
    *line++ = '\0';

    while (*line != '\0' && isspace(*line)) line++;
    if (*line == '\0') return;

    /*@-temptrans@*/ /* FIX: line alias is saved */
    if (opt[0] == '-' && opt[1] == '-')
	item->option.longName = opt + 2;
    else if (opt[0] == '-' && opt[2] == '\0')
	item->option.shortName = opt[1];
    /*@=temptrans@*/

    if (poptParseArgvString(line, &item->argc, &item->argv)) return;

    /*@-modobserver@*/
    item->option.argInfo = POPT_ARGFLAG_DOC_HIDDEN;
    for (i = 0, j = 0; i < item->argc; i++, j++) {
	const char * f;
	if (!strncmp(item->argv[i], "--POPTdesc=", sizeof("--POPTdesc=")-1)) {
	    f = item->argv[i] + sizeof("--POPTdesc=");
	    if (f[0] == '$' && f[1] == '"') f++;
	    item->option.descrip = f;
	    item->option.argInfo &= ~POPT_ARGFLAG_DOC_HIDDEN;
	    j--;
	} else
	if (!strncmp(item->argv[i], "--POPTargs=", sizeof("--POPTargs=")-1)) {
	    f = item->argv[i] + sizeof("--POPTargs=");
	    if (f[0] == '$' && f[1] == '"') f++;
	    item->option.argDescrip = f;
	    item->option.argInfo &= ~POPT_ARGFLAG_DOC_HIDDEN;
	    item->option.argInfo |= POPT_ARG_STRING;
	    j--;
	} else
	if (j != i)
	    item->argv[j] = item->argv[i];
    }
    if (j != i) {
	item->argv[j] = NULL;
	item->argc = j;
    }
    /*@=modobserver@*/
/*@=boundswrite@*/

    /*@-nullstate@*/ /* FIX: item->argv[] may be NULL */
    if (!strcmp(entryType, "alias"))
	(void) poptAddItem(con, item, 0);
    else if (!strcmp(entryType, "exec"))
	(void) poptAddItem(con, item, 1);
    /*@=nullstate@*/
}
/*@=compmempass@*/

int poptReadConfigFile(poptContext con, const char * fn)
{
    const char * file, * chptr, * end;
    char * buf;
/*@dependent@*/ char * dst;
    int fd, rc;
    off_t fileLength;

    fd = open(fn, O_RDONLY);
    if (fd < 0)
	return (errno == ENOENT ? 0 : POPT_ERROR_ERRNO);

    fileLength = lseek(fd, 0, SEEK_END);
    if (fileLength == -1 || lseek(fd, 0, 0) == -1) {
	rc = errno;
	(void) close(fd);
	/*@-mods@*/
	errno = rc;
	/*@=mods@*/
	return POPT_ERROR_ERRNO;
    }

    file = alloca(fileLength + 1);
    if (read(fd, (char *)file, fileLength) != fileLength) {
	rc = errno;
	(void) close(fd);
	/*@-mods@*/
	errno = rc;
	/*@=mods@*/
	return POPT_ERROR_ERRNO;
    }
    if (close(fd) == -1)
	return POPT_ERROR_ERRNO;

/*@-boundswrite@*/
    dst = buf = alloca(fileLength + 1);

    chptr = file;
    end = (file + fileLength);
    /*@-infloops@*/	/* LCL: can't detect chptr++ */
    while (chptr < end) {
	switch (*chptr) {
	  case '\n':
	    *dst = '\0';
	    dst = buf;
	    while (*dst && isspace(*dst)) dst++;
	    if (*dst && *dst != '#')
		configLine(con, dst);
	    chptr++;
	    /*@switchbreak@*/ break;
	  case '\\':
	    *dst++ = *chptr++;
	    if (chptr < end) {
		if (*chptr == '\n') 
		    dst--, chptr++;	
		    /* \ at the end of a line does not insert a \n */
		else
		    *dst++ = *chptr++;
	    }
	    /*@switchbreak@*/ break;
	  default:
	    *dst++ = *chptr++;
	    /*@switchbreak@*/ break;
	}
    }
    /*@=infloops@*/
/*@=boundswrite@*/

    return 0;
}

int poptReadDefaultConfig(poptContext con, /*@unused@*/ int useEnv)
{
    char * fn, * home;
    int rc;

    /*@-type@*/
    if (!con->appName) return 0;
    /*@=type@*/

    rc = poptReadConfigFile(con, "/etc/popt");
    if (rc) return rc;
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
    if (getuid() != geteuid()) return 0;
#endif

    if ((home = getenv("HOME"))) {
	fn = alloca(strlen(home) + 20);
	strcpy(fn, home);
	strcat(fn, "/.popt");
	rc = poptReadConfigFile(con, fn);
	if (rc) return rc;
    }

    return 0;
}
