///
///	@file 	dsi.c
/// @brief 	Development side includes
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
///
///	usage:  dsiPatch [-I incDir] [< fileList] [files ...]
///
////////////////////////////////// Includes ////////////////////////////////////

#include	<ctype.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#if WIN || _WIN32
	#include	<direct.h>
	#include	<io.h>
#else
	#include	<libgen.h>
	#include	<unistd.h>
#endif

#include	"getopt.h"

#include	"posixRemap.h"

////////////////////////////////// Defines /////////////////////////////////////

#define	MAX_LINE	(64 * 1024)
#define	MAX_FNAME	(4096)

#if WIN || _WIN32
#define TEXT_FMT	"b"
#else
#define TEXT_FMT	""
#endif

/////////////////////////////////// Locals /////////////////////////////////////

static int		finished;
static int		verbose;
static char		parentDir[MAX_FNAME];
static char		*incDir;

///////////////////////////// Forward Declarations /////////////////////////////

static void		openSignals();
static void		catchInterrupt(int signo);
static int 		patchFileList(FILE *fp);
static int 		patch(char *path);
static char 	*getNextTok(char *tokBuf, int tokBufSize, char *start);
static int 		matchLink(FILE *ofp, char *tag, char **start);

///////////////////////////////////// Code /////////////////////////////////////

