////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// utils.c

// This module provides general purpose utilities for the WavPack command-line
// utilities and the self-extraction module.

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#include <conio.h>
#elif defined(__GNUC__)
#include <glob.h>
#endif

#ifndef WIN32
#include <locale.h>
#include <iconv.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "wavpack.h"


#ifdef WIN32

int copy_timestamp (const char *src_filename, const char *dst_filename)
{
    FILETIME last_modified;
    HANDLE src, dst;
    int res = TRUE;

    if (*src_filename == '-' || *dst_filename == '-')
	return res;

    src = CreateFile (src_filename, GENERIC_READ, FILE_SHARE_READ, NULL,
	 OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    dst = CreateFile (dst_filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
	 OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	
    if (src == INVALID_HANDLE_VALUE || dst == INVALID_HANDLE_VALUE ||
	!GetFileTime (src, NULL, NULL, &last_modified) ||
	!SetFileTime (dst, NULL, NULL, &last_modified))
	    res = FALSE;

    if (src != INVALID_HANDLE_VALUE)
	CloseHandle (src);

    if (dst != INVALID_HANDLE_VALUE)
	CloseHandle (dst);

    return res;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function parses a filename (with or without full path) and returns  //
// a pointer to the extension (including the "."). If no extension is found //
// then NULL is returned. Extensions with more than 3 letters don't count.  //
//////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)

static int is_second_byte (char *filespec, char *pos);

char *filespec_ext (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    LANGID langid = GetSystemDefaultLangID ();

    while (--cp >= filespec) {

	if (langid == 0x411 && is_second_byte (filespec, cp))
	    --cp;

	if (*cp == '\\' || *cp == ':')
	    return NULL;

	if (*cp == '.') {
	    if (strlen (cp) > 1 && strlen (cp) <= 4)
		return cp;
	    else
		return NULL;
	}
    }

    return NULL;
}

#else

char *filespec_ext (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec) {

	if (*cp == '\\' || *cp == ':')
	    return NULL;

	if (*cp == '.') {
	    if (strlen (cp) > 1 && strlen (cp) <= 4)
		return cp;
	    else
		return NULL;
	}
    }

    return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function determines if the specified filespec is a valid pathname.  //
// If not, NULL is returned. If it is in the format of a pathname, then the //
// original pointer is returned. If the format is ambiguous, then a lookup  //
// is performed to determine if it is in fact a valid path, and if so a "\" //
// is appended so that the pathname can be used and the original pointer is //
// returned.                                                                //
//////////////////////////////////////////////////////////////////////////////

#if defined(__GNUC__) && !defined(WIN32)

char *filespec_path (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    glob_t globs;
    struct stat fstats;

    if (cp == filespec || filespec_wild (filespec))
	return NULL;

    if (*--cp == '\\' || *cp == ':')
	return filespec;

    if (*cp == '.' && cp == filespec)
	return strcat (filespec, "\\");

    if (glob (filespec, GLOB_MARK|GLOB_NOSORT, NULL, &globs) == 0 &&
	globs.gl_pathc > 0)
    {
	/* test if the file is a directory */
	if (stat(globs.gl_pathv[0], &fstats) == 0 && (fstats.st_mode & S_IFDIR)) {
		globfree(&globs);
		filespec[0] = '\0';
		return strcat (filespec, globs.gl_pathv[0]);
	}
    }
    globfree(&globs);

    return NULL;
}

#else

char *filespec_path (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    LANGID langid = GetSystemDefaultLangID ();
    struct _finddata_t finddata;
    int32_t file;

    if (cp == filespec || filespec_wild (filespec))
	return NULL;

    --cp;

    if (langid == 0x411 && is_second_byte (filespec, cp))
	--cp;

    if (*cp == '\\' || *cp == ':')
	return filespec;

    if (*cp == '.' && cp == filespec)
	return strcat (filespec, "\\");

    if ((file = _findfirst (filespec, &finddata)) != -1L &&
	(finddata.attrib & _A_SUBDIR)) {
	    _findclose (file);
	    return strcat (filespec, "\\");
    }
    if (file != -1L)
	    _findclose(file);
    
    return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function returns non-NULL if the specified filename spec has any    //
// wildcard characters.                                                     //
//////////////////////////////////////////////////////////////////////////////

char *filespec_wild (char *filespec)
{
    return strpbrk (filespec, "*?");
}

//////////////////////////////////////////////////////////////////////////////
// This function parses a filename (with or without full path) and returns  //
// a pointer to the actual filename, or NULL if no filename can be found.   //
//////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)

char *filespec_name (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    LANGID langid = GetSystemDefaultLangID ();

    while (--cp >= filespec) {
	if (langid == 0x411 && is_second_byte (filespec, cp))
	    --cp;

	if (*cp == '\\' || *cp == ':')
	    break;
    }

    if (strlen (cp + 1))
	return cp + 1;
    else
	return NULL;
}

#else

char *filespec_name (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec)
	if (*cp == '\\' || *cp == ':')
	    break;

    if (strlen (cp + 1))
	return cp + 1;
    else
	return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function returns TRUE if "pos" is pointing to the second byte of a  //
// double-byte character in the string "filespec" which is assumed to be    //
// shift-JIS.                                                               //
//////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)

static int is_second_byte (char *filespec, char *pos)
{
    uchar *cp = pos;

    while (cp > filespec && ((cp [-1] >= 0x81 && cp [-1] <= 0x9f) ||
                             (cp [-1] >= 0xe0 && cp [-1] <= 0xfc)))
	cp--;

    return ((int) pos - (int) cp) & 1;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function allows the user to type 'y', 'n', or 'a' (with Enter) in   //
// response to a system query. The return value is the key typed as         //
// lowercase (regardless of the typed case).                                //
//////////////////////////////////////////////////////////////////////////////

static int waiting_input;

char yna (void)
{
    char choice = 0;
    int key;

    waiting_input = 1;

    while (1) {
#if defined(WIN32)
	key = getch ();
#else
	key = fgetc(stdin);
#endif
	if (key == 3) {
	    fprintf (stderr, "^C\n");
	    exit (1);
	}
	else if (key == '\r' || key == '\n') {
	    if (choice) {
		fprintf (stderr, "\r\n");
		break;
	    }
	    else
		fprintf (stderr, "%c", 7);
	}
	else if (key == 'Y' || key == 'y') {
	    fprintf (stderr, "%c\b", key);
	    choice = 'y';
	}
	else if (key == 'N' || key == 'n') {
	    fprintf (stderr, "%c\b", key);
	    choice = 'n';
	}
	else if (key == 'A' || key == 'a') {
	    fprintf (stderr, "%c\b", key);
	    choice = 'a';
	}
	else
	    fprintf (stderr, "%c", 7);
    }

    waiting_input = 0;

    return choice;
}

//////////////////////////////////////////////////////////////////////////////
// Display the specified message on the console through stderr. Note that   //
// the cursor may start anywhere in the line and all text already on the    //
// line is erased. A terminating newline is not needed and function works   //
// with printf strings and args.                                            //
//////////////////////////////////////////////////////////////////////////////

void error_line (char *error, ...)
{
    char error_msg [512];
    va_list argptr;

    error_msg [0] = '\r';
    va_start (argptr, error);
    vsprintf (error_msg + 1, error, argptr);
    va_end (argptr);
    fputs (error_msg, stderr);
    finish_line ();
#if 0
    {
	FILE *error_log = fopen ("c:\\wavpack.log", "a+");

	if (error_log) {
	    fputs (error_msg + 1, error_log);
	    fputc ('\n', error_log);
	    fclose (error_log);
	}
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Function to intercept ^C or ^Break typed at the console.                 //
//////////////////////////////////////////////////////////////////////////////
#if defined(WIN32)
static int break_flag;

BOOL WINAPI ctrl_handler (DWORD ctrl)
{
    if (ctrl == CTRL_C_EVENT) {
	break_flag = TRUE;
	return TRUE;
    }

    if (ctrl == CTRL_BREAK_EVENT) {

	if (waiting_input) {
#ifdef __BORLANDC__
	    fprintf (stderr, "^C\n");
#endif
	    return FALSE;
	}
	else {
	    break_flag = TRUE;
	    return TRUE;
	}
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Function to initialize console for intercepting ^C and ^Break.           //
//////////////////////////////////////////////////////////////////////////////

void setup_break (void)
{
    HANDLE hConIn = GetStdHandle (STD_INPUT_HANDLE);

    SetConsoleMode (hConIn, ENABLE_PROCESSED_INPUT);
    FlushConsoleInputBuffer (hConIn);
    SetConsoleCtrlHandler (ctrl_handler, TRUE);
    break_flag = 0;
}

//////////////////////////////////////////////////////////////////////////////
// Function to determine whether ^C or ^Break has been issued by user.      //
//////////////////////////////////////////////////////////////////////////////

int check_break (void)
{
    return break_flag;
}

//////////////////////////////////////////////////////////////////////////////
// Function to clear the stderr console to the end of the current line (and //
// go to the beginning next line).                                          //
//////////////////////////////////////////////////////////////////////////////

void finish_line (void)
{
    HANDLE hConIn = GetStdHandle (STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    if (hConIn && GetConsoleScreenBufferInfo (hConIn, &coninfo)) {
	char spaces = coninfo.dwSize.X - coninfo.dwCursorPosition.X;

	while (spaces--)
	    fputc (' ', stderr);
    }
    else
	fputc ('\n', stderr);
}
#else
//////////////////////////////////////////////////////////////////////////////
// Function to clear the stderr console to the end of the current line (and //
// go to the beginning next line).                                          //
//////////////////////////////////////////////////////////////////////////////

void finish_line (void)
{
/*    char spaces = 1;

    while (spaces--)
	putc (' ', stderr);
    else*/
	fputc ('\n', stderr);
}

//////////////////////////////////////////////////////////////////////////////
// Function to initialize console for intercepting ^C and ^Break.           //
//////////////////////////////////////////////////////////////////////////////

void setup_break (void)
{
}

//////////////////////////////////////////////////////////////////////////////
// Function to determine whether ^C or ^Break has been issued by user.      //
//////////////////////////////////////////////////////////////////////////////

int check_break (void)
{
    return 0;
}

#endif

//////////////////////////// File I/O Wrapper ////////////////////////////////

int DoReadFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToRead, uint32_t *lpNumberOfBytesRead)
{
    uint32_t bcount;

    *lpNumberOfBytesRead = 0;

    while (nNumberOfBytesToRead) {
	bcount = fread ((uchar *) lpBuffer + *lpNumberOfBytesRead, 1, nNumberOfBytesToRead, hFile);

	if (bcount) {
	    *lpNumberOfBytesRead += bcount;
	    nNumberOfBytesToRead -= bcount;
	}
	else
	    break;
    }

    return !ferror (hFile);
}

int DoWriteFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t *lpNumberOfBytesWritten)
{
    uint32_t bcount;

    *lpNumberOfBytesWritten = 0;

    while (nNumberOfBytesToWrite) {
	bcount = fwrite ((uchar *) lpBuffer + *lpNumberOfBytesWritten, 1, nNumberOfBytesToWrite, hFile);

	if (bcount) {
	    *lpNumberOfBytesWritten += bcount;
	    nNumberOfBytesToWrite -= bcount;
	}
	else
	    break;
    }

    return !ferror (hFile);
}

uint32_t DoGetFileSize (FILE *hFile)
{
    struct stat statbuf;

    if (!hFile || fstat (fileno (hFile), &statbuf) || !(statbuf.st_mode & S_IFREG))
	return 0;

    return statbuf.st_size;
}

uint32_t DoGetFilePosition (FILE *hFile)
{
    return ftell (hFile);
}

int DoSetFilePositionAbsolute (FILE *hFile, uint32_t pos)
{
    return fseek (hFile, pos, SEEK_SET);
}

int DoSetFilePositionRelative (FILE *hFile, int32_t pos, int mode)
{
    return fseek (hFile, pos, mode);
}

// if ungetc() is not available, a seek of -1 is fine also because we do not
// change the byte read.

int DoUngetc (int c, FILE *hFile)
{
    return ungetc (c, hFile);
}

int DoCloseHandle (FILE *hFile)
{
    return hFile ? !fclose (hFile) : 0;
}

int DoTruncateFile (FILE *hFile)
{
    if (hFile) {
	fflush (hFile);
#if defined(WIN32)
	return !chsize (fileno (hFile), 0);
#else
	return !ftruncate(fileno (hFile), 0);
#endif
    }

    return 0;
}

int DoDeleteFile (char *filename)
{
    return !remove (filename);
}

// Convert the Unicode wide-format string into a UTF-8 string using no more
// than the specified buffer length. The wide-format string must be NULL
// terminated and the resulting string will be NULL terminated. The actual
// number of characters converted (not counting terminator) is returned, which
// may be less than the number of characters in the wide string if the buffer
// length is exceeded.

static int WideCharToUTF8 (const ushort *Wide, uchar *pUTF8, int len)
{
    const ushort *pWide = Wide;
    int outndx = 0;

    while (*pWide) {
	if (*pWide < 0x80 && outndx + 1 < len)
	    pUTF8 [outndx++] = (uchar) *pWide++;
	else if (*pWide < 0x800 && outndx + 2 < len) {
	    pUTF8 [outndx++] = (uchar) (0xc0 | ((*pWide >> 6) & 0x1f));
	    pUTF8 [outndx++] = (uchar) (0x80 | (*pWide++ & 0x3f));
	}
	else if (outndx + 3 < len) {
	    pUTF8 [outndx++] = (uchar) (0xe0 | ((*pWide >> 12) & 0xf));
	    pUTF8 [outndx++] = (uchar) (0x80 | ((*pWide >> 6) & 0x3f));
	    pUTF8 [outndx++] = (uchar) (0x80 | (*pWide++ & 0x3f));
	}
	else
	    break;
    }

    pUTF8 [outndx] = 0;
    return pWide - Wide;
}

// Convert a Ansi string into its Unicode UTF-8 format equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

void AnsiToUTF8 (char *string, int len)
{
    int max_chars = strlen (string);
#if defined(WIN32)
    ushort *temp = (ushort *) malloc ((max_chars + 1) * 2);

    MultiByteToWideChar (CP_ACP, 0, string, -1, temp, max_chars + 1);
    WideCharToUTF8 (temp, (uchar *) string, len);
#else
    char *temp = malloc (len);
//  memset(temp, 0, len);
    char *outp = temp;
    const char *inp = string;
    size_t insize = max_chars;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");
    iconv_t converter = iconv_open ("UTF-8", "");
    err = iconv (converter, &inp, &insize, &outp, &outsize);
    iconv_close (converter);
    setlocale (LC_CTYPE, old_locale);

    if (err == -1) {
	free(temp);
	return;
    }

    memmove (string, temp, len);
#endif
    free (temp);
}
