// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
/*
 * TIFF C Library by Troels K.
 * TIFF file IO, using straight C Runtime Library file functions.
 */
#ifdef WIN32
 #include <windows.h>
#endif
#include <stdio.h>
#include "tiffiop.h"

#ifdef _LINUX
#define BOOL bool
#endif 

#include "../CxImage/xfile.h"

static tsize_t 
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((CxFile*)fd)->Read(buf, 1, size);
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
	return ((CxFile*)fd)->Write(buf, 1, size);
}

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
   BOOL bOK;
   long pos = 0;
   bOK = ((CxFile*)fd)->Seek(off, whence);
   if (bOK) switch(whence){
	   case SEEK_SET:
		   pos = off;
		   break;
	   case SEEK_CUR:
		   pos = ((CxFile*)fd)->Tell();
		   break;
	   case SEEK_END:
		   pos = ((CxFile*)fd)->Tell();
		   break;
   }
   return (toff_t)pos;
}

// Return nonzero if error
static int
_tiffCloseProc(thandle_t fd)
{
//	return !((CxFile*)fd)->Close(); // "//" needed for memory files <DP>
	return 0;
}

#include <sys/stat.h>

static toff_t
_tiffSizeProc(thandle_t fd)
{
	return ((CxFile*)fd)->Size();
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
}

// Open a TIFF file descriptor for read/writing.

TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fd,
	    _tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc,
	    _tiffSizeProc, _tiffMapProc, _tiffUnmapProc);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

/*
 * Open a TIFF file for read/writing.
 */

TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
   FILE* stream = fopen(name, mode);
	if (stream == NULL) 
   {
		TIFFError(module, "%s: Cannot open", name);
		return NULL;
	}
	return (TIFFFdOpen((int)stream, name, mode));
}


extern "C" TIFF* TIFFOpenEx(CxFile* stream, const char* mode)
{
	return (TIFFFdOpen((int)stream, "TIFF IMAGE", mode));
}

#ifdef __GNUC__
extern	char* malloc();
extern	char* realloc();
#else
#include <malloc.h>
#endif

tdata_t
_TIFFmalloc(tsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
	free(p);
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
Win32WarningHandler(const char* module, const char* fmt, va_list ap)
{
#ifdef _DEBUG
#if (!defined(_CONSOLE) && defined(WIN32))
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Warning";
	LPCTSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPTSTR)szDefaultModule : (LPTSTR)module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (lstrlen(szTmp) +
			lstrlen(szTitleText) + lstrlen(fmt) + 128)*sizeof(TCHAR))) == NULL)
		return;
	wsprintf(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (lstrlen(szTitle)+2)*sizeof(TCHAR);
	wvsprintf(szTmp, fmt, ap);
	MessageBox(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONINFORMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
#endif
}
TIFFErrorHandler _TIFFwarningHandler = Win32WarningHandler;

static void
Win32ErrorHandler(const char* module, const char* fmt, va_list ap)
{
#ifdef _DEBUG
#if (!defined(_CONSOLE) && defined(WIN32))
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Error";
	LPCTSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPTSTR)szDefaultModule : (LPTSTR)module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (lstrlen(szTmp) +
			lstrlen(szTitleText) + lstrlen(fmt) + 128)*sizeof(TCHAR))) == NULL)
		return;
	wsprintf(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (lstrlen(szTitle)+2)*sizeof(TCHAR);
	wvsprintf(szTmp, fmt, ap);
	MessageBox(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
#endif
}
TIFFErrorHandler _TIFFerrorHandler = Win32ErrorHandler;