int main(int argc, char *argv[])
{
	int 		errors, c, i;

	verbose = errors = 0;
	incDir = ".";

	while ((c = getopt(argc, argv, "I:v?")) != EOF) {
		switch(c) {
		case 'I':
			incDir = optarg;
			break;

		case 'v':
			verbose++;
			break;

		case '?':
			errors++;
			break;
		}
	}
	if (errors) {
		fprintf(stderr, 
			"dsi: usage: [-I incDir] [< fileList] files ...\n");
		exit(2);
	}

	openSignals();

	if (optind >= argc) {
		patchFileList(stdin);
		
	} else {
		for (i = optind; !finished && i < argc; i++) {
			patch(argv[i]);
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read a list of files from stdin and patch each in-turn.
//	Keep going on errors.
//

static int patchFileList(FILE *fp)
{
	char	buf[MAX_FNAME];
	char	*cp;
	int		len;

	while (!finished && !feof(fp)) {
		if (fgets(buf, sizeof(buf), fp) == 0) {
			break;
		}
		cp = buf;
		for (cp = buf; *cp && isspace(*cp); ) {
			cp++;
		}
		len = strlen(cp);
		if (cp[len - 1] == '\n') {
			cp[len - 1] = '\0';
		}
		patch(cp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Patch a file
//

static int patch(char *path)
{
	struct stat	sbuf;
	FILE		*ifp, *ofp, *dsiFp;
	char		dsiName[MAX_FNAME], dsiPath[MAX_FNAME];
	char		tmpFile[MAX_FNAME], saveFile[MAX_FNAME];
	char		searchPat[MAX_FNAME], truePat[MAX_FNAME], falsePat[MAX_FNAME];
	char		*dsiBuf, *cp, *ep, *start, *end, *tok, *tag, *ext, *inBuf;
	char		*startDsi, *ip;
	int			c, rc, i, level, len, line, patched;
	
	dsiBuf = 0;
	patched = 0;
	ifp = ofp = dsiFp = 0;

	if (*path == '.' && path[1] == '/') {
		path += 2;
	}
	if (*path == '/') {
		fprintf(stderr, "dsi: Path must be relative: %s\n", path);
		return -1;
	}

	for (level = 0, cp = path; *cp; cp++) {
		if (*cp == '/') {
			level++;
		}
	}

	if (verbose) {
		printf("Patching %s\n", path);
	}

	cp = parentDir;
	for (i = 0; i < level; i++) {
		*cp++ = '.';
		*cp++ = '.';
		*cp++ = '/';
	}
	*cp = '\0';
			
	ifp = fopen(path, "r" TEXT_FMT);
	if (ifp == 0) {
		fprintf(stderr, "dsi: Can't open %s\n", path);
		return -1;
	}
	stat(path, &sbuf);
	inBuf = (char*) malloc(sbuf.st_size + 1);
	rc = fread(inBuf, 1, sbuf.st_size, ifp);
	if (rc < 0) {
		fprintf(stderr, "dsi: Can't read file %s\n", path);
		free(inBuf);
		return -1;
	}
	inBuf[rc] = '\0';

	sprintf(tmpFile, "%s.new", path);
	ofp = fopen(tmpFile, "w" TEXT_FMT);
	if (ofp == 0) {
		fprintf(stderr, "dsi: Can't open %s\n", tmpFile);
		goto error;
	}

	line = 0;
	for (ip = inBuf; *ip; ip++) {
		start = ip;
		if (dsiFp == 0) {
			if (*ip == '\n') {
				fputc(*ip, ofp);
				line++;
				continue;
			}
			if (*ip != '<' || ip[1] != '!' || ip[2] != '-' || ip[3] != '-') {
				fputc(*ip, ofp);
				continue;
			}

			tok = "<!-- BeginDsi \"";
			len = strlen(tok);
			if (strncmp(start, tok, strlen(tok)) == 0) {
				//
				//	Cleanup if file has been corrupted by HTML editors
				//
				if (start > inBuf && start[-1] != '\n') {
					fprintf(ofp, "\n\n");
				}

				startDsi = start;
				start += len - 1;

				end = getNextTok(dsiName, sizeof(dsiName), start);
				if (end == 0) {
					fprintf(stderr, 
						"dsi: Syntax error for DSI in %s at line %d\n", 
						path, line);
					goto error;
				}
				for (start = end; *start && isspace(*start); start++);

				//
				//	Parse any pattern match args
				//		"searchPattern" "trueReplace" "falseReplace"
				//
				if (*start == '"') {
					start = getNextTok(searchPat, sizeof(searchPat), start);
					start = getNextTok(truePat, sizeof(truePat), start);
					start = getNextTok(falsePat, sizeof(falsePat), start);
				}
				if (start == 0) {
					fprintf(stderr, 
						"dsi: Syntax error for DSI in %s at line %d\n", 
						path, line);
					goto error;
				}

				for (; *start && isspace(*start); start++);
				end = strstr(start, "-->");
				if (end == 0) {
					fprintf(stderr, 
						"dsi: Missing closing comment in %s at line %d\n", 
						path, line);
					goto error;
				}
				c = end[3];
				end[3] = '\0';
				fprintf(ofp, "%s\n", startDsi);
				end[3] = c;

				if (dsiName[0] != '/') {
					sprintf(dsiPath, "%s/%s", incDir, dsiName);
				} else {
					strcpy(dsiPath, dsiName);
				}
				dsiFp = fopen(dsiPath, "r" TEXT_FMT);
				if (dsiFp == 0) {
					fprintf(stderr, 
						"dsi: Can't open DSI %s. Referenced in %s at line %d\n",
						dsiPath, path, line);
					goto error;
				}
				stat(dsiPath, &sbuf);

				if (verbose > 1) {
					printf("     DSI %s\n", dsiPath);
				}
				ip = &end[2];

			} else {
				fputc(*ip, ofp);
			}

		} else {
			tok = "<!-- EndDsi -->";
			len = strlen(tok);
			if (strncmp(start, tok, strlen(tok)) == 0) {
				dsiBuf = (char*) malloc(sbuf.st_size + 1);
				rc = fread(dsiBuf, 1, sbuf.st_size, dsiFp);
				if (rc < 0) {
					fprintf(stderr, "dsi: Can't read DSI %s\n", dsiPath);
					return -1;
				}
				dsiBuf[rc] = '\0';
				fclose(dsiFp);
				dsiFp = 0;				
				patched++;

				if (level == 0 && 0) {
					fwrite(dsiBuf, 1, rc, ofp);

				} else {
					for (cp = dsiBuf; *cp; ) {
						//
						//	MOB -- this is a very fragile parser. It really
						//	needs to be more flexible.
						//

						//
						//	<a href=
						//	<link .... href=
						//
						if (matchLink(ofp, " href=\"", &cp)) {
							continue;
						} 
						if (matchLink(ofp, "\thref=\"", &cp)) {
							continue;
						} 

						//
						//	<img src=
						//	<script ... src=
						//
						if (matchLink(ofp, " src=\"", &cp)) {
							continue;
						} 
						if (matchLink(ofp, "\tsrc=\"", &cp)) {
							continue;
						} 

						//
						//	<form ... action=
						//
						if (matchLink(ofp, "action=\"", &cp)) {
							continue;
						} 

						//
						//	_ROOT_ = "
						//
						if (matchLink(ofp, "_ROOT_=\"", &cp)) {
							continue;
						} 

						//
						//	Special _DSI_ pattern editing
						//
						tag = "_DSI_";
						len = strlen(tag);
						if (strncmp(cp, tag, len) == 0) {
							cp += len - 1;
							ep = strchr(cp, '"');
							if (strncmp(cp, searchPat, strlen(searchPat)) == 0){
								fprintf(ofp, "%s\"", truePat);

							} else {
								fprintf(ofp, "%s\"", falsePat);
							}
							cp = ep + 1;
							continue;
						}

						//
						//	Javascript patches
						//
						ext = strchr(dsiPath, '.');
						if (ext && strcmp(ext, ".js") == 0) {
							//
							//	= /*DSI*/ "
							//
							if (matchLink(ofp, "= /*DSI*/ \"", &cp)) {
								continue;
							} 

							//
							//	src = '
							//
							if (matchLink(ofp, "src == '", &cp)) {
								continue;
							} 
						}

						fputc(*cp, ofp);
						cp++;

					}
				}
				fprintf(ofp, "%s", tok);
				ip = &start[strlen(tok) - 1];
				//
				//	Cleanup if file has been corrupted by HTML editors
				//
				if (ip[1] != '\0' && 
					ip[2] != '\0' && ip[2] != '\n') {
					fputc('\n', ofp);
				}

			} else {
				//	Don't output as it is being replaced with DSI content
			}
		}
	}
	fclose(ifp);

	if (! patched) {
		fclose(ofp);
		unlink(tmpFile);

	} else {
		//
		//	If we found a DSI in the file, rename the patched file
		//
		fclose(ofp);
		sprintf(saveFile, "%s.dsiSave", path);
		unlink(saveFile);
		chmod(path, 0755);
		rename(path, saveFile);
		if (rename(tmpFile, path) < 0) {
			fprintf(stderr, "dsi: Can't rename %s to %s\n", tmpFile, path);
			rename(saveFile, path);
			return -1;
		}
		unlink(saveFile);
	}
	return 0;

error:
	if (ifp) {
		fclose(ifp);
	}
	if (ofp) {
		fclose(ofp);
	}
	if (inBuf) {
		free(inBuf);
	}
	if (dsiBuf) {
		free(dsiBuf);
	}
	return -1;
}
	
////////////////////////////////////////////////////////////////////////////////
//
//	Initialize signals
//

static void openSignals() 
{
#if !WIN && !_WIN32
	struct sigaction	act;

	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);

	act.sa_handler = catchInterrupt;
	sigaction(SIGINT, &act, 0);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void catchInterrupt(int signo)
{
	finished++;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Parse the next quoted string in the input beginning at start. Return the
//	token (minus quotes) in the tokBuf. Return with the input pointer one past
//	the trailing quote. Return 0 on all errors.
//

static char *getNextTok(char *tokBuf, int tokBufSize, char *start)
{
	char	*end;

	if (start == 0 || *start == '\0') {
		return 0;
	}

	while (*start && isspace(*start)) {
		start++;
	}

	if (*start != '"') {
		fprintf(stderr, "dsi: Missing opening quote\n");
		return 0;
	}

	start++;
	for (end = start; *end && *end != '"'; ) {
		end++;
	}
	if (*end != '"') {
		fprintf(stderr, "dsi: Missing closing quote\n");
		return 0;
	}
	if ((end - start) >= (tokBufSize - 1)) {
		fprintf(stderr, "dsi: Token too big\n");
		return 0;
	}
	strncpy(tokBuf, start, end - start);
	tokBuf[end - start] = '\0';
	return end + 1;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Match a link that needs patching 
//

static int matchLink(FILE *ofp, char *tag, char **start)
{
	int		len;

	len = strlen(tag);
	if (strncmp(*start, tag, len) == 0) {
		*start += len;
		if ((*start)[0] == '#' || strncmp(*start, "http://", 7) == 0) {
			fprintf(ofp, "%s", tag);
		} else {
			fprintf(ofp, "%s%s", tag, parentDir);
		}
		return 1;
	}
	return 0;
}

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4
//
