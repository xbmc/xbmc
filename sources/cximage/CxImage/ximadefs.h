#if !defined(__ximadefs_h)
#define __ximadefs_h

#if defined(_AFXDLL)||defined(_USRDLL)
 #define DLL_EXP __declspec(dllexport)
#else
 #define DLL_EXP
#endif

#if CXIMAGE_SUPPORT_JP2 || CXIMAGE_SUPPORT_JPC || CXIMAGE_SUPPORT_PGX || CXIMAGE_SUPPORT_PNM || CXIMAGE_SUPPORT_RAS
 #define CXIMAGE_SUPPORT_JASPER 1
#else
 #define CXIMAGE_SUPPORT_JASPER 0
#endif

#ifdef XBMC
#undef CXIMAGE_SUPPORT_JASPER 
#endif
#if CXIMAGE_SUPPORT_DSP
#undef CXIMAGE_SUPPORT_TRANSFORMATION
 #define CXIMAGE_SUPPORT_TRANSFORMATION 1
#endif

#if CXIMAGE_SUPPORT_TRANSFORMATION || CXIMAGE_SUPPORT_TIF || CXIMAGE_SUPPORT_TGA || CXIMAGE_SUPPORT_BMP || CXIMAGE_SUPPORT_WINDOWS
 #define CXIMAGE_SUPPORT_BASICTRANSFORMATIONS 1
#endif

#if CXIMAGE_SUPPORT_WINCE
 #undef CXIMAGE_SUPPORT_WMF
 #define CXIMAGE_SUPPORT_WMF 0
 #undef	CXIMAGE_SUPPORT_WINDOWS
 #define	CXIMAGE_SUPPORT_WINDOWS 0
#endif

#ifndef WIN32
 #undef CXIMAGE_SUPPORT_WINDOWS
 #define CXIMAGE_SUPPORT_WINDOWS 0
#endif

#ifdef _XBOX
 #undef CXIMAGE_SUPPORT_WINDOWS
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <math.h>



#include <stdlib.h>
#include <string.h>

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;

typedef DWORD          COLORREF;

#ifdef _LINUX
typedef unsigned int   HANDLE;
typedef void*          HRGN;

#define	BOOL bool
#define	TRUE true
#define	FALSE false
#endif

#ifdef _LINUX
typedef struct tagRECT
{
	long    left;
	long    top;
	long    right;
	long    bottom;
} RECT;

typedef struct tagPOINT
{
	long  x;
	long  y;
} POINT;
#endif
#if defined(_XBOX) || defined(_LINUX)
typedef struct tagRGBQUAD {
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} RGBQUAD;
#endif
#pragma pack(1)

#if defined(_XBOX) || defined(_LINUX)
typedef struct tagBITMAPINFOHEADER{
	DWORD      biSize;
	long       biWidth;
	long       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	long       biXPelsPerMeter;
	long       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER {
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPCOREHEADER {
	DWORD   bcSize;
	WORD    bcWidth;
	WORD    bcHeight;
	WORD    bcPlanes;
	WORD    bcBitCount;
} BITMAPCOREHEADER;

typedef struct tagRGBTRIPLE {
	BYTE    rgbtBlue;
	BYTE    rgbtGreen;
	BYTE    rgbtRed;
} RGBTRIPLE;
#endif

#pragma pack()

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#if !defined(_XBOX) && !defined(WIN32)

  typedef struct tagcomplex {
	  double x,y;
  } _complex;

  #define _cabs(c) sqrt(c.x*c.x+c.y*c.y)
#endif



#endif //__ximadefs
