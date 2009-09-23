/*
 * cgi.c -- CGI processing (for the GoAhead Web server
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: cgi.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/********************************** Description *******************************/
/*
 *	This module implements the /cgi-bin handler. CGI processing differs from
 *	goforms processing in that each CGI request is executed as a separate 
 *	process, rather than within the webserver process. For each CGI request the
 *	environment of the new process must be set to include all the CGI variables
 *	and its standard input and output must be directed to the socket.  This
 *	is done using temporary files.
 */

/*********************************** Includes *********************************/
#include	"wsIntrn.h"
#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/************************************ Locals **********************************/
typedef struct {				/* Struct for CGI tasks which have completed */
	webs_t	wp;					/* pointer to session websRec */
	char_t	*stdIn;				/* file desc. for task's temp input fd */
	char_t	*stdOut;			/* file desc. for task's temp output fd */
	char_t	*cgiPath;			/* path to executable process file */
	char_t	**argp;				/* pointer to buf containing argv tokens */
	char_t	**envp;				/* pointer to array of environment strings */
	int		handle;				/* process handle of the task */
	long	fplacemark;			/* seek location for CGI output file */
} cgiRec;
static cgiRec	**cgiList;		/* hAlloc chain list of wp's to be closed */
static int		cgiMax;			/* Size of hAlloc list */

/************************************* Code ***********************************/

/*
 *	Process a form request. Returns 1 always to indicate it handled the URL
 */
int websCgiHandler(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, 
		char_t *url, char_t *path, char_t* query)
{
	cgiRec		*cgip;
	sym_t		*s;
	char_t		cgiBuf[FNAMESIZE], *stdIn, *stdOut, cwd[FNAMESIZE];
	char_t		*cp, *cgiName, *cgiPath, **argp, **envp, **ep;
	int			n, envpsize, argpsize, pHandle, cid;
	a_assert(websValid(wp));
	a_assert(url && *url);
	a_assert(path && *path == '/');
	websStats.cgiHits++;
/*
 *	Extract the form name and then build the full path name.  The form
 *	name will follow the first '/' in path.
 */
	gstrncpy(cgiBuf, path, TSZ(cgiBuf));
	if ((cgiName = gstrchr(&cgiBuf[1], '/')) == NULL) {
		websError(wp, 200, T("Missing CGI name"));
		return 1;
	}
	cgiName++;
	if ((cp = gstrchr(cgiName, '/')) != NULL) {
		*cp = '\0';
	}
	fmtAlloc(&cgiPath, FNAMESIZE, T("%s/%s/%s"), websGetDefaultDir(),
		CGI_BIN, cgiName);
#ifndef VXWORKS
/*
 *	See if the file exists and is executable.  If not error out.
 *	Don't do this step for VxWorks, since the module may already
 *	be part of the OS image, rather than in the file system.
 */
	{
		gstat_t		sbuf;
		if (gstat(cgiPath, &sbuf) != 0 || (sbuf.st_mode & S_IFREG) == 0) {
			websError(wp, 200, T("CGI process file does not exist"));
			bfree(B_L, cgiPath);
			return 1;
		}
#if (defined (WIN) || defined (CE))
		if (gstrstr(cgiPath, T(".exe")) == NULL &&
			gstrstr(cgiPath, T(".bat")) == NULL) {
#elif (defined (NW))
			if (gstrstr(cgiPath, T(".nlm")) == NULL) {
#else
		if (gaccess(cgiPath, X_OK) != 0) {
#endif /* WIN || CE */
			websError(wp, 200, T("CGI process file is not executable"));
			bfree(B_L, cgiPath);
			return 1;
		}
	}
#endif /* ! VXWORKS */

         
/*
 *	Get the CWD for resetting after launching the child process CGI
 */
	ggetcwd(cwd, FNAMESIZE);
/*
 *	Retrieve the directory of the child process CGI
 */
	if ((cp = gstrrchr(cgiPath, '/')) != NULL) {
		*cp = '\0';
		gchdir(cgiPath);
		*cp = '/';
	}
/*
 *	Build command line arguments.  Only used if there is no non-encoded
 *	= character.  This is indicative of a ISINDEX query.  POST separators
 *	are & and others are +.  argp will point to a balloc'd array of 
 *	pointers.  Each pointer will point to substring within the
 *	query string.  This array of string pointers is how the spawn or 
 *	exec routines expect command line arguments to be passed.  Since 
 *	we don't know ahead of time how many individual items there are in
 *	the query string, the for loop includes logic to grow the array 
 *	size via brealloc.
 */
	argpsize = 10;
	argp = balloc(B_L, argpsize * sizeof(char_t *));
	*argp = cgiPath;
	n = 1;
	if (gstrchr(query, '=') == NULL) {
		websDecodeUrl(query, query, gstrlen(query));
		for (cp = gstrtok(query, T(" ")); cp != NULL; ) {
			*(argp+n) = cp;
			n++;
			if (n >= argpsize) {
				argpsize *= 2;
				argp = brealloc(B_L, argp, argpsize * sizeof(char_t *));
			}
			cp = gstrtok(NULL, T(" "));
		}
	}
	*(argp+n) = NULL;
/*
 *	Add all CGI variables to the environment strings to be passed
 *	to the spawned CGI process.  This includes a few we don't 
 *	already have in the symbol table, plus all those that are in
 *	the cgiVars symbol table.  envp will point to a balloc'd array of 
 *	pointers.  Each pointer will point to a balloc'd string containing
 *	the keyword value pair in the form keyword=value.  Since we don't
 *	know ahead of time how many environment strings there will be the
 *	for loop includes logic to grow the array size via brealloc.
 */
	envpsize = WEBS_SYM_INIT;
	envp = balloc(B_L, envpsize * sizeof(char_t *));
	n = 0;
	fmtAlloc(envp+n, FNAMESIZE, T("%s=%s"),T("PATH_TRANSLATED"), cgiPath);
	n++;
	fmtAlloc(envp+n, FNAMESIZE, T("%s=%s/%s"),T("SCRIPT_NAME"),
		CGI_BIN, cgiName);
	n++;
	fmtAlloc(envp+n, FNAMESIZE, T("%s=%s"),T("REMOTE_USER"), wp->userName);
	n++;
	fmtAlloc(envp+n, FNAMESIZE, T("%s=%s"),T("AUTH_TYPE"), wp->authType);
	n++;
	for (s = symFirst(wp->cgiVars); s != NULL; s = symNext(wp->cgiVars)) {
		if (s->content.valid && s->content.type == vtype_string &&
			gstrcmp(s->name.value.string, T("REMOTE_HOST")) != 0 &&
			gstrcmp(s->name.value.string, T("HTTP_AUTHORIZATION")) != 0) {
			fmtAlloc(envp+n, FNAMESIZE, T("%s=%s"), s->name.value.string,
				s->content.value.string);
			n++;
			if (n >= envpsize) {
				envpsize *= 2;
				envp = brealloc(B_L, envp, envpsize * sizeof(char_t *));
			}
		}
	}
	*(envp+n) = NULL;
/*
 *	Create temporary file name(s) for the child's stdin and stdout.
 *	For POST data the stdin temp file (and name) should already exist.
 */
	if (wp->cgiStdin == NULL) {
		wp->cgiStdin = websGetCgiCommName();
	} 
	stdIn = wp->cgiStdin;
	stdOut = websGetCgiCommName();
/*
 *	Now launch the process.  If not successful, do the cleanup of resources.
 *	If successful, the cleanup will be done after the process completes.
 */
	if ((pHandle = websLaunchCgiProc(cgiPath, argp, envp, stdIn, stdOut)) 
		== -1) {
		websError(wp, 200, T("failed to spawn CGI task"));
		for (ep = envp; *ep != NULL; ep++) {
			bfreeSafe(B_L, *ep);
		}
		bfreeSafe(B_L, cgiPath);
		bfreeSafe(B_L, argp);
		bfreeSafe(B_L, envp);
		bfreeSafe(B_L, stdOut);
	} else {
/*
 *		If the spawn was successful, put this wp on a queue to be
 *		checked for completion.
 */
		cid = hAllocEntry((void***) &cgiList, &cgiMax, sizeof(cgiRec));
		cgip = cgiList[cid];
		cgip->handle = pHandle;
		cgip->stdIn = stdIn;
		cgip->stdOut = stdOut;
		cgip->cgiPath = cgiPath;
		cgip->argp = argp;
		cgip->envp = envp;
		cgip->wp = wp;
		cgip->fplacemark = 0;
		websTimeoutCancel(wp);
	}
/*
 *	Restore the current working directory after spawning child CGI
 */
 	gchdir(cwd);
	return 1;
}



/******************************************************************************/
/*
 *	Any entry in the cgiList need to be checked to see if it has
 */
void websCgiGatherOutput (cgiRec *cgip)
{
	gstat_t	sbuf;
	char_t	cgiBuf[FNAMESIZE];
	if ((gstat(cgip->stdOut, &sbuf) == 0) && 
		(sbuf.st_size > cgip->fplacemark)) {
		int fdout;
		fdout = gopen(cgip->stdOut, O_RDONLY | O_BINARY, 0444 );
/*
 *		Check to see if any data is available in the
 *		output file and send its contents to the socket.
 */
		if (fdout >= 0) {
			webs_t	wp = cgip->wp;
			int		nRead;
/*
 *			Write the HTTP header on our first pass
 */
			if (cgip->fplacemark == 0) {
				websWrite(wp, T("HTTP/1.0 200 OK\r\n"));
			}
			glseek(fdout, cgip->fplacemark, SEEK_SET);
			while ((nRead = gread(fdout, cgiBuf, FNAMESIZE)) > 0) {
				websWriteBlock(wp, cgiBuf, nRead);
				cgip->fplacemark += nRead;
			}
			gclose(fdout);
		}
	}
}



/******************************************************************************/
/*
 *	Any entry in the cgiList need to be checked to see if it has
 *	completed, and if so, process its output and clean up.
 */
void websCgiCleanup()
{
	cgiRec	*cgip;
	webs_t	wp;
	char_t	**ep;
	int		cid, nTries;
	for (cid = 0; cid < cgiMax; cid++) {
		if ((cgip = cgiList[cid]) != NULL) {
			wp = cgip->wp;
			websCgiGatherOutput (cgip);
			if (websCheckCgiProc(cgip->handle) == 0) {
/*
 *				We get here if the CGI process has terminated.  Clean up.
 */
				nTries = 0;
/*				
 *				Make sure we didn't miss something during a task switch.
 *				Maximum wait is 100 times 10 msecs (1 second).
 */
				while ((cgip->fplacemark == 0) && (nTries < 100)) {
					websCgiGatherOutput(cgip);
/*					
 *					There are some cases when we detect app exit 
 *					before the file is ready. 
 */
					if (cgip->fplacemark == 0) {
#ifdef WIN
						Sleep(10);
#endif /* WIN*/
					}
					nTries++;
				}
				if (cgip->fplacemark == 0) {
					websError(wp, 200, T("CGI generated no output"));
				} else {
					websDone(wp, 200);
				}
/*
 *				Remove the temporary re-direction files
 */
				gunlink(cgip->stdIn);
				gunlink(cgip->stdOut);
/*
 *				Free all the memory buffers pointed to by cgip.
 *				The stdin file name (wp->cgiStdin) gets freed as
 *				part of websFree().
 */
				cgiMax = hFree((void***) &cgiList, cid);
				for (ep = cgip->envp; ep != NULL && *ep != NULL; ep++) {
					bfreeSafe(B_L, *ep);
				}
				bfreeSafe(B_L, cgip->cgiPath);
				bfreeSafe(B_L, cgip->argp);
				bfreeSafe(B_L, cgip->envp);
				bfreeSafe(B_L, cgip->stdOut);
				bfreeSafe(B_L, cgip);
			}
		}
	}
}
/******************************************************************************/

